#include "ProcessWorkSharedQueue.h"
#include "ProcessWorkAtomic.h"
#include "CSystem/CSystemAPI.h"
#include <algorithm>
#include <new>
#if defined(__APPLE__)
#include <pthread.h>
#endif

static std::atomic<uint64_t> s_queueSerial(0);
#ifdef _MSC_VER
static __declspec(thread) uint64_t s_queueInstance = 0;
static __declspec(thread) int64_t s_queueToken = 0;
static __declspec(thread) ProcessWorkProducerState* s_queueProducer = nullptr;
#else
static thread_local uint64_t s_queueInstance = 0;
static thread_local int64_t s_queueToken = 0;
static thread_local ProcessWorkProducerState* s_queueProducer = nullptr;
#endif

/** 取得不截断的本进程实际线程标识，不能用可能碰撞的哈希标识SPSC发布者
@return 当前平台的完整原生标识；查询失败或平台不支持时返回0
*/
static uint64_t ProcessWorkSendingThreadId()
{
#if defined(__APPLE__)
    uint64_t thread = 0;
    return pthread_threadid_np(nullptr, &thread) == 0 ? thread : 0;
#elif defined(_WIN32) || defined(__linux__)
    return CSystem::SystemThreadId();
#else
    return 0;
#endif
}

ProcessWorkProducerState::ProcessWorkProducerState() :
m_index(0), m_shared(nullptr), m_bodyPage(nullptr), m_bodyPageIndex(UINT32_MAX), m_tailNode(nullptr), m_tail(0)
{

}

ProcessWorkReadState::ProcessWorkReadState(uint32_t index, ProcessWorkSharedStream* stream) :
m_index(index), m_shared(stream), m_node(nullptr), m_head(0)
{

}

ProcessWorkSharedQueue::ProcessWorkSharedQueue(ProcessWorkStorage& storage, ProcessWorkSharedControl& control) :
m_storage(&storage), m_control(&control), m_instance(s_queueSerial.fetch_add(1) + 1),
m_seenStreamEpoch(-1), m_seenRequestEpoch(0), m_preparedPages(0), m_freeCursor(1), m_nodeCursor(0),
m_readCursor(0), m_roundCount(0), m_readQuota(0), m_readBytes(0), m_compactStreams(false), m_frontReader(nullptr), m_frontSlot(0), m_frontOffset(-1)
{
    m_pendingHead[0] = m_pendingHead[1] = 0;
    m_pendingTail[0] = m_pendingTail[1] = 0;
}

ProcessWorkSharedQueue::~ProcessWorkSharedQueue()
{

}

bool ProcessWorkSharedQueue::configureProducers(const LockFreeMapConfig& config)
{
    return m_producers.lockFreeMapConfigure(config) == LOCK_FREE_MAP_OK;
}

bool ProcessWorkSharedQueue::initialize()
{
    ProcessWorkPage* page = m_storage->page(0, true);
    if (page == nullptr)
    {
        return false;
    }
    ProcessWorkAtomic::store(&page->m_metadata->m_nodes[0].m_owner, int64_t(0));
    m_preparedPages = 1;
    return true;
}

bool ProcessWorkSharedQueue::activate(uint32_t index)
{
    ProcessWorkPage* page = index == 0 ? nullptr : m_storage->page(index, true);
    if (page == nullptr)
    {
        return false;
    }
    ProcessWorkAtomic::replace(&page->m_metadata->m_nodes[index & 255].m_owner, int64_t(-1), int64_t(0));
    return true;
}

ProcessWorkProducerState* ProcessWorkSharedQueue::producer(int64_t token, int32_t* failure, bool retryContention)
{
    if (s_queueInstance == m_instance && s_queueToken == token)
    {
        return s_queueProducer;
    }
    return registerProducer(token, failure, retryContention);
}

ProcessWorkProducerState* ProcessWorkSharedQueue::registerProducer(int64_t token, int32_t* failure, bool retryContention)
{
    if (failure != nullptr)
    {
        *failure = PROCESS_WORK_RESOURCE;
    }
    try
    {
        const uint64_t thread = ProcessWorkSendingThreadId();
        if (thread == 0)
        {
            if (failure != nullptr)
            {
                *failure = PROCESS_WORK_UNAVAILABLE;
            }
            return nullptr;
        }
        const std::pair<int64_t, uint64_t> key(token, thread);
        std::shared_ptr<ProcessWorkProducerState> state;
        LockFreeMapToken position;
        if (m_producers.lockFreeMapGet(key, state, position) != LOCK_FREE_MAP_OK)
        {
            std::shared_ptr<ProcessWorkProducerState> candidate(new ProcessWorkProducerState);
            // 容量等待交给外层发送循环，避免在登记阶段绕过try容量开关和关闭检测。
            const LockFreeMapStatus result = m_producers.tryGetOrInsert(key, candidate, state, position, retryContention, false);
            if (result != LOCK_FREE_MAP_EXISTS && result != LOCK_FREE_MAP_COMMITTED)
            {
                if (failure != nullptr)
                {
                    *failure = result == LOCK_FREE_MAP_ALLOCATION_FAILURE ? PROCESS_WORK_NO_MEMORY :
                        result == LOCK_FREE_MAP_BUSY ? PROCESS_WORK_BUSY :
                        result == LOCK_FREE_MAP_CAPACITY || result == LOCK_FREE_MAP_RECORD_LIMIT ||
                        result == LOCK_FREE_MAP_GROW_DENIED ? PROCESS_WORK_CAPACITY :
                        result == LOCK_FREE_MAP_RESERVED ? PROCESS_WORK_RESOURCE : PROCESS_WORK_UNAVAILABLE;
                }
                return nullptr;
            }
        }
        if (!state)
        {
            return nullptr;
        }
        if (state->m_shared == nullptr)
        {
            const uint32_t count = static_cast<uint32_t>((std::min)(ProcessWorkAtomic::load(&m_control->m_nextStream), int64_t(0xffffff)));
            // 同一物理发送线程可能经PID/名称缓存的不同本地对象发送，不能因此分裂自身FIFO。
            for (uint32_t index = 1; index <= count; ++index)
            {
                ProcessWorkSharedStream* shared = m_storage->stream(index);
                if (shared != nullptr && ProcessWorkAtomic::load(&shared->m_owner) == token &&
                    static_cast<uint64_t>(ProcessWorkAtomic::load(&shared->m_thread)) == thread)
                {
                    state->m_index = index;
                    state->m_shared = shared;
                    break;
                }
            }
            if (state->m_shared == nullptr)
            {
                if (state->m_index == 0)
                {
                    if (ProcessWorkAtomic::load(&m_control->m_nextStream) >= 0xffffff)
                    {
                        if (failure != nullptr)
                        {
                            *failure = PROCESS_WORK_CAPACITY;
                        }
                        return nullptr;
                    }
                    const int64_t index = ProcessWorkAtomic::increment(&m_control->m_nextStream);
                    if (index <= 0 || index > 0xffffff)
                    {
                        if (failure != nullptr)
                        {
                            *failure = PROCESS_WORK_CAPACITY;
                        }
                        return nullptr;
                    }
                    state->m_index = static_cast<uint32_t>(index);
                }
                ProcessWorkSharedStream* shared = m_storage->stream(state->m_index);
                if (shared == nullptr)
                {
                    return nullptr;
                }
                // 负token先登记构造所有者；进程若在owner正式发布前崩溃，接收器仍能回收初始段。
                ProcessWorkAtomic::store(&shared->m_owner, -token);
                const int64_t generation = int64_t(1) << 32;
                const int64_t first = generation | (0x1000000U + state->m_index + 1);
                shared->m_thread = static_cast<int64_t>(thread);
                shared->m_writeNode = first;
                shared->m_writeOffset = 0;
                shared->m_head = first;
                shared->m_headSlot = 0;
                shared->m_first.m_owner = -2;
                shared->m_first.m_link = generation;
                for (int32_t i = 0; i < kProcessWorkSegmentCells; ++i)
                {
                    shared->m_first.m_cells[i] = generation;
                }
                // 下标不会重用，消费者只在owner发布后读取完整登记和初始段。
                ProcessWorkAtomic::store(&shared->m_owner, token);
                ProcessWorkAtomic::increment(&m_control->m_streamEpoch);
                state->m_shared = shared;
            }
        }
        s_queueInstance = m_instance;
        s_queueToken = token;
        s_queueProducer = state.get();
        return state.get();
    }
    catch (const std::bad_alloc&)
    {
        if (failure != nullptr)
        {
            *failure = PROCESS_WORK_NO_MEMORY;
        }
        return nullptr;
    }
    catch (...)
    {
        if (failure != nullptr)
        {
            *failure = PROCESS_WORK_UNAVAILABLE;
        }
        return nullptr;
    }
}

bool ProcessWorkSharedQueue::resetNode(ProcessWorkSharedNode& value)
{
    const uint64_t generation = static_cast<uint64_t>(ProcessWorkAtomic::load(&value.m_link)) >> 32;
    if (generation == 0xffffffffULL)
    {
        ProcessWorkAtomic::store(&value.m_owner, int64_t(-3));
        return false;
    }
    const int64_t empty = static_cast<int64_t>((generation + 1) << 32);
    ProcessWorkAtomic::store(&value.m_link, empty);
    for (int32_t i = 0; i < kProcessWorkSegmentCells; ++i)
    {
        ProcessWorkAtomic::store(&value.m_cells[i], empty);
    }
    return true;
}

bool ProcessWorkSharedQueue::reserve(int64_t token, uint32_t extent, uint32_t start, uint32_t& index, bool serving, bool* contended)
{
    // 两类申请者都只竞争FREE段；releaseNode会直接把归还段交给等待者。
    // serving保留现有内部调用形式，不把未被授予的空闲段封锁给单一执行流。
    static_cast<void>(serving);
    if (token <= 0 || extent > 0xffffff)
    {
        return false;
    }
    const uint32_t count = nodeCount(extent);
    uint32_t ordinal = start < count ? start : start % count;
    for (uint32_t step = 0; step < count; ++step)
    {
        const uint32_t candidate = nodeAt(ordinal, extent);
        ProcessWorkSharedNode* value = node(candidate);
        if (value != nullptr && ProcessWorkAtomic::load(&value->m_owner) == 0)
        {
            if (ProcessWorkAtomic::replace(&value->m_owner, int64_t(0), token))
            {
                if (resetNode(*value))
                {
                    index = candidate;
                    return true;
                }
            }
            else if (contended != nullptr)
            {
                *contended = true;
            }
        }
        ordinal = ordinal + 1 == count ? 0 : ordinal + 1;
    }
    return false;
}

void ProcessWorkSharedQueue::cancel(uint32_t index, int64_t token)
{
    ProcessWorkSharedNode* value = node(index);
    if (value != nullptr)
    {
        ProcessWorkAtomic::replace(&value->m_owner, token, int64_t(0));
    }
}

bool ProcessWorkSharedQueue::enqueue(int64_t token, int32_t bufferIndex, bool inlined, uint32_t extent, uint32_t& hint)
{
    ProcessWorkProducerState* state = token > 0 ? producer(token) : nullptr;
    return state != nullptr && enqueue(*state, bufferIndex, inlined, extent, hint);
}

bool ProcessWorkSharedQueue::enqueueSegment(ProcessWorkProducerState& state, ProcessWorkSharedNode& last,
    int64_t tail, int64_t payload, uint32_t extent, uint32_t& hint, uint32_t granted, bool* contended)
{
    ProcessWorkSharedStream& stream = *state.m_shared;
    uint32_t index = granted;
    if (index == UINT32_MAX && !reserve(ProcessWorkAtomic::load(&stream.m_owner), extent, hint, index, false, contended))
    {
        return false;
    }
    hint = index + 1;
    ProcessWorkSharedNode* candidate = node(index);
    if (candidate == nullptr)
    {
        return false;
    }
    const int64_t generation = ProcessWorkAtomic::load(&candidate->m_link);
    ProcessWorkAtomic::store(&candidate->m_cells[0], generation | payload);
    const int64_t oldGeneration = static_cast<int64_t>(static_cast<uint64_t>(tail) & 0xffffffff00000000ULL);
    // 第一条正文和新段均已完成；此release链接是提交点，消费者不等待后续写尾提示。
    ProcessWorkAtomic::store(&last.m_link, oldGeneration | (index + 1));
    ProcessWorkAtomic::store(&stream.m_writeNode, generation | (index + 1));
    ProcessWorkAtomic::store(&stream.m_writeOffset, int64_t(1));
    return true;
}

void ProcessWorkSharedQueue::request(ProcessWorkProducerState& state, bool forNode)
{
    ProcessWorkSharedStream& stream = *state.m_shared;
    if (ProcessWorkAtomic::load(&stream.m_request) == 0)
    {
        ProcessWorkAtomic::store(&stream.m_cancel, int64_t(0));
        ProcessWorkAtomic::store(&stream.m_request, forNode ? int64_t(2) : int64_t(1));
        ProcessWorkAtomic::increment(&m_control->m_requestEpoch);
    }
}

bool ProcessWorkSharedQueue::takeGrant(ProcessWorkProducerState& state, bool forNode, uint32_t& index)
{
    ProcessWorkSharedStream& stream = *state.m_shared;
    if (ProcessWorkAtomic::load(&stream.m_cancel) != 0 ||
        ProcessWorkAtomic::load(&stream.m_request) != (forNode ? 6 : 5))
    {
        return false;
    }
    index = static_cast<uint32_t>(ProcessWorkAtomic::load(&stream.m_grant));
    const int64_t token = ProcessWorkAtomic::load(&stream.m_owner);
    // 映射失败时保留授予，取消或死亡回收仍能找到该资源；不能先清请求再丢掉所有权。
    if (forNode)
    {
        ProcessWorkSharedNode* granted = node(index);
        if (granted == nullptr || ProcessWorkAtomic::load(&granted->m_owner) != token)
        {
            return false;
        }
    }
    else
    {
        ProcessWorkSharedBlock* granted = index == 0 ? nullptr : m_storage->block(index);
        if (granted == nullptr || ProcessWorkAtomic::load(&granted->m_owner) != token)
        {
            return false;
        }
    }
    ProcessWorkAtomic::store(&stream.m_request, int64_t(0));
    return true;
}

void ProcessWorkSharedQueue::cancelRequest(ProcessWorkProducerState& state)
{
    if (ProcessWorkAtomic::load(&state.m_shared->m_request) != 0)
    {
        ProcessWorkAtomic::store(&state.m_shared->m_cancel, int64_t(1));
        ProcessWorkAtomic::increment(&m_control->m_requestEpoch);
    }
}

void ProcessWorkSharedQueue::refreshRequests()
{
    const int64_t epoch = ProcessWorkAtomic::load(&m_control->m_requestEpoch);
    if (epoch == m_seenRequestEpoch)
    {
        return;
    }
    if (!refreshStreams())
    {
        return;
    }
    // 活动登记在首次发布时加入，死亡且排空后移出；不反复扫描全部历史线程下标。
    for (size_t position = 0; position < m_active.size(); ++position)
    {
        const uint32_t index = m_active[position]->m_index;
        ProcessWorkSharedStream* stream = m_active[position]->m_shared;
        const int64_t token = ProcessWorkAtomic::load(&stream->m_owner);
        const int64_t request = ProcessWorkAtomic::load(&stream->m_request);
        if (token <= 0 || request == 0)
        {
            continue;
        }
        if (ProcessWorkAtomic::load(&stream->m_cancel) != 0 || ProcessWorkAtomic::load(&stream->m_dead) != 0)
        {
            if (request == 5)
            {
                ProcessWorkSharedBlock* body = m_storage->block(static_cast<uint32_t>(ProcessWorkAtomic::load(&stream->m_grant)));
                if (body != nullptr)
                {
                    ProcessWorkAtomic::replace(&body->m_owner, token, int64_t(0));
                }
            }
            else if (request == 6)
            {
                cancel(static_cast<uint32_t>(ProcessWorkAtomic::load(&stream->m_grant)), token);
            }
            if (request != 3 && request != 4)
            {
                ProcessWorkAtomic::store(&stream->m_request, int64_t(0));
            }
            continue;
        }
        if (request == 1 || request == 2)
        {
            const uint32_t kind = static_cast<uint32_t>(request - 1);
            stream->m_nextWaiter = 0;
            if (m_pendingTail[kind] == 0)
            {
                m_pendingHead[kind] = index;
            }
            else
            {
                m_storage->stream(m_pendingTail[kind])->m_nextWaiter = index;
            }
            m_pendingTail[kind] = index;
            ProcessWorkAtomic::store(&stream->m_request, request + 2);
        }
    }
    m_seenRequestEpoch = epoch;
}

ProcessWorkSharedStream* ProcessWorkSharedQueue::pending(uint32_t kind, uint32_t& index)
{
    while (m_pendingHead[kind] != 0)
    {
        index = m_pendingHead[kind];
        ProcessWorkSharedStream* stream = m_storage->stream(index);
        if (stream == nullptr)
        {
            return nullptr;
        }
        if (ProcessWorkAtomic::load(&stream->m_cancel) == 0 && ProcessWorkAtomic::load(&stream->m_dead) == 0)
        {
            return stream;
        }
        m_pendingHead[kind] = static_cast<uint32_t>(stream->m_nextWaiter);
        if (m_pendingHead[kind] == 0)
        {
            m_pendingTail[kind] = 0;
        }
        ProcessWorkAtomic::store(&stream->m_request, int64_t(0));
    }
    return nullptr;
}

void ProcessWorkSharedQueue::grant(uint32_t kind, ProcessWorkSharedStream& stream, uint32_t resource)
{
    m_pendingHead[kind] = static_cast<uint32_t>(stream.m_nextWaiter);
    if (m_pendingHead[kind] == 0)
    {
        m_pendingTail[kind] = 0;
    }
    stream.m_grant = resource;
    ProcessWorkAtomic::store(&stream.m_request, static_cast<int64_t>(kind + 5));
    if (m_pendingHead[0] == 0 && m_pendingHead[1] == 0)
    {
        // 最后一位等待者已获得专属资源，剩余FREE槽可以恢复正常申请。
        // 只确认完整扫描过的代次；并发新请求增加epoch后，归还资源仍会先服务等待者。
        ProcessWorkAtomic::store(&m_control->m_servedEpoch, m_seenRequestEpoch);
    }
}

void ProcessWorkSharedQueue::service(uint32_t extent)
{
    const uint32_t streams = static_cast<uint32_t>((std::min)(ProcessWorkAtomic::load(&m_control->m_nextStream), int64_t(0xffffff)));
    const uint32_t pages = (streams >> 8) + 1;
    if (m_preparedPages < pages)
    {
        // 每次至多创建一页，系统内存保护仍由Storage执行，避免线程突增造成集中申请。
        if (m_storage->page(m_preparedPages << 8, true) != nullptr)
        {
            ++m_preparedPages;
        }
    }
    compactStreams();
    refreshRequests();
    for (uint32_t work = 0; work < 64; ++work)
    {
        uint32_t waiting = 0;
        ProcessWorkSharedStream* stream = pending(0, waiting);
        if (stream == nullptr)
        {
            break;
        }
        bool assigned = false;
        for (uint32_t step = 0; step < extent; ++step)
        {
            if (m_freeCursor == 0 || m_freeCursor > extent)
            {
                m_freeCursor = 1;
            }
            const uint32_t index = m_freeCursor++;
            ProcessWorkSharedBlock* body = m_storage->block(index);
            if (body != nullptr && ProcessWorkAtomic::load(&body->m_owner) == 0 &&
                ProcessWorkAtomic::replace(&body->m_owner, int64_t(0), ProcessWorkAtomic::load(&stream->m_owner)))
            {
                grant(0, *stream, index);
                assigned = true;
                break;
            }
        }
        if (!assigned)
        {
            // 有限热探测失败不等于满载；授予方完整观察正文池后才发布满载提示。
            ProcessWorkAtomic::replace(&m_control->m_full, 0, 1);
            break;
        }
    }
    for (uint32_t work = 0; work < 64; ++work)
    {
        uint32_t waiting = 0, index = 0;
        ProcessWorkSharedStream* stream = pending(1, waiting);
        if (stream == nullptr || !reserve(ProcessWorkAtomic::load(&stream->m_owner), extent, m_nodeCursor, index, true))
        {
            break;
        }
        m_nodeCursor = index + 1;
        grant(1, *stream, index);
    }
    if (m_pendingHead[0] == 0 && m_pendingHead[1] == 0)
    {
        // 只确认完整扫描观察到的代次，之后发布的请求不会被此store抹掉。
        ProcessWorkAtomic::store(&m_control->m_servedEpoch, m_seenRequestEpoch);
    }
}

void ProcessWorkSharedQueue::releaseBodyToWaiter(uint32_t index, ProcessWorkSharedBlock& body)
{
    if (hasWaiters())
    {
        refreshRequests();
        uint32_t waiting = 0;
        ProcessWorkSharedStream* stream = pending(0, waiting);
        if (stream != nullptr)
        {
            // 不经过FREE状态，快到的新发送者无法夺走已经排队者的授予。
            ProcessWorkAtomic::store(&body.m_owner, ProcessWorkAtomic::load(&stream->m_owner));
            grant(0, *stream, index);
            return;
        }
    }
    ProcessWorkAtomic::store(&body.m_owner, int64_t(0));
}

void ProcessWorkSharedQueue::releaseNode(uint32_t index)
{
    ProcessWorkSharedNode* value = node(index);
    if (hasWaiters())
    {
        refreshRequests();
        uint32_t waiting = 0;
        ProcessWorkSharedStream* stream = pending(1, waiting);
        if (stream != nullptr)
        {
            if (resetNode(*value))
            {
                ProcessWorkAtomic::store(&value->m_owner, ProcessWorkAtomic::load(&stream->m_owner));
                grant(1, *stream, index);
            }
            return;
        }
    }
    ProcessWorkAtomic::store(&value->m_owner, int64_t(0));
}

bool ProcessWorkSharedQueue::refreshStreams()
{
    const int64_t epoch = ProcessWorkAtomic::load(&m_control->m_streamEpoch);
    if (epoch == m_seenStreamEpoch)
    {
        return true;
    }
    try
    {
        const uint32_t count = static_cast<uint32_t>((std::min)(ProcessWorkAtomic::load(&m_control->m_nextStream), int64_t(0xffffff)));
        if (m_readers.size() <= count)
        {
            m_readers.resize(static_cast<size_t>(count) + 1);
        }
        for (uint32_t index = 1; index <= count; ++index)
        {
            if (m_readers[index])
            {
                continue;
            }
            ProcessWorkSharedStream* stream = m_storage->stream(index);
            if (stream != nullptr && ProcessWorkAtomic::load(&stream->m_owner) > 0)
            {
                std::unique_ptr<ProcessWorkReadState> state(new ProcessWorkReadState(index, stream));
                m_active.push_back(state.get());
                m_readers[index] = std::move(state);
            }
        }
        m_seenStreamEpoch = epoch;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void ProcessWorkSharedQueue::compactStreams()
{
    if (!m_compactStreams)
    {
        return;
    }
    ProcessWorkReadState* current = m_readCursor < m_active.size() ? m_active[m_readCursor] : nullptr;
    size_t retained = 0;
    size_t inRound = 0;
    size_t next = m_active.size();
    for (size_t i = 0; i < m_active.size(); ++i)
    {
        ProcessWorkReadState* reader = m_active[i];
        if (ProcessWorkAtomic::load(&reader->m_shared->m_owner) > 0)
        {
            if (i >= m_readCursor && next == m_active.size())
            {
                next = retained;
            }
            if (i < m_roundCount)
            {
                ++inRound;
            }
            m_active[retained++] = reader;
        }
    }
    m_active.resize(retained);
    m_roundCount = inRound;
    m_readCursor = next < retained ? next : 0;
    if (m_roundCount == 0 || m_readCursor >= m_roundCount)
    {
        m_readCursor = 0;
        m_roundCount = retained;
    }
    if (retained == 0 || m_active[m_readCursor] != current)
    {
        m_readQuota = 0;
        m_readBytes = 0;
    }
    // 删除别的流不能重置当前流已用配额，否则持续退出的线程会使当前流一直续杯。
    m_compactStreams = false;
}

void ProcessWorkSharedQueue::recoverToken(int64_t token)
{
    const uint32_t count = static_cast<uint32_t>((std::min)(ProcessWorkAtomic::load(&m_control->m_nextStream), int64_t(0xffffff)));
    for (uint32_t index = 1; index <= count; ++index)
    {
        ProcessWorkSharedStream* stream = m_storage->stream(index);
        if (stream == nullptr)
        {
            continue;
        }
        const int64_t owner = ProcessWorkAtomic::load(&stream->m_owner);
        if (owner == token)
        {
            ProcessWorkAtomic::store(&stream->m_dead, int64_t(1));
            ProcessWorkAtomic::store(&stream->m_cancel, int64_t(1));
        }
        else if (owner == -token)
        {
            // 未完成登记不能有任何提交；只在确认进程死亡之后释放其独占的初始段。
            ProcessWorkAtomic::store(&stream->m_dead, int64_t(1));
            ProcessWorkAtomic::store(&stream->m_head, int64_t(0));
            ProcessWorkAtomic::store(&stream->m_first.m_owner, int64_t(0));
            ProcessWorkAtomic::store(&stream->m_owner, int64_t(0));
        }
    }
    // owner发布后、登记通知递增前也可能崩溃；强制发现这些已经完整发布的死亡流。
    ProcessWorkAtomic::increment(&m_control->m_streamEpoch);
    ProcessWorkAtomic::increment(&m_control->m_requestEpoch);
    refreshRequests();
}

bool ProcessWorkSharedQueue::markCommitted(uint32_t extent, std::vector<unsigned char>& nodes, std::vector<unsigned char>& bodies)
{
    nodes.assign(nodeCount(extent), 0);
    bodies.assign(static_cast<size_t>(extent) + 1, 0);
    const uint32_t count = static_cast<uint32_t>(nodes.size()) - extent - 1;
    for (uint32_t streamIndex = 1; streamIndex <= count; ++streamIndex)
    {
        ProcessWorkSharedStream* stream = m_storage->stream(streamIndex);
        if (stream == nullptr || ProcessWorkAtomic::load(&stream->m_owner) <= 0)
        {
            continue;
        }
        uint32_t encoded = static_cast<uint32_t>(ProcessWorkAtomic::load(&stream->m_head));
        bool firstNode = true;
        for (uint32_t visited = 0; encoded != 0 && visited < nodes.size(); ++visited)
        {
            const uint32_t index = encoded - 1;
            const uint32_t ordinal = index <= 0xffffff ? index : extent + 1 + index - 0x1000001;
            if (ordinal >= nodes.size() || nodes[ordinal] != 0)
            {
                return false;
            }
            ProcessWorkSharedNode* value = node(index);
            if (value == nullptr)
            {
                return false;
            }
            nodes[ordinal] = 1;
            const int32_t start = firstNode ? static_cast<int32_t>(stream->m_headSlot) : 0;
            if (start < 0 || start > kProcessWorkSegmentCells)
            {
                return false;
            }
            for (int32_t cell = start; cell < kProcessWorkSegmentCells; ++cell)
            {
                const uint32_t body = static_cast<uint32_t>(ProcessWorkAtomic::load(&value->m_cells[cell])) & 0xffffffU;
                if (body != 0 && body <= extent)
                {
                    bodies[body] = 1;
                }
            }
            firstNode = false;
            encoded = static_cast<uint32_t>(ProcessWorkAtomic::load(&value->m_link));
        }
        if (encoded != 0)
        {
            return false;
        }
    }
    return true;
}
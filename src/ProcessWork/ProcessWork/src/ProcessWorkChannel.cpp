#include "ProcessWorkChannel.h"
#include "AtomicMath/AtomicMathAPI.h"
#include "ProcessWorkAtomic.h"
#include "ProcessWorkStorage.h"
#include "ProcessWorkSharedQueue.h"
#include "ProcessWorkReceiveQueue.h"
#include "ProcessWorkTime.h"
#include <algorithm>
#include "CSystem/CSystemAPI.h"
#include <cstring>
#include <limits.h>
#include <cstddef>
#include <utility>
#include <thread>
#include <new>

static_assert(sizeof(ProcessWorkSharedDirectory) == 24, "v5 directory layout");
static_assert(sizeof(ProcessWorkSharedPeer) == 24, "v5 peer layout");
static_assert(sizeof(ProcessWorkSharedControl) == 6528, "v5 control layout");
static_assert(sizeof(ProcessWorkSharedBlock) == 64 && sizeof(ProcessWorkSharedNode) == 256, "v5 independent owner cache lines");
static_assert(sizeof(ProcessWorkSharedStream) == 384, "v5 stream layout");
static_assert(sizeof(ProcessWorkSharedMetadata) == 180288, "v5 metadata layout");
static_assert(offsetof(ProcessWorkSharedControl, m_nextStream) == 64 && offsetof(ProcessWorkSharedControl, m_streamEpoch) == 128, "v5 producer registry isolation");

#ifdef _MSC_VER
static __declspec(thread) uint32_t s_reservationCursor = 0;
static __declspec(thread) uint32_t s_nodeReservationCursor = 0;
static __declspec(thread) uint32_t s_sendBurst = 0;
#else
static thread_local uint32_t s_reservationCursor = 0;
static thread_local uint32_t s_nodeReservationCursor = 0;
static thread_local uint32_t s_sendBurst = 0;
#endif

/** 产生线程私有扫描起点；它只是提示，不承担容量预留或消息排序
@param [in] pid 本进程PID，用于分散不同进程的初始位置
@return 可回绕的无符号提示值，实际所有权仍由共享CAS决定
*/
static uint32_t ChannelReservationStart(int32_t pid)
{
    if (s_reservationCursor == 0)
    {
        s_reservationCursor = (CSystem::SystemThreadId() ^ static_cast<uint32_t>(pid)) * 2654435761U + 1;
    }
    return s_reservationCursor++;
}

/** 取得本进程单调毫秒。
@return 不受墙上时钟调整影响的时间
*/
static uint64_t ChannelNow()
{
    return ProcessWorkTime::milliseconds();
}

/** 复制内联小正文，固定宽度memcpy避免旧编译器为每条短消息调用通用复制例程
@param [out] target 至少length个可写字节，与source不重叠
@param [in] source 至少length个可读字节；length为0时可以为nullptr
@param [in] length 0..36字节；只读取声明范围，不对齐访问交给memcpy处理
*/
static void ChannelCopyInline(char* target, const char* source, uint32_t length)
{
    while (length >= 8)
    {
        std::memcpy(target, source, 8);
        target += 8;
        source += 8;
        length -= 8;
    }
    if ((length & 4) != 0)
    {
        std::memcpy(target, source, 4);
        target += 4;
        source += 4;
    }
    if ((length & 2) != 0)
    {
        std::memcpy(target, source, 2);
        target += 2;
        source += 2;
    }
    if ((length & 1) != 0)
    {
        *target = *source;
    }
}

/** 通过已验证跨进程原语读取64位值。
@param [in] value 8字节对齐共享字段
@return 原子快照
*/
static int64_t ChannelLoad64(int64_t* value)
{
    return ProcessWorkAtomic::load(value);
}

/** 通过跨进程原语读取32位值。
@param [in] value 对齐共享字段
@return 原子快照
*/
static int32_t ChannelLoad32(int32_t* value)
{
    return ProcessWorkAtomic::load(value);
}

/** 确认当前平台提供共享控制字段需要的原生原子宽度
@return false 时拒绝建立通道，避免回退到只在本进程有效的隐藏锁
*/
static bool ChannelAtomicsSupported()
{
    std::atomic<int32_t> probe32(0);
    std::atomic<int64_t> probe64(0);
    return probe32.is_lock_free() && probe64.is_lock_free();
}

ProcessWorkChannel::ProcessWorkChannel() :
m_receiver(false), m_stopped(false), m_pid(0), m_identity(0), m_selfIdentity(0), m_session(0),
m_selfPid(0), m_peerIndex(-1), m_yieldThreshold(16), m_directory(nullptr), m_control(nullptr),
m_nextGrowth(0), m_growthRemaining(0), m_successfulGrowth(0), m_nextMaintenance(0), m_peerCursor(0),
m_readPage(nullptr), m_readPageIndex(UINT32_MAX),
m_nextLivenessCheck(0), m_retryDelay(false)
{
    const uint32_t processors = std::thread::hardware_concurrency();
    if (processors != 0)
    {
        m_yieldThreshold = (std::max)(uint32_t(2), processors / 2);
    }
}

ProcessWorkChannel::~ProcessWorkChannel()
{
    close();
}

std::string ProcessWorkChannel::objectName(char kind, uint32_t pid, uint64_t identity,
    uint32_t session, uint32_t index, uint32_t generation)
{
    return ProcessWorkStorage::objectName(kind, pid, identity, session, index, generation);
}

bool ProcessWorkChannel::openReceiver(int32_t bytes, int32_t initial, int32_t threshold,
    int32_t increment, int32_t idleSeconds, int32_t rounds)
{
    return openReceiver(bytes, initial, threshold, increment, idleSeconds, rounds, ProcessWorkMemoryConfig());
}

bool ProcessWorkChannel::openReceiver(int32_t bytes, int32_t initial, int32_t threshold,
    int32_t increment, int32_t idleSeconds, int32_t rounds, const ProcessWorkMemoryConfig& memory)
{
    const int64_t limit = rounds == -1 ? -1 : static_cast<int64_t>(initial) + static_cast<int64_t>(increment) * rounds;
    if (m_control != nullptr || bytes < 12 || bytes == INT32_MAX || initial < 1 || initial > 0xffffff ||
        threshold < 0 || increment < 0 || idleSeconds < 0 || rounds < -1 || limit > 0xffffff ||
        ((increment == 0) != (rounds == 0)) || !ProcessWorkMemory::valid(memory))
    {
        return false;
    }
    // 同时估算地址空间占用，不能先申请海量初始块再发现系统已承受不了。
    // 完全内联的小包不创建逐正文映射；仅计元数据及管理余量，不能按不存在的映射重复预扣。
    const uint64_t bodyBytes = static_cast<size_t>(bytes) <= sizeof(static_cast<ProcessWorkSharedBlock*>(nullptr)->m_inlineData) ?
        0 : (static_cast<uint64_t>(bytes) + 65536) * initial;
    const uint64_t initialBytes = bodyBytes + 2 * 65536 +
        (static_cast<uint64_t>(initial) / 256 + 1) * (sizeof(ProcessWorkSharedMetadata) + 65536);
    if (!ProcessWorkMemory::allows(memory, initialBytes))
    {
        return false;
    }
    m_memoryConfig = memory;
    try
    {
        if (!ChannelAtomicsSupported())
        {
            return false;
        }
        m_pid = CSystem::currentProcessPid();
        if (!CSystem::processIdentity(m_pid, m_identity))
        {
            return false;
        }
        m_directoryMemory.reset(new SharedMemory("P3D" + std::to_string(m_pid), sizeof(ProcessWorkSharedDirectory), true));
        m_directoryMemory->preserveName();
        m_directory = static_cast<ProcessWorkSharedDirectory*>(m_directoryMemory->writeWithoutLock());
        if (m_directory == nullptr || m_directoryMemory->realSize() < sizeof(ProcessWorkSharedDirectory))
        {
            close();
            return false;
        }
        int64_t* directoryIdentity = reinterpret_cast<int64_t*>(&m_directory->m_identity);
        const int64_t previous = ChannelLoad64(&m_directory->m_session);
        const int64_t previousIdentity = ChannelLoad64(directoryIdentity);
        if (static_cast<uint64_t>(previousIdentity) != m_identity)
        {
            // 先发布本进程身份再争用准备标志，禁止两个初始化者把尚未写身份的-1同时认领。
            if (!AtomicMath::compareAndSwap(directoryIdentity, previousIdentity, static_cast<int64_t>(m_identity)))
            {
                close();
                return false;
            }
            if (previous != 0)
            {
                AtomicMath::compareAndSwap(&m_directory->m_session, previous, static_cast<int64_t>(0));
            }
        }
        if (!AtomicMath::compareAndSwap(&m_directory->m_session, static_cast<int64_t>(0), static_cast<int64_t>(-1)))
        {
            close();
            return false;
        }
        m_receiver = true;
        if (ChannelLoad64(&m_directory->m_nextSession) < 0 || ChannelLoad64(&m_directory->m_nextSession) >= 0xffffff)
        {
            close();
            return false;
        }
        m_session = AtomicMath::selfAddOne(&m_directory->m_nextSession);
        if (m_session <= 0 || m_session > 0xffffff)
        {
            close();
            return false;
        }
        m_controlMemory.reset(new SharedMemory(objectName('C', m_pid, m_identity, static_cast<uint32_t>(m_session)),
            sizeof(ProcessWorkSharedControl), false));
        m_control = static_cast<ProcessWorkSharedControl*>(m_controlMemory->writeWithoutLock());
        if (m_control == nullptr || m_controlMemory->isFailed() || m_controlMemory->realSize() < sizeof(ProcessWorkSharedControl))
        {
            close();
            return false;
        }
        std::memset(m_control, 0, sizeof(ProcessWorkSharedControl));
        m_control->m_version = 6;
        m_control->m_bytes = sizeof(ProcessWorkSharedControl);
        m_control->m_blockBytes = bytes;
        // 积压额度小于最大包长仍允许初始化，单条永远放不下的消息在发送端接纳前拒绝。
        m_control->m_maxPayloadBytes = memory.m_localQueueBytes == -1 ? bytes - 12 :
            static_cast<int32_t>((std::min)(static_cast<int64_t>(bytes - 12),
                (std::max)(int64_t(-1), memory.m_localQueueBytes - 277)));
        m_control->m_initial = initial;
        m_control->m_limit = static_cast<int32_t>(limit);
        m_control->m_threshold = threshold;
        m_control->m_increment = increment;
        m_control->m_idleSeconds = idleSeconds;
        m_storage.reset(new ProcessWorkStorage(m_pid, m_identity, static_cast<uint32_t>(m_session), bytes, initial, true, memory));
        m_sharedQueue.reset(new ProcessWorkSharedQueue(*m_storage, *m_control));
        if (!m_sharedQueue->configureProducers(m_producerConfig) || !m_sharedQueue->initialize())
        {
            close();
            return false;
        }
        if (m_dataReady.createProcessResult(objectName('R', m_pid, m_identity, static_cast<uint32_t>(m_session))).status != SEMAPHORE_OK ||
            m_spaceReady.createProcessResult(objectName('W', m_pid, m_identity, static_cast<uint32_t>(m_session))).status != SEMAPHORE_OK)
        {
            close();
            return false;
        }
        for (int32_t i = 1; i <= initial; ++i)
        {
            if (!createBlock(i))
            {
                close();
                return false;
            }
        }
        return true;
    }
    catch (...)
    {
        close();
        return false;
    }
}

bool ProcessWorkChannel::createBlock(int32_t index)
{
    if (!m_storage->create(index, ChannelNow()) || !m_sharedQueue->activate(index))
    {
        return false;
    }
    AtomicMath::selfAddOne(&m_control->m_active);
    const int32_t extent = ChannelLoad32(&m_control->m_extent);
    if (index > extent)
    {
        ProcessWorkAtomic::replace(&m_control->m_extent, extent, index);
    }
    signalSpace();
    return true;
}

void ProcessWorkChannel::publish()
{
    if (m_receiver && !m_stopped && m_control != nullptr)
    {
        AtomicMath::compareAndSwap(&m_control->m_ready, 0, 1);
        AtomicMath::compareAndSwap(&m_directory->m_session, static_cast<int64_t>(-1), m_session);
    }
}

void ProcessWorkChannel::stop()
{
    if (!m_receiver)
    {
        return;
    }
    m_stopped = true;
    if (m_control != nullptr)
    {
        AtomicMath::compareAndSwap(&m_control->m_ready, 1, 0);
    }
    signalData();
    signalSpace();
}

void ProcessWorkChannel::close()
{
    stop();
    m_sharedQueue.reset();
    m_readPage = nullptr;
    m_readPageIndex = UINT32_MAX;
    m_storage.reset();
    m_growthRemaining = 0;
    m_successfulGrowth = 0;
    m_nextGrowth = 0;
    m_nextMaintenance = 0;
    m_control = nullptr;
    m_controlMemory.reset();
    m_dataReady.closeProcessSemaphore();
    m_spaceReady.closeProcessSemaphore();
    if (m_receiver && m_directory != nullptr)
    {
        // POSIX先撤入口，旧映射保留占用标志，禁止迟到的打开者在已撤名对象上发布。
        // Windows依靠内核引用管理名称，原入口保留代次计数并解除占用。
        if (!m_directoryMemory->unlinkName())
        {
            // 只释放一次；发布0后新实例可能立即认领-1，旧实例不能再次清除它。
            const int64_t ownedSession = ChannelLoad64(&m_directory->m_session);
            if (ownedSession == m_session || ownedSession == -1)
            {
                AtomicMath::compareAndSwap(&m_directory->m_session, ownedSession, static_cast<int64_t>(0));
            }
        }
    }
    m_directory = nullptr;
    m_directoryMemory.reset();
    m_receiver = false;
    m_stopped = false;
    m_session = 0;
    m_peerIndex = -1;
}

bool ProcessWorkChannel::configureProducers(const LockFreeMapConfig& config)
{
    if (m_control != nullptr || m_storage || m_sharedQueue)
    {
        return false;
    }
    try
    {
        m_producerConfig = config;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ProcessWorkChannel::openSender(int32_t pid)
{
    if (pid <= 0 || m_control != nullptr || !ChannelAtomicsSupported())
    {
        return false;
    }
    try
    {
        m_pid = pid;
        m_selfPid = CSystem::currentProcessPid();
        if (!CSystem::processIdentity(pid, m_identity) || !CSystem::processIdentity(m_selfPid, m_selfIdentity))
        {
            return false;
        }
        m_directoryMemory.reset(new SharedMemory("P3D" + std::to_string(pid)));
        m_directory = static_cast<ProcessWorkSharedDirectory*>(m_directoryMemory->writeWithoutLock());
        if (m_directory == nullptr || m_directoryMemory->realSize() < sizeof(ProcessWorkSharedDirectory))
        {
            close();
            return false;
        }
        m_session = ChannelLoad64(&m_directory->m_session);
        if (m_session <= 0 || m_session > 0xffffff || static_cast<uint64_t>(ChannelLoad64(reinterpret_cast<int64_t*>(&m_directory->m_identity))) != m_identity)
        {
            close();
            return false;
        }
        m_controlMemory.reset(new SharedMemory(objectName('C', pid, m_identity, static_cast<uint32_t>(m_session))));
        m_control = static_cast<ProcessWorkSharedControl*>(m_controlMemory->writeWithoutLock());
        if (m_control == nullptr || m_controlMemory->realSize() < sizeof(ProcessWorkSharedControl) || !ready() ||
            m_control->m_version != 6 || m_control->m_bytes != sizeof(ProcessWorkSharedControl) ||
            m_control->m_initial < 1 || m_control->m_initial > 0xffffff ||
            (m_control->m_limit != -1 && (m_control->m_limit < m_control->m_initial || m_control->m_limit > 0xffffff)) ||
            ChannelLoad32(&m_control->m_extent) < m_control->m_initial || ChannelLoad32(&m_control->m_extent) > 0xffffff ||
            m_control->m_blockBytes < 12 || m_control->m_blockBytes == INT32_MAX ||
            m_control->m_maxPayloadBytes < -1 || m_control->m_maxPayloadBytes > m_control->m_blockBytes - 12)
        {
            close();
            return false;
        }
        if (m_dataReady.openProcessResult(objectName('R', pid, m_identity, static_cast<uint32_t>(m_session))).status != SEMAPHORE_OK ||
            m_spaceReady.openProcessResult(objectName('W', pid, m_identity, static_cast<uint32_t>(m_session))).status != SEMAPHORE_OK)
        {
            close();
            return false;
        }
        m_storage.reset(new ProcessWorkStorage(pid, m_identity, static_cast<uint32_t>(m_session),
            m_control->m_blockBytes, m_control->m_initial, false, ProcessWorkMemoryConfig()));
        m_sharedQueue.reset(new ProcessWorkSharedQueue(*m_storage, *m_control));
        if (!m_sharedQueue->configureProducers(m_producerConfig) || m_storage->page(0) == nullptr)
        {
            close();
            return false;
        }
        // 登记只在open阶段选择下标，发送热路径不修改本地共享指针或容器。
        for (int32_t pass = 0; pass < 2 && m_peerIndex < 0; ++pass)
        {
            for (int32_t i = 0; i < 256; ++i)
            {
                ProcessWorkSharedPeer& peer = m_control->m_peers[i];
                // 申请轮也复查同PID：另一个本进程通道可能刚在此处赢得CAS。
                if (pass == 1)
                {
                    AtomicMath::compareAndSwap(&peer.m_pid, 0, m_selfPid);
                }
                if (ChannelLoad32(&peer.m_pid) == m_selfPid &&
                    (ChannelLoad64(&peer.m_token) == 0 || static_cast<uint64_t>(ChannelLoad64(reinterpret_cast<int64_t*>(&peer.m_identity))) == m_selfIdentity) &&
                    ChannelLoad32(&peer.m_pid) == m_selfPid)
                {
                    m_peerIndex = i;
                    break;
                }
            }
        }
        if (m_peerIndex < 0)
        {
            close();
            return false;
        }
        signalData();
        if (!ready())
        {
            // 打开末尾与接收器停止交错时，失败对象也必须回滚，允许原对象重试。
            close();
            return false;
        }
        return true;
    }
    catch (...)
    {
        close();
        return false;
    }
}

bool ProcessWorkChannel::ready() const
{
    return m_directory != nullptr && m_control != nullptr && ChannelLoad32(&m_control->m_ready) == 1 &&
        ChannelLoad64(&m_directory->m_session) == m_session && static_cast<uint64_t>(ChannelLoad64(reinterpret_cast<int64_t*>(&m_directory->m_identity))) == m_identity;
}

int32_t ProcessWorkChannel::payloadCapacity() const
{
    const int32_t bytes = ready() ? m_control->m_blockBytes : 0;
    return bytes >= 12 && bytes < INT32_MAX ? bytes - 12 : 0;
}

bool ProcessWorkChannel::matchesIdentity(uint64_t identity) const
{
    return identity == m_identity && ready();
}

int64_t ProcessWorkChannel::senderToken()
{
    if (m_peerIndex < 0)
    {
        return 0;
    }
    ProcessWorkSharedPeer& peer = m_control->m_peers[m_peerIndex];
    const int64_t token = ChannelLoad64(&peer.m_token);
    return token > 0 && static_cast<uint64_t>(ChannelLoad64(reinterpret_cast<int64_t*>(&peer.m_identity))) == m_selfIdentity &&
        ChannelLoad32(&peer.m_pid) == m_selfPid ? token : 0;
}

int32_t ProcessWorkChannel::trySend(const char* data, int32_t length, int32_t type,
    bool retryContention, bool retryCapacity, const std::atomic<bool>* stop)
{
    uint32_t contention = 0;
    uint32_t capacity = 0;
    while (stop == nullptr || !stop->load(std::memory_order_acquire))
    {
        const int32_t result = transmit(data, length, type, retryContention, retryCapacity, capacity, stop);
        if (result == PROCESS_WORK_BUSY && retryContention)
        {
            if (++contention == 64)
            {
                CSystem::Sleep(1);
                contention = 0;
            }
            else
            {
                std::this_thread::yield();
            }
            continue;
        }
        if (retryCapacity && (result == PROCESS_WORK_CAPACITY || result == PROCESS_WORK_RESOURCE))
        {
            if (!ProcessWorkTime::waitCapacity(capacity) ||
                (capacity == 4096 && !waitForSpaceImpl(0, false)))
            {
                return PROCESS_WORK_UNAVAILABLE;
            }
            continue;
        }
        return result;
    }
    return PROCESS_WORK_UNAVAILABLE;
}

int32_t ProcessWorkChannel::send(const char* data, int32_t length, int32_t type, const std::atomic<bool>* stop)
{
    return trySend(data, length, type, true, true, stop);
}

bool ProcessWorkChannel::waitForGrant(ProcessWorkProducerState& producer, bool node, uint32_t& index, uint32_t& attempts, const std::atomic<bool>* stop)
{
    // 只有发布过授予请求的慢路径才需要取消守卫；借用登记不在此销毁。
    auto cancelRequest = [this](ProcessWorkProducerState* value)
    {
        if (ProcessWorkAtomic::load(&value->m_shared->m_request) != 0)
        {
            m_sharedQueue->cancelRequest(*value);
            signalData();
        }
    };
    std::unique_ptr<ProcessWorkProducerState, decltype(cancelRequest)> requestScope(&producer, cancelRequest);
    m_sharedQueue->request(producer, node);
    signalData();
    while (ready() && (stop == nullptr || !stop->load(std::memory_order_acquire)))
    {
        if (m_sharedQueue->takeGrant(producer, node, index))
        {
            return true;
        }
        if (!ProcessWorkTime::waitCapacity(attempts))
        {
            break;
        }
        if (attempts == 4096)
        {
            // 休眠后仍检查进程身份；对端崩溃不能留下永远等待的请求。
            if (!waitForSpaceImpl(0, false))
            {
                break;
            }
            signalData();
        }
    }
    return false;
}

int32_t ProcessWorkChannel::unavailableResource(bool node, uint32_t extent, bool contended)
{
    bool mappingUnavailable = false;
    const uint32_t count = node ? m_sharedQueue->nodeCount(extent) : extent;
    for (uint32_t position = 0; position < count; ++position)
    {
        const uint32_t index = node ? m_sharedQueue->nodeAt(position, extent) : position + 1;
        const int64_t* owner = nullptr;
        if (node)
        {
            ProcessWorkSharedNode* value = m_sharedQueue->node(index);
            owner = value == nullptr ? nullptr : &value->m_owner;
        }
        else
        {
            ProcessWorkSharedBlock* value = m_storage->block(index);
            owner = value == nullptr ? nullptr : &value->m_owner;
        }
        if (owner == nullptr)
        {
            mappingUnavailable = true;
        }
        else if (ProcessWorkAtomic::load(owner) == 0)
        {
            return contended ? PROCESS_WORK_BUSY : PROCESS_WORK_RESOURCE;
        }
    }
    return mappingUnavailable ? PROCESS_WORK_RESOURCE : PROCESS_WORK_CAPACITY;
}

int32_t ProcessWorkChannel::transmit(const char* data, int32_t length, int32_t type,
    bool retryContention, bool retryCapacity, uint32_t& capacityAttempts, const std::atomic<bool>* stop)
{
    if (length < 0 || (length != 0 && data == nullptr) || type < 0)
    {
        return PROCESS_WORK_INVALID;
    }
    if (m_receiver || !ready())
    {
        return PROCESS_WORK_UNAVAILABLE;
    }
    if (length > m_control->m_maxPayloadBytes)
    {
        return PROCESS_WORK_TOO_LARGE;
    }
    const int64_t token = senderToken();
    if (token == 0)
    {
        signalData();
        return ChannelLoad64(&m_control->m_nextToken) == INT64_MAX ? PROCESS_WORK_UNAVAILABLE : PROCESS_WORK_RESOURCE;
    }
    int32_t preparation = PROCESS_WORK_RESOURCE;
    ProcessWorkProducerState* producer = m_sharedQueue->producer(token, &preparation, retryContention);
    if (producer == nullptr)
    {
        signalData();
        m_retryDelay.store(true, std::memory_order_relaxed);
        return preparation;
    }
    // 取消与授予可能交错；旧取消由接收器确认之前，不重新使用同一请求记录。
    if (ProcessWorkAtomic::load(&producer->m_shared->m_request) != 0)
    {
        signalData();
        return PROCESS_WORK_RESOURCE;
    }
    const uint32_t extent = static_cast<uint32_t>(ChannelLoad32(&m_control->m_extent));
    const uint32_t start = ChannelReservationStart(m_selfPid);
    if (extent == 0)
    {
        return PROCESS_WORK_UNAVAILABLE;
    }
    uint32_t index = start < extent ? start + 1 : start % extent + 1;
    ProcessWorkSharedBlock* block = nullptr;
    bool contended = false;
    for (uint32_t step = 0; step < extent; ++step)
    {
        if (producer->m_bodyPage == nullptr || producer->m_bodyPageIndex != (index >> 8))
        {
            producer->m_bodyPage = m_storage->page(index);
            producer->m_bodyPageIndex = index >> 8;
        }
        ProcessWorkSharedBlock* candidate = producer->m_bodyPage == nullptr ? nullptr :
            &producer->m_bodyPage->m_metadata->m_blocks[index & 255];
        if (candidate != nullptr && ProcessWorkAtomic::load(&candidate->m_owner) == 0)
        {
            if (ProcessWorkAtomic::replace(&candidate->m_owner, int64_t(0), token))
            {
                block = candidate;
                break;
            }
            contended = true;
        }
        index = index == extent ? 1 : index + 1;
    }
    if (block == nullptr)
    {
        ProcessWorkAtomic::replace(&m_control->m_demand, 0, 1);
        signalData();
        const int32_t failure = unavailableResource(false, extent, contended);
        if (failure == PROCESS_WORK_CAPACITY)
        {
            ProcessWorkAtomic::replace(&m_control->m_full, 0, 1);
        }
        if (!retryCapacity || (failure == PROCESS_WORK_BUSY && !retryContention))
        {
            return failure;
        }
        if (!waitForGrant(*producer, false, index, capacityAttempts, stop))
        {
            return PROCESS_WORK_UNAVAILABLE;
        }
        block = m_storage->block(index);
        if (block == nullptr || ProcessWorkAtomic::load(&block->m_owner) != token)
        {
            return PROCESS_WORK_UNAVAILABLE;
        }
    }
    const bool inlined = static_cast<size_t>(length) + 12 <= sizeof(block->m_inlineData);
    // 内联正文优先复用最近256槽；独立正文映射保留分散步长，避免过早追上正在读取的槽。
    // 两者只改变下次扫描提示，忙时仍检查整个共享池，不私占容量或跳过已排队授予。
    if (inlined)
    {
        s_reservationCursor = (index & 255U) != 0 ? index : index - 256;
    }
    else
    {
        s_reservationCursor = index + ((producer->m_index - 1) & 3U) * 2;
    }
    // 参数为本次尚未交付的资源；按对应所有者规则释放，不传播异常。
    auto releaseBlock = [this, token](ProcessWorkSharedBlock* value)
    {
        ProcessWorkAtomic::replace(&value->m_owner, token, int64_t(0));
        signalSpace();
    };
    std::unique_ptr<ProcessWorkSharedBlock, decltype(releaseBlock)> bodyReservation(block, releaseBlock);
    std::unique_ptr<SharedMemory> temporary;
    char* target = inlined ? block->m_inlineData :
        m_storage->data(index, static_cast<uint32_t>(ChannelLoad64(&block->m_generation)), temporary);
    if (target == nullptr)
    {
        // 已取得独占正文槽，映射失败不是CAS竞争；RAII归还槽后让调用者处理资源失败。
        return PROCESS_WORK_NO_MEMORY;
    }
    const int32_t header[3] = { m_selfPid, type, length };
    std::memcpy(target, header, sizeof(header));
    if (inlined)
    {
        ChannelCopyInline(target + 12, data, static_cast<uint32_t>(length));
    }
    else if (length != 0)
    {
        std::memcpy(target + 12, data, length);
    }
    if (s_nodeReservationCursor == 0)
    {
        s_nodeReservationCursor = start;
    }
    if (!ready())
    {
        return PROCESS_WORK_UNAVAILABLE;
    }
    bool nodeContended = false;
    if (!m_sharedQueue->enqueue(*producer, index, inlined, extent, s_nodeReservationCursor, UINT32_MAX, &nodeContended))
    {
        const int32_t failure = unavailableResource(true, extent, nodeContended);
        if (!retryCapacity || (failure == PROCESS_WORK_BUSY && !retryContention))
        {
            return failure;
        }
        uint32_t granted = 0;
        if (!waitForGrant(*producer, true, granted, capacityAttempts, stop))
        {
            return PROCESS_WORK_UNAVAILABLE;
        }
        // 参数仅用于RAII持有候选；尚未链接时由原token归还段。
        auto cancelNode = [this, token, granted](ProcessWorkSharedNode*)
        {
            m_sharedQueue->cancel(granted, token);
        };
        std::unique_ptr<ProcessWorkSharedNode, decltype(cancelNode)> reservedNode(m_sharedQueue->node(granted), cancelNode);
        if (!ready() || (stop != nullptr && stop->load()) ||
            !m_sharedQueue->enqueue(*producer, index, inlined, extent, s_nodeReservationCursor, granted))
        {
            return PROCESS_WORK_UNAVAILABLE;
        }
        reservedNode.release();
    }
    bodyReservation.release();
    // 阈值为0时只有下一次申请失败才请求扩容，最后一个成功发送不提前扩容。
    if (m_control->m_threshold > 0)
    {
        ProcessWorkAtomic::replace(&m_control->m_demand, 0, 1);
    }
    signalData();
    if ((++s_sendBurst & 255U) == 0 && ChannelLoad64(&m_control->m_nextStream) > m_yieldThreshold)
    {
        // 已发布成功；让出当前调度机会，不等待持有者，也不增加定时休眠。
        std::this_thread::yield();
    }
    return PROCESS_WORK_SENT;
}

bool ProcessWorkChannel::waitForSpace()
{
    return waitForSpaceImpl(20);
}

bool ProcessWorkChannel::waitForSpaceImpl(int32_t timeout, bool probeSpace)
{
    if (!ready())
    {
        return false;
    }
    if (m_retryDelay.exchange(false, std::memory_order_relaxed))
    {
        CSystem::Sleep(1);
    }
    ProcessWorkAtomic::replace(&m_control->m_spaceSignal, 0, 1);
    bool available = false;
    // 公平请求只能由接收器授予。别人的空正文不是本请求已获准，
    // 尤其等待队列段时不能因此跳过休眠，反复占用接收器需要的CPU。
    if (probeSpace && senderToken() > 0)
    {
        const int32_t extent = ChannelLoad32(&m_control->m_extent);
        for (int32_t i = 1; i <= extent; ++i)
        {
            ProcessWorkSharedBlock* block = m_storage->block(i);
            if (block != nullptr && ChannelLoad64(&block->m_owner) == 0)
            {
                available = true;
                break;
            }
        }
    }
    if (!available)
    {
        m_spaceReady.processWaitResult(timeout);
    }
    const uint64_t now = ChannelNow();
    uint64_t previous = m_nextLivenessCheck.load(std::memory_order_relaxed);
    if (m_pid != m_selfPid && now >= previous && m_nextLivenessCheck.compare_exchange_strong(previous, now + 20, std::memory_order_relaxed))
    {
        uint64_t current = 0;
        const bool known = CSystem::processIdentity(m_pid, current);
        if ((known && current != m_identity) || CSystem::processDefinitelyExited(m_pid))
        {
            ProcessWorkAtomic::replace(&m_control->m_ready, 1, 0);
            try
            {
                cleanupExitedReceiver();
            }
            catch (...)
            {
                /* 已确认死亡，撤名失败不影响结束等待。 */
            }
            return false;
        }
    }
    return ready();
}

void ProcessWorkChannel::cleanupExitedReceiver()
{
#ifndef _WIN32
    // 只含创建标识和实例号的名称不会属于PID复用后的新接收器。
    // 不解除任何缓存映射，也不撤销可被新进程重新发布的P3D/P3N固定入口。
    if (m_control == nullptr || m_receiver)
    {
        return;
    }
    const int32_t extent = ChannelLoad32(&m_control->m_extent);
    for (int32_t i = 1; i <= extent; ++i)
    {
        ProcessWorkSharedBlock* block = m_storage->block(i);
        const int64_t generation = block == nullptr ? 0 : ChannelLoad64(&block->m_generation);
        if (generation > 0 && generation <= 0xffffff && static_cast<size_t>(m_control->m_blockBytes) > sizeof(block->m_inlineData))
        {
            SharedMemory memory(objectName('B', m_pid, m_identity, static_cast<uint32_t>(m_session), i, static_cast<uint32_t>(generation)));
            memory.unlinkName();
        }
    }
    const int32_t metadataExtent = (std::max)(extent, static_cast<int32_t>((std::min)(ChannelLoad64(&m_control->m_nextStream), int64_t(0xffffff))));
    for (int32_t page = 0; page <= metadataExtent / 256; ++page)
    {
        SharedMemory memory(objectName('M', m_pid, m_identity, static_cast<uint32_t>(m_session), page));
        memory.unlinkName();
    }
    // 多个发送线程可能同时到达；仅以独立临时句柄撤名，不修改共享缓存对象成员。
    SharedMemory control(objectName('C', m_pid, m_identity, static_cast<uint32_t>(m_session)));
    control.unlinkName();
    Semaphore data;
    if (data.tryOpenProcessSemaphore(objectName('R', m_pid, m_identity, static_cast<uint32_t>(m_session))))
    {
        data.unlinkProcessName();
    }
    Semaphore space;
    if (space.tryOpenProcessSemaphore(objectName('W', m_pid, m_identity, static_cast<uint32_t>(m_session))))
    {
        space.unlinkProcessName();
    }
#endif
}

void ProcessWorkChannel::signalData()
{
    if (m_control != nullptr && ChannelLoad32(&m_control->m_dataSignal) == 0 && ProcessWorkAtomic::replace(&m_control->m_dataSignal, 0, 1))
    {
        m_dataReady.tryProcessSignal(true, false);
    }
}

void ProcessWorkChannel::signalSpace()
{
    if (m_control != nullptr && ChannelLoad32(&m_control->m_spaceSignal) == 1 && ProcessWorkAtomic::replace(&m_control->m_spaceSignal, 1, 0))
    {
        m_spaceReady.tryProcessSignal(true, false);
    }
}

void ProcessWorkChannel::wake()
{
    m_dataReady.tryProcessSignal(true, false);
}

void ProcessWorkChannel::waitForData()
{
    if (m_control == nullptr)
    {
        return;
    }
    ProcessWorkAtomic::replace(&m_control->m_dataSignal, 1, 0);
    if (m_stopped.load(std::memory_order_acquire))
    {
        m_dataReady.processWaitResult(20);
        return;
    }
    if (m_retryDelay.exchange(false, std::memory_order_relaxed))
    {
        m_dataReady.processWaitResult(1);
        return;
    }
    uint32_t node = 0;
    int32_t buffer = 0;
    if (!m_sharedQueue->front(node, buffer))
    {
        m_dataReady.processWaitResult(20);
    }
}

int32_t ProcessWorkChannel::bufferCount() const
{
    return m_control == nullptr ? 0 : ChannelLoad32(&m_control->m_active);
}

int32_t ProcessWorkChannel::pump(LockFreeQueue<char*>& queue, Semaphore& notification, std::atomic<int32_t>& fullCount)
{
    return pumpImpl(&queue, notification, fullCount, nullptr);
}

int32_t ProcessWorkChannel::pump(ProcessWorkReceiveQueue& queue, std::atomic<int32_t>& fullCount)
{
    return pumpImpl(nullptr, queue.notification(), fullCount, &queue);
}

int32_t ProcessWorkChannel::pumpImpl(LockFreeQueue<char*>* queue, Semaphore& notification, std::atomic<int32_t>& fullCount,
    ProcessWorkReceiveQueue* batches)
{
    if (!m_receiver || m_control == nullptr || m_stopped.load(std::memory_order_acquire))
    {
        return 0;
    }
    const uint64_t now = ChannelNow();
    if (now >= m_nextMaintenance)
    {
        try
        {
            maintain(notification, fullCount);
        }
        catch (...)
        {
            // 死亡快照等临时资源不足不阻止接收已经提交的正文。
            m_retryDelay.store(true, std::memory_order_relaxed);
        }
        m_nextMaintenance = now + 20;
    }
    m_sharedQueue->service(static_cast<uint32_t>(ChannelLoad32(&m_control->m_extent)));
    signalSpace();
    if (batches != nullptr && !batches->flush())
    {
        m_retryDelay.store(true, std::memory_order_relaxed);
        return 0;
    }
    int32_t copied = 0;
    // 单次搬运有工作预算，避免持续高负载推迟停止请求和维护。
    for (int32_t step = 0; step < 256; ++step)
    {
        uint32_t nodeIndex = 0;
        int32_t index = 0;
        bool inlined = false;
        if (!m_sharedQueue->front(nodeIndex, index, inlined))
        {
            break;
        }
        if (index <= 0 || index > ChannelLoad32(&m_control->m_extent))
        {
            stop();
            break;
        }
        if (m_readPage == nullptr || m_readPageIndex != (static_cast<uint32_t>(index) >> 8))
        {
            m_readPage = m_storage->page(static_cast<uint32_t>(index));
            m_readPageIndex = static_cast<uint32_t>(index) >> 8;
        }
        ProcessWorkSharedBlock* block = m_readPage == nullptr ? nullptr :
            &m_readPage->m_metadata->m_blocks[static_cast<uint32_t>(index) & 255];
        if (block == nullptr)
        {
            stop();
            break;
        }
        const int64_t owner = ChannelLoad64(&block->m_owner);
        // 唯一消费者直接借用已提交正文；维护按未消费单元判断存活，不需再写一次所有者。
        // 本地预算不足时保留提交单元及原token，死进程回收也不能释放该正文。
        if (owner <= 0)
        {
            stop();
            break;
        }
        std::unique_ptr<SharedMemory> temporary;
        char* source = inlined ? block->m_inlineData :
            m_storage->data(index, static_cast<uint32_t>(ChannelLoad64(&block->m_generation)), temporary);
        if (source == nullptr)
        {
            m_retryDelay.store(true, std::memory_order_relaxed);
            break;
        }
        int32_t header[3] = {};
        std::memcpy(header, source, sizeof(header));
        const int32_t capacity = inlined ? (std::min)(m_control->m_blockBytes, static_cast<int32_t>(sizeof(block->m_inlineData))) : m_control->m_blockBytes;
        if (header[0] > 0 && header[1] >= 0 && header[2] >= 0 && header[2] <= capacity - 12)
        {
            try
            {
                if (batches != nullptr)
                {
                    if (!batches->append(source, static_cast<size_t>(header[2]) + 12))
                    {
                        m_retryDelay.store(true, std::memory_order_relaxed);
                        break;
                    }
                }
                else
                {
                    const size_t bytes = static_cast<size_t>(header[2]) + 13;
                    std::unique_ptr<char[]> packet(new (std::nothrow) char[bytes]);
                    if (!packet)
                    {
                        m_retryDelay.store(true, std::memory_order_relaxed);
                        break;
                    }
                    std::memcpy(packet.get(), source, static_cast<size_t>(header[2]) + 12);
                    packet.get()[static_cast<size_t>(header[2]) + 12] = '\0';
                    const LockFreeQueueResult result = queue->tryPush(packet.get());
                    if (result != LockFreeQueueSuccess)
                    {
                        m_retryDelay.store(true, std::memory_order_relaxed);
                        break;
                    }
                    packet.release();
                    notification.trySignal(true, false);
                }
            }
            catch (...)
            {
                // 尚未交付本地队列，保留共享正文和顺序供下次重试。
                m_retryDelay.store(true, std::memory_order_relaxed);
                break;
            }
        }
        if (!m_sharedQueue->consume(nodeIndex, header[2] >= 0 ? static_cast<uint32_t>(header[2]) + 12 : 0))
        {
            // 本地队列已经接收所有权，不能再次复制同一共享正文。
            // 单消费者下不应失败；协议损坏时撤销入口，等待owner关闭重建。
            stop();
            break;
        }
        if (index > m_control->m_initial)
        {
            // 仅扩容槽参与闲置回收；初始槽无需逐包写入从不使用的缩容时间。
            m_readPage->m_idleSince[static_cast<uint32_t>(index) & 255] = 0;
        }
        m_sharedQueue->releaseBody(static_cast<uint32_t>(index), *block);
        signalSpace();
        ++copied;
    }
    if (batches != nullptr && !batches->flush())
    {
        m_retryDelay.store(true, std::memory_order_relaxed);
    }
    return copied;
}

void ProcessWorkChannel::maintain(Semaphore& notification, std::atomic<int32_t>& fullCount)
{
    approveSenders();
    recoverExitedSenders();
    resizeBuffers(ChannelNow(), notification, fullCount);
}

void ProcessWorkChannel::approveSenders()
{
    for (int32_t i = 0; i < 256; ++i)
    {
        ProcessWorkSharedPeer& peer = m_control->m_peers[i];
        const int32_t pid = ChannelLoad32(&peer.m_pid);
        if (pid <= 0 || ChannelLoad64(&peer.m_token) != 0)
        {
            continue;
        }
        uint64_t identity = 0;
        const int64_t previousToken = ChannelLoad64(&m_control->m_nextToken);
        if (((pid == m_pid && (identity = m_identity) != 0) || CSystem::processIdentity(pid, identity)) && previousToken < INT64_MAX)
        {
            int64_t* publishedIdentity = reinterpret_cast<int64_t*>(&peer.m_identity);
            AtomicMath::compareAndSwap(publishedIdentity, ChannelLoad64(publishedIdentity), static_cast<int64_t>(identity));
            AtomicMath::compareAndSwap(&peer.m_token, static_cast<int64_t>(0), previousToken + 1);
            // 唯一登记写者；先批准token再发布计数，耗尽状态不能早于最后一次批准。
            AtomicMath::compareAndSwap(&m_control->m_nextToken, previousToken, previousToken + 1);
            signalSpace();
        }
        else if (CSystem::processDefinitelyExited(pid))
        {
            AtomicMath::compareAndSwap(&peer.m_pid, pid, 0);
        }
    }
}

void ProcessWorkChannel::recoverExitedSenders()
{
    for (int32_t step = 0; step < 16; ++step)
    {
        ProcessWorkSharedPeer& peer = m_control->m_peers[m_peerCursor];
        m_peerCursor = (m_peerCursor + 1) % 256;
        const int32_t pid = ChannelLoad32(&peer.m_pid);
        const int64_t token = ChannelLoad64(&peer.m_token);
        if (pid <= 0 || token <= 0 || pid == m_pid)
        {
            continue;
        }
        uint64_t identity = 0;
        const bool known = CSystem::processIdentity(pid, identity);
        if (!CSystem::processDefinitelyExited(pid) && (!known || identity == static_cast<uint64_t>(ChannelLoad64(reinterpret_cast<int64_t*>(&peer.m_identity)))))
        {
            continue;
        }
        const uint32_t extent = static_cast<uint32_t>(ChannelLoad32(&m_control->m_extent));
        // 只在确认死亡之后检查已提交链；接收器此时不消费，死进程不会新增提交。
        // 任意查询或临时分配失败都保留资源，不以超时或暂停作为死亡证据。
        m_sharedQueue->recoverToken(token);
        std::vector<unsigned char> committedNodes;
        std::vector<unsigned char> committedBodies;
        if (!m_sharedQueue->markCommitted(extent, committedNodes, committedBodies))
        {
            return;
        }
        bool referenced = false;
        const uint32_t streams = static_cast<uint32_t>((std::min)(ChannelLoad64(&m_control->m_nextStream), int64_t(0xffffff)));
        for (uint32_t streamIndex = 1; streamIndex <= streams; ++streamIndex)
        {
            ProcessWorkSharedStream* stream = m_storage->stream(streamIndex);
            referenced = referenced || (stream != nullptr && ChannelLoad64(&stream->m_owner) == token);
        }
        for (uint32_t ordinal = 0; ordinal < committedNodes.size(); ++ordinal)
        {
            ProcessWorkSharedNode* node = m_sharedQueue->node(m_sharedQueue->nodeAt(ordinal, extent));
            if (node != nullptr && ChannelLoad64(&node->m_owner) == token)
            {
                if (committedNodes[ordinal] != 0)
                {
                    referenced = true;
                }
                else
                {
                    ProcessWorkAtomic::replace(&node->m_owner, token, int64_t(0));
                }
            }
        }
        for (uint32_t i = 1; i <= extent; ++i)
        {
            ProcessWorkSharedBlock* block = i == 0 ? nullptr : m_storage->block(i);
            if (block != nullptr && ChannelLoad64(&block->m_owner) == token)
            {
                if (committedBodies[i] != 0)
                {
                    referenced = true;
                }
                else
                {
                    ProcessWorkAtomic::replace(&block->m_owner, token, int64_t(0));
                    *m_storage->idleSince(i) = 0;
                    signalSpace();
                }
            }
        }
        if (!referenced)
        {
            ProcessWorkAtomic::replace(&peer.m_pid, pid, -pid);
            ProcessWorkAtomic::replace(&peer.m_token, token, int64_t(0));
            ProcessWorkAtomic::replace(&peer.m_pid, -pid, 0);
        }
    }
}

void ProcessWorkChannel::resizeBuffers(uint64_t now, Semaphore& notification, std::atomic<int32_t>& fullCount)
{
    const bool demand = ChannelLoad32(&m_control->m_demand) == 1 && ProcessWorkAtomic::replace(&m_control->m_demand, 1, 0);
    const int32_t extent = ChannelLoad32(&m_control->m_extent);
    if (m_control->m_increment == 0 && m_growthRemaining == 0 &&
        (m_control->m_idleSeconds == 0 || extent == m_control->m_initial) && ChannelLoad32(&m_control->m_full) == 0)
    {
        // 固定容量且没有满载通知时，无需周期性遍历所有正文槽或写入闲置时间。
        // 之后才到达的满载请求保留在m_full，由下一次维护正常报告。
        return;
    }
    int32_t free = 0;
    int32_t firstInactive = 0;
    for (int32_t i = 1; i <= extent; ++i)
    {
        ProcessWorkSharedBlock* block = m_storage->block(i);
        if (block == nullptr)
        {
            continue;
        }
        const int64_t owner = ChannelLoad64(&block->m_owner);
        uint64_t* idle = m_storage->idleSince(i);
        if (owner == 0)
        {
            ++free;
            if (*idle == 0)
            {
                *idle = now;
            }
        }
        else
        {
            *idle = now;
            if (owner == -1 && firstInactive == 0 && ChannelLoad64(&block->m_generation) < 0xffffff)
            {
                firstInactive = i;
            }
        }
    }
    if (ProcessWorkAtomic::replace(&m_control->m_full, 1, 2))
    {
        if (fullCount.exchange(bufferCount()) == 0)
        {
            notification.trySignal(true, false);
        }
    }
    if (demand && free <= m_control->m_threshold && m_control->m_increment > 0 && m_growthRemaining == 0)
    {
        uint64_t amount = static_cast<uint64_t>(m_control->m_increment);
        if (m_memoryConfig.m_policy)
        {
            ProcessWorkMemoryGrowContext context;
            context.m_kind = ProcessWorkSharedBuffers;
            context.m_initialCapacity = m_control->m_initial;
            context.m_currentCapacity = bufferCount();
            context.m_successfulGrowCount = m_successfulGrowth;
            context.m_requiredGrowth = 1;
            try
            {
                amount = m_memoryConfig.m_policy->grow(context);
            }
            catch (...)
            {
                m_nextGrowth = now + m_memoryConfig.m_retryMilliseconds;
                amount = 0;
            }
        }
        const uint64_t maximum = m_control->m_limit == -1 ? 0xffffff : static_cast<uint64_t>(m_control->m_limit);
        const uint64_t current = bufferCount();
        m_growthRemaining = current >= maximum ? 0 : static_cast<int32_t>((std::min)(amount, maximum - current));
    }
    const int32_t limit = m_control->m_limit == -1 ? 0xffffff : m_control->m_limit;
    if (m_growthRemaining > 0 && now >= m_nextGrowth)
    {
        const uint64_t previous = bufferCount();
        uint64_t added = 0;
        const uint64_t perBlock = static_cast<uint64_t>(m_control->m_blockBytes) + 65536;
        const uint64_t budget = (std::max)(uint64_t(1), m_memoryConfig.m_growthBytes / perBlock);
        int32_t index = firstInactive == 0 ? extent + 1 : firstInactive;
        for (uint64_t work = 0; work < budget && m_growthRemaining > 0 && index <= limit; ++index)
        {
            ProcessWorkSharedBlock* block = index <= extent ? m_storage->block(index) : nullptr;
            if (block != nullptr && (ChannelLoad64(&block->m_owner) != -1 || ChannelLoad64(&block->m_generation) >= 0xffffff))
            {
                continue;
            }
            if (!createBlock(index))
            {
                m_nextGrowth = now + m_memoryConfig.m_retryMilliseconds;
                break;
            }
            --m_growthRemaining;
            ++added;
            ++work;
            ++free;
        }
        if (index > limit)
        {
            m_growthRemaining = 0;
        }
        if (added != 0)
        {
            ++m_successfulGrowth;
            if (m_memoryConfig.m_policy)
            {
                ProcessWorkMemoryGrowth growth;
                growth.m_kind = ProcessWorkSharedBuffers;
                growth.m_previousCapacity = previous;
                growth.m_addedCapacity = added;
                growth.m_currentCapacity = previous + added;
                growth.m_successfulGrowCount = m_successfulGrowth;
                try
                {
                    m_memoryConfig.m_policy->grown(growth);
                }
                catch (...)
                {
                    // 已提交的共享块仍保留，通知异常不能使发送方重放已提交消息。
                }
            }
        }
    }
    if (free > 0)
    {
        ProcessWorkAtomic::replace(&m_control->m_full, 2, 0);
    }
    if (m_control->m_idleSeconds == 0 || m_growthRemaining > 0)
    {
        return;
    }
    for (int32_t i = m_control->m_initial + 1; i <= extent; ++i)
    {
        uint64_t* idle = m_storage->idleSince(i);
        if (idle != nullptr && now >= *idle && now - *idle >= static_cast<uint64_t>(m_control->m_idleSeconds) * 1000 && m_storage->retire(i))
        {
            AtomicMath::selfSubOne(&m_control->m_active);
        }
    }
}
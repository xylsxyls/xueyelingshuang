#include "NetSenderSubmissionState.h"
#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetSenderSubmission.h"
#include <chrono>
#include <cstring>
#include <new>
#if !defined(__EMSCRIPTEN__)
#include "CSystem/CSystemAPI.h"
#endif

NetSenderSubmissionFront::NetSenderSubmissionFront() :
m_entry(nullptr), m_command(), m_published(false), m_cancelled(false), m_retired(false), m_sent(0)
{

}

NetSenderSubmissionPayload::NetSenderSubmissionPayload(NetBufferPool& pool, std::atomic<uint64_t>& budget, const uint8_t* bytes, uint32_t size) :
m_bytes(nullptr), m_size(size), m_status(NI_NO_MEMORY), m_budget(budget), m_reserved(false)
{
    m_status = pool.allocate(size, m_buffer);
    if (m_status == NI_OK)
    {
        m_bytes = m_buffer.data();
        if (size != 0)
        {
            std::memcpy(m_bytes, bytes, size);
        }
        m_budget.fetch_add(size);
        m_reserved = true;
    }
}

NetSenderSubmissionPayload::~NetSenderSubmissionPayload()
{
    NetInterface::wipe(m_bytes, m_bytes == nullptr ? 0 : m_size);
    if (m_reserved)
    {
        m_budget.fetch_sub(m_size);
    }
}

NetSenderSubmissionInput::NetSenderSubmissionInput(std::atomic<uint32_t>& count) :
m_kind(NET_SUBMIT_NONE), m_count(count)
{
    m_count.fetch_add(1);
}

NetSenderSubmissionInput::~NetSenderSubmissionInput()
{
    m_count.fetch_sub(1);
}

NetSenderSubmissionAccess::NetSenderSubmissionAccess(NetSenderSubmissionState& state) :
m_state(state), m_accepted(false)
{
    uint32_t count = state.m_admission.load(std::memory_order_acquire);
    uint32_t attempts = 0;
    while (count < 0x7fffffffU)
    {
        if (state.m_admission.compare_exchange_weak(count, count + 1, std::memory_order_acquire, std::memory_order_acquire))
        {
            m_accepted = true;
            break;
        }
        NetSenderSubmissionState::pauseContention(attempts);
    }
}

NetSenderSubmissionAccess::~NetSenderSubmissionAccess()
{
    if (m_accepted)
    {
        m_state.m_admission.fetch_sub(1, std::memory_order_release);
    }
}

bool NetSenderSubmissionAccess::accepted() const
{
    return m_accepted;
}

NetSenderSubmissionReceipt::NetSenderSubmissionReceipt() :
m_next(nullptr),
m_entry(nullptr),
m_ticket(0),
m_generation(0),
m_deadline(0),
m_result(NI_READY),
m_consumed(false),
m_signalState(0),
m_staged(false)
{

}

NetSenderSubmissionEntry::NetSenderSubmissionEntry() :
m_storage(nullptr),
m_pool(nullptr),
m_frameBudget(nullptr),
m_command(),
m_input(nullptr),
m_size(0),
m_phase(NET_SUBMISSION_RESERVED),
m_previousRequest(nullptr),
m_nextRequest(nullptr),
m_previousPending(nullptr),
m_nextPending(nullptr),
m_receipt(nullptr),
m_pending(false),
m_cancelled(false),
m_cancelTaken(false),
m_attempt(0)
{

}

NetSenderSubmissionEntry::~NetSenderSubmissionEntry()
{
    clear();
}

void NetSenderSubmissionEntry::clear()
{
    if (m_frameBudget != nullptr)
    {
        NetInterface::wipe(m_input, m_size);
        m_frameBudget->fetch_sub(m_size);
        m_frameBudget = nullptr;
        m_frameBuffer.reset();
    }
    m_input = nullptr;
    m_size = 0;
    m_payload.reset();
}

/** 复制已验证的短正文，定长复制覆盖实际范围，不越过调用者的缓冲
@param [out] output 至少size字节的独占存储
@param [in] input 至少size字节的输入，size为0时允许为空
@param [in] size 正文长度，不超过64字节
*/
static inline void CopySubmissionBody(uint8_t* output, const uint8_t* input, uint32_t size)
{
    if (size >= 8)
    {
        std::memcpy(output, input, size);
    }
    else if (size >= 4)
    {
        std::memcpy(output, input, 4);
        std::memcpy(output + size - 4, input + size - 4, 4);
    }
    else if (size >= 2)
    {
        std::memcpy(output, input, 2);
        std::memcpy(output + size - 2, input + size - 2, 2);
    }
    else if (size != 0)
    {
        *output = *input;
    }
}

NetSenderSubmissionItem::NetSenderSubmissionItem() :
m_inlineFrame(), m_inlineGeneration(0), m_borrowedInline(false), m_frame(nullptr)
{

}

NetSenderSubmissionItem::NetSenderSubmissionItem(const NetObjectReference<NetSenderSubmissionInput>& input) :
m_inlineFrame(), m_inlineGeneration(0), m_borrowedInline(false), m_input(input), m_frame(nullptr)
{

}

NetSenderSubmissionItem::NetSenderSubmissionItem(const NetInterfaceFrame& frame, uint64_t generation) :
m_inlineFrame(frame), m_inlineGeneration(generation), m_borrowedInline(true), m_frame(nullptr)
{
    // tryPushAndFence在发布前同步复制本项；栈上描述不再多复制和擦除一份明文。
}

NetSenderSubmissionItem::NetSenderSubmissionItem(const NetSenderSubmissionItem& other) :
m_inlineFrame(other.m_inlineFrame), m_inlineGeneration(other.m_inlineGeneration), m_borrowedInline(false),
m_input(other.m_input), m_frame(other.m_frame)
{
    m_inlineFrame.m_payload = nullptr;
    if (m_inlineFrame.m_payloadSize != 0)
    {
        CopySubmissionBody(m_inlineBody, other.m_borrowedInline ? other.m_inlineFrame.m_payload : other.m_inlineBody,
            m_inlineFrame.m_payloadSize);
    }
}

NetSenderSubmissionItem& NetSenderSubmissionItem::operator=(const NetSenderSubmissionItem& other)
{
    if (this != &other)
    {
        if (!m_borrowedInline && m_inlineFrame.m_payloadSize != 0)
        {
            NetInterface::wipe(m_inlineBody, m_inlineFrame.m_payloadSize);
        }
        m_input = other.m_input;
        m_frame = other.m_frame;
        m_inlineFrame = other.m_inlineFrame;
        m_inlineGeneration = other.m_inlineGeneration;
        m_inlineFrame.m_payload = nullptr;
        m_borrowedInline = false;
        if (m_inlineFrame.m_payloadSize != 0)
        {
            CopySubmissionBody(m_inlineBody, other.m_borrowedInline ? other.m_inlineFrame.m_payload : other.m_inlineBody,
                m_inlineFrame.m_payloadSize);
        }
    }
    return *this;
}

NetSenderSubmissionItem::~NetSenderSubmissionItem()
{
    if (!m_borrowedInline && m_inlineFrame.m_payloadSize != 0)
    {
        NetInterface::wipe(m_inlineBody, m_inlineFrame.m_payloadSize);
    }
}

NetSenderSubmissionState::NetSenderSubmissionState(const NetSenderSubmissionPools& pools) :
m_gate(),
m_gateConfigured(false),
m_generation(0),
m_product(0),
m_nextId(0),
m_nextSync(0),
m_round(0),
m_maintainedRound(UINT64_MAX),
m_chunk(0),
m_bytes(0),
m_asyncBytes(0),
m_asyncReturnedBytes(0),
m_controlBytes(0),
m_requestCount(0),
m_frameCount(0),
m_controlCount(0),
m_inputCount(0),
m_receiptCount(0),
m_pendingReceiptCount(0),
m_ready(false),
m_closeRequest(0),
m_admission(0x80000000U),
m_initializationResult(NI_OK),
m_bodies(pools.m_bodies),
m_controlBodies(pools.m_controlBodies),
m_requestsPool(pools.m_requests),
m_framesPool(pools.m_frames),
m_controlsPool(pools.m_controls),
m_frontsPool(pools.m_fronts),
m_receiptsPool(pools.m_receipts),
m_inputsPool(pools.m_inputs),
m_payloadsPool(pools.m_payloads),
m_controlPayloadsPool(pools.m_controlPayloads),
m_fronts(128, pools.m_requestIndex),
m_tickets(128, pools.m_receiptIndex),
m_incoming(false),
m_inlineOutstanding(0),
m_directCount(0),
m_directIndex(0),
m_directAttempt(0),
m_directBorrowed(false),
m_closing(false),
m_closeTaken(false),
m_reason(0),
m_requests(nullptr),
m_requestsTail(nullptr),
m_pending(nullptr),
m_pendingTail(nullptr),
m_taken(nullptr),
m_takenKind(NET_SUBMIT_NONE),
m_receipts(nullptr),
m_draining(nullptr)
{
    LockFreeQueueResult configured = m_incoming.configurePool(pools.m_queue);
    if (configured == LockFreeQueueSuccess)
    {
        // 仅分散入口计数，不改变发布顺序合同；生产者与唯一消费者不逐条争写总计数。
        configured = m_incoming.configureProducerCounters(true);
    }
    m_initializationResult = queueResult(configured == LockFreeQueueSuccess ? m_incoming.tryInit() : configured);
    const int32_t states[] = {m_bodies.status(), m_controlBodies.status(), m_requestsPool.status(),
        m_framesPool.status(), m_controlsPool.status(), m_frontsPool.status(), m_receiptsPool.status(),
        m_inputsPool.status(), m_payloadsPool.status(), m_controlPayloadsPool.status(),
        indexResult(LockFreeHashMapResult(m_fronts.status(), m_fronts.status() == LockFreeHashMapOk)),
        indexResult(LockFreeHashMapResult(m_tickets.status(), m_tickets.status() == LockFreeHashMapOk))};
    for (size_t index = 0; m_initializationResult == NI_OK && index < sizeof(states) / sizeof(states[0]); ++index)
    {
        m_initializationResult = states[index];
    }
}

NetSenderSubmissionState::~NetSenderSubmissionState()
{
    quiesce();
    // 外层已退出所有借用者；先解除借用保护，退休项若已脱离链表则单独收回。
    NetSenderSubmissionEntry* borrowed = m_taken;
    m_taken = nullptr;
    m_takenKind = NET_SUBMIT_NONE;
    if (borrowed != nullptr && borrowed->m_phase == NET_SUBMISSION_RETIRED)
    {
        retire(borrowed);
    }
    NetSenderSubmissionCommand unused = {};
    do
    {
        dropFrames();
        while (m_requests != nullptr)
        {
            take(m_requests, unused);
        }
        pump();
    }
    // quiesce后size精确；每批先回收工作区，不能持有一批短帧等待下一批出队。
    while (m_incoming.size() != 0);
    dropFrames();
    while (m_requests != nullptr)
    {
        take(m_requests, unused);
    }
    m_receipts = nullptr;
    m_tickets.tryClear(true, false);
}

void NetSenderSubmissionState::quiesce()
{
    m_admission.fetch_or(0x80000000U, std::memory_order_acq_rel);
    while (m_admission.load(std::memory_order_acquire) != 0x80000000U)
    {
        if (m_gate.m_pause != nullptr)
        {
            try
            {
                m_gate.m_pause(m_gate.m_context);
            }
            catch (...)
            {
                requestClose(NI_INVALID_STATE);
#if !defined(__EMSCRIPTEN__)
                CSystem::Sleep(1);
#endif
            }
        }
#if !defined(__EMSCRIPTEN__)
        else
        {
            CSystem::Sleep(1);
        }
#endif
        // 单线程Wasm不会有并行提交者；多线程提供方必须配置pause，不占用JS事件循环等待。
    }
}

uint64_t NetSenderSubmissionState::nextId(std::atomic<uint64_t>& counter)
{
    uint64_t value = counter.load();
    uint32_t attempts = 0;
    while (value != UINT64_MAX)
    {
        if (counter.compare_exchange_weak(value, value + 1))
        {
            return value + 1;
        }
        pauseContention(attempts);
    }
    return 0;
}

void NetSenderSubmissionState::pauseContention(uint32_t& attempts)
{
    if (++attempts == 64)
    {
        attempts = 0;
#if !defined(__EMSCRIPTEN__)
        CSystem::Sleep(0);
#endif
    }
}

int32_t NetSenderSubmissionState::indexResult(const LockFreeHashMapResult& result)
{
    if (result.m_committed)
    {
        return NI_OK;
    }
    switch (result.m_status)
    {
    case LockFreeHashMapCapacityLimit:
    case LockFreeHashMapGrowthDeclined:
    case LockFreeHashMapProtectionUnavailable:
    case LockFreeHashMapVersionExhausted:
    case LockFreeHashMapDirectoryFull:
        return NI_LIMIT_EXCEEDED;
    case LockFreeHashMapNoMemory:
        return NI_NO_MEMORY;
    case LockFreeHashMapNotFound:
        return NI_NOT_FOUND;
    case LockFreeHashMapInvalidArgument:
        return NI_INVALID_ARGUMENT;
    case LockFreeHashMapUnsupported:
        return NI_NOT_SUPPORTED;
    default:
        return NI_INVALID_STATE;
    }
}

int32_t NetSenderSubmissionState::queueResult(LockFreeQueueResult result)
{
    switch (result)
    {
    case LockFreeQueueSuccess:
        return NI_OK;
    case LockFreeQueueCapacityLimit:
    case LockFreeQueueGrowthDeclined:
    case LockFreeQueueSequenceExhausted:
    case LockFreeQueueResourceUnavailable:
        return NI_LIMIT_EXCEEDED;
    case LockFreeQueueAllocationFailure:
        return NI_NO_MEMORY;
    case LockFreeQueueInvalidConfiguration:
        return NI_INVALID_ARGUMENT;
    case LockFreeQueueAtomicUnavailable:
        return NI_NOT_SUPPORTED;
    default:
        return NI_INVALID_STATE;
    }
}

void NetSenderSubmissionState::requestClose(int32_t reason)
{
    uint64_t expected = 0;
    m_closeRequest.compare_exchange_strong(expected, (UINT64_C(1) << 32) | static_cast<uint32_t>(reason));
    m_ready.store(false);
}

void NetSenderSubmissionState::wake()
{
    if (m_gate.m_wake != nullptr)
    {
        try
        {
            m_gate.m_wake(m_gate.m_context);
        }
        catch (...)
        {
            // 入队已经提交，绝不能沿生产者失败路径再次归还预算。
            requestClose(NI_INVALID_STATE);
        }
    }
}

NetObjectReference<NetSenderSubmissionReceipt> NetSenderSubmissionState::reserveReceipt(uint64_t generation, uint64_t deadline, int32_t& result)
{
    NetObjectReference<NetSenderSubmissionReceipt> receipt;
    result = m_receiptsPool.create(receipt);
    if (result != NI_OK)
    {
        return receipt;
    }
    receipt->m_ticket = nextId(m_nextSync);
    receipt->m_generation = generation;
    receipt->m_deadline = deadline;
    if (receipt->m_ticket == 0)
    {
        result = NI_LIMIT_EXCEEDED;
    }
    else
    {
        result = indexResult(m_tickets.trySet(receipt->m_ticket, receipt, nullptr, true, false));
        if (result == NI_OK)
        {
            m_receiptCount.fetch_add(1);
            m_pendingReceiptCount.fetch_add(1);
            return receipt;
        }
    }
    receipt.reset();
    return receipt;
}

void NetSenderSubmissionState::rollbackReceipt(const NetObjectReference<NetSenderSubmissionReceipt>& receipt)
{
    if (receipt)
    {
        receipt->m_consumed.store(true);
        m_pendingReceiptCount.fetch_sub(1);
        const int32_t result = indexResult(m_tickets.tryErase(receipt->m_ticket, true, false));
        if (result != NI_OK)
        {
            requestClose(result);
        }
        m_receiptCount.fetch_sub(1);
    }
}

void NetSenderSubmissionState::materializeDirect()
{
    if (m_directBorrowed)
    {
        return;
    }
    for (uint32_t index = m_directIndex; index < m_directCount; ++index)
    {
        NetSenderSubmissionItem& item = m_inlineItems[index];
        NetSenderSubmissionEntry& entry = m_inlineEntries[index];
        entry.m_phase = NET_SUBMISSION_QUEUED;
        entry.m_attempt = index == m_directIndex ? m_directAttempt : 0;
        entry.m_command.m_kind = NET_SUBMIT_DATA;
        entry.m_command.m_generation = item.m_inlineGeneration;
        entry.m_command.m_id = item.m_inlineFrame.m_requestId;
        entry.m_command.m_frame.m_kind = NI_DATA;
        entry.m_size = item.m_inlineFrame.m_payloadSize;
        entry.m_storage = &item;
        entry.m_input = entry.m_size == 0 ? nullptr : item.m_inlineBody;
        entry.m_frameBudget = &m_asyncBytes;
        item.m_inlineFrame.m_payload = entry.m_input;
        ++m_inlineOutstanding;
        append(&entry);
    }
    m_directIndex = m_directCount = 0;
    m_directAttempt = 0;
}

void NetSenderSubmissionState::dropDirect()
{
    if (m_directCount == 0)
    {
        return;
    }
    const uint32_t first = m_directIndex + (m_directBorrowed ? 1 : 0);
    const uint32_t end = m_directCount;
    uint64_t returned = 0;
    for (uint32_t index = first; index < end; ++index)
    {
        NetSenderSubmissionItem& item = m_inlineItems[index];
        returned += item.m_inlineFrame.m_payloadSize;
        NetInterface::wipe(item.m_inlineBody, item.m_inlineFrame.m_payloadSize);
        item.m_inlineFrame.m_payloadSize = 0;
    }
    if (returned != 0)
    {
        m_asyncReturnedBytes.store(m_asyncReturnedBytes.load(std::memory_order_relaxed) + returned,
            std::memory_order_release);
    }
    m_directCount = m_directBorrowed ? m_directIndex + 1 : 0;
    if (m_directCount == 0)
    {
        m_directIndex = 0;
    }
    if (!m_directBorrowed)
    {
        m_directAttempt = 0;
    }
}

void NetSenderSubmissionState::pump(bool deferData)
{
    if (!deferData || m_pending != nullptr || m_requests != nullptr || m_receipts != nullptr)
    {
        materializeDirect();
    }
    // 只有owner读写链结；等待者仅置consumed，实际摘除不与finish借用并发。
    NetSenderSubmissionReceipt** link = &m_receipts;
    while (*link != nullptr)
    {
        NetSenderSubmissionReceipt* receipt = *link;
        if (!receipt->m_consumed.load())
        {
            link = &receipt->m_next;
            continue;
        }
        *link = receipt->m_next;
        if (receipt->m_entry != nullptr)
        {
            receipt->m_entry->m_receipt = nullptr;
        }
        const uint64_t ticket = receipt->m_ticket;
        const int32_t result = indexResult(m_tickets.tryErase(ticket, true, false));
        if (result != NI_OK)
        {
            requestClose(result);
        }
    }
    // 每批只领取已发布的FIFO前缀，由基础队列合并归还节点；不等待凑满批次。
    NetSenderSubmissionItem* items = m_inlineItems;
    for (uint32_t batch = 0; batch < 8 && m_inlineOutstanding == 0 && m_directCount == 0; ++batch)
    {
        size_t consumed = 0;
        if (!m_incoming.popReadyPrefix(items, 32, consumed))
        {
            break;
        }
        bool direct = deferData && m_pending == nullptr && m_requests == nullptr && m_receipts == nullptr;
        for (size_t index = 0; direct && index < consumed; ++index)
        {
            // 空正文沿用通用工作条目路径，避免无载荷高频交接时直接前缀消费退化。
            direct = items[index].m_inlineFrame.m_kind == NI_DATA && items[index].m_inlineFrame.m_payloadSize != 0;
        }
        if (direct)
        {
            m_directCount = static_cast<uint32_t>(consumed);
            m_directIndex = 0;
            m_directAttempt = 0;
            break;
        }
        for (size_t index = 0; index < consumed; ++index)
        {
            NetSenderSubmissionItem& item = items[index];
            if (item.m_inlineFrame.m_kind != 0)
            {
                NetSenderSubmissionEntry& entry = m_inlineEntries[index];
                // 此工作区上一批已完整退休，只有本轮owner能够访问或复用。
                entry.m_phase = NET_SUBMISSION_QUEUED;
                entry.m_attempt = 0;
                // 工作区只承载DATA；首次构造已清零其他命令字段，复用时不重复清整份RPC命令。
                entry.m_command.m_kind = NET_SUBMIT_DATA;
                entry.m_command.m_generation = item.m_inlineGeneration;
                entry.m_command.m_id = item.m_inlineFrame.m_requestId;
                // 帧元数据和正文都直接借用持久入口，避免给工作记录再复制整帧。
                entry.m_command.m_frame.m_kind = NI_DATA;
                entry.m_size = item.m_inlineFrame.m_payloadSize;
                // 前缀存储与工作区同寿命；不再复制第二份正文。retire擦除后才允许下一批覆盖。
                entry.m_storage = &item;
                entry.m_input = entry.m_size == 0 ? nullptr : item.m_inlineBody;
                entry.m_frameBudget = &m_asyncBytes;
                item.m_inlineFrame.m_payload = entry.m_input;
                ++m_inlineOutstanding;
                append(&entry);
                continue;
            }
            if (item.m_frame)
            {
                NetSenderSubmissionEntry* entry = item.m_frame;
                item.m_frame = nullptr;
                entry->m_phase = NET_SUBMISSION_QUEUED;
                append(entry);
                continue;
            }
            NetObjectReference<NetSenderSubmissionInput>& input = item.m_input;
            NetSenderSubmissionEntry* entry = nullptr;
            if (input->m_kind == NET_SUBMIT_BEGIN)
            {
                entry = input->m_entry.release();
                entry->m_previousRequest = m_requestsTail;
                if (m_requestsTail != nullptr)
                {
                    m_requestsTail->m_nextRequest = entry;
                }
                else
                {
                    m_requests = entry;
                }
                m_requestsTail = entry;
            }
            else
            {
                entry = input->m_kind == NET_SUBMIT_DATA ? input->m_entry.release() : find(input->m_front->m_command.m_id);
                if (entry != nullptr && entry->m_phase != NET_SUBMISSION_RETIRED)
                {
                    // 发布后的输入字段只由owner访问；转交引用，不为同一正文增减共享计数。
                    entry->m_payload = std::move(input->m_payload);
                    entry->m_input = entry->m_payload->m_bytes;
                    entry->m_size = entry->m_payload->m_size;
                    entry->m_phase = NET_SUBMISSION_QUEUED;
                    if (input->m_kind == NET_SUBMIT_DATA)
                    {
                        entry->m_command.m_frame.m_payload = entry->m_input;
                    }
                    append(entry);
                }
                if (input->m_receipt)
                {
                    addReceipt(input->m_receipt.get(), entry);
                    if (entry == nullptr)
                    {
                        settle(input->m_receipt.get(), NI_CANCELLED);
                    }
                }
            }
            // 通用输入的所有权已转移到owner，不能由持久消费缓冲延长其计数或租约。
            input.reset();
        }
    }
    for (NetSenderSubmissionEntry* entry = m_requests; entry != nullptr; entry = entry->m_nextRequest)
    {
        if (entry->m_front->m_cancelled.load())
        {
            entry->m_cancelled = true;
        }
    }
    const uint64_t closed = m_closeRequest.load();
    if (closed != 0)
    {
        m_closing = true;
        m_reason = static_cast<int32_t>(static_cast<uint32_t>(closed));
    }
    if (m_closing || !m_ready.load(std::memory_order_acquire))
    {
        dropFrames();
    }
    if (m_maintainedRound != m_round || (m_pending == nullptr && m_directCount == 0))
    {
        m_maintainedRound = m_round;
        m_incoming.maintain(16);
        m_fronts.maintain(16);
        m_tickets.maintain(16);
    }
}

NetSenderSubmissionEntry* NetSenderSubmissionState::find(uint64_t id) const
{
    NetObjectReference<NetSenderSubmissionFront> front;
    if (!m_fronts.get(id, front) || front->m_retired.load())
    {
        return nullptr;
    }
    return front->m_entry;
}

void NetSenderSubmissionState::append(NetSenderSubmissionEntry* entry)
{
    entry->m_previousPending = m_pendingTail;
    entry->m_nextPending = nullptr;
    entry->m_pending = true;
    if (m_pendingTail != nullptr)
    {
        m_pendingTail->m_nextPending = entry;
    }
    else
    {
        m_pending = entry;
    }
    m_pendingTail = entry;
}

void NetSenderSubmissionState::remove(NetSenderSubmissionEntry* entry)
{
    if (!entry->m_pending)
    {
        return;
    }
    if (entry->m_previousPending != nullptr)
    {
        entry->m_previousPending->m_nextPending = entry->m_nextPending;
    }
    else
    {
        m_pending = entry->m_nextPending;
    }
    if (entry->m_nextPending != nullptr)
    {
        entry->m_nextPending->m_previousPending = entry->m_previousPending;
    }
    else
    {
        m_pendingTail = entry->m_previousPending;
    }
    entry->m_previousPending = nullptr;
    entry->m_nextPending = nullptr;
    entry->m_pending = false;
}

void NetSenderSubmissionState::clear(NetSenderSubmissionEntry* entry)
{
    if (entry->m_frameBudget != nullptr)
    {
        // 只由owner执行；不再让接纳者和回收者共同RMW同一个字节计数。
        if (entry->m_size != 0)
        {
            NetInterface::wipe(entry->m_input, entry->m_size);
            m_asyncReturnedBytes.store(m_asyncReturnedBytes.load(std::memory_order_relaxed) + entry->m_size,
                std::memory_order_release);
        }
        entry->m_frameBudget = nullptr;
        entry->m_frameBuffer.reset();
        if (entry->m_pool == nullptr)
        {
            static_cast<NetSenderSubmissionItem*>(entry->m_storage)->m_inlineFrame.m_payloadSize = 0;
        }
    }
    entry->clear();
}

void NetSenderSubmissionState::retire(NetSenderSubmissionEntry* entry)
{
    remove(entry);
    if (entry->m_phase != NET_SUBMISSION_RETIRED)
    {
        if (entry->m_command.m_kind == NET_SUBMIT_DATA)
        {
            if (entry->m_frameBudget == nullptr)
            {
                (entry->m_command.m_frame.m_kind == NI_DATA ? m_frameCount : m_controlCount)--;
            }
        }
        entry->m_phase = NET_SUBMISSION_RETIRED;
    }
    // 回调可能先完成业务或断线，正文仍须覆盖当前协议调用的整个借用期。
    if (entry != m_taken)
    {
        clear(entry);
        if (entry->m_receipt != nullptr)
        {
            entry->m_receipt->m_entry = nullptr;
        }
        if (entry->m_pool != nullptr)
        {
            entry->m_pool->release(entry->m_storage);
        }
        else
        {
            --m_inlineOutstanding;
        }
    }
}

void NetSenderSubmissionState::take(NetSenderSubmissionEntry* entry, NetSenderSubmissionCommand& command)
{
    command = entry->m_command;
    entry->m_front->m_retired.store(true);
    const int32_t result = indexResult(m_fronts.tryErase(entry->m_command.m_id, true, false));
    if (result != NI_OK)
    {
        requestClose(result);
    }
    if (entry->m_previousRequest != nullptr)
    {
        entry->m_previousRequest->m_nextRequest = entry->m_nextRequest;
    }
    else
    {
        m_requests = entry->m_nextRequest;
    }
    if (entry->m_nextRequest != nullptr)
    {
        entry->m_nextRequest->m_previousRequest = entry->m_previousRequest;
    }
    else
    {
        m_requestsTail = entry->m_previousRequest;
    }
    entry->m_previousRequest = nullptr;
    entry->m_nextRequest = nullptr;
    --m_requestCount;
    retire(entry);
}

void NetSenderSubmissionState::addReceipt(NetSenderSubmissionReceipt* receipt, NetSenderSubmissionEntry* entry)
{
    receipt->m_entry = entry;
    receipt->m_next = m_receipts;
    m_receipts = receipt;
    if (entry != nullptr)
    {
        entry->m_receipt = receipt;
    }
}

void NetSenderSubmissionState::settle(NetSenderSubmissionReceipt* receipt, int32_t result)
{
    int32_t expected = NI_READY;
    if (receipt->m_result.compare_exchange_strong(expected, result, std::memory_order_release, std::memory_order_relaxed))
    {
        m_pendingReceiptCount.fetch_sub(1);
        if ((receipt->m_signalState.fetch_or(4, std::memory_order_acq_rel) & 2) != 0)
        {
            try { receipt->m_signal->notify(); }
            catch (...) { /* 通知不能改变已经发布的交付结果。 */ }
        }
    }
    if (m_draining == receipt)
    {
        m_draining = nullptr;
    }
}

void NetSenderSubmissionState::expireFrames(uint64_t nowMs)
{
    for (NetSenderSubmissionReceipt* receipt = m_receipts; receipt != nullptr; receipt = receipt->m_next)
    {
        NetSenderSubmissionEntry* entry = receipt->m_entry;
        if (receipt->m_result == NI_READY && !receipt->m_staged && nowMs >= receipt->m_deadline &&
            entry != nullptr && entry->m_command.m_kind == NET_SUBMIT_DATA)
        {
            settle(receipt, NI_TIMEOUT);
            retire(entry);
        }
    }
}

void NetSenderSubmissionState::dropFrames()
{
    dropDirect();
    NetSenderSubmissionEntry* entry = m_pending;
    while (entry != nullptr)
    {
        NetSenderSubmissionEntry* next = entry->m_nextPending;
        if (entry->m_command.m_kind == NET_SUBMIT_DATA)
        {
            retire(entry);
        }
        entry = next;
    }
}

bool NetSenderSubmissionState::pendingReceipts() const
{
    for (NetSenderSubmissionReceipt* receipt = m_receipts; receipt != nullptr; receipt = receipt->m_next)
    {
        if (receipt->m_result == NI_READY)
        {
            return true;
        }
    }
    return false;
}
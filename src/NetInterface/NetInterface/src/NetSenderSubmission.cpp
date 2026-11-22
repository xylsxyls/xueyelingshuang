#include "NetExecution.h"
#include "NetSenderSubmission.h"
#include "NetInterfaceMacro.h"
#include "NetSenderSubmissionState.h"
#include "NetInterface.h"
#include <chrono>
#include <cstring>
#include <new>

NetSenderSubmission::NetSenderSubmission() :
m_state(new NetSenderSubmissionState())
{

}

NetSenderSubmission::~NetSenderSubmission()
{
    delete m_state;
}

int32_t NetSenderSubmission::configureGate(const NetSenderSubmissionGate& gate)
{
    if (m_state->m_gateConfigured || m_state->m_generation != 0)
    {
        return NI_INVALID_STATE;
    }
    m_state->m_gate = gate;
    m_state->m_gateConfigured = true;
    return NI_OK;
}

int32_t NetSenderSubmission::configurePools(const NetSenderSubmissionPools& pools)
{
    if (m_state->m_generation.load() != 0 || !idle())
    {
        return NI_INVALID_STATE;
    }
    NetSenderSubmissionState* replacement = nullptr;
    try
    {
        replacement = new NetSenderSubmissionState(pools);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
    if (replacement->m_initializationResult != NI_OK)
    {
        const int32_t result = replacement->m_initializationResult;
        delete replacement;
        return result;
    }
    replacement->m_gate = m_state->m_gate;
    replacement->m_gateConfigured = m_state->m_gateConfigured;
    delete m_state;
    m_state = replacement;
    return NI_OK;
}

int32_t NetSenderSubmission::start(uint64_t generation, uint32_t product)
{
    if (generation == 0 || !idle())
    {
        return NI_INVALID_STATE;
    }
    m_state->quiesce();
    m_state->pump();
    if (!idle())
    {
        if (m_state->m_ready.load() && m_state->m_closeRequest.load() == 0)
        {
            m_state->m_admission.store(0, std::memory_order_release);
        }
        return NI_INVALID_STATE;
    }
    if (m_state->m_initializationResult != NI_OK)
    {
        return m_state->m_initializationResult;
    }
    // 全部请求/回执已经收敛，清掉可能由撤销失败留下的不可再使用身份。
    if (!m_state->m_fronts.tryClear(true, false).m_committed || !m_state->m_tickets.tryClear(true, false).m_committed)
    {
        return NI_INVALID_STATE;
    }
    m_state->m_generation = generation;
    m_state->m_product = product;
    m_state->m_ready = false;
    m_state->m_closeRequest = 0;
    m_state->m_closing = false;
    m_state->m_closeTaken = false;
    m_state->m_reason = 0;
    return NI_OK;
}

int32_t NetSenderSubmission::ready(uint64_t generation, uint32_t requests, uint32_t chunk)
{
    if (m_state->m_initializationResult != NI_OK)
    {
        return m_state->m_initializationResult;
    }
    if (m_state->m_generation.load() == 0)
    {
        return NI_INVALID_STATE;
    }
    if (generation == 0 || generation < m_state->m_generation.load() || requests == 0 ||
        chunk < 1024 || chunk > 65536)
    {
        return NI_PROTOCOL_ERROR;
    }
    if (m_state->m_closeRequest.load() != 0)
    {
        return NI_CANCELLED;
    }
    m_state->quiesce();
    m_state->pump();
    if (generation != m_state->m_generation.load() && (m_state->m_requests != nullptr || m_state->m_pending != nullptr ||
        m_state->m_takenKind != NET_SUBMIT_NONE || m_state->pendingReceipts()))
    {
        // 拒绝新代握手不撤销旧代的合法提交；原会话继续拥有待发正文和借用回执。
        if (m_state->m_ready.load() && m_state->m_closeRequest.load() == 0)
        {
            m_state->m_admission.store(0, std::memory_order_release);
        }
        return NI_PROTOCOL_ERROR;
    }
    if (m_state->m_closeRequest.load() != 0)
    {
        return NI_CANCELLED;
    }
    m_state->m_generation = generation;
    m_state->m_chunk = chunk;
    m_state->m_ready = true;
    m_state->m_admission.store(0, std::memory_order_release);
    return NI_OK;
}

uint64_t NetSenderSubmission::now()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

int32_t NetSenderSubmission::begin(uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& id)
{
    id = 0;
    const uint64_t started = now();
    if (product == 0 || timeoutMs == 0 || timeoutMs > 60000 || started > UINT64_MAX - timeoutMs)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetSenderSubmissionAccess access(*m_state);
    if (!access.accepted() || !m_state->m_ready.load() || m_state->m_closeRequest.load() != 0)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->m_product.load() != 0 && product != m_state->m_product.load())
    {
        return NI_INVALID_ARGUMENT;
    }
    m_state->m_requestCount.fetch_add(1);
    const uint64_t key = NetSenderSubmissionState::nextId(m_state->m_nextId);
    NetObjectReference<NetSenderSubmissionFront> front;
    NetObjectReference<NetSenderSubmissionInput> input;
    bool indexed = false;
    int32_t result = key == 0 ? NI_LIMIT_EXCEEDED : m_state->m_frontsPool.create(front);
    if (result == NI_OK)
    {
        result = m_state->m_inputsPool.create(input, m_state->m_inputCount);
    }
    if (result == NI_OK)
    {
        result = m_state->m_requestsPool.create(input->m_entry);
    }
    if (result == NI_OK)
    {
        input->m_kind = NET_SUBMIT_BEGIN;
        input->m_entry->m_storage = input->m_entry.storage();
        input->m_entry->m_pool = &m_state->m_requestsPool;
        NetSenderSubmissionCommand& command = input->m_entry->m_command;
        command.m_kind = NET_SUBMIT_BEGIN;
        command.m_generation = m_state->m_generation.load();
        command.m_id = key;
        command.m_product = product;
        command.m_type = type;
        command.m_started = started;
        command.m_timeout = timeoutMs;
        front->m_command = command;
        front->m_entry = input->m_entry.get();
        input->m_entry->m_front = front;
        result = NetSenderSubmissionState::indexResult(m_state->m_fronts.trySet(key, front, nullptr, true, false));
        indexed = result == NI_OK;
        if (indexed)
        {
            result = NetSenderSubmissionState::queueResult(m_state->m_incoming.tryPushAndFence(input, true, false));
        }
        if (result == NI_OK)
        {
            front->m_published.store(true, std::memory_order_release);
            id = key;
            m_state->wake();
            return NI_OK;
        }
    }
    if (indexed)
    {
        front->m_retired.store(true);
        const int32_t cleanup = NetSenderSubmissionState::indexResult(m_state->m_fronts.tryErase(key, true, false));
        if (cleanup != NI_OK)
        {
            m_state->requestClose(cleanup);
        }
    }
    m_state->m_requestCount.fetch_sub(1);
    return result;
}

int32_t NetSenderSubmission::send(uint64_t id, const uint8_t* bytes, uint32_t size)
{
    uint64_t unused = 0;
    return submitRequest(id, bytes, size, false, unused);
}

int32_t NetSenderSubmission::sendSynchronous(uint64_t id, const uint8_t* bytes, uint32_t size, uint64_t& ticket)
{
    return submitRequest(id, bytes, size, true, ticket);
}

int32_t NetSenderSubmission::submitRequest(uint64_t id, const uint8_t* bytes, uint32_t size, bool synchronous, uint64_t& ticket)
{
    ticket = 0;
    if (size > 1048560 || (size != 0 && (bytes == nullptr || reinterpret_cast<uintptr_t>(bytes) > UINTPTR_MAX - size)))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetSenderSubmissionAccess access(*m_state);
    if (!access.accepted() || !m_state->m_ready.load() || m_state->m_closeRequest.load() != 0)
    {
        return NI_INVALID_STATE;
    }
    NetObjectReference<NetSenderSubmissionFront> front;
    if (!m_state->m_fronts.get(id, front) || !front->m_published.load() || front->m_retired.load() || front->m_cancelled.load())
    {
        return NI_NOT_FOUND;
    }
    const uint64_t deadline = front->m_command.m_started + front->m_command.m_timeout;
    if (now() >= deadline)
    {
        return NI_TIMEOUT;
    }
    uint32_t expected = 0;
    if (!front->m_sent.compare_exchange_strong(expected, 1))
    {
        return NI_INVALID_STATE;
    }
    NetObjectReference<NetSenderSubmissionReceipt> receipt;
    NetObjectReference<NetSenderSubmissionPayload> payload;
    NetObjectReference<NetSenderSubmissionInput> input;
    int32_t result = m_state->m_payloadsPool.create(payload, m_state->m_bodies, m_state->m_bytes, bytes, size);
    if (result == NI_OK)
    {
        result = payload->m_status;
    }
    if (result == NI_OK && synchronous)
    {
        receipt = m_state->reserveReceipt(front->m_command.m_generation, deadline, result);
    }
    if (result == NI_OK)
    {
        result = m_state->m_inputsPool.create(input, m_state->m_inputCount);
    }
    if (result == NI_OK)
    {
        input->m_kind = NET_SUBMIT_SEND;
        input->m_front = front;
        input->m_payload = std::move(payload);
        input->m_receipt = receipt;
        result = NetSenderSubmissionState::queueResult(m_state->m_incoming.tryPushAndFence(input, true, false));
        if (result == NI_OK)
        {
            front->m_sent.store(2);
            ticket = receipt ? receipt->m_ticket : 0;
            m_state->wake();
            return NI_OK;
        }
    }
    m_state->rollbackReceipt(receipt);
    front->m_sent.store(0);
    return result;
}

int32_t NetSenderSubmission::cancel(uint64_t id)
{
    NetSenderSubmissionAccess access(*m_state);
    if (!access.accepted())
    {
        return NI_NOT_FOUND;
    }
    NetObjectReference<NetSenderSubmissionFront> front;
    if (!m_state->m_fronts.get(id, front) || !front->m_published.load() || front->m_retired.load() || front->m_cancelled.exchange(true))
    {
        return NI_NOT_FOUND;
    }
    m_state->wake();
    return NI_OK;
}

int32_t NetSenderSubmission::data(const NetInterfaceFrame& frame)
{
    uint64_t unused = 0;
    return submitData(frame, false, unused);
}

int32_t NetSenderSubmission::dataSynchronous(const NetInterfaceFrame& frame, uint64_t& ticket)
{
    return submitData(frame, true, ticket);
}

int32_t NetSenderSubmission::submitData(const NetInterfaceFrame& frame, bool synchronous, uint64_t& ticket)
{
    ticket = 0;
    if ((frame.m_kind != NI_DATA && frame.m_kind != NI_DATA_ACK &&
        !(frame.m_kind == NI_CANCEL && frame.m_messageType == NI_FILE_DATA_TYPE && frame.m_payloadSize == 0)) ||
        frame.m_productId == 0 || frame.m_requestId == 0 || (frame.m_payloadSize != 0 &&
        (frame.m_payload == nullptr || reinterpret_cast<uintptr_t>(frame.m_payload) > UINTPTR_MAX - frame.m_payloadSize)))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetSenderSubmissionAccess access(*m_state);
    if (!access.accepted() || !m_state->m_ready.load() || m_state->m_closeRequest.load() != 0)
    {
        return NI_INVALID_STATE;
    }
    if (frame.m_payloadSize > m_state->m_chunk.load() + 128 ||
        (m_state->m_product.load() != 0 && frame.m_productId != m_state->m_product.load()))
    {
        return NI_INVALID_ARGUMENT;
    }
    const bool control = frame.m_kind != NI_DATA;
    std::atomic<uint32_t>& count = control ? m_state->m_controlCount : m_state->m_frameCount;
    // 只有同步票据使用交付截止，普通DATA不逐条读取系统时钟。
    const uint64_t current = synchronous ? now() : 0;
    if (current > UINT64_MAX - 5000)
    {
        return NI_LIMIT_EXCEEDED;
    }
    // DATA本身已经带完整业务编号，不再为同一帧分配无人消费的第二个全局编号。
    const uint64_t id = frame.m_requestId;
    if (!synchronous && !control && frame.m_payloadSize <= 64)
    {
        NetSenderSubmissionItem item(frame, m_state->m_generation.load());
        // 空正文仍占队列节点，但不占字节预算；不为加零争用原子计数的缓存行。
        if (frame.m_payloadSize != 0)
        {
            m_state->m_asyncBytes.fetch_add(frame.m_payloadSize, std::memory_order_relaxed);
        }
        const int32_t result = NetSenderSubmissionState::queueResult(m_state->m_incoming.tryPushAndFence(item, true, false));
        if (result == NI_OK)
        {
            m_state->wake();
        }
        else if (frame.m_payloadSize != 0)
        {
            m_state->m_asyncBytes.fetch_sub(frame.m_payloadSize, std::memory_order_relaxed);
        }
        return result;
    }
    if (!synchronous && !control)
    {
        NetSenderSubmissionItem item;
        NetObjectReference<NetSenderSubmissionEntry> owned;
        int32_t result = m_state->m_framesPool.create(owned);
        if (result != NI_OK)
        {
            return result;
        }
        NetSenderSubmissionEntry& entry = *owned;
        entry.m_storage = owned.storage();
        entry.m_pool = &m_state->m_framesPool;
        // 不超过64字节的异步DATA已经走入口内联路径，此分支只接纳长正文。
        result = m_state->m_bodies.allocate(frame.m_payloadSize, entry.m_frameBuffer);
        if (result != NI_OK)
        {
            return result;
        }
        entry.m_input = entry.m_frameBuffer.data();
        entry.m_size = frame.m_payloadSize;
        if (entry.m_size != 0)
        {
            std::memcpy(entry.m_input, frame.m_payload, entry.m_size);
        }
        m_state->m_asyncBytes.fetch_add(entry.m_size, std::memory_order_relaxed);
        entry.m_frameBudget = &m_state->m_asyncBytes;
        entry.m_command.m_kind = NET_SUBMIT_DATA;
        entry.m_command.m_generation = m_state->m_generation.load();
        entry.m_command.m_id = id;
        entry.m_command.m_frame = frame;
        entry.m_command.m_frame.m_payload = entry.m_input;
        // 发布成功后owner可以立即销毁帧，生产者之后不再解引用该地址。
        item.m_frame = owned.release();
        result = NetSenderSubmissionState::queueResult(m_state->m_incoming.tryPushAndFence(item, true, false));
        if (result == NI_OK)
        {
            m_state->wake();
        }
        else
        {
            m_state->m_framesPool.release(item.m_frame->m_storage);
        }
        return result;
    }
    NetObjectReference<NetSenderSubmissionReceipt> receipt;
    NetObjectReference<NetSenderSubmissionPayload> payload;
    NetObjectReference<NetSenderSubmissionInput> input;
    NetObjectPool<NetSenderSubmissionPayload>& payloadPool = control ? m_state->m_controlPayloadsPool : m_state->m_payloadsPool;
    NetObjectPool<NetSenderSubmissionEntry>& entries = control ? m_state->m_controlsPool : m_state->m_framesPool;
    int32_t result = payloadPool.create(payload, control ? m_state->m_controlBodies : m_state->m_bodies,
        control ? m_state->m_controlBytes : m_state->m_bytes, frame.m_payload, frame.m_payloadSize);
    if (result == NI_OK)
    {
        result = payload->m_status;
    }
    if (result == NI_OK && synchronous)
    {
        receipt = m_state->reserveReceipt(m_state->m_generation.load(), current + 5000, result);
    }
    if (result == NI_OK)
    {
        result = m_state->m_inputsPool.create(input, m_state->m_inputCount);
    }
    if (result == NI_OK)
    {
        result = entries.create(input->m_entry);
    }
    if (result == NI_OK)
    {
        input->m_kind = NET_SUBMIT_DATA;
        input->m_entry->m_storage = input->m_entry.storage();
        input->m_entry->m_pool = &entries;
        input->m_entry->m_command.m_kind = NET_SUBMIT_DATA;
        input->m_entry->m_command.m_generation = m_state->m_generation.load();
        input->m_entry->m_command.m_id = id;
        input->m_entry->m_command.m_frame = frame;
        input->m_entry->m_command.m_frame.m_payload = nullptr;
        input->m_payload = std::move(payload);
        input->m_receipt = receipt;
        count.fetch_add(1);
        result = NetSenderSubmissionState::queueResult(m_state->m_incoming.tryPushAndFence(input, true, false));
        if (result == NI_OK)
        {
            ticket = receipt ? receipt->m_ticket : 0;
            m_state->wake();
            return NI_OK;
        }
        count.fetch_sub(1);
    }
    m_state->rollbackReceipt(receipt);
    return result;
}

void NetSenderSubmission::close(int32_t reason)
{
    m_state->requestClose(reason);
    m_state->wake();
}

bool NetSenderSubmission::closeRequested(int32_t& reason) const
{
    const uint64_t closed = m_state->m_closeRequest.load();
    reason = closed == 0 ? 0 : static_cast<int32_t>(static_cast<uint32_t>(closed));
    return closed != 0;
}

void NetSenderSubmission::state(uint64_t& generation, bool& ready) const
{
    generation = m_state->m_generation.load();
    ready = m_state->m_ready.load() && m_state->m_closeRequest.load() == 0;
    // 代次切换期间宁可报告未就绪，不把前代代次与后代就绪组合。
    if (generation != m_state->m_generation.load())
    {
        generation = m_state->m_generation.load();
        ready = false;
    }
}

int32_t NetSenderSubmission::beginPoll()
{
    m_state->pump(true);
    if (m_state->m_takenKind != NET_SUBMIT_NONE)
    {
        return NI_INVALID_STATE;
    }
    if (++m_state->m_round == 0)
    {
        m_state->m_round = 1;
        m_state->m_directAttempt = 0;
        for (NetSenderSubmissionEntry* entry = m_state->m_pending; entry != nullptr; entry = entry->m_nextPending)
        {
            entry->m_attempt = 0;
        }
    }
    return NI_OK;
}

const NetInterfaceFrame* NetSenderSubmission::nextData()
{
    if (m_state->m_takenKind != NET_SUBMIT_NONE || m_state->m_requests != nullptr ||
        m_state->m_receipts != nullptr || m_state->m_closeRequest.load(std::memory_order_acquire) != 0 ||
        !m_state->m_ready.load(std::memory_order_acquire))
    {
        return nullptr;
    }
    if (m_state->m_pending == nullptr)
    {
        m_state->pump();
    }
    NetSenderSubmissionEntry* entry = m_state->m_pending;
    if (m_state->m_closing || m_state->m_requests != nullptr || m_state->m_receipts != nullptr ||
        m_state->m_draining != nullptr || entry == nullptr ||
        entry->m_command.m_kind != NET_SUBMIT_DATA || entry->m_command.m_frame.m_kind != NI_DATA ||
        entry->m_receipt != nullptr || entry->m_cancelTaken || entry->m_attempt == m_state->m_round)
    {
        return nullptr;
    }
    // 与next相同的唯一借用标记；回调关闭时retire仍须等finish再清理正文。
    entry->m_attempt = m_state->m_round;
    m_state->m_taken = entry;
    m_state->m_takenKind = NET_SUBMIT_DATA;
    return entry->m_pool == nullptr ? &static_cast<NetSenderSubmissionItem*>(entry->m_storage)->m_inlineFrame :
        &entry->m_command.m_frame;
}

int32_t NetSenderSubmission::drainData(void* context, int32_t (*consumer)(void* context, const NetInterfaceFrame& frame),
    uint32_t maximum, uint32_t byteBudget, uint32_t& messages, uint64_t& bytes)
{
    messages = 0;
    bytes = 0;
    if (consumer == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_state->m_takenKind != NET_SUBMIT_NONE)
    {
        return NI_INVALID_STATE;
    }
    while (messages < maximum && bytes < byteBudget)
    {
        if (m_state->m_pending == nullptr && m_state->m_requests == nullptr && m_state->m_receipts == nullptr)
        {
            if (m_state->m_directCount == 0)
            {
                m_state->pump(true);
            }
            if (m_state->m_directCount != 0)
            {
                if (m_state->m_closeRequest.load(std::memory_order_acquire) != 0 ||
                    !m_state->m_ready.load(std::memory_order_acquire) || m_state->m_directAttempt == m_state->m_round)
                {
                    break;
                }
                NetSenderSubmissionItem& item = m_state->m_inlineItems[m_state->m_directIndex];
                NetInterfaceFrame& frame = item.m_inlineFrame;
                frame.m_payload = frame.m_payloadSize == 0 ? nullptr : item.m_inlineBody;
                const uint32_t size = frame.m_payloadSize;
                m_state->m_directBorrowed = true;
                m_state->m_takenKind = NET_SUBMIT_DATA;
                m_state->m_directAttempt = m_state->m_round;
                int32_t result = NI_INVALID_STATE;
                try
                {
                    result = consumer(context, frame);
                }
                catch (...)
                {
                    // 未知接纳状态由调用方关闭会话；同一poll内不重新执行本条。
                }
                m_state->m_takenKind = NET_SUBMIT_NONE;
                m_state->m_directBorrowed = false;
                if (result == NI_OK)
                {
                    // 本项由当前owner独占，直接归还；不为每条DATA进入整批关闭清理路径。
                    if (size != 0)
                    {
                        NetInterface::wipe(item.m_inlineBody, size);
                        m_state->m_asyncReturnedBytes.store(
                            m_state->m_asyncReturnedBytes.load(std::memory_order_relaxed) + size, std::memory_order_release);
                    }
                    frame.m_payloadSize = 0;
                    if (++m_state->m_directIndex == m_state->m_directCount)
                    {
                        m_state->m_directIndex = m_state->m_directCount = 0;
                    }
                    m_state->m_directAttempt = 0;
                    ++messages;
                    bytes += size;
                }
                if (m_state->m_closeRequest.load(std::memory_order_acquire) != 0 ||
                    !m_state->m_ready.load(std::memory_order_acquire))
                {
                    m_state->dropDirect();
                }
                if (result != NI_OK)
                {
                    return result;
                }
                continue;
            }
        }
        const NetInterfaceFrame* frame = nextData();
        if (frame == nullptr)
        {
            break;
        }
        const uint32_t size = frame->m_payloadSize;
        int32_t result = NI_INVALID_STATE;
        try
        {
            result = consumer(context, *frame);
        }
        catch (...)
        {
            // 未知接纳状态不重放；先结束借用，让调用方按错误关闭本代会话。
        }
        finish(result);
        if (result != NI_OK)
        {
            return result;
        }
        ++messages;
        bytes += size;
    }
    return NI_OK;
}

int32_t NetSenderSubmission::next(NetSenderSubmissionCommand& command)
{
    return next(command, false);
}

int32_t NetSenderSubmission::next(NetSenderSubmissionCommand& command, bool dataOnly)
{
    // 已导入的纯DATA前缀由owner独占，逐条取出不重复扫描跨线程队列和回收域。
    // 请求/回执仍实时检查取消和超时；关闭使用原子意图，不能被待发前缀延后。
    if (m_state->m_pending == nullptr || m_state->m_requests != nullptr ||
        m_state->m_receipts != nullptr || m_state->m_closeRequest.load() != 0)
    {
        m_state->pump();
    }
    std::memset(&command, 0, sizeof(command));
    if (m_state->m_takenKind != NET_SUBMIT_NONE)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->m_closing)
    {
        if (m_state->m_closeTaken)
        {
            return NI_NEED_MORE;
        }
        m_state->m_closeTaken = true;
        m_state->m_takenKind = NET_SUBMIT_CLOSE;
        command.m_kind = NET_SUBMIT_CLOSE;
        command.m_generation = m_state->m_generation;
        command.m_status = m_state->m_reason;
        return NI_READY;
    }
    if (!m_state->m_ready)
    {
        return NI_NEED_MORE;
    }
    const uint64_t current = m_state->m_requests != nullptr || m_state->m_receipts != nullptr ? now() : 0;
    m_state->expireFrames(current);
    for (NetSenderSubmissionEntry* entry = m_state->m_requests; entry != nullptr; entry = entry->m_nextRequest)
    {
        const bool registered = entry->m_phase == NET_SUBMISSION_REGISTERED || entry->m_phase == NET_SUBMISSION_SENT;
        if (!entry->m_cancelTaken && (entry->m_cancelled ||
            (!registered && current >= entry->m_command.m_started + entry->m_command.m_timeout)))
        {
            if (dataOnly && registered)
            {
                // CANCEL使用协议控制队列；先交付前缀，不能越过已接纳的业务帧。
                return NI_NEED_MORE;
            }
            entry->m_cancelTaken = true;
            command = entry->m_command;
            command.m_kind = registered ? NET_SUBMIT_CANCEL : NET_SUBMIT_TERMINAL;
            command.m_status = entry->m_cancelled ? NI_RPC_CANCELLED : NI_RPC_TIMEOUT;
            m_state->m_taken = entry;
            m_state->m_takenKind = command.m_kind;
            return NI_READY;
        }
    }
    NetSenderSubmissionEntry* entry = m_state->m_pending;
    if (m_state->m_draining != nullptr || entry == nullptr || entry->m_cancelTaken)
    {
        return NI_NEED_MORE;
    }
    const bool frame = entry->m_command.m_kind == NET_SUBMIT_DATA;
    if (dataOnly && frame && entry->m_command.m_frame.m_kind != NI_DATA)
    {
        return NI_NEED_MORE;
    }
    const bool send = frame || entry->m_phase == NET_SUBMISSION_REGISTERED;
    if (send && entry->m_attempt == m_state->m_round)
    {
        return NI_NEED_MORE;
    }
    command = entry->m_command;
    if (entry->m_pool == nullptr)
    {
        command.m_frame = static_cast<NetSenderSubmissionItem*>(entry->m_storage)->m_inlineFrame;
    }
    command.m_kind = frame ? NET_SUBMIT_DATA : send ? NET_SUBMIT_SEND : NET_SUBMIT_BEGIN;
    if (send)
    {
        entry->m_attempt = m_state->m_round;
        if (!frame)
        {
            command.m_bytes = entry->m_input;
            command.m_size = entry->m_size;
        }
    }
    m_state->m_taken = entry;
    m_state->m_takenKind = command.m_kind;
    return NI_READY;
}

void NetSenderSubmission::finish(int32_t result)
{
    // drainData独占回调借用的结束点；外部重入finish不能提前擦除正在发送的正文。
    if (m_state->m_directBorrowed)
    {
        return;
    }
    NetSenderSubmissionEntry* entry = m_state->m_taken;
    const int32_t kind = m_state->m_takenKind;
    // 普通内联DATA由owner工作区独占，无池租约或回执；成功时直接摘除FIFO首项。
    // 回调已退休、非首项、同步票据及背压仍走通用路径，不能提前复用借用中的正文。
    if (entry != nullptr && entry == m_state->m_pending && entry->m_pool == nullptr &&
        entry->m_receipt == nullptr && kind == NET_SUBMIT_DATA && result == NI_OK &&
        entry->m_phase != NET_SUBMISSION_RETIRED)
    {
        m_state->m_pending = entry->m_nextPending;
        if (m_state->m_pending != nullptr)
        {
            m_state->m_pending->m_previousPending = nullptr;
        }
        else
        {
            m_state->m_pendingTail = nullptr;
        }
        NetInterface::wipe(entry->m_input, entry->m_size);
        m_state->m_asyncReturnedBytes.store(m_state->m_asyncReturnedBytes.load(std::memory_order_relaxed) + entry->m_size,
            std::memory_order_release);
        static_cast<NetSenderSubmissionItem*>(entry->m_storage)->m_inlineFrame.m_payloadSize = 0;
        entry->m_frameBudget = nullptr;
        entry->m_input = nullptr;
        entry->m_size = 0;
        entry->m_phase = NET_SUBMISSION_RETIRED;
        entry->m_previousPending = nullptr;
        entry->m_nextPending = nullptr;
        entry->m_pending = false;
        --m_state->m_inlineOutstanding;
        m_state->m_taken = nullptr;
        m_state->m_takenKind = NET_SUBMIT_NONE;
        return;
    }
    if (entry != nullptr)
    {
        NetSenderSubmissionReceipt* receipt = entry->m_receipt;
        if (receipt != nullptr && receipt->m_result == NI_READY)
        {
            if (kind == NET_SUBMIT_SEND || kind == NET_SUBMIT_DATA)
            {
                if (result == NI_OK)
                {
                    receipt->m_staged = true;
                    m_state->m_draining = receipt;
                }
                else if (result != NI_LIMIT_EXCEEDED)
                {
                    m_state->settle(receipt, result);
                }
            }
            else if (kind == NET_SUBMIT_BEGIN && result < 0)
            {
                m_state->settle(receipt, result == NI_LIMIT_EXCEEDED ? NI_INVALID_STATE : result);
            }
            else if (kind == NET_SUBMIT_CANCEL || kind == NET_SUBMIT_TERMINAL)
            {
                const bool timeout = !entry->m_cancelled && kind == NET_SUBMIT_TERMINAL;
                m_state->settle(receipt, timeout ? NI_TIMEOUT : NI_CANCELLED);
            }
        }
        if (entry->m_phase != NET_SUBMISSION_RETIRED && result == NI_OK)
        {
            if (kind == NET_SUBMIT_BEGIN)
            {
                entry->m_phase = NET_SUBMISSION_REGISTERED;
            }
            else if (kind == NET_SUBMIT_SEND)
            {
                entry->m_phase = NET_SUBMISSION_SENT;
                m_state->remove(entry);
                m_state->clear(entry);
            }
            else if (kind == NET_SUBMIT_CANCEL)
            {
                // 已取消请求只等协议终态，不再让其未发正文堵住后续FIFO。
                m_state->remove(entry);
                m_state->clear(entry);
            }
        }
        m_state->m_taken = nullptr;
        if (entry->m_phase == NET_SUBMISSION_RETIRED || (kind == NET_SUBMIT_DATA && result == NI_OK))
        {
            m_state->retire(entry);
        }
    }
    m_state->m_takenKind = NET_SUBMIT_NONE;
}

bool NetSenderSubmission::complete(uint64_t generation, uint64_t id, NetSenderSubmissionCommand& command)
{
    m_state->pump();
    std::memset(&command, 0, sizeof(command));
    NetSenderSubmissionEntry* entry = m_state->find(id);
    if (entry == nullptr || entry->m_command.m_generation != generation)
    {
        return false;
    }
    m_state->take(entry, command);
    return true;
}

void NetSenderSubmission::disconnected(uint64_t generation)
{
    if (generation < m_state->m_generation)
    {
        return;
    }
    m_state->m_ready = false;
    m_state->quiesce();
    do
    {
        m_state->pump();
    }
    // 没有工作区借用时完整收敛已接纳入口；有借用时由finish后的owner轮次继续。
    while (m_state->m_incoming.size() != 0 && m_state->m_inlineOutstanding == 0 && !m_state->m_directBorrowed);
    m_state->m_generation = generation;
    for (NetSenderSubmissionReceipt* receipt = m_state->m_receipts; receipt != nullptr; receipt = receipt->m_next)
    {
        m_state->settle(receipt, NI_INVALID_STATE);
    }
    m_state->dropFrames();
}

bool NetSenderSubmission::takeClosed(NetSenderSubmissionCommand& command)
{
    m_state->pump();
    std::memset(&command, 0, sizeof(command));
    if (m_state->m_requests == nullptr)
    {
        return false;
    }
    m_state->take(m_state->m_requests, command);
    return true;
}

bool NetSenderSubmission::idle() const
{
    // 提交者退出前入口已发布；入口为空且owner不持有动作时，不另维护逐帧原子计数。
    // idle是owner快照；生命周期修改仍须先quiesce并由外部阻止新的调用。
    return (m_state->m_admission.load(std::memory_order_acquire) & 0x7fffffffU) == 0 &&
        m_state->m_incoming.size() == 0 &&
        m_state->m_directCount == 0 &&
        m_state->m_inputCount.load() == 0 && m_state->m_requestCount.load() == 0 && m_state->m_frameCount.load() == 0 &&
        m_state->m_controlCount.load() == 0 && m_state->m_receiptCount.load() == 0 &&
        m_state->m_requests == nullptr && m_state->m_pending == nullptr && m_state->m_takenKind == NET_SUBMIT_NONE;
}

uint32_t NetSenderSubmission::queuedBytes() const
{
    // 先读完成侧；其release保证之前接纳记录的原子增量对本次读取可见。
    const uint64_t returned = m_state->m_asyncReturnedBytes.load(std::memory_order_acquire);
    const uint64_t bytes = m_state->m_bytes.load() + (m_state->m_asyncBytes.load(std::memory_order_acquire) - returned);
    return bytes > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(bytes);
}

bool NetSenderSubmission::synchronousPending() const
{
    return m_state->m_pendingReceiptCount.load() != 0;
}

void NetSenderSubmission::advanceSynchronous(int32_t outputState)
{
    m_state->pump();
    if (m_state->m_takenKind != NET_SUBMIT_NONE)
    {
        return;
    }
    const uint64_t current = now();
    if (m_state->m_ready && !m_state->m_closing)
    {
        m_state->expireFrames(current);
    }
    for (NetSenderSubmissionReceipt* receipt = m_state->m_receipts; receipt != nullptr; receipt = receipt->m_next)
    {
        if (receipt->m_result != NI_READY)
        {
            continue;
        }
        NetExecution::deadline(receipt->m_deadline, current);
        int32_t result = NI_READY;
        if (m_state->m_closing || !m_state->m_ready || receipt->m_generation != m_state->m_generation)
        {
            result = m_state->m_reason < 0 ? m_state->m_reason : NI_CANCELLED;
        }
        else if (current >= receipt->m_deadline)
        {
            result = NI_TIMEOUT;
        }
        else if (receipt->m_staged)
        {
            // 只有明确OK才证明交付；其它正数也仍是未完成。
            result = outputState <= NI_OK ? outputState : NI_READY;
        }
        if (result != NI_READY)
        {
            m_state->settle(receipt, result);
            if (receipt->m_staged && result < 0 && !m_state->m_closing)
            {
                // 前缀可能已发送；停止本代并丢弃后缀，不把旧正文带进重连。
                m_state->m_ready = false;
                m_state->m_closing = true;
                m_state->m_reason = result;
                m_state->requestClose(result);
            }
        }
    }
    if (m_state->m_closing)
    {
        const int32_t reason = m_state->m_reason < 0 ? m_state->m_reason : NI_CANCELLED;
        for (NetSenderSubmissionReceipt* receipt = m_state->m_receipts; receipt != nullptr; receipt = receipt->m_next)
        {
            m_state->settle(receipt, reason);
        }
    }
}

int32_t NetSenderSubmission::synchronousResult(uint64_t ticket, bool consume)
{
    NetObjectReference<NetSenderSubmissionReceipt> receipt;
    if (!m_state->m_tickets.get(ticket, receipt))
    {
        decltype(m_state->m_tickets)::Snapshot snapshot;
        const LockFreeHashMapResult found = m_state->m_tickets.find(ticket, snapshot);
        if (found.m_status != LockFreeHashMapOk)
        {
            if (found.m_status == LockFreeHashMapNotFound)
            {
                return NI_NOT_FOUND;
            }
            m_state->requestClose(NI_NO_MEMORY);
            m_state->wake();
            return NI_NO_MEMORY;
        }
        receipt = *snapshot.m_value;
    }
    if (receipt->m_consumed.load())
    {
        return NI_NOT_FOUND;
    }
    const int32_t result = receipt->m_result.load(std::memory_order_acquire);
    if (consume && result != NI_READY)
    {
        if (receipt->m_consumed.exchange(true))
        {
            return NI_NOT_FOUND;
        }
        m_state->m_receiptCount.fetch_sub(1);
        m_state->wake();
    }
    return result;
}

int32_t NetSenderSubmission::observeSynchronous(uint64_t ticket, const std::shared_ptr<NetSenderSubmissionSignal>& signal)
{
    if (!signal) { return NI_INVALID_ARGUMENT; }
    NetObjectReference<NetSenderSubmissionReceipt> receipt;
    if (!m_state->m_tickets.get(ticket, receipt))
    {
        decltype(m_state->m_tickets)::Snapshot snapshot;
        const LockFreeHashMapResult found = m_state->m_tickets.find(ticket, snapshot);
        if (found.m_status != LockFreeHashMapOk)
        {
            return found.m_status == LockFreeHashMapNotFound ? NI_NOT_FOUND : NI_NO_MEMORY;
        }
        receipt = *snapshot.m_value;
    }
    if (receipt->m_consumed.load(std::memory_order_acquire)) { return NI_NOT_FOUND; }
    if ((receipt->m_signalState.fetch_or(1, std::memory_order_acq_rel) & 1) != 0)
    {
        return NI_INVALID_STATE;
    }
    receipt->m_signal = signal;
    // 同一原子字上的RMW形成顺序：后到者看到另一位，恰好承担一次通知。
    if ((receipt->m_signalState.fetch_or(2, std::memory_order_acq_rel) & 4) != 0)
    {
        try { signal->notify(); }
        catch (...) { /* 等待者必须始终以票据状态为准。 */ }
    }
    return NI_OK;
}
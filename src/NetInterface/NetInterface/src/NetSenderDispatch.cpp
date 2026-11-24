#include "NetSenderDispatch.h"
#include "NetInterfaceMacro.h"
#include "NetSenderDispatchMap.h"
#include "NetExecution.h"
#include <cstring>

#if !defined(__EMSCRIPTEN__)
#include <chrono>
#include <thread>
#endif

NetSenderDispatch::NetSenderDispatch(NetClientSession* session) :
m_session(session),
m_submission(&m_local),
m_listener(nullptr),
m_polling(false),
m_callbackDepth(0),
m_closedGeneration(0),
m_error(NI_OK),
m_nextWire(0),
m_mapping(new NetSenderDispatchMap()),
m_outputPending(false),
m_commandBudget(2304),
m_byteBudget(2097152)
{

}

NetSenderDispatch::~NetSenderDispatch()
{
    delete m_mapping;
}

int32_t NetSenderDispatch::bindSubmission(NetSenderSubmission* submission)
{
    if (submission == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_session == nullptr || m_listener != nullptr || executing() || !quiescent())
    {
        return NI_INVALID_STATE;
    }
    m_submission = submission;
    return NI_OK;
}

int32_t NetSenderDispatch::configurePollBudgets(uint32_t commands, uint32_t bytes)
{
    if (m_listener != nullptr || !quiescent()) { return NI_INVALID_STATE; }
    if (commands == 0 || bytes == 0) { return NI_INVALID_ARGUMENT; }
    m_commandBudget = commands;
    m_byteBudget = bytes;
    return NI_OK;
}

int32_t NetSenderDispatch::prepare(NetSessionListener* listener)
{
    if (listener == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_session == nullptr || executing() || !quiescent())
    {
        return NI_INVALID_STATE;
    }
    m_listener = listener;
    m_error = NI_OK;
    return NI_OK;
}

int32_t NetSenderDispatch::started(uint64_t generation, uint32_t product)
{
    return m_submission->start(generation, product);
}

NetSessionListener* NetSenderDispatch::listener()
{
    return this;
}

NetSenderSubmission& NetSenderDispatch::submission()
{
    return *m_submission;
}

bool NetSenderDispatch::executing() const
{
    return m_polling || m_callbackDepth != 0 || (m_session != nullptr && m_session->executing());
}

bool NetSenderDispatch::quiescent() const
{
    return !executing() && m_session != nullptr && m_session->quiescent() && m_submission->idle() &&
        m_mapping->size() == 0 && !m_outputPending;
}

int32_t NetSenderDispatch::error() const
{
    return m_error;
}

void NetSenderDispatch::fail(int32_t reason)
{
    m_error = reason;
    m_submission->close(reason);
    if (m_session != nullptr)
    {
        m_session->close(reason);
    }
}

void NetSenderDispatch::report(const NetSenderSubmissionCommand& metadata, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    if (m_listener == nullptr)
    {
        return;
    }
    ++m_callbackDepth;
    try
    {
        m_listener->onSessionResult(metadata.m_generation, metadata.m_id, metadata.m_product, metadata.m_type, status, bytes, size);
    }
    catch (...)
    {
        fail(NI_INVALID_STATE);
    }
    --m_callbackDepth;
}

void NetSenderDispatch::onSessionReady(uint64_t generation, uint32_t requests, uint32_t chunk)
{
    const int32_t ready = m_submission->ready(generation, requests, chunk);
    if (ready != NI_OK)
    {
        fail(ready);
        return;
    }
    ++m_callbackDepth;
    try
    {
        m_listener->onSessionReady(generation, requests, chunk);
    }
    catch (...)
    {
        fail(NI_INVALID_STATE);
    }
    --m_callbackDepth;
}

void NetSenderDispatch::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    uint64_t current = 0;
    bool ready = false;
    m_submission->state(current, ready);
    if (generation != current)
    {
        return;
    }
    const uint64_t local = m_mapping->local(requestId);
    if (local == 0)
    {
        return;
    }
    m_mapping->erase(requestId);
    NetSenderSubmissionCommand metadata = {};
    if (!m_submission->complete(generation, local, metadata))
    {
        return;
    }
    if (metadata.m_product != product || metadata.m_type != type)
    {
        report(metadata, NI_RPC_FAILED, nullptr, 0);
        fail(NI_PROTOCOL_ERROR);
        return;
    }
    report(metadata, status, bytes, size);
}

bool NetSenderDispatch::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    uint64_t current = 0;
    bool ready = false;
    m_submission->state(current, ready);
    if (!ready || generation != current)
    {
        return true;
    }
    ++m_callbackDepth;
    bool accepted = true;
    try
    {
        accepted = m_listener->onSessionFrame(generation, frame);
    }
    catch (...)
    {
        fail(NI_INVALID_STATE);
    }
    --m_callbackDepth;
    return accepted;
}

void NetSenderDispatch::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    uint64_t current = 0;
    bool ready = false;
    m_submission->state(current, ready);
    if (generation == 0 || generation != current || generation <= m_closedGeneration)
    {
        return;
    }
    m_closedGeneration = generation;
    m_mapping->clear();
    m_outputPending = false;
    m_submission->disconnected(generation);
    NetSenderSubmissionCommand metadata = {};
    while (m_submission->takeClosed(metadata))
    {
        report(metadata, reason == 0 || reason == NI_CANCELLED ? NI_RPC_CANCELLED : NI_RPC_UNAVAILABLE, nullptr, 0);
    }
    if (m_listener == nullptr)
    {
        return;
    }
    ++m_callbackDepth;
    try
    {
        m_listener->onSessionClosed(generation, reason, detail);
    }
    catch (...)
    {
        m_error = NI_INVALID_STATE;
    }
    --m_callbackDepth;
}

void NetSenderDispatch::execute(const NetSenderSubmissionCommand& command)
{
    int32_t result = NI_OK;
    if (command.m_kind == NET_SUBMIT_CLOSE)
    {
        m_session->close(command.m_status);
    }
    else if (command.m_kind == NET_SUBMIT_TERMINAL)
    {
        NetSenderSubmissionCommand metadata = {};
        if (m_submission->complete(command.m_generation, command.m_id, metadata))
        {
            report(metadata, static_cast<uint32_t>(command.m_status), nullptr, 0);
        }
    }
    else if (command.m_kind == NET_SUBMIT_BEGIN)
    {
        uint64_t wire = 0;
        if (m_nextWire == UINT64_MAX)
        {
            result = NI_LIMIT_EXCEEDED;
        }
        else
        {
            wire = ++m_nextWire;
            result = m_mapping->insert(wire, command.m_id);
            if (result == NI_OK)
            {
                result = m_session->beginRequestAt(wire, command.m_product, command.m_type, command.m_started, command.m_timeout);
            }
        }
        if (result != NI_OK)
        {
            m_mapping->erase(wire);
            NetSenderSubmissionCommand metadata = {};
            if (m_submission->complete(command.m_generation, command.m_id, metadata))
            {
                const uint32_t status = result == NI_TIMEOUT ? NI_RPC_TIMEOUT : result == NI_LIMIT_EXCEEDED ? NI_RPC_BUSY : NI_RPC_UNAVAILABLE;
                report(metadata, status, nullptr, 0);
            }
        }
    }
    else if (command.m_kind == NET_SUBMIT_SEND)
    {
        const uint64_t wire = wireId(command.m_id);
        result = wire == 0 ? NI_INVALID_STATE : m_session->sendRequest(wire, command.m_bytes, command.m_size);
        if (result < 0 && result != NI_LIMIT_EXCEEDED && result != NI_TIMEOUT)
        {
            fail(result);
        }
    }
    else if (command.m_kind == NET_SUBMIT_CANCEL)
    {
        const uint64_t wire = wireId(command.m_id);
        result = wire == 0 ? NI_NOT_FOUND : m_session->cancel(wire);
        if (result != NI_OK)
        {
            fail(result);
        }
    }
    else if (command.m_kind == NET_SUBMIT_DATA)
    {
        result = m_session->sendData(command.m_frame);
        if (result != NI_OK && result != NI_LIMIT_EXCEEDED)
        {
            fail(result);
        }
    }
    m_submission->finish(result);
    if (result == NI_OK && (command.m_kind == NET_SUBMIT_SEND || command.m_kind == NET_SUBMIT_DATA))
    {
        m_outputPending = true;
    }
}

uint64_t NetSenderDispatch::wireId(uint64_t local) const
{
    return m_mapping->wire(local);
}

bool NetSenderDispatch::advanceOutput()
{
    if (!m_outputPending)
    {
        return true;
    }
    const int32_t result = m_session->outputState();
    if (m_submission->synchronousPending())
    {
        m_submission->advanceSynchronous(result);
    }
    if (result == NI_READY)
    {
        return false;
    }
    m_outputPending = false;
    if (result != NI_OK)
    {
        fail(result); return false;
    }
    return true;
}

/** 在同一协议owner上接纳一条DATA，不创建任务或改变会话顺序
@param [in] context 当前有效的客户端会话，本次消费借用
@param [in] frame 提交层持有的正文视图，仅调用期间有效
@return 会话接纳结果
*/
static int32_t DispatchData(void* context, const NetInterfaceFrame& frame)
{
    return static_cast<NetClientSession*>(context)->sendData(frame);
}

bool NetSenderDispatch::poll()
{
    if (executing() || m_session == nullptr)
    {
        m_error = NI_INVALID_STATE;
        return false;
    }
    m_polling = true;
    bool active = true;
    try
    {
        int32_t closeReason = 0;
        if (m_submission->closeRequested(closeReason))
        {
            m_session->close(closeReason);
        }
        if (m_submission->synchronousPending())
        {
            m_submission->advanceSynchronous(m_session->outputState());
        }
        // 截止检查可能刚决定丢弃部分发送的后缀，必须先通知协议再推进运输。
        if (m_submission->closeRequested(closeReason))
        {
            m_session->close(closeReason);
        }
        active = m_session->poll();
        if (m_submission->beginPoll() != NI_OK)
        {
            fail(NI_INVALID_STATE);
        }
        uint64_t bytes = 0;
        bool delivered = advanceOutput();
        // 只消费已经就绪的前缀；每轮仍受消息/字节双量子约束，不等待凑批。
        uint32_t step = 0;
        while (step < m_commandBudget && bytes < m_byteBudget)
        {
            uint32_t messages = 0;
            uint64_t dataBytes = 0;
            const int32_t result = m_submission->drainData(m_session, DispatchData, m_commandBudget - step,
                m_byteBudget - static_cast<uint32_t>(bytes), messages, dataBytes);
            step += messages;
            bytes += dataBytes;
            m_outputPending = messages != 0 || m_outputPending;
            if (result != NI_OK)
            {
                if (result != NI_LIMIT_EXCEEDED)
                {
                    fail(result);
                }
                break;
            }
            if (messages != 0)
            {
                delivered = !m_outputPending;
                continue;
            }
            NetSenderSubmissionCommand command;
            if (m_submission->next(command, !delivered) != NI_READY)
            {
                if (!delivered)
                {
                    // 仅控制屏障、同步票据或背压要求推进；普通post可连续进入业务FIFO。
                    active = m_session->poll();
                    delivered = advanceOutput();
                    if (delivered)
                    {
                        continue;
                    }
                }
                break;
            }
            ++step;
            bytes += static_cast<uint64_t>(command.m_size) + command.m_frame.m_payloadSize;
            execute(command);
            // 同一owner尚未推进运输，已提交输出按未排空处理；不为每条DATA重复穿越查询层。
            // 控制屏障或同步票据会令next暂停，再由上面的poll/advanceOutput确认真实交付。
            delivered = !m_outputPending;
        }
        if (step == m_commandBudget || bytes >= m_byteBudget) { NetExecution::progress(); }
        active = m_session->poll();
        if (m_submission->synchronousPending())
        {
            m_submission->advanceSynchronous(m_session->outputState());
        }
    }
    catch (...)
    {
        // 输入固定由提交层持有，异常也必须结束当前借用再结算会话。
        m_submission->finish(NI_NO_MEMORY);
        fail(NI_NO_MEMORY);
    }
    m_polling = false;
    return active || !quiescent();
}

void NetSenderDispatch::shutdown()
{
    m_submission->close(0);
    if (executing() || m_session == nullptr)
    {
        return;
    }
    m_session->shutdown();
    if (m_session->quiescent())
    {
        uint64_t generation = 0;
        bool ready = false;
        m_submission->state(generation, ready);
        onSessionClosed(generation, 0, 0);
    }
}

int32_t NetSenderDispatch::sendRequest(uint64_t id, const uint8_t* bytes, uint32_t size)
{
#if defined(__EMSCRIPTEN__)
    // JS公共send使用异步票据等待，不能阻塞唯一事件循环等待宿主事件。
    (void)id; (void)bytes; (void)size;
    return NI_NOT_SUPPORTED;
#else
    if (executing() || m_session == nullptr)
    {
        return NI_INVALID_STATE;
    }
    int32_t capability = NI_INVALID_STATE;
    try { capability = m_session->outputState(); }
    catch (...) { return NI_INVALID_STATE; }
    if (capability < 0)
    {
        return capability;
    }
    uint64_t ticket = 0;
    const int32_t accepted = m_submission->sendSynchronous(id, bytes, size, ticket);
    return accepted == NI_OK ? waitSynchronous(ticket) : accepted;
#endif
}

int32_t NetSenderDispatch::sendData(const NetInterfaceFrame& frame)
{
#if defined(__EMSCRIPTEN__)
    (void)frame;
    return NI_NOT_SUPPORTED;
#else
    if (executing() || m_session == nullptr)
    {
        return NI_INVALID_STATE;
    }
    int32_t capability = NI_INVALID_STATE;
    try { capability = m_session->outputState(); }
    catch (...) { return NI_INVALID_STATE; }
    if (capability < 0)
    {
        return capability;
    }
    uint64_t ticket = 0;
    const int32_t accepted = m_submission->dataSynchronous(frame, ticket);
    return accepted == NI_OK ? waitSynchronous(ticket) : accepted;
#endif
}

int32_t NetSenderDispatch::waitSynchronous(uint64_t ticket)
{
#if defined(__EMSCRIPTEN__)
    (void)ticket;
    return NI_NOT_SUPPORTED;
#else
    for (;;)
    {
        const int32_t result = m_submission->synchronousResult(ticket, true);
        if (result != NI_READY)
        {
            return result;
        }
        poll();
        if (m_submission->synchronousResult(ticket, false) == NI_READY)
        {
            try { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
            catch (...) { fail(NI_INVALID_STATE); }
        }
    }
#endif
}
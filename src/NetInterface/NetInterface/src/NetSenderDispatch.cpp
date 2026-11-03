#include "NetInterfaceMacro.h"
#include "NetSenderDispatch.h"
#include "NetSenderDispatchMap.h"
#include <cstring>
#if !defined(__EMSCRIPTEN__)
#include <chrono>
#include <thread>
#endif

NetSenderDispatchMap::NetSenderDispatchMap()
{
    clear();
}

uint32_t NetSenderDispatchMap::hash(uint64_t key)
{
    return static_cast<uint32_t>(key ^ (key >> 32)) & 255;
}

uint32_t NetSenderDispatchMap::bucket(uint64_t key, bool byLocal) const
{
    const uint16_t* index = byLocal ? m_local : m_wire;
    uint32_t position = hash(key);
    for (uint32_t step = 0; step < 256; ++step)
    {
        const uint32_t slot = index[position];
        if (slot == 0 || (byLocal ? m_entries[slot - 1].m_local : m_entries[slot - 1].m_wire) == key)
        {
            return position;
        }
        position = (position + 1) & 255;
    }
    return 256;
}

bool NetSenderDispatchMap::insert(uint64_t wire, uint64_t local)
{
    if (wire == 0 || local == 0 || m_free == 0)
    {
        return false;
    }
    const uint32_t wireBucket = bucket(wire, false);
    const uint32_t localBucket = bucket(local, true);
    if (wireBucket == 256 || localBucket == 256 || m_wire[wireBucket] != 0 || m_local[localBucket] != 0)
    {
        return false;
    }
    const uint32_t slot = m_free;
    NetSenderDispatchEntry& entry = m_entries[slot - 1];
    m_free = static_cast<uint32_t>(entry.m_local);
    entry.m_wire = wire;
    entry.m_local = local;
    m_wire[wireBucket] = static_cast<uint16_t>(slot);
    m_local[localBucket] = static_cast<uint16_t>(slot);
    ++m_count;
    return true;
}

uint64_t NetSenderDispatchMap::local(uint64_t wire) const
{
    const uint32_t position = bucket(wire, false);
    return position == 256 || m_wire[position] == 0 ? 0 : m_entries[m_wire[position] - 1].m_local;
}

uint64_t NetSenderDispatchMap::wire(uint64_t local) const
{
    const uint32_t position = bucket(local, true);
    return position == 256 || m_local[position] == 0 ? 0 : m_entries[m_local[position] - 1].m_wire;
}

void NetSenderDispatchMap::remove(uint64_t key, bool byLocal)
{
    uint16_t* index = byLocal ? m_local : m_wire;
    uint32_t hole = bucket(key, byLocal);
    if (hole == 256 || index[hole] == 0)
    {
        return;
    }
    uint32_t next = (hole + 1) & 255;
    for (uint32_t step = 0; step < 256 && index[next] != 0; ++step)
    {
        const NetSenderDispatchEntry& entry = m_entries[index[next] - 1];
        const uint32_t origin = hash(byLocal ? entry.m_local : entry.m_wire);
        // 仅把探测路径经过空洞的槽前移，跨数组末端仍按环形距离比较。
        if (((hole - origin) & 255) < ((next - origin) & 255))
        {
            index[hole] = index[next];
            hole = next;
        }
        next = (next + 1) & 255;
    }
    index[hole] = 0;
}

void NetSenderDispatchMap::erase(uint64_t wire)
{
    const uint32_t position = bucket(wire, false);
    if (position == 256 || m_wire[position] == 0)
    {
        return;
    }
    const uint32_t slot = m_wire[position];
    NetSenderDispatchEntry& entry = m_entries[slot - 1];
    remove(entry.m_wire, false);
    remove(entry.m_local, true);
    entry.m_wire = 0;
    entry.m_local = m_free;
    m_free = slot;
    --m_count;
}

void NetSenderDispatchMap::clear()
{
    std::memset(m_wire, 0, sizeof(m_wire));
    std::memset(m_local, 0, sizeof(m_local));
    for (uint32_t slot = 0; slot < 128; ++slot)
    {
        m_entries[slot].m_wire = 0;
        m_entries[slot].m_local = slot == 127 ? 0 : slot + 2;
    }
    m_free = 1;
    m_count = 0;
}

uint32_t NetSenderDispatchMap::size() const
{
    return m_count;
}

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
m_outputPending(false)
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
            result = m_mapping->insert(wire, command.m_id) ?
                m_session->beginRequestAt(wire, command.m_product, command.m_type, command.m_started, command.m_timeout) : NI_LIMIT_EXCEEDED;
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
        uint32_t bytes = 0;
        for (uint32_t step = 0; step < 64 && bytes < 2097152; ++step)
        {
            if (!advanceOutput())
            {
                // 允许已接纳帧及内部维护信令继续推进；不在后面追加可能越过的业务帧。
                active = m_session->poll();
                if (!advanceOutput())
                {
                    break;
                }
            }
            NetSenderSubmissionCommand command = {};
            if (m_submission->next(command) != NI_READY)
            {
                break;
            }
            bytes += command.m_size + command.m_frame.m_payloadSize;
            execute(command);
        }
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

// NET_SENDER_DISPATCH_UNIT_BEGIN
//#if defined(STATIC_LIB) || defined(__EMSCRIPTEN__)
//#include "NetInterface/NetSenderDispatchMap.h"
//#endif
//#include <map>
//#include <iterator>
//#include "NetInterface/NetSenderDispatchTest.h"
//#include <cstdio>
//#include <cstring>
//NetSenderDispatchTestChannel::NetSenderDispatchTestChannel(const NetInterfaceHandshakeOptions& options) :
//m_reply(),
//m_handshake(new NetInterfaceHandshake(options, NetSenderSubmission::now())),
//m_listener(nullptr),
//m_announced(false),
//m_closing(false),
//m_reason(0)
//{
//
//}
//
//NetSenderDispatchTestChannel::~NetSenderDispatchTestChannel()
//{
//    shutdown();
//}
//
//int32_t NetSenderDispatchTestChannel::connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
//    NetTransportListener* listener, uint64_t& peer)
//{
//    (void)address;
//    (void)port;
//    (void)options;
//    peer = 0;
//    if (m_listener != nullptr || listener == nullptr)
//    {
//        return NI_INVALID_STATE;
//    }
//    m_listener = listener;
//    peer = 1;
//    return NI_OK;
//}
//
//bool NetSenderDispatchTestChannel::poll()
//{
//    if (m_closing)
//    {
//        shutdown();
//        return false;
//    }
//    if (m_listener == nullptr)
//    {
//        return false;
//    }
//    if (!m_announced)
//    {
//        m_announced = true;
//        m_listener->onReady(1);
//    }
//    NetInterfaceFrame frame = {};
//    if (!m_closing && m_handshake->peekOutput(frame) == NI_READY && m_listener->onFrame(1, frame))
//    {
//        m_handshake->consumeOutput();
//    }
//    if (!m_closing && m_reply.m_kind != 0 && m_listener->onFrame(1, m_reply))
//    {
//        m_reply = NetInterfaceFrame();
//        m_payload.clear();
//    }
//    return true;
//}
//
//int32_t NetSenderDispatchTestChannel::send(uint64_t peer, const NetInterfaceFrame& frame)
//{
//    if (peer != 1 || m_listener == nullptr || m_closing)
//    {
//        return NI_INVALID_STATE;
//    }
//    if (frame.m_kind == NI_REQUEST)
//    {
//        if (m_reply.m_kind != 0) { return NI_LIMIT_EXCEEDED; }
//        NetInterfaceRpcView view = {};
//        if (NetInterfaceRpc::parse(frame, view) != NI_OK)
//{
//    return NI_PROTOCOL_ERROR;
//}

//        m_requests.push_back(frame.m_requestId);
//        m_bytes.push_back(view.m_size == 0 ? 0 : view.m_data[0]);
//        m_payload.resize(view.m_size + 16);
//        uint32_t required = 0;
//        const int32_t encoded = NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, view.m_data, view.m_size, m_payload.data(), static_cast<uint32_t>(m_payload.size()), required);
//        if (encoded != NI_OK) { return encoded; }
//        m_reply = frame;
//        m_reply.m_kind = NI_RESPONSE;
//        m_reply.m_payload = m_payload.data();
//        m_reply.m_payloadSize = required;
//        return NI_OK;
//    }
//    if (frame.m_kind == NI_CANCEL) { return NI_OK; }
//    const int32_t accepted = m_handshake->accept(frame, NetSenderSubmission::now());
//    return accepted < 0 ? accepted : NI_OK;
//}
//
//void NetSenderDispatchTestChannel::close(uint64_t peer, int32_t reason)
//{
//    if (peer == 1 && !m_closing)
//    {
//        m_closing = true;
//        m_reason = reason;
//    }
//}
//
//int32_t NetSenderDispatchTestChannel::outputState(uint64_t peer) const
//{
//    return peer == 1 && m_listener != nullptr && !m_closing ? NI_OK : NI_INVALID_STATE;
//}
//
//void NetSenderDispatchTestChannel::shutdown()
//{
//    m_closing = true;
//    NetTransportListener* listener = m_listener;
//    m_listener = nullptr;
//    if (listener != nullptr)
//    {
//        listener->onClosed(1, m_reason, 0);
//    }
//}
//
//
//NetSenderDispatchTestListener::NetSenderDispatchTestListener() :
//m_dispatch(nullptr), m_ready(0), m_closed(0), m_bad(false)
//{
//}
//void NetSenderDispatchTestListener::onSessionReady(uint64_t generation, uint32_t requests, uint32_t chunk)
//{
//    ++m_ready;
//    m_bad = m_bad || generation == 0 || requests != 4 || chunk != 4096;
//}
//void NetSenderDispatchTestListener::onSessionResult(uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
//{
//    (void)type;
//    m_bad = m_bad || generation == 0 || product != 101 || !m_dispatch->executing() || m_dispatch->quiescent();
//    for (size_t i = 0; i < m_ids.size(); ++i) { m_bad = m_bad || m_ids[i] == id; }
//    m_ids.push_back(id);
//    m_status.push_back(status);
//    if (status == NI_RPC_SUCCESS) { m_bad = m_bad || size != 1 || bytes == nullptr || bytes[0] != 7; }
//    m_dispatch->shutdown();
//}
//bool NetSenderDispatchTestListener::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
//{
//    (void)generation; (void)frame; m_bad = true; return true;
//}
//void NetSenderDispatchTestListener::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
//{
//    (void)reason; (void)detail; m_bad = m_bad || generation == 0; ++m_closed;
//}
//int32_t NetSenderDispatchTestListener::entropy(void* context, uint8_t* bytes, uint32_t size)
//{
//    (void)context; std::memset(bytes, 0x41, size); return 0;
//}
//#include "NetInterface/NetSenderDispatchAuditTest.h"
//#include <cstdio>
//#include <cstring>
//NetSenderDispatchAuditChannel::NetSenderDispatchAuditChannel(const NetInterfaceHandshakeOptions& options) :
//m_reply(), m_autoReply(true), m_busy(false),
//m_handshake(new NetInterfaceHandshake(options, NetSenderSubmission::now())),
//m_listener(nullptr),
//m_announced(false),
//m_closing(false),
//m_reason(0)
//{
//
//}
//
//NetSenderDispatchAuditChannel::~NetSenderDispatchAuditChannel()
//{
//    shutdown();
//}
//
//int32_t NetSenderDispatchAuditChannel::connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
//    NetTransportListener* listener, uint64_t& peer)
//{
//    (void)address;
//    (void)port;
//    (void)options;
//    peer = 0;
//    if (m_listener != nullptr || listener == nullptr)
//    {
//        return NI_INVALID_STATE;
//    }
//    m_listener = listener;
//    peer = 1;
//    return NI_OK;
//}
//
//bool NetSenderDispatchAuditChannel::poll()
//{
//    if (m_closing)
//    {
//        shutdown();
//        return false;
//    }
//    if (m_listener == nullptr)
//    {
//        return false;
//    }
//    if (!m_announced)
//    {
//        m_announced = true;
//        m_listener->onReady(1);
//    }
//    NetInterfaceFrame frame = {};
//    if (!m_closing && m_handshake->peekOutput(frame) == NI_READY && m_listener->onFrame(1, frame))
//    {
//        m_handshake->consumeOutput();
//    }
//    if (!m_closing && m_reply.m_kind != 0 && m_listener->onFrame(1, m_reply))
//    {
//        m_reply = NetInterfaceFrame();
//        m_payload.clear();
//    }
//    return true;
//}
//
//int32_t NetSenderDispatchAuditChannel::send(uint64_t peer, const NetInterfaceFrame& frame)
//{
//    if (peer != 1 || m_listener == nullptr || m_closing)
//    {
//        return NI_INVALID_STATE;
//    }
//    if (frame.m_kind == NI_REQUEST)
//    {
//        if (m_busy) { return NI_LIMIT_EXCEEDED; }
//        if (m_reply.m_kind != 0) { return NI_LIMIT_EXCEEDED; }
//        NetInterfaceRpcView view = {};
//        if (NetInterfaceRpc::parse(frame, view) != NI_OK)
//{
//    return NI_PROTOCOL_ERROR;
//}

//        m_requests.push_back(frame.m_requestId);
//        m_bytes.push_back(view.m_size == 0 ? 0 : view.m_data[0]);
//        if (!m_autoReply) { return NI_OK; }
//        m_payload.resize(view.m_size + 16);
//        uint32_t required = 0;
//        const int32_t encoded = NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, view.m_data, view.m_size, m_payload.data(), static_cast<uint32_t>(m_payload.size()), required);
//        if (encoded != NI_OK) { return encoded; }
//        m_reply = frame;
//        m_reply.m_kind = NI_RESPONSE;
//        m_reply.m_payload = m_payload.data();
//        m_reply.m_payloadSize = required;
//        return NI_OK;
//    }
//    if (frame.m_kind == NI_CANCEL) { return NI_OK; }
//    const int32_t accepted = m_handshake->accept(frame, NetSenderSubmission::now());
//    return accepted < 0 ? accepted : NI_OK;
//}
//
//void NetSenderDispatchAuditChannel::close(uint64_t peer, int32_t reason)
//{
//    if (peer == 1 && !m_closing)
//    {
//        m_closing = true;
//        m_reason = reason;
//    }
//}
//
//int32_t NetSenderDispatchAuditChannel::outputState(uint64_t peer) const
//{
//    return peer == 1 && m_listener != nullptr && !m_closing ? NI_OK : NI_INVALID_STATE;
//}
//
//void NetSenderDispatchAuditChannel::shutdown()
//{
//    m_closing = true;
//    NetTransportListener* listener = m_listener;
//    m_listener = nullptr;
//    if (listener != nullptr)
//    {
//        listener->onClosed(1, m_reason, 0);
//    }
//}
//
//
//NetSenderDispatchAuditListener::NetSenderDispatchAuditListener() :
//m_dispatch(nullptr), m_ready(0), m_closed(0), m_bad(false)
//{
//}
//void NetSenderDispatchAuditListener::onSessionReady(uint64_t generation, uint32_t requests, uint32_t chunk)
//{
//    ++m_ready;
//    m_bad = m_bad || generation == 0 || requests != 128 || chunk != 4096;
//}
//void NetSenderDispatchAuditListener::onSessionResult(uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
//{
//    (void)type;
//    m_bad = m_bad || generation == 0 || product != 101 || !m_dispatch->executing() || m_dispatch->quiescent();
//    for (size_t i = 0; i < m_ids.size(); ++i) { m_bad = m_bad || m_ids[i] == id; }
//    m_ids.push_back(id);
//    m_status.push_back(status);
//    if (status == NI_RPC_SUCCESS) { m_bad = m_bad || size != 1 || bytes == nullptr || bytes[0] != 7; }
//
//}
//bool NetSenderDispatchAuditListener::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
//{
//    (void)generation; (void)frame; m_bad = true; return true;
//}
//void NetSenderDispatchAuditListener::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
//{
//    (void)reason; (void)detail; m_bad = m_bad || generation == 0; ++m_closed;
//}
//int32_t NetSenderDispatchAuditListener::entropy(void* context, uint8_t* bytes, uint32_t size)
//{
//    (void)context; std::memset(bytes, 0x41, size); return 0;
//}
//
//#if defined(_WIN32) && defined(_DEBUG)
//#include <crtdbg.h>
//static int32_t g_dispatchRejectAfter = -1;
//static uint32_t g_dispatchAllocations = 0;
///** 独立单元的映射分配检测与构造故障
//@param [in] operation CRT操作
//@param [in] data 未使用
//@param [in] size 未使用
//@param [in] block 内存类型
//@param [in] request 未使用
//@param [in] file 未使用
//@param [in] line 未使用
//@return 是否允许分配
//*/
//static int DispatchMemoryHook(int operation, void*, size_t, int block, long, const unsigned char*, int)
//{
//    if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK)
//    {
//        ++g_dispatchAllocations;
//        if (g_dispatchRejectAfter == 0) { return 0; }
//        if (g_dispatchRejectAfter > 0) { --g_dispatchRejectAfter; }
//    }
//    return 1;
//}
//#endif
//int main()
//{
//    uint32_t checks = 0;
//#define DISPATCH_CHECK(condition) { ++checks; if (!(condition)) { std::printf("FAIL line=%u %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
//#if defined(_WIN32) && defined(_DEBUG)
//    _CrtMemState before;
//    _CrtMemCheckpoint(&before);
//#endif
//    {
//    for (uint32_t mode = 0; mode < 4; ++mode)
//    {
//        NetInterfaceHandshakeOptions client = {};
//        client.m_platform = 1; client.m_capabilities = 7; client.m_maxRequests = 4; client.m_maxChunk = 4096; client.m_timeoutMs = 5000;
//        std::memset(client.m_device, 0x41, 16); std::memset(client.m_nonce, 0x42, 16);
//        NetInterfaceHandshakeOptions server = client;
//        server.m_server = 1; server.m_platform = 0;
//        std::memset(server.m_device, 0, 16); std::memset(server.m_nonce, 0x43, 16);
//        NetSenderDispatchTestChannel channel(server);
//        NetClientSession session;
//        NetSenderDispatch dispatch(&session);
//        NetSenderDispatchTestListener listener;
//        listener.m_dispatch = &dispatch;
//        DISPATCH_CHECK(session.bindChannel(&channel) == NI_OK);
//        DISPATCH_CHECK(dispatch.prepare(&listener) == NI_OK);
//        NetInterfaceTlsOptions tls = {}; tls.handshakeTimeoutMs = 5000; tls.entropy = listener.entropy;
//        DISPATCH_CHECK(session.connect("mock", 1, tls, client, dispatch.listener()) == NI_OK);
//        DISPATCH_CHECK(dispatch.started(session.generation(), 0) == NI_OK);
//        for (uint32_t i = 0; i < 8 && listener.m_ready == 0; ++i) { dispatch.poll(); }
//        DISPATCH_CHECK(listener.m_ready == 1 && !listener.m_bad);
//        uint64_t id = 0;
//        DISPATCH_CHECK(dispatch.submission().begin(101, 10, mode == 2 ? 5 : 1000, id) == NI_OK && id != 0);
//        if (mode == 0)
//        {
//            uint8_t data = 7;
//            DISPATCH_CHECK(dispatch.submission().send(id, &data, 1) == NI_OK);
//            data = 99;
//        }
//        if (mode == 1) { DISPATCH_CHECK(dispatch.submission().cancel(id) == NI_OK); }
//        if (mode == 2) { const uint64_t until = NetSenderSubmission::now() + 10; while (NetSenderSubmission::now() < until) {} }
//        if (mode == 3) { dispatch.submission().close(0); }
//        for (uint32_t i = 0; i < 16 && !dispatch.quiescent(); ++i) { dispatch.poll(); }
//        DISPATCH_CHECK(dispatch.quiescent() && !listener.m_bad && listener.m_closed == 1);
//        DISPATCH_CHECK(listener.m_ids.size() == 1 && listener.m_ids[0] == id);
//        DISPATCH_CHECK(listener.m_status[0] == (mode == 0 ? NI_RPC_SUCCESS : mode == 2 ? NI_RPC_TIMEOUT : NI_RPC_CANCELLED));
//        DISPATCH_CHECK(channel.m_requests.size() == (mode == 0 ? 1 : 0));
//        if (mode == 0) { DISPATCH_CHECK(channel.m_bytes[0] == 7); }
//        DISPATCH_CHECK(dispatch.submission().cancel(id) == NI_NOT_FOUND);
//        DISPATCH_CHECK(!dispatch.poll() && listener.m_closed == 1);
//    }
//
//    for (uint32_t mode = 0; mode < 3; ++mode)
//    {
//        NetInterfaceHandshakeOptions options = {};
//        options.m_platform = 1;
//        options.m_capabilities = 7;
//        options.m_maxRequests = 128;
//        options.m_maxChunk = 4096;
//        options.m_timeoutMs = 5000;
//        std::memset(options.m_device, 0x41, 16);
//        std::memset(options.m_nonce, 0x42, 16);
//        NetInterfaceHandshakeOptions server = options;
//        server.m_server = 1;
//        server.m_platform = 0;
//        std::memset(server.m_device, 0, 16);
//        std::memset(server.m_nonce, 0x43, 16);
//        NetSenderDispatchAuditChannel channel(server);
//        NetClientSession session;
//        NetSenderDispatch dispatch(&session);
//        NetSenderDispatchAuditListener listener;
//        listener.m_dispatch = &dispatch;
//        DISPATCH_CHECK(session.bindChannel(&channel) == NI_OK && dispatch.prepare(&listener) == NI_OK);
//        NetInterfaceTlsOptions tls = {};
//        tls.handshakeTimeoutMs = 5000;
//        tls.entropy = listener.entropy;
//        DISPATCH_CHECK(session.connect("mock", 1, tls, options, dispatch.listener()) == NI_OK);
//        DISPATCH_CHECK(dispatch.started(session.generation(), 0) == NI_OK);
//        for (uint32_t step = 0; step < 16 && listener.m_ready == 0; ++step) { dispatch.poll(); }
//        DISPATCH_CHECK(listener.m_ready == 1 && !listener.m_bad);
//        channel.m_autoReply = false;
//        const uint32_t count = mode == 0 ? 128 : 2;
//        uint64_t ids[128] = {};
//        uint8_t byte = 7;
//        for (uint32_t index = 0; index < count; ++index)
//        {
//            DISPATCH_CHECK(dispatch.submission().begin(101, 700, 5000, ids[index]) == NI_OK);
//        }
//        for (uint32_t index = 0; index < count; ++index)
//        {
//            DISPATCH_CHECK(dispatch.submission().send(ids[count - index - 1], &byte, 1) == NI_OK);
//        }
//        channel.m_busy = mode == 1;
//        if (channel.m_busy)
//        {
//            for (uint32_t step = 0; step < 128; ++step) { dispatch.poll(); }
//            DISPATCH_CHECK(channel.m_requests.empty() && listener.m_ids.empty());
//            channel.m_busy = false;
//        }
//        for (uint32_t step = 0; step < 16 && channel.m_requests.size() != count; ++step) { dispatch.poll(); }
//        DISPATCH_CHECK(channel.m_requests.size() == count && listener.m_ids.empty());
//        const uint64_t generation = session.generation();
//        // 注入不属于当前代的内部回调，不能摘除当前请求或把错误代次报告给业务。
//        dispatch.listener()->onSessionResult(generation + 1, channel.m_requests[0], 101, 700, NI_RPC_SUCCESS, &byte, 1);
//        dispatch.listener()->onSessionClosed(generation + 1, NI_INVALID_STATE, 0);
//        dispatch.listener()->onSessionResult(generation, UINT64_MAX, 101, 700, NI_RPC_SUCCESS, &byte, 1);
//        DISPATCH_CHECK(listener.m_ids.empty() && listener.m_closed == 0);
//        uint64_t observed = 0;
//        bool ready = false;
//        dispatch.submission().state(observed, ready);
//        DISPATCH_CHECK(observed == generation && ready);
//        if (mode == 2)
//        {
//            for (uint32_t index = 0; index < count; ++index)
//            {
//                DISPATCH_CHECK(dispatch.submission().cancel(ids[index]) == NI_OK);
//            }
//            for (uint32_t step = 0; step < 8 && listener.m_ids.size() != count; ++step) { dispatch.poll(); }
//            DISPATCH_CHECK(listener.m_ids.size() == count);
//            for (uint32_t index = 0; index < count; ++index) { DISPATCH_CHECK(listener.m_status[index] == NI_RPC_CANCELLED); }
//        }
//        else
//        {
//            for (uint32_t index = 0; index < count; ++index)
//            {
//                channel.m_payload.resize(17);
//                uint32_t required = 0;
//                DISPATCH_CHECK(NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, &byte, 1,
//                    channel.m_payload.data(), 17, required) == NI_OK);
//                channel.m_reply = NetInterfaceFrame();
//                channel.m_reply.m_kind = NI_RESPONSE;
//                channel.m_reply.m_productId = 101;
//                channel.m_reply.m_messageType = 700;
//                channel.m_reply.m_requestId = channel.m_requests[count - index - 1];
//                channel.m_reply.m_payload = channel.m_payload.data();
//                channel.m_reply.m_payloadSize = required;
//                dispatch.poll();
//                DISPATCH_CHECK(listener.m_ids.size() == index + 1 && listener.m_ids.back() == ids[index] && listener.m_status.back() == NI_RPC_SUCCESS);
//            }
//        }
//        dispatch.shutdown();
//        DISPATCH_CHECK(dispatch.quiescent() && listener.m_closed == 1 && !listener.m_bad);
//    }
//
//#if defined(STATIC_LIB) || defined(__EMSCRIPTEN__)
//    {
//        NetSenderDispatchMap table;
//        std::map<uint64_t, uint64_t> expected;
//        uint32_t random = 341123;
//        uint64_t sequence = 0;
//        for (uint32_t step = 0; step < 4096; ++step)
//        {
//            random = random * 1664525U + 1013904223U;
//            if (step < 128 || (expected.size() < 128 && ((random >> 16) % 3 != 0 || expected.empty())))
//            {
//                ++sequence;
//                // 两半异或固定为254，同时覆盖最长碰撞链和跨数组末端回移。
//                const uint64_t wire = (sequence << 32) | (sequence ^ 254);
//                const uint64_t local = ((sequence + 0x10000) << 32) | ((sequence + 0x10000) ^ 254);
//                DISPATCH_CHECK(table.insert(wire, local));
//                expected[wire] = local;
//                DISPATCH_CHECK(!table.insert(wire, local + 1) && !table.insert(wire + 1, local));
//            }
//            else
//            {
//                auto item = expected.begin();
//                std::advance(item, random % expected.size());
//                const uint64_t wire = item->first;
//                const uint64_t local = item->second;
//                table.erase(wire);
//                expected.erase(item);
//                DISPATCH_CHECK(table.local(wire) == 0 && table.wire(local) == 0);
//                table.erase(wire);
//            }
//            DISPATCH_CHECK(table.size() == expected.size());
//            DISPATCH_CHECK(!table.insert(0, UINT64_MAX) && !table.insert(UINT64_MAX, 0));
//            DISPATCH_CHECK(table.local(0) == 0 && table.wire(0) == 0);
//            if (expected.size() == 128) { DISPATCH_CHECK(!table.insert(UINT64_MAX, UINT64_MAX)); }
//            for (auto item = expected.begin(); item != expected.end(); ++item)
//            {
//                DISPATCH_CHECK(table.local(item->first) == item->second && table.wire(item->second) == item->first);
//            }
//        }
//        table.clear();
//        DISPATCH_CHECK(table.size() == 0 && table.local(UINT64_MAX) == 0 && table.wire(UINT64_MAX) == 0);
//        DISPATCH_CHECK(table.insert(UINT64_MAX, UINT64_MAX) && table.local(UINT64_MAX) == UINT64_MAX && table.wire(UINT64_MAX) == UINT64_MAX);
//        table.erase(UINT64_MAX);
//        DISPATCH_CHECK(table.size() == 0);
//    }
//#endif
//
//#if defined(_WIN32) && defined(_DEBUG) && defined(STATIC_LIB)
//    {
//        NetSenderDispatchMap table;
//        g_dispatchAllocations = 0;
//        g_dispatchRejectAfter = 0;
//        const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(DispatchMemoryHook);
//        bool valid = true;
//        for (uint32_t round = 0; round < 128; ++round)
//        {
//            for (uint32_t index = 1; index <= 128; ++index) { valid = valid && table.insert(index, 129 - index); }
//            for (uint32_t index = 1; index <= 128; ++index) { valid = valid && table.wire(129 - index) == index; table.erase(index); }
//        }
//        _CrtSetAllocHook(previous);
//        g_dispatchRejectAfter = -1;
//        DISPATCH_CHECK(valid && g_dispatchAllocations == 0 && table.size() == 0);
//    }
//    {
//        NetClientSession session;
//        bool complete = false;
//        uint32_t failures = 0;
//        for (int32_t step = 0; step < 32 && !complete; ++step)
//        {
//            g_dispatchRejectAfter = step;
//            const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(DispatchMemoryHook);
//            NetSenderDispatch* dispatch = nullptr;
//            try { dispatch = new NetSenderDispatch(&session); }
//            catch (...) { ++failures; }
//            _CrtSetAllocHook(previous);
//            g_dispatchRejectAfter = -1;
//            complete = dispatch != nullptr;
//            delete dispatch;
//        }
//        DISPATCH_CHECK(complete && failures != 0);
//    }
//#endif
//
//    }
//#if defined(_WIN32) && defined(_DEBUG)
//    _CrtMemState after;
//    _CrtMemCheckpoint(&after);
//    DISPATCH_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
//#endif
//    std::printf("Dispatch checks=%u PASS\n", checks);
//    return 0;
//#undef DISPATCH_CHECK
//}
// NET_SENDER_DISPATCH_UNIT_END
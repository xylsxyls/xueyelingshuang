#include "NetInterfaceMacro.h"
#include "NetInterfaceFileFlowState.h"
#include "NetInterfaceFileGuard.h"
#include <algorithm>
#include <cstring>

NetInterfaceFileFlowState::NetInterfaceFileFlowState(const NetInterfaceFileInfo& info, uint32_t product, bool sender,
    uint64_t nowMs, uint32_t timeoutMs) :
m_info(info),
m_hash(nullptr),
m_phase(info.offset == 0 ? NI_FILE_PHASE_TRANSFER : NI_FILE_PHASE_PREFIX),
m_outputKind(NI_FILE_OUTPUT_NONE),
m_primed(0),
m_position(info.offset),
m_acknowledged(info.offset),
m_now(nowMs),
m_progress(nowMs),
m_outputAckOffset(0),
m_product(product),
m_timeout(timeoutMs),
m_outputSize(0),
m_writeBatchChunks(1),
m_outputAckFlags(0),
m_error(NI_OK),
m_sender(sender),
m_writeTaken(false),
m_ackPending(false)
{
    uint8_t probe[NET_INTERFACE_FILE_INFO_SIZE] = {};
    NetInterfaceFileGuard wipe(probe, sizeof(probe));
    if (product == 0 || timeoutMs < 1000 || timeoutMs > 60000 || info.state != NI_FILE_TRANSFERRING ||
        NetInterfaceFile::encodeInfo(info, probe, sizeof(probe)) != NI_OK)
    {
        m_error = NI_INVALID_ARGUMENT;
    }
    else
    {
        m_error = NetInterface::hashCreate(&m_hash);
    }
    if (m_error != NI_OK)
    {
        m_phase = NI_FILE_PHASE_FAILED;
    }
}

NetInterfaceFileFlowState::~NetInterfaceFileFlowState()
{
    clearData();
    clearOutput();
    NetInterface::wipe(reinterpret_cast<uint8_t*>(&m_info), sizeof(m_info));
    NetInterface::hashDestroy(m_hash);
}

void NetInterfaceFileFlowState::clearData()
{
    NetInterface::wipe(m_data.empty() ? nullptr : m_data.data(), static_cast<uint32_t>(m_data.size()));
    m_data.clear();
    m_writeTaken = false;
}

void NetInterfaceFileFlowState::clearOutput()
{
    NetInterface::wipe(m_output.empty() ? nullptr : m_output.data(), static_cast<uint32_t>(m_output.size()));
    m_output.clear();
    m_outputKind = NI_FILE_OUTPUT_NONE;
    m_outputSize = 0;
}

void NetInterfaceFileFlowState::prepareOutput(uint32_t size)
{
    // 只在空输出槽使用；在写入秘密之前一次预留本角色的最大帧。
    m_output.reserve(m_sender ? NET_INTERFACE_FILE_DATA_HEADER_SIZE + m_info.chunk : NET_INTERFACE_FILE_ACK_SIZE);
    m_output.resize(size);
}

int32_t NetInterfaceFileFlowState::status() const
{
    return m_phase == NI_FILE_PHASE_FAILED ? m_error : m_phase == NI_FILE_PHASE_DONE ? NI_READY : NI_OK;
}

NetInterfaceFileFlow::NetInterfaceFileFlow(const NetInterfaceFileInfo& info, uint32_t product, bool sender, uint64_t nowMs,
    uint32_t timeoutMs) :
m_state(new NetInterfaceFileFlowState(info, product, sender, nowMs, timeoutMs))
{

}

NetInterfaceFileFlow::~NetInterfaceFileFlow()
{
    delete m_state;
}

int32_t NetInterfaceFileFlow::setWriteBatchChunks(uint32_t chunks)
{
    if (chunks == 0 || chunks > m_state->m_info.window)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_state->m_sender || (m_state->m_phase != NI_FILE_PHASE_PREFIX && m_state->m_phase != NI_FILE_PHASE_TRANSFER) ||
        m_state->m_position != m_state->m_info.offset || !m_state->m_data.empty())
    {
        return NI_INVALID_STATE;
    }
    m_state->m_writeBatchChunks = chunks;
    return NI_OK;
}

int32_t NetInterfaceFileFlow::fail(int32_t result)
{
    if (m_state->m_phase != NI_FILE_PHASE_DONE && m_state->m_phase != NI_FILE_PHASE_FAILED)
    {
        m_state->m_error = result < 0 ? result : NI_INVALID_STATE;
        m_state->m_phase = NI_FILE_PHASE_FAILED;
        m_state->clearData();
        m_state->clearOutput();
        m_state->m_ackPending = !m_state->m_sender;
    }
    return m_state->status();
}

int32_t NetInterfaceFileFlow::prime(uint64_t offset, const uint8_t* bytes, uint32_t size)
{
    if (m_state->m_phase == NI_FILE_PHASE_FAILED)
    {
        return m_state->m_error;
    }
    if (m_state->m_phase != NI_FILE_PHASE_PREFIX || offset != m_state->m_primed || size == 0 ||
        size > m_state->m_info.offset - m_state->m_primed)
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t result = NetInterface::hashUpdate(m_state->m_hash, bytes, size);
    if (result != NI_OK)
    {
        return fail(result);
    }
    m_state->m_primed += size;
    m_state->m_progress = m_state->m_now;
    if (m_state->m_primed == m_state->m_info.offset)
    {
        m_state->m_phase = NI_FILE_PHASE_TRANSFER;
    }
    return NI_OK;
}

int32_t NetInterfaceFileFlow::progress(uint64_t nowMs)
{
    if (m_state->m_phase == NI_FILE_PHASE_FAILED || m_state->m_phase == NI_FILE_PHASE_DONE)
    {
        return m_state->status();
    }
    if (nowMs < m_state->m_now)
    {
        return fail(NI_INVALID_ARGUMENT);
    }
    m_state->m_now = nowMs;
    if (nowMs - m_state->m_progress >= m_state->m_timeout)
    {
        return fail(NI_TIMEOUT);
    }
    return m_state->m_phase == NI_FILE_PHASE_PREFIX || m_state->m_phase == NI_FILE_PHASE_COMMIT_WAIT ||
        m_state->m_phase == NI_FILE_PHASE_COMMIT_ACTIVE ? NI_NEED_MORE : NI_OK;
}

int32_t NetInterfaceFileFlow::readRequest(uint64_t& offset, uint32_t& size) const
{
    offset = 0;
    size = 0;
    if (m_state->m_phase == NI_FILE_PHASE_FAILED)
    {
        return m_state->m_error;
    }
    if (!m_state->m_sender || m_state->m_phase != NI_FILE_PHASE_TRANSFER || m_state->m_outputKind != NI_FILE_OUTPUT_NONE ||
        m_state->m_position == m_state->m_info.total ||
        m_state->m_position - m_state->m_acknowledged >= static_cast<uint64_t>(m_state->m_info.chunk) * m_state->m_info.window)
    {
        return NI_NEED_MORE;
    }
    offset = m_state->m_position;
    size = static_cast<uint32_t>((std::min)(static_cast<uint64_t>(m_state->m_info.chunk), m_state->m_info.total - offset));
    return NI_READY;
}

int32_t NetInterfaceFileFlow::produced(uint64_t offset, const uint8_t* bytes, uint32_t size)
{
    uint64_t expectedOffset = 0;
    uint32_t expectedSize = 0;
    const int32_t requested = readRequest(expectedOffset, expectedSize);
    if (requested < 0)
    {
        return requested;
    }
    if (requested != NI_READY)
    {
        return NI_LIMIT_EXCEEDED;
    }
    if (bytes == nullptr || offset != expectedOffset || size != expectedSize)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFileData data = {};
    NetInterfaceFileGuard wipe(&data, sizeof(data));
    data.offset = offset;
    data.size = size;
    data.bytes = bytes;
    std::memcpy(data.token, m_state->m_info.token, 32);
    if (NetInterface::sha256(bytes, size, data.hash, sizeof(data.hash)) != NI_OK)
    {
        return fail(NI_INVALID_STATE);
    }
    try
    {
        m_state->prepareOutput(NET_INTERFACE_FILE_DATA_HEADER_SIZE + size);
    }
    catch (...)
    {
        return fail(NI_NO_MEMORY);
    }
    uint32_t required = 0;
    const int32_t encoded = NetInterfaceFile::encodeData(data, m_state->m_output.data(),
        static_cast<uint32_t>(m_state->m_output.size()), required);
    if (encoded != NI_OK)
    {
        return fail(encoded);
    }
    const int32_t hashed = NetInterface::hashUpdate(m_state->m_hash, bytes, size);
    if (hashed != NI_OK)
    {
        return fail(hashed);
    }
    m_state->m_outputSize = size;
    m_state->m_outputKind = NI_FILE_OUTPUT_DATA;
    return NI_OK;
}

int32_t NetInterfaceFileFlow::accept(const NetInterfaceFrame& frame)
{
    if (m_state->m_phase == NI_FILE_PHASE_FAILED)
    {
        return m_state->m_error;
    }
    if (m_state->m_phase == NI_FILE_PHASE_DONE)
    {
        return NI_NOT_FOUND;
    }
    if (m_state->m_phase == NI_FILE_PHASE_PREFIX)
    {
        return NI_LIMIT_EXCEEDED;
    }
    if (frame.m_productId != m_state->m_product || frame.m_requestId != m_state->m_info.stream)
    {
        return fail(NI_PROTOCOL_ERROR);
    }
    if (frame.m_kind == NI_EVENT)
    {
        NetInterfaceFileNotice notice = {};
        NetInterfaceFileGuard wipe(&notice, sizeof(notice));
        if (NetInterfaceFile::parseNotice(frame, notice) != NI_OK || !NetInterfaceFile::equal(notice.token, m_state->m_info.token, 32) ||
            !NetInterfaceFile::equal(notice.transfer, m_state->m_info.transfer, 16))
        {
            return fail(NI_PROTOCOL_ERROR);
        }
        fail(notice.result);
        return NI_OK;
    }
    if (m_state->m_sender)
    {
        return acceptAck(frame);
    }
    return acceptData(frame);
}

int32_t NetInterfaceFileFlow::acceptAck(const NetInterfaceFrame& frame)
{
    NetInterfaceFileAck ack = {};
    NetInterfaceFileGuard wipe(&ack, sizeof(ack));
    if (NetInterfaceFile::parseAck(frame, ack) != NI_OK || !NetInterfaceFile::equal(ack.token, m_state->m_info.token, 32) ||
        ack.offset > m_state->m_position || (ack.offset != m_state->m_info.total && ack.offset % m_state->m_info.chunk != 0))
    {
        return fail(NI_PROTOCOL_ERROR);
    }
    if (ack.flags == 2)
    {
        return fail(ack.result);
    }
    if (ack.flags == 1)
    {
        if (m_state->m_phase != NI_FILE_PHASE_FINAL_ACK || ack.offset != m_state->m_info.total ||
            !NetInterfaceFile::equal(ack.hash, m_state->m_info.hash, 32))
        {
            return fail(NI_PROTOCOL_ERROR);
        }
        m_state->m_acknowledged = ack.offset;
        m_state->m_phase = NI_FILE_PHASE_DONE;
    }
    else if (ack.offset > m_state->m_acknowledged)
    {
        m_state->m_acknowledged = ack.offset;
        m_state->m_progress = m_state->m_now;
    }
    return NI_OK;
}

int32_t NetInterfaceFileFlow::acceptData(const NetInterfaceFrame& frame)
{
    NetInterfaceFileData data = {};
    NetInterfaceFileGuard wipe(&data, sizeof(data));
    if (NetInterfaceFile::parseData(frame, data) != NI_OK || !NetInterfaceFile::equal(data.token, m_state->m_info.token, 32) ||
        data.offset > m_state->m_info.total || data.size > m_state->m_info.total - data.offset || data.size > m_state->m_info.chunk)
    {
        return fail(NI_PROTOCOL_ERROR);
    }
    if (data.flags == 1)
    {
        // 重复END也必须验证相同的结束位置和摘要，不能在等待提交时跳过。
        if (data.offset != m_state->m_info.total || !NetInterfaceFile::equal(data.hash, m_state->m_info.hash, 32))
        {
            return fail(NI_PROTOCOL_ERROR);
        }
        if (!m_state->m_data.empty())
        {
            return NI_LIMIT_EXCEEDED;
        }
        if (m_state->m_position != data.offset)
        {
            return fail(NI_PROTOCOL_ERROR);
        }
        if (m_state->m_phase == NI_FILE_PHASE_COMMIT_WAIT || m_state->m_phase == NI_FILE_PHASE_COMMIT_ACTIVE)
        {
            return NI_OK;
        }
        uint8_t actual[32] = {};
        NetInterfaceFileGuard wipeHash(actual, sizeof(actual));
        if (NetInterface::hashFinish(m_state->m_hash, actual, sizeof(actual)) != NI_OK ||
            !NetInterfaceFile::equal(actual, m_state->m_info.hash, 32))
        {
            return fail(NI_PROTOCOL_ERROR);
        }
        m_state->m_phase = NI_FILE_PHASE_COMMIT_WAIT;
        m_state->m_progress = m_state->m_now;
        return NI_OK;
    }
    if (m_state->m_phase != NI_FILE_PHASE_TRANSFER || data.offset % m_state->m_info.chunk != 0 ||
        data.size != (std::min)(static_cast<uint64_t>(m_state->m_info.chunk), m_state->m_info.total - data.offset))
    {
        return fail(NI_PROTOCOL_ERROR);
    }
    uint8_t actual[32] = {};
    NetInterfaceFileGuard wipeHash(actual, sizeof(actual));
    const uint64_t received = m_state->m_position + m_state->m_data.size();
    if (NetInterface::sha256(data.bytes, data.size, actual, sizeof(actual)) != NI_OK ||
        !NetInterfaceFile::equal(actual, data.hash, 32) || data.offset > received)
    {
        return fail(NI_PROTOCOL_ERROR);
    }
    if (data.offset < m_state->m_position)
    {
        if (data.size > m_state->m_position - data.offset)
        {
            return fail(NI_PROTOCOL_ERROR);
        }
        m_state->m_ackPending = true;
        return NI_OK;
    }
    if (data.offset < received)
    {
        // 已持有的未落盘重复块只做相等检查，不提前ACK，也不堵住后续补满批次的输入。
        if (data.size > received - data.offset || std::memcmp(m_state->m_data.data() +
            static_cast<size_t>(data.offset - m_state->m_position), data.bytes, data.size) != 0)
        {
            return fail(NI_PROTOCOL_ERROR);
        }
        return NI_OK;
    }
    const uint32_t capacity = m_state->m_info.chunk * m_state->m_writeBatchChunks;
    if (m_state->m_writeTaken || data.size > capacity - static_cast<uint32_t>(m_state->m_data.size()))
    {
        return NI_LIMIT_EXCEEDED;
    }
    try
    {
        // 首块写入之前预留整批空间；之后扩展size不会搬移含秘密的旧副本。
        m_state->m_data.reserve(capacity);
        const size_t offset = m_state->m_data.size();
        m_state->m_data.resize(offset + data.size);
        std::memcpy(m_state->m_data.data() + offset, data.bytes, data.size);
    }
    catch (...)
    {
        return fail(NI_NO_MEMORY);
    }
    return NI_OK;
}

int32_t NetInterfaceFileFlow::writeRequest(uint64_t& offset, const uint8_t*& bytes, uint32_t& size)
{
    offset = 0;
    bytes = nullptr;
    size = 0;
    if (m_state->m_phase == NI_FILE_PHASE_FAILED)
    {
        return m_state->m_error;
    }
    if (m_state->m_sender || m_state->m_data.empty() || m_state->m_writeTaken ||
        (m_state->m_data.size() < m_state->m_info.chunk * m_state->m_writeBatchChunks &&
         m_state->m_position + m_state->m_data.size() < m_state->m_info.total))
    {
        return NI_NEED_MORE;
    }
    offset = m_state->m_position;
    bytes = m_state->m_data.data();
    size = static_cast<uint32_t>(m_state->m_data.size());
    m_state->m_writeTaken = true;
    return NI_READY;
}

int32_t NetInterfaceFileFlow::written(uint64_t offset, uint32_t size, int32_t result)
{
    if (m_state->m_phase == NI_FILE_PHASE_FAILED)
    {
        return m_state->m_error;
    }
    if (!m_state->m_writeTaken || offset != m_state->m_position || size != m_state->m_data.size())
    {
        return NI_INVALID_STATE;
    }
    if (result != NI_OK)
    {
        return fail(result);
    }
    const int32_t hashed = NetInterface::hashUpdate(m_state->m_hash, m_state->m_data.data(), size);
    if (hashed != NI_OK)
    {
        return fail(hashed);
    }
    m_state->m_position += size;
    m_state->m_progress = m_state->m_now;
    m_state->clearData();
    m_state->m_ackPending = true;
    return NI_OK;
}

int32_t NetInterfaceFileFlow::takeCommit()
{
    if (m_state->m_phase == NI_FILE_PHASE_FAILED)
    {
        return m_state->m_error;
    }
    if (m_state->m_sender || m_state->m_phase != NI_FILE_PHASE_COMMIT_WAIT)
    {
        return NI_NEED_MORE;
    }
    m_state->m_phase = NI_FILE_PHASE_COMMIT_ACTIVE;
    return NI_READY;
}

int32_t NetInterfaceFileFlow::committed(int32_t result)
{
    if (m_state->m_phase == NI_FILE_PHASE_FAILED)
    {
        return m_state->m_error;
    }
    if (m_state->m_phase != NI_FILE_PHASE_COMMIT_ACTIVE)
    {
        return NI_INVALID_STATE;
    }
    if (result != NI_OK)
    {
        return fail(result);
    }
    m_state->m_phase = NI_FILE_PHASE_DONE;
    m_state->m_ackPending = true;
    return NI_READY;
}

int32_t NetInterfaceFileFlow::peekOutput(NetInterfaceFrame& frame)
{
    std::memset(&frame, 0, sizeof(frame));
    if (m_state->m_outputKind == NI_FILE_OUTPUT_NONE)
    {
        if (!m_state->m_sender && m_state->m_ackPending)
        {
            NetInterfaceFileAck ack = {};
            NetInterfaceFileGuard wipe(&ack, sizeof(ack));
            ack.flags = m_state->m_phase == NI_FILE_PHASE_FAILED ? 2 : m_state->m_phase == NI_FILE_PHASE_DONE ? 1 : 0;
            ack.result = m_state->m_phase == NI_FILE_PHASE_FAILED ? m_state->m_error : NI_OK;
            ack.offset = m_state->m_position;
            std::memcpy(ack.token, m_state->m_info.token, 32);
            if (m_state->m_phase == NI_FILE_PHASE_DONE)
            {
                std::memcpy(ack.hash, m_state->m_info.hash, 32);
            }
            try
            {
                m_state->prepareOutput(NET_INTERFACE_FILE_ACK_SIZE);
            }
            catch (...)
            {
                return fail(NI_NO_MEMORY);
            }
            const int32_t result = NetInterfaceFile::encodeAck(ack, m_state->m_output.data(), static_cast<uint32_t>(m_state->m_output.size()));
            if (result != NI_OK)
            {
                return fail(result);
            }
            m_state->m_outputAckOffset = ack.offset;
            m_state->m_outputAckFlags = ack.flags;
            m_state->m_outputKind = NI_FILE_OUTPUT_ACK;
        }
        else if (m_state->m_sender && m_state->m_phase == NI_FILE_PHASE_TRANSFER &&
            m_state->m_position == m_state->m_info.total && m_state->m_acknowledged == m_state->m_position)
        {
            NetInterfaceFileData end = {};
            NetInterfaceFileGuard wipe(&end, sizeof(end));
            end.flags = 1;
            end.offset = m_state->m_position;
            std::memcpy(end.token, m_state->m_info.token, 32);
            if (NetInterface::hashFinish(m_state->m_hash, end.hash, sizeof(end.hash)) != NI_OK ||
                !NetInterfaceFile::equal(end.hash, m_state->m_info.hash, 32))
            {
                return fail(NI_PROTOCOL_ERROR);
            }
            try
            {
                m_state->prepareOutput(NET_INTERFACE_FILE_DATA_HEADER_SIZE);
            }
            catch (...)
            {
                return fail(NI_NO_MEMORY);
            }
            uint32_t required = 0;
            const int32_t result = NetInterfaceFile::encodeData(end, m_state->m_output.data(),
                static_cast<uint32_t>(m_state->m_output.size()), required);
            if (result != NI_OK)
            {
                return fail(result);
            }
            m_state->m_outputKind = NI_FILE_OUTPUT_END;
        }
        else
        {
            return m_state->m_phase == NI_FILE_PHASE_FAILED ? m_state->m_error : NI_NEED_MORE;
        }
    }
    frame.m_kind = m_state->m_outputKind == NI_FILE_OUTPUT_ACK ? NI_DATA_ACK : NI_DATA;
    frame.m_productId = m_state->m_product;
    frame.m_requestId = m_state->m_info.stream;
    frame.m_messageType = NI_FILE_DATA_TYPE;
    frame.m_payload = m_state->m_output.data();
    frame.m_payloadSize = static_cast<uint32_t>(m_state->m_output.size());
    return NI_READY;
}

int32_t NetInterfaceFileFlow::consumeOutput()
{
    if (m_state->m_outputKind == NI_FILE_OUTPUT_NONE)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->m_sender)
    {
        m_state->m_position += m_state->m_outputSize;
        if (m_state->m_outputKind == NI_FILE_OUTPUT_END)
        {
            m_state->m_phase = NI_FILE_PHASE_FINAL_ACK;
        }
        m_state->m_progress = m_state->m_now;
    }
    else
    {
        const uint32_t flags = m_state->m_phase == NI_FILE_PHASE_FAILED ? 2 : m_state->m_phase == NI_FILE_PHASE_DONE ? 1 : 0;
        m_state->m_ackPending = m_state->m_outputAckOffset != m_state->m_position || m_state->m_outputAckFlags != flags;
    }
    m_state->clearOutput();
    return NI_OK;
}

void NetInterfaceFileFlow::cancel(int32_t reason)
{
    fail(reason);
}

void NetInterfaceFileFlow::snapshot(NetInterfaceFileInfo& info, int32_t& result) const
{
    info = m_state->m_info;
    info.offset = m_state->m_sender ? m_state->m_acknowledged : m_state->m_position;
    result = m_state->status();
    if (m_state->m_phase == NI_FILE_PHASE_FAILED)
    {
        info.state = result == NI_CANCELLED ? NI_FILE_CANCELLED : NI_FILE_FAILED;
        info.stream = 0;
    }
    else if (m_state->m_phase == NI_FILE_PHASE_DONE)
    {
        info.state = NI_FILE_COMPLETED;
        info.stream = 0;
    }
    else if (m_state->m_phase == NI_FILE_PHASE_PREFIX)
    {
        info.state = NI_FILE_PREPARING;
        info.stream = 0;
    }
}

// NET_FILE_PROTOCOL_UNIT_BEGIN
// #include "NetInterface.h"
// #include <algorithm>
// #include <cstdio>
// #include <cstring>
// #include <vector>
// #if defined(_WIN32) && defined(_DEBUG)
// #include <crtdbg.h>
// #include <malloc.h>
// #endif
//
// #include "NetInterface/NetInterfaceFileAuditTest.h"
// uint32_t NetInterfaceFileAuditTest::m_checks = 0;
//
// #define FILE_VERIFY(condition) { ++m_checks; if (!(condition)) { std::printf("FAIL %u %s\n", static_cast<uint32_t>(__LINE__), #condition); return false; } }
//
// NetInterfaceFileInfo NetInterfaceFileAuditTest::info(uint64_t size, uint32_t chunk, uint32_t window, uint64_t resume)
// {
//     NetInterfaceFileInfo value = {};
//     value.direction = 1; value.state = NI_FILE_TRANSFERRING; value.chunk = chunk; value.window = window;
//     value.total = size; value.offset = resume; value.stream = UINT64_C(0xfedcba9876543210); value.expiresUtc = UINT64_C(4000000000);
//     for (uint32_t index = 0; index < 32; ++index) { value.token[index] = static_cast<uint8_t>(index + 0x31); }
//     value.transfer[0] = 0x17; value.scope[15] = 0x5b;
//     return value;
// }
//
// NetInterfaceFrame NetInterfaceFileAuditTest::frame(const NetInterfaceFileInfo& info, uint16_t kind, const uint8_t* bytes, uint32_t size)
// {
//     NetInterfaceFrame value = {};
//     value.m_kind = kind; value.m_productId = 7; value.m_requestId = info.stream;
//     value.m_messageType = NI_FILE_DATA_TYPE; value.m_payload = bytes; value.m_payloadSize = size;
//     return value;
// }
//
// bool NetInterfaceFileAuditTest::codec()
// {
//     NetInterfaceFileInfo original = info(2048,1024,2,1024);
//     uint8_t wire[NET_INTERFACE_FILE_INFO_SIZE] = {};
//     FILE_VERIFY(NetInterfaceFile::encodeInfo(original,wire,sizeof(wire)) == NI_OK);
//     FILE_VERIFY(std::memcmp(wire,"NFI2",4)==0 && wire[7]==2 && wire[11]==1 && wire[15]==2 && wire[18]==4);
//     FILE_VERIFY(wire[30]==8 && wire[38]==4 && wire[40]==0xfe && wire[47]==0x10 && wire[56]==0x17 && wire[151]==0x5b);
//     NetInterfaceFileInfo parsed = {};
//     FILE_VERIFY(NetInterfaceFile::parseInfo(wire,sizeof(wire),parsed)==NI_OK && parsed.total==2048 && parsed.offset==1024 && parsed.stream==original.stream);
//     for(uint32_t size=0;size<160;++size)
//     {
//         std::memset(&parsed,0x77,sizeof(parsed));
//         FILE_VERIFY(NetInterfaceFile::parseInfo(wire,size,parsed)==NI_PROTOCOL_ERROR && parsed.token[0]==0 && parsed.stream==0);
//     }
//     for(uint32_t index=152;index<160;++index)
//     {
//         wire[index]=1;
//         FILE_VERIFY(NetInterfaceFile::parseInfo(wire,sizeof(wire),parsed)==NI_PROTOCOL_ERROR && parsed.total==0);
//         wire[index]=0;
//     }
//     const uint8_t saved=wire[0];
//     FILE_VERIFY(NetInterfaceFile::parseInfo(wire,sizeof(wire),*reinterpret_cast<NetInterfaceFileInfo*>(wire))==NI_INVALID_ARGUMENT && wire[0]==saved);
//     uint64_t aligned[40]={};
//     NetInterfaceFileInfo* inplace=reinterpret_cast<NetInterfaceFileInfo*>(aligned);
//     *inplace=original;
//     FILE_VERIFY(NetInterfaceFile::encodeInfo(*inplace,reinterpret_cast<uint8_t*>(aligned),sizeof(aligned))==NI_OK && std::memcmp(aligned,wire,sizeof(wire))==0);
//     uint32_t fields[12]={}; uint8_t transfer[16]={}; uint8_t token[32]={}; uint8_t hash[32]={};
//     FILE_VERIFY(NetInterface::fileInfoFields(wire,160,fields,transfer,token,hash)==NI_OK && fields[6]==1024 && fields[9]==0xfedcba98U);
//     FILE_VERIFY(NetInterface::fileInfoFields(wire,160,fields,transfer,token,token)==NI_INVALID_ARGUMENT);
//     FILE_VERIFY(NetInterface::fileInfoFields(wire,160,reinterpret_cast<uint32_t*>(wire),transfer,token,hash)==NI_INVALID_ARGUMENT && wire[0]==saved);
//     std::vector<uint8_t> data(65536,0x5a); std::vector<uint8_t> encoded(65624,0x7f);
//     for(uint32_t size=1;size<=65536;size=size==1?1024:size==1024?65536:65537)
//     {
//         NetInterfaceFileData value={};value.size=size;value.bytes=data.data();value.offset=UINT64_C(0x1fffffffffffffff)-size;
//         std::memcpy(value.token,original.token,32);
//         uint32_t required=0;
//         FILE_VERIFY(NetInterfaceFile::encodeData(value,encoded.data(),static_cast<uint32_t>(encoded.size()),required)==NI_OK && required==size+88);
//         NetInterfaceFileData result={}; NetInterfaceFrame input=frame(original,NI_DATA,encoded.data(),required);
//         FILE_VERIFY(NetInterfaceFile::parseData(input,result)==NI_OK && result.bytes==encoded.data()+88 && result.size==size && result.offset==value.offset);
//         FILE_VERIFY(std::memcmp(result.bytes,data.data(),size)==0);
//         value.flags=1;
//         FILE_VERIFY(NetInterfaceFile::encodeData(value,encoded.data(),static_cast<uint32_t>(encoded.size()),value.flags)==NI_INVALID_ARGUMENT && value.flags==1);
//         value.flags=0;value.bytes=encoded.data();required=99;
//         FILE_VERIFY(NetInterfaceFile::encodeData(value,encoded.data(),static_cast<uint32_t>(encoded.size()),required)==NI_INVALID_ARGUMENT && required==0);
//         value.bytes=data.data();value.offset++;
//         FILE_VERIFY(NetInterfaceFile::encodeData(value,encoded.data(),static_cast<uint32_t>(encoded.size()),required)==NI_INVALID_ARGUMENT && required==0);
//         input.m_productId=0;
//         FILE_VERIFY(NetInterfaceFile::parseData(input,result)==NI_PROTOCOL_ERROR && result.bytes==nullptr);
//     }
//     for(uint32_t operation=NI_FILE_OPEN_UPLOAD;operation<=NI_FILE_CANCEL;++operation)
//     {
//         NetInterfaceFileControl control={};control.operation=operation;control.transfer[0]=1;
//         if(operation<=NI_FILE_OPEN_DOWNLOAD)
//         {
//             control.chunk=1024;control.window=2;control.key=data.data();control.keySize=512;
//         }
//         else{control.token[0]=1;}
//         control.authorization=data.data();control.authorizationSize=2048;
//         uint8_t output[2680]={};uint32_t required=0;
//         FILE_VERIFY(NetInterfaceFile::encodeControl(control,output,sizeof(output),required)==NI_OK);
//         NetInterfaceFileControl decoded={};
//         FILE_VERIFY(NetInterfaceFile::parseControl(operation,output,required,decoded)==NI_OK && decoded.authorizationSize==2048 && decoded.operation==operation);
//         FILE_VERIFY(NetInterfaceFile::encodeControl(control,output,sizeof(output),control.operation)==NI_INVALID_ARGUMENT && control.operation==operation);
//         output[28]=1;
//         FILE_VERIFY(NetInterfaceFile::parseControl(operation,output,required,decoded)==NI_PROTOCOL_ERROR && decoded.operation==0);
//         control.authorizationSize=2049;
//         FILE_VERIFY(NetInterfaceFile::encodeControl(control,output,sizeof(output),required)==NI_INVALID_ARGUMENT && required==0);
//     }
//     NetInterfaceFileAck ack={};ack.flags=2;ack.result=INT32_MIN;std::memcpy(ack.token,original.token,32);
//     uint8_t ackBytes[88]={};
//     FILE_VERIFY(NetInterfaceFile::encodeAck(ack,ackBytes,sizeof(ackBytes))==NI_OK);
//     NetInterfaceFileAck decodedAck={};
//     FILE_VERIFY(NetInterfaceFile::parseAck(frame(original,NI_DATA_ACK,ackBytes,88),decodedAck)==NI_OK && decodedAck.result==INT32_MIN);
//     ack.flags=1;
//     FILE_VERIFY(NetInterfaceFile::encodeAck(ack,ackBytes,sizeof(ackBytes))==NI_INVALID_ARGUMENT);
//     NetInterfaceFileNotice notice={};notice.result=INT32_MIN;std::memcpy(notice.token,original.token,32);notice.transfer[0]=1;
//     uint8_t noticeBytes[56]={};
//     FILE_VERIFY(NetInterfaceFile::encodeNotice(notice,noticeBytes,sizeof(noticeBytes))==NI_OK);
//     NetInterfaceFrame event=frame(original,NI_EVENT,noticeBytes,56);event.m_messageType=NI_FILE_CANCEL;
//     NetInterfaceFileNotice decodedNotice={};
//     FILE_VERIFY(NetInterfaceFile::parseNotice(event,decodedNotice)==NI_OK && decodedNotice.result==INT32_MIN);
//     return true;
// }
//
// bool NetInterfaceFileAuditTest::transfer(uint32_t total,uint32_t chunk,uint32_t window,uint32_t batch,uint32_t resume)
// {
//     std::vector<uint8_t> source(total);std::vector<uint8_t> disk(total,0);
//     for(uint32_t index=0;index<total;++index){source[index]=static_cast<uint8_t>(index*17+index/37);}
//     if(resume){std::memcpy(disk.data(),source.data(),resume);}
//     NetInterfaceFileInfo metadata=info(total,chunk,window,resume);
//     FILE_VERIFY(NetInterface::sha256(total?source.data():nullptr,total,metadata.hash,32)==NI_OK);
//     NetInterfaceFileFlow sender(metadata,7,true,1000),receiver(metadata,7,false,1000);
//     FILE_VERIFY(receiver.setWriteBatchChunks(batch)==NI_OK);
//     if(resume)
//     {
//         const uint32_t first=resume/2;
//         if(first){FILE_VERIFY(sender.prime(0,source.data(),first)==NI_OK && receiver.prime(0,source.data(),first)==NI_OK);}
//         FILE_VERIFY(sender.prime(first,source.data()+first,resume-first)==NI_OK && receiver.prime(first,source.data()+first,resume-first)==NI_OK);
//     }
//     uint64_t durable=resume;bool committed=false;uint32_t rounds=0;bool finished=false;
//     for(;rounds<10000;++rounds)
//     {
//         FILE_VERIFY(sender.progress(1000+rounds)>=0 && receiver.progress(1000+rounds)>=0);
//         uint64_t offset=0;uint32_t size=0;
//         if(sender.readRequest(offset,size)==NI_READY)
//         {
//             FILE_VERIFY(offset+size<=source.size());
//             FILE_VERIFY(sender.produced(offset,source.data()+static_cast<size_t>(offset),size)==NI_OK);
//         }
//         NetInterfaceFrame outgoing={};
//         if(sender.peekOutput(outgoing)==NI_READY)
//         {
//             NetInterfaceFrame repeated={};
//             FILE_VERIFY(sender.peekOutput(repeated)==NI_READY && repeated.m_payload==outgoing.m_payload && repeated.m_payloadSize==outgoing.m_payloadSize);
//             const int32_t accepted=receiver.accept(outgoing);
//             FILE_VERIFY(accepted==NI_OK || accepted==NI_LIMIT_EXCEEDED);
//             if(accepted==NI_OK)
//             {
//                 FILE_VERIFY(receiver.accept(outgoing)==NI_OK);
//                 FILE_VERIFY(sender.consumeOutput()==NI_OK);
//             }
//         }
//         const uint8_t* bytes=nullptr;
//         if(receiver.writeRequest(offset,bytes,size)==NI_READY)
//         {
//             FILE_VERIFY(offset==durable && offset+size<=total && std::memcmp(bytes,source.data()+static_cast<size_t>(offset),size)==0);
//             FILE_VERIFY(receiver.written(offset+1,size,NI_OK)==NI_INVALID_STATE);
//             std::memcpy(disk.data()+static_cast<size_t>(offset),bytes,size);durable+=size;
//             FILE_VERIFY(receiver.written(offset,size,NI_OK)==NI_OK);
//         }
//         if(receiver.takeCommit()==NI_READY)
//         {
//             FILE_VERIFY(durable==total && disk==source && !committed);
//             FILE_VERIFY(receiver.takeCommit()==NI_NEED_MORE);
//             committed=true;
//             FILE_VERIFY(receiver.committed(NI_OK)==NI_READY);
//         }
//         NetInterfaceFrame incoming={};
//         if(receiver.peekOutput(incoming)==NI_READY)
//         {
//             NetInterfaceFileAck ack={};
//             FILE_VERIFY(NetInterfaceFile::parseAck(incoming,ack)==NI_OK && ack.offset<=durable && (ack.flags!=1 || committed));
//             FILE_VERIFY(sender.accept(incoming)==NI_OK);
//             FILE_VERIFY(receiver.consumeOutput()==NI_OK);
//         }
//         if(sender.progress(1000+rounds)==NI_READY)
//         {
//             FILE_VERIFY(committed && receiver.progress(1000+rounds)==NI_READY && disk==source);
//             sender.cancel();receiver.cancel();
//             FILE_VERIFY(sender.progress(0)==NI_READY && receiver.progress(0)==NI_READY);
//             finished=true;break;
//         }
//     }
//     FILE_VERIFY(finished && rounds<10000);
//     NetInterfaceFileInfo snapshot={};int32_t result=0;sender.snapshot(snapshot,result);
//     FILE_VERIFY(snapshot.state==NI_FILE_COMPLETED && snapshot.stream==0 && snapshot.offset==total && result==NI_READY);
//     return true;
// }
//
// bool NetInterfaceFileAuditTest::boundaries()
// {
//     NetInterfaceFileInfo metadata=info(0,1024,2,0);
//     FILE_VERIFY(NetInterface::sha256(nullptr,0,metadata.hash,32)==NI_OK);
//     NetInterfaceFileData end={};end.flags=1;std::memcpy(end.token,metadata.token,32);std::memcpy(end.hash,metadata.hash,32);
//     uint8_t encoded[88]={};uint32_t required=0;
//     FILE_VERIFY(NetInterfaceFile::encodeData(end,encoded,sizeof(encoded),required)==NI_OK);
//     for(uint32_t taken=0;taken<2;++taken)
//     {
//         NetInterfaceFileFlow receiver(metadata,7,false,1000);
//         FILE_VERIFY(receiver.accept(frame(metadata,NI_DATA,encoded,88))==NI_OK);
//         if(taken){FILE_VERIFY(receiver.takeCommit()==NI_READY);}
//         encoded[56]^=1;
//         FILE_VERIFY(receiver.accept(frame(metadata,NI_DATA,encoded,88))==NI_PROTOCOL_ERROR);
//         FILE_VERIFY(receiver.committed(NI_OK)==NI_PROTOCOL_ERROR && receiver.progress(1000)==NI_PROTOCOL_ERROR);
//         encoded[56]^=1;
//     }
//     std::vector<uint8_t> contents(3072,0x5a),wire(1112);
//     metadata=info(3072,1024,2,0);NetInterface::sha256(contents.data(),3072,metadata.hash,32);
//     NetInterfaceFileData block={};block.bytes=contents.data();block.size=1024;std::memcpy(block.token,metadata.token,32);
//     NetInterface::sha256(block.bytes,block.size,block.hash,32);
//     NetInterfaceFile::encodeData(block,wire.data(),static_cast<uint32_t>(wire.size()),required);
//     {
//         NetInterfaceFileFlow receiver(metadata,7,false,1000);FILE_VERIFY(receiver.setWriteBatchChunks(2)==NI_OK);
//         FILE_VERIFY(receiver.accept(frame(metadata,NI_DATA,wire.data(),required))==NI_OK);
//         FILE_VERIFY(receiver.accept(frame(metadata,NI_DATA,wire.data(),required))==NI_OK);
//         NetInterfaceFrame ack={};FILE_VERIFY(receiver.peekOutput(ack)==NI_NEED_MORE);
//         block.offset=1024;NetInterfaceFile::encodeData(block,wire.data(),static_cast<uint32_t>(wire.size()),required);
//         FILE_VERIFY(receiver.accept(frame(metadata,NI_DATA,wire.data(),required))==NI_OK);
//         uint64_t offset=0;uint32_t size=0;const uint8_t* bytes=nullptr;
//         FILE_VERIFY(receiver.writeRequest(offset,bytes,size)==NI_READY && size==2048);
//         const uint8_t* borrowed=bytes;
//         FILE_VERIFY(receiver.accept(frame(metadata,NI_DATA,wire.data(),required))==NI_OK && bytes==borrowed && bytes[1024]==0x5a);
//         wire[88]^=1;NetInterface::sha256(wire.data()+88,1024,wire.data()+56,32);
//         FILE_VERIFY(receiver.accept(frame(metadata,NI_DATA,wire.data(),required))==NI_PROTOCOL_ERROR);
//         FILE_VERIFY(receiver.written(offset,size,NI_OK)==NI_PROTOCOL_ERROR);
//     }
//     {
//         NetInterfaceFileFlow sender(metadata,7,true,1000,1000);
//         FILE_VERIFY(sender.produced(0,contents.data(),1024)==NI_OK && sender.consumeOutput()==NI_OK);
//         NetInterfaceFileAck ack={};std::memcpy(ack.token,metadata.token,32);uint8_t bytes[88]={};
//         FILE_VERIFY(NetInterfaceFile::encodeAck(ack,bytes,88)==NI_OK);
//         for(uint32_t now=1001;now<2000;now+=13)
//         {
//             FILE_VERIFY(sender.progress(now)==NI_OK && sender.accept(frame(metadata,NI_DATA_ACK,bytes,88))==NI_OK);
//         }
//         FILE_VERIFY(sender.progress(2000)==NI_TIMEOUT && sender.progress(1000)==NI_TIMEOUT);
//     }
//     {
//         NetInterfaceFileFlow receiver(metadata,7,false,UINT64_MAX-1000,1000);
//         FILE_VERIFY(receiver.progress(UINT64_MAX-1)==NI_OK && receiver.progress(UINT64_MAX)==NI_TIMEOUT);
//     }
//     return true;
// }
//
// bool NetInterfaceFileAuditTest::cApi()
// {
//     NetInterfaceFileInfo metadata = info(0, 1024, 2, 0);
//     FILE_VERIFY(NetInterface::sha256(nullptr, 0, metadata.hash, 32) == NI_OK);
//     uint8_t wire[160] = {};
//     FILE_VERIFY(NetInterfaceFile::encodeInfo(metadata, wire, sizeof(wire)) == NI_OK);
//     NetInterfaceFileFlowHandle receiver = nullptr;
//     FILE_VERIFY(NetInterface::fileFlowCreateWire(wire, 159, 7, 0, 1000, 0, 1000, &receiver) == NI_PROTOCOL_ERROR && receiver == nullptr);
//     FILE_VERIFY(NetInterface::fileFlowCreate(&metadata, 7, 2, 1000, 0, 1000, &receiver) == NI_INVALID_ARGUMENT && receiver == nullptr);
//     FILE_VERIFY(NetInterface::fileFlowCreateWire(wire, 160, 7, 0, 1000, 0, 1000, &receiver) == NI_OK && receiver != nullptr);
//     uint32_t fields[6] = {};
//     const uint8_t* bytes = reinterpret_cast<const uint8_t*>(1);
//     FILE_VERIFY(NetInterface::fileFlowWrite(receiver, fields, &bytes) == NI_NEED_MORE && bytes == nullptr && fields[2] == 0);
//     FILE_VERIFY(NetInterface::fileFlowPeek(receiver, fields, &bytes) == NI_NEED_MORE && bytes == nullptr);
//     FILE_VERIFY(NetInterface::fileFlowPeek(receiver, fields, reinterpret_cast<const uint8_t**>(fields)) == NI_INVALID_ARGUMENT);
//     FILE_VERIFY(NetInterface::fileFlowWrite(receiver, fields, reinterpret_cast<const uint8_t**>(fields)) == NI_INVALID_ARGUMENT);
//     NetInterfaceFileInfo snapshot = {};
//     int32_t result = 99;
//     FILE_VERIFY(NetInterface::fileFlowSnapshot(receiver, &snapshot, &result) == NI_OK && result == NI_OK && snapshot.total == 0);
//     FILE_VERIFY(NetInterface::fileFlowSnapshot(receiver, &snapshot, reinterpret_cast<int32_t*>(&snapshot)) == NI_INVALID_ARGUMENT);
//     FILE_VERIFY(NetInterface::fileFlowSnapshotWire(receiver, wire, 160, reinterpret_cast<int32_t*>(wire)) == NI_INVALID_ARGUMENT);
//     FILE_VERIFY(NetInterface::fileFlowSnapshotWire(receiver, wire, 159, &result) == NI_BUFFER_TOO_SMALL);
//     FILE_VERIFY(NetInterface::fileFlowSnapshotWire(receiver, wire, 160, &result) == NI_OK && result == NI_OK);
//     FILE_VERIFY(NetInterface::fileFlowCancel(receiver, NI_OK) == NI_INVALID_ARGUMENT);
//     FILE_VERIFY(NetInterface::fileFlowCancel(receiver, NI_CANCELLED) == NI_OK);
//     FILE_VERIFY(NetInterface::fileFlowPeek(receiver, fields, &bytes) == NI_READY && fields[0] == NI_DATA_ACK && fields[5] == 88);
//     NetInterfaceFileAck ack = {};
//     FILE_VERIFY(NetInterfaceFile::parseAck(frame(metadata, NI_DATA_ACK, bytes, fields[5]), ack) == NI_OK && ack.flags == 2 && ack.result == NI_CANCELLED);
//     FILE_VERIFY(NetInterface::fileFlowConsume(receiver) == NI_OK);
//     FILE_VERIFY(NetInterface::fileFlowPeek(receiver, fields, &bytes) == NI_CANCELLED && bytes == nullptr && fields[5] == 0);
//     NetInterface::fileFlowDestroy(receiver);
//     FILE_VERIFY(NetInterface::fileFlowRead(nullptr, fields) == NI_INVALID_ARGUMENT && fields[0] == 0);
//     FILE_VERIFY(NetInterface::fileFlowConsume(nullptr) == NI_INVALID_ARGUMENT);
//     NetInterface::fileFlowDestroy(nullptr);
//     return true;
// }
//
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_FILE_AUDIT_NO_HEAP)
// int32_t NetInterfaceFileAuditTest::m_rejectAfter = -1;
// uint32_t NetInterfaceFileAuditTest::m_allocations = 0;
// uint32_t NetInterfaceFileAuditTest::m_exposed = 0;
//
// int __cdecl NetInterfaceFileAuditTest::memoryHook(int operation, void* bytes, size_t, int block, long, const unsigned char*, int)
// {
//     if (block != _NORMAL_BLOCK)
//     {
//         return 1;
//     }
//     if (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC)
//     {
//         ++m_allocations;
//         if (m_rejectAfter == 0)
//         {
//             return 0;
//         }
//         if (m_rejectAfter > 0)
//         {
//             --m_rejectAfter;
//         }
//     }
//     if (operation == _HOOK_FREE && bytes != nullptr && _msize_dbg(bytes, _NORMAL_BLOCK) >= 32)
//     {
//         const uint8_t* value = static_cast<const uint8_t*>(bytes);
//         uint32_t index = 0;
//         while (index < 32 && value[index] == 0x5a)
//         {
//             ++index;
//         }
//         if (index == 32)
//         {
//             ++m_exposed;
//         }
//     }
//     return 1;
// }
//
// bool NetInterfaceFileAuditTest::memory()
// {
//     NetInterfaceFileInfo metadata = info(524288, 1024, 2, 0);
//     bool created = false;
//     uint32_t rejected = 0;
//     for (int32_t step = 0; step < 16 && !created; ++step)
//     {
//         NetInterfaceFileFlowHandle handle = nullptr;
//         m_rejectAfter = step;
//         const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(memoryHook);
//         const int32_t result = NetInterface::fileFlowCreate(&metadata, 7, 0, 1000, 0, 1000, &handle);
//         _CrtSetAllocHook(previous);
//         m_rejectAfter = -1;
//         if (result == NI_OK)
//         {
//             created = handle != nullptr;
//         }
//         else
//         {
//             FILE_VERIFY(result == NI_NO_MEMORY && handle == nullptr);
//             ++rejected;
//         }
//         NetInterface::fileFlowDestroy(handle);
//     }
//     FILE_VERIFY(created && rejected >= 2);
//     uint8_t payload[1024] = {};
//     std::memset(payload, 0x5a, sizeof(payload));
//     uint8_t wire[1112] = {};
//     NetInterfaceFileData data = {};
//     data.bytes = payload;
//     data.size = sizeof(payload);
//     std::memcpy(data.token, metadata.token, 32);
//     FILE_VERIFY(NetInterface::sha256(payload, sizeof(payload), data.hash, 32) == NI_OK);
//     uint32_t required = 0;
//     FILE_VERIFY(NetInterfaceFile::encodeData(data, wire, sizeof(wire), required) == NI_OK);
//     for (uint32_t role = 0; role < 2; ++role)
//     {
//         NetInterfaceFileFlow flow(metadata, 7, role != 0, 1000);
//         m_rejectAfter = 0;
//         const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(memoryHook);
//         const int32_t result = role ? flow.produced(0, payload, sizeof(payload)) : flow.accept(frame(metadata, NI_DATA, wire, sizeof(wire)));
//         _CrtSetAllocHook(previous);
//         m_rejectAfter = -1;
//         FILE_VERIFY(result == NI_NO_MEMORY && flow.progress(1000) == NI_NO_MEMORY);
//     }
//     m_exposed = 0;
//     uint32_t warmAllocations = 0;
//     {
//         NetInterfaceFileFlow receiver(metadata, 7, false, 1000);
//         FILE_VERIFY(receiver.setWriteBatchChunks(2) == NI_OK);
//         for (uint32_t batch = 0; batch < 256; ++batch)
//         {
//             m_allocations = 0;
//             const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(memoryHook);
//             bool valid = true;
//             for (uint32_t index = 0; index < 2; ++index)
//             {
//                 data.offset = static_cast<uint64_t>(batch * 2 + index) * 1024;
//                 valid = valid && NetInterfaceFile::encodeData(data, wire, sizeof(wire), required) == NI_OK;
//                 valid = valid && receiver.accept(frame(metadata, NI_DATA, wire, sizeof(wire))) == NI_OK;
//             }
//             uint64_t offset = 0;
//             uint32_t size = 0;
//             const uint8_t* bytes = nullptr;
//             valid = valid && receiver.writeRequest(offset, bytes, size) == NI_READY && size == 2048;
//             valid = valid && receiver.written(offset, size, NI_OK) == NI_OK;
//             NetInterfaceFrame ack = {};
//             valid = valid && receiver.peekOutput(ack) == NI_READY && receiver.consumeOutput() == NI_OK;
//             _CrtSetAllocHook(previous);
//             FILE_VERIFY(valid);
//             if (batch != 0)
//             {
//                 warmAllocations += m_allocations;
//             }
//         }
//     }
//     FILE_VERIFY(m_exposed == 0 && warmAllocations == 0);
//     return true;
// }
// #endif
//
//
// bool NetInterfaceFileAuditTest::run()
// {
//     m_checks=0;
// #if defined(_WIN32) && defined(_DEBUG)
//     _CrtMemState before;_CrtMemCheckpoint(&before);
// #endif
//     FILE_VERIFY(codec() && boundaries() && cApi());
// #if defined(_WIN32) && defined(_DEBUG) && !defined(NET_FILE_AUDIT_NO_HEAP)
//     FILE_VERIFY(memory());
// #endif
//     const uint32_t lengths[]={0,1,1023,1024,1025,8193,65537,262145};
//     const uint32_t chunks[]={1024,4096,65536};
//     const uint32_t windows[]={1,2,8};
//     for(uint32_t a=0;a<8;++a)for(uint32_t b=0;b<3;++b)for(uint32_t c=0;c<3;++c)
//     {
//         FILE_VERIFY(transfer(lengths[a],chunks[b],windows[c],1,0));
//         FILE_VERIFY(transfer(lengths[a],chunks[b],windows[c],windows[c],0));
//         if(lengths[a]>=chunks[b]){FILE_VERIFY(transfer(lengths[a],chunks[b],windows[c],windows[c],chunks[b]));}
//         FILE_VERIFY(transfer(lengths[a],chunks[b],windows[c],windows[c],lengths[a]));
//     }
// #if defined(_WIN32) && defined(_DEBUG)
//     _CrtMemState after;_CrtMemCheckpoint(&after);
//     FILE_VERIFY(before.lCounts[_NORMAL_BLOCK]==after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK]==after.lSizes[_NORMAL_BLOCK]);
// #endif
//     return true;
// }
//
// int main()
// {
//     if(!NetInterfaceFileAuditTest::run()){return 1;}
//     std::printf("File protocol checks=%u PASS\n",NetInterfaceFileAuditTest::m_checks);
//     return 0;
// }
// #undef FILE_VERIFY
// NET_FILE_PROTOCOL_UNIT_END
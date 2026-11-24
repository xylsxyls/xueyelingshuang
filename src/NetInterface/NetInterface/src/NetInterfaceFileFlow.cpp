#include "NetInterfaceFileFlow.h"
#include "NetInterfaceMacro.h"
#include "NetInterfaceFileFlowState.h"
#include "NetInterfaceFileGuard.h"
#include <algorithm>
#include <cstring>

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
    if (m_state->m_phase == NI_FILE_PHASE_FAILED)
    {
        return m_state->m_error;
    }
    const int32_t error = result < 0 ? result : NI_INVALID_STATE;
    // 宿主已经提交的文件保持完成；准备最终 ACK 失败时，本次 peek 仍须报告错误。
    if (m_state->m_phase != NI_FILE_PHASE_DONE)
    {
        m_state->m_error = error;
        m_state->m_phase = NI_FILE_PHASE_FAILED;
        m_state->clearData();
        m_state->clearOutput();
        m_state->m_ackPending = !m_state->m_sender;
    }
    return error;
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
        // 只有首次接纳新的连续字节才刷新期限；持久化偏移仍由 written 更新。
        m_state->m_progress = m_state->m_now;
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
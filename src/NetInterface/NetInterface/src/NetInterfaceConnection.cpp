#include "NetInterfaceMacro.h"
#include "NetInterfaceConnection.h"
#include "NetInterfaceFileGuard.h"
#include "NetConnectionQueue.h"
#include "CEncodeDecode/CEncodeDecodeAPI.h"
#include "NetExecution.h"
#include "NetInterfaceCompression.h"
#include "NetInterfaceCompact.h"
#include <algorithm>
#include <cstring>
#include <new>

NetInterfaceConnection::NetInterfaceConnection(const NetInterfaceTlsOptions& options) :
NetInterfaceConnection(options, NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT)
{

}

NetInterfaceConnection::NetInterfaceConnection(const NetInterfaceTlsOptions& options, uint32_t payloadLimit) :
NetInterfaceConnection(options, payloadLimit, NetConnectionPoolConfig())
{

}

NetInterfaceConnection::NetInterfaceConnection(const NetInterfaceTlsOptions& options, uint32_t payloadLimit,
    const NetConnectionPoolConfig& pools) :
NetInterfaceConnection(options, payloadLimit, pools, CEncodeDecodeTlsQueueLimits())
{

}

NetInterfaceConnection::NetInterfaceConnection(const NetInterfaceTlsOptions& options, uint32_t payloadLimit,
    const NetConnectionPoolConfig& pools, const CEncodeDecodeTlsQueueLimits& limits) :
m_compact(false),
m_previousCompactData(),
m_tls(options, limits),
m_decoder(payloadLimit),
m_error(!pools.validCodecSettings() || payloadLimit == 0 || payloadLimit > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT + 32 ? NI_INVALID_ARGUMENT : 0),
m_closed(false),
m_readBlocked(false),
m_plainSize(0),
m_plainOffset(0),
m_queue(new (std::nothrow) NetConnectionQueue(pools)),
m_activeOffset(0),
m_started(false),
m_controlPayloadLimit(pools.m_controlPayloadLimit),
m_progressFrameBudget(pools.m_progressFrameBudget),
m_progressStepBudget(pools.m_progressStepBudget),
m_compressionMinPayload(pools.m_compressionMinPayload),
m_compressionLevel(pools.m_compressionLevel)
{
    if (m_error == 0)
    {
        m_error = m_decoder.configureDecoderRetention(pools.m_decoderRetentionBytes);
        if (m_error == NI_OK)
        {
            m_error = m_queue ? m_queue->status() : NI_NO_MEMORY;
        }
    }
}

NetInterfaceConnection::~NetInterfaceConnection()
{
    clearWrites();
    wipe(m_plain, sizeof(m_plain));
}

void NetInterfaceConnection::wipe(uint8_t* bytes, uint32_t size)
{
    // 复用加密库按平台实现的不可消除擦除，大块TLS明文不另写逐字节循环。
    NetInterface::wipe(bytes, size);
}

int32_t NetInterfaceConnection::configurePools(const NetConnectionPoolConfig& config)
{
    if (m_started || m_error != 0)
    {
        return NI_INVALID_STATE;
    }
    if (!config.validCodecSettings()) { return NI_INVALID_ARGUMENT; }
    try
    {
        std::unique_ptr<NetConnectionQueue> replacement(new (std::nothrow) NetConnectionQueue(config));
        int32_t result = replacement ? replacement->status() : NI_NO_MEMORY;
        if (result == NI_OK)
        {
            result = m_decoder.configureDecoderRetention(config.m_decoderRetentionBytes);
        }
        if (result == NI_OK)
        {
            m_queue.swap(replacement);
            m_controlPayloadLimit = config.m_controlPayloadLimit;
            m_progressFrameBudget = config.m_progressFrameBudget;
            m_progressStepBudget = config.m_progressStepBudget;
            m_compressionMinPayload = config.m_compressionMinPayload;
            m_compressionLevel = config.m_compressionLevel;
        }
        return result;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

void NetInterfaceConnection::completeActive()
{
    m_queue->complete();
    m_activeOffset = 0;
}

void NetInterfaceConnection::clearWrites()
{
    if (m_queue)
    {
        m_queue->clear();
    }
    m_activeOffset = 0;
}

int32_t NetInterfaceConnection::feed(const uint8_t* data, uint32_t size, uint32_t& consumed)
{
    if (m_error != 0)
    {
        return NetInterfaceFileGuard::prepareCount(data, size, &consumed) ? m_error : NI_INVALID_ARGUMENT;
    }
    const int32_t result = m_tls.feed(data, size, consumed);
    if (result == NET_INTERFACE_TLS_ARGUMENT) { return result; }
    m_started = true;
    if (consumed != 0)
    {
        m_readBlocked = false;
    }
    return result;
}

int32_t NetInterfaceConnection::decodePending(uint32_t& consumed)
{
    consumed = 0;
    const int32_t available = m_decoder.status();
    if (available != NI_NEED_MORE || m_plainOffset == m_plainSize)
    {
        return available < 0 ? fail(available) : available;
    }
    const int32_t result = m_decoder.feed(m_plain + m_plainOffset, m_plainSize - m_plainOffset, consumed);
    m_plainOffset += consumed;
    if (m_plainOffset == m_plainSize)
    {
        // 解码器已经持有独立正文；整段TLS明文解析完再统一擦除，避免逐短帧清理调用。
        // 未解析完时仍由本连接独占，错误和析构路径也会擦除整个缓冲。
        wipe(m_plain, m_plainSize);
        m_plainOffset = m_plainSize = 0;
    }
    if (result < 0)
    {
        fail(result);
    }
    return result;
}

int32_t NetInterfaceConnection::progress(uint64_t monotonicMs)
{
    m_started = true;
    if (m_error != 0 || m_closed)
    {
        return status();
    }
    uint32_t frames = 0;
    for (uint32_t step = 0; step < m_progressStepBudget; ++step)
    {
        bool moved = false;
        const int32_t tlsStatus = m_tls.progress(monotonicMs);
        if (tlsStatus < 0)
        {
            return fail(tlsStatus);
        }
        if (tlsStatus != NET_INTERFACE_TLS_READY)
        {
            return tlsStatus;
        }
        // 只聚合已经排队的帧，不等待新消息；帧预算覆盖本次整个progress。
        // TLS复制成功后才释放帧，记录已交给mbedTLS时append会背压且保持重试输入不变。
        while (frames < m_progressFrameBudget)
        {
            NetConnectionWrite* active = m_queue->active();
            if (active == nullptr)
            {
                break;
            }
            NetConnectionWrite& write = *active;
            uint32_t accepted = 0;
            const int32_t result = m_tls.append(write.m_buffer.data() + m_activeOffset, write.m_size - m_activeOffset, accepted);
            if (result < 0)
            {
                return fail(result);
            }
            m_activeOffset += accepted;
            moved = accepted != 0 || moved;
            if (m_activeOffset == write.m_size)
            {
                completeActive();
                ++frames;
            }
            if (accepted == 0)
            {
                break;
            }
        }
        uint32_t decoded = 0;
        const int32_t frameStatus = decodePending(decoded);
        moved = decoded != 0 || moved;
        if (m_error != 0)
        {
            return m_error;
        }
        if (!m_readBlocked && frameStatus != NI_READY && m_plainSize == 0)
        {
            uint32_t received = 0;
            const int32_t result = m_tls.read(m_plain, sizeof(m_plain), received);
            // WANT_IO以后重复空读不能产生进展；宿主事件负责解除等待。
            // WANT_WRITE同样在consumeOutput释放密文容量后恢复，不能只等新输入。
            m_readBlocked = result == NET_INTERFACE_TLS_WANT_IO && received == 0;
            if (result < 0)
            {
                return fail(result);
            }
            if (result == NET_INTERFACE_TLS_CLOSED)
            {
                const int32_t frameEnd = m_decoder.finish();
                if (frameEnd < 0)
                {
                    return fail(frameEnd);
                }
                m_closed = true;
                clearWrites();
                return NET_INTERFACE_TLS_CLOSED;
            }
            m_plainSize = received;
            moved = received != 0 || moved;
            decodePending(decoded);
            moved = decoded != 0 || moved;
        }
        if (m_error != 0 || !moved)
        {
            break;
        }
        if (step + 1 == m_progressStepBudget)
        {
            // 本轮因预算让出时，已接纳的明文可能尚未转成可写密文，宿主应再推进一次。
            NetExecution::progress();
        }
    }
    if (frames == m_progressFrameBudget && !m_queue->empty())
    {
        NetExecution::progress();
    }
    return status();
}

int32_t NetInterfaceConnection::send(const NetInterfaceFrame& frame)
{
    m_started = true;
    const int32_t current = status();
    if (current != NET_INTERFACE_TLS_READY)
    {
        return current < 0 ? current : NI_INVALID_STATE;
    }
    if (frame.m_payloadSize > m_decoder.payloadLimit())
    {
        return NI_LIMIT_EXCEEDED;
    }
    uint32_t required = 0;
    const bool compress = NetInterfaceCompression::supports(frame) &&
        std::binary_search(m_compressionTypes.begin(), m_compressionTypes.end(), frame.m_messageType);
    const bool compact = m_compact && !compress;
    // 只在不存在待发前缀时使用TLS已有存储；短DATA不再逐帧申请/释放中转记录。
    // TLS最多转换一条已满记录，不创建额外缓冲池；容量不足时完整回退，不能重复发送部分帧。
    if (compact && frame.m_kind == NI_DATA && frame.m_payloadSize <= 64 && m_queue->empty())
    {
        if (frame.m_payloadSize != 0 && (frame.m_payload == nullptr ||
            reinterpret_cast<uintptr_t>(frame.m_payload) > UINTPTR_MAX - frame.m_payloadSize))
        {
            return NI_INVALID_ARGUMENT;
        }
        // 常用增量头和正文分别复制到TLS拥有的同一记录，避免先拼接再擦除一份短正文。
        if (m_previousCompactData.m_kind == NI_DATA && frame.m_productId == m_previousCompactData.m_productId &&
            frame.m_messageType == m_previousCompactData.m_messageType && frame.m_requestId > m_previousCompactData.m_requestId &&
            frame.m_requestId - m_previousCompactData.m_requestId < 128)
        {
            const bool next = frame.m_payloadSize < 64 && frame.m_requestId - m_previousCompactData.m_requestId == 1;
            const uint8_t header[] = {next ? static_cast<uint8_t>(0x80 | frame.m_payloadSize) : static_cast<uint8_t>(0xe4),
                static_cast<uint8_t>(frame.m_payloadSize), static_cast<uint8_t>(frame.m_requestId - m_previousCompactData.m_requestId)};
            const int32_t accepted = m_tls.appendExact(header, next ? 1 : sizeof(header), frame.m_payload, frame.m_payloadSize);
            if (accepted == NET_INTERFACE_TLS_OK)
            {
                // 此分支已确认产品、类型和DATA种类相同；只有编号与长度发生变化。
                m_previousCompactData.m_requestId = frame.m_requestId;
                m_previousCompactData.m_payloadSize = frame.m_payloadSize;
                return NI_OK;
            }
            // 明文仍由提交队列持有；TLS真实缓冲已满时保留队首并让owner推进网络。
            // 不把同一短帧转存到第二个帧池，否则一轮突发会令后续消息全部退化为逐条分配。
            return accepted == NET_INTERFACE_TLS_WANT_IO ? NI_LIMIT_EXCEEDED :
                (accepted < 0 ? accepted : NI_INVALID_STATE);
        }
        uint8_t encoded[89];
        const int32_t encodedResult = NetInterfaceCompact::encode(frame, encoded, sizeof(encoded), required, &m_previousCompactData);
        if (encodedResult != NI_OK)
        {
            return encodedResult;
        }
        const int32_t accepted = m_tls.appendExact(encoded, required);
        wipe(encoded, required);
        if (accepted == NET_INTERFACE_TLS_OK)
        {
            m_previousCompactData = frame;
            m_previousCompactData.m_payload = nullptr;
            return NI_OK;
        }
        return accepted == NET_INTERFACE_TLS_WANT_IO ? NI_LIMIT_EXCEEDED :
            (accepted < 0 ? accepted : NI_INVALID_STATE);
    }
    const int32_t validation = compact ? NetInterfaceCompact::encode(frame, nullptr, 0, required, &m_previousCompactData) :
        NetInterface::encode(frame, nullptr, 0, required);
    if (validation != NI_BUFFER_TOO_SMALL)
    {
        return validation;
    }
    const bool control = (m_controlPayloadLimit < 0 || frame.m_payloadSize <= static_cast<uint32_t>(m_controlPayloadLimit)) &&
        (frame.m_kind == NI_HELLO || frame.m_kind == NI_HELLO_REPLY || frame.m_kind == NI_EVENT ||
        frame.m_kind == NI_CANCEL || frame.m_kind == NI_PING || frame.m_kind == NI_PONG || frame.m_kind == NI_DATA_ACK);
    NetObjectReference<NetConnectionWrite> write;
    const int32_t allocated = m_queue->allocate(control, required, write);
    if (allocated != NI_OK)
    {
        return allocated;
    }
    // 编码失败可能留下前缀，租约析构必须擦除本次编码可写的全部区域。
    write->m_size = required;
    const uint32_t capacity = required;
    const int32_t result = compress ? NetInterfaceCompression::encode(frame, write->m_buffer.data(), capacity, required,
        m_compressionMinPayload, m_compressionLevel) :
        (compact ? NetInterfaceCompact::encode(frame, write->m_buffer.data(), capacity, required, &m_previousCompactData) :
        NetInterface::encode(frame, write->m_buffer.data(), capacity, required));
    if (result != NI_OK)
    {
        return result;
    }
    write->m_size = required;
    m_queue->append(control, write.release());
    if (compact && frame.m_kind == NI_DATA)
    {
        m_previousCompactData = frame;
        m_previousCompactData.m_payload = nullptr;
    }
    // 本轮运输可能已经推进完毕；下一轮须将新排队的明文转换成密文。
    NetExecution::progress();
    return NI_OK;
}

int32_t NetInterfaceConnection::configureCompression(const uint32_t* types, uint32_t count, bool negotiated)
{
    if ((count != 0 && types == nullptr))
    {
        return NI_INVALID_ARGUMENT;
    }
    for (uint32_t index = 0; index < count; ++index)
    {
        if (types[index] == 0)
        {
            return NI_INVALID_ARGUMENT;
        }
    }
    try
    {
        std::vector<uint32_t> copied;
        if (negotiated && count != 0)
        {
            copied.assign(types, types + count);
            std::sort(copied.begin(), copied.end());
            copied.erase(std::unique(copied.begin(), copied.end()), copied.end());
        }
        m_compressionTypes.swap(copied);
        m_decoder.allowCompression(negotiated);
        return NI_OK;
    }
    catch (...) { return NI_NO_MEMORY; }
}

int32_t NetInterfaceConnection::peek(NetInterfaceFrame& frame) const
{
    if (m_error != 0)
    {
        frame = NetInterfaceFrame();
        return m_error;
    }
    return m_decoder.peek(frame);
}

int32_t NetInterfaceConnection::peek(const NetInterfaceFrame*& frame) const
{
    frame = nullptr;
    return m_error != 0 ? m_error : m_decoder.peek(frame);
}

int32_t NetInterfaceConnection::consume()
{
    const int32_t result = m_error != 0 ? m_error : m_decoder.consume();
    if (result == NI_OK)
    {
        NetExecution::progress();
    }
    return result;
}

int32_t NetInterfaceConnection::decodeBuffered()
{
    if (m_error != 0)
    {
        return m_error;
    }
    uint32_t consumed = 0;
    return decodePending(consumed);
}

int32_t NetInterfaceConnection::peekOutput(const uint8_t*& data, uint32_t& size) const
{
    return m_tls.peekOutput(data, size);
}

int32_t NetInterfaceConnection::consumeOutput(uint32_t size)
{
    const int32_t result = m_tls.consumeOutput(size);
    if (result == NI_OK && size != 0)
    {
        m_readBlocked = false;
    }
    return result;
}

int32_t NetInterfaceConnection::finishInput()
{
    if (m_error != 0) { return m_error; }
    bool first = false;
    const int32_t result = m_tls.finishInput(first);
    if (first) { m_readBlocked = false; }
    return result;
}

uint32_t NetInterfaceConnection::decoderBufferCapacity() const
{
    return m_decoder.decoderBufferCapacity();
}

int32_t NetInterfaceConnection::status() const
{
    return m_error != 0 ? m_error : (m_closed ? NET_INTERFACE_TLS_CLOSED : m_tls.status());
}

int32_t NetInterfaceConnection::detail() const
{
    return m_tls.detail();
}

int32_t NetInterfaceConnection::fail(int32_t error)
{
    if (m_error == 0)
    {
        m_error = error;
        clearWrites();
        wipe(m_plain, sizeof(m_plain));
        m_plainSize = m_plainOffset = 0;
    }
    return m_error;
}

int32_t NetInterfaceConnection::outputState() const
{
    if (m_error < 0)
    {
        return m_error;
    }
    if (m_queue && !m_queue->empty())
    {
        return NI_READY;
    }
    return m_tls.outputState();
}

void NetInterfaceConnection::configureCompact(bool enabled)
{
    if (m_compact != enabled)
    {
        m_previousCompactData = NetInterfaceFrame();
    }
    m_compact = enabled;
    m_decoder.allowCompact(enabled);
}
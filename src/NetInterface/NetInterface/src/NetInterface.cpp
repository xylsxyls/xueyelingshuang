#include "NetInterfaceCompact.h"
#include "NetInterface.h"
#include "NetInterfaceMacro.h"
#include "NetInterfaceRequests.h"
#include <cstring>
#include "NetInterfaceCredit.h"
#include "NetInterfaceDirectory.h"
#include "NetInterfaceMessage.h"
#include <new>
#include "NetInterfaceLink.h"
#include "NetInterfaceDirectoryHelper.h"
#include "NetInterfaceGatewayTypes.h"
#include "ProtoMessageManager/ProtoMessageManagerAPI.h"
#include <utility>
#include "NetInterfaceCertificate.h"
#include "NetInterfaceConnection.h"
#include <sstream>
#include "CEncodeDecode/CEncodeDecodeAPI.h"
#include "NetInterfaceFile.h"
#include "NetInterfaceFileGuard.h"
#include "NetInterfaceFileFlowState.h"
#include "NetInterfaceHandshake.h"
#include "NetInterfaceRpc.h"
#include "NetInterfaceTls.h"
#include "NetInterfaceTunnel.h"
#include "NetInterfaceCompression.h"
#include <algorithm>

static const char g_networkAuthority[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIDFTCCAf2gAwIBAgIUNr7MWN6soOVUJlBfxQjnNDCgSmswDQYJKoZIhvcNAQEL\n"
"BQAwGjEYMBYGA1UEAwwPQ29vayBHYXRld2F5IENBMB4XDTI2MTAxNDAyMzUwMloX\n"
"DTM2MTAxMTAyMzUwMlowGjEYMBYGA1UEAwwPQ29vayBHYXRld2F5IENBMIIBIjAN\n"
"BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA7SRDR09NFIWyPnj+AybUMPQ0IIYi\n"
"YdNpjBlDPOrJEu8/8H/2B6YbfObr1E/3SrglgVeVZneTkpfVybd6nZ9uzot+7gqm\n"
"OQZYXWZIdv/+dItoqFGFYIBhQOcvwIokdrBeg13nRXGY2S9AiPgRtAdKLda1gKJh\n"
"eUMDf1X2E76oFWDnEzMbgvdceo+f8EVgPQBIndWFOVDuO3rIQFb6y9j/Iw9CW5Pt\n"
"CBbX5foSM4BaRkRkoIh9IWOmfD1PidWrWTbfq7nxOvCr1m/sv/HSoa5seM6lOmJz\n"
"VjMW5NvZzCt0gwG9zKX33FssDYeEcwMmEpkIZsytoTgQcy+G+gRV2iVBlQIDAQAB\n"
"o1MwUTAdBgNVHQ4EFgQUG6c74EL3DU+dQaJ4x0vFFIZjbjUwHwYDVR0jBBgwFoAU\n"
"G6c74EL3DU+dQaJ4x0vFFIZjbjUwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0B\n"
"AQsFAAOCAQEAmKRdL8Yo6X18gy1wnyfcXbKAgEdTBYvMWFQ/exZ/opPEF1sB04Hp\n"
"0qOh3O0phuBZrQj9XaL8qrOY4rz+M+uFn/511i5jKBmMAfit7zrRrysRPBB/aZKK\n"
"4jeapKJwh+7d4kpFmZMGeniOIJCQ4hNUwSKR52bjxzbS57xBhtgc8aBv5VXj6Yei\n"
"ytLNldA2lP328YwccMesOWeq6qM9Kcd86+syo3/kl0+38mFWpDok2oTyQ8HhhJ4J\n"
"hD8MbABWKMCEFK1GqPchhm2JHDtwHRwcnbTiQpbUdFOBCr3SU5SYjWcg1Y+yU869\n"
"8bRWfr9/8RKJdMKLmwzUrf6yTmgBbPfllw==\n"
"-----END CERTIFICATE-----\n";

/** 输出固定记录，零记录同样填零
@param [in] request POD元数据
@param [out] output 十六项，不能空
*/
static void PackRequest(const NetInterfaceRequest& request, uint32_t* output)
{
    const uint64_t values[7] = {request.m_route,  request.m_source, request.m_original, request.m_target,
                                request.m_origin, request.m_parent, request.m_deadline};
    for (uint32_t index = 0; index < 7; ++index)
    {
        output[index * 2] = static_cast<uint32_t>(values[index]);
        output[index * 2 + 1] = static_cast<uint32_t>(values[index] >> 32);
    }
    output[14] = request.m_product;
    output[15] = request.m_type;
}

/** 无精度损失组合宿主低/高字
@param [in] words 至少两项
@return uint64
*/
static uint64_t RequestPair(const uint32_t* words)
{
    return (static_cast<uint64_t>(words[1]) << 32) | words[0];
}

/** 检查长度并复制输出；不足空间不修改调用方缓冲
@param [in] value 包含NUL的原始字节
@param [out] output 调用方内存，可在capacity为0时为空
@param [in] capacity 容量
@param [out] required 必需字节数，不含额外结束符
@return NetInterfaceResult
*/
static int32_t CopyMessageOutput(const std::string& value, uint8_t* output, uint32_t capacity, uint32_t* required)
{
    *required = static_cast<uint32_t>(value.size());
    if (value.size() > capacity)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    if (!value.empty())
    {
        std::memcpy(output, value.data(), value.size());
    }
    return NI_OK;
}

/** 复制借用记录的布局无关字段
@param [in] record 原生记录
@param [out] fields 五项
@param [out] payload 借用密文
*/
static void LinkApiRecordFields(const NetInterfaceTunnelRecord& record, uint32_t* fields, const uint8_t** payload)
{
    fields[0] = record.m_kind;
    fields[1] = record.m_product;
    fields[2] = static_cast<uint32_t>(record.m_channel);
    fields[3] = static_cast<uint32_t>(record.m_channel >> 32);
    fields[4] = record.m_size;
    *payload = record.m_data;
}

/** 不通过平台结构布局传递64位通道号
@param [in] fields 五项路由字段
@param [in] payload 字节
@return 借用记录
*/
static NetInterfaceTunnelRecord LinkApiRecord(const uint32_t* fields, const uint8_t* payload)
{
    NetInterfaceTunnelRecord record = {};
    record.m_kind = fields[0];
    record.m_product = fields[1];
    record.m_channel = (static_cast<uint64_t>(fields[3]) << 32) | fields[2];
    record.m_size = fields[4];
    record.m_data = payload;
    return record;
}

/** 通过同一解析器创建不透明候选，不从缓存或地址授予身份
@param [in] data 输入
@param [in] size 有界字节数
@param [in] utcMs 0为发现表，否则为缓存当前UTC
@param [out] handle 失败清空
@return NI_OK或错误
*/
static int32_t DirectoryCreate(const uint8_t* data, uint32_t size, uint64_t utcMs, NetInterfaceDirectoryHandle* handle)
{
    if (handle == nullptr || NetInterfaceDirectoryHelper::directoryRangesOverlap(data, size, handle, sizeof(*handle)))
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    try
    {
        std::vector<NetInterfaceDirectoryPoint> points;
        const int32_t result = utcMs == 0 ? NetInterfaceDirectory::parse(data, size, points) :
            NetInterfaceDirectory::parseCache(data, size, utcMs, points);
        if (result != NI_OK) { return result; }
        *handle = reinterpret_cast<NetInterfaceDirectoryHandle>(new std::vector<NetInterfaceDirectoryPoint>(std::move(points)));
        return NI_OK;
    }
    catch (...) { return NI_NO_MEMORY; }
}

/** 比较有界借用区间，不做可能溢出的指针加法
@param [in] first 首地址，可空
@param [in] firstSize 首区间字节数
@param [in] second 次地址，可空
@param [in] secondSize 次区间字节数
@return 是否重叠
*/
static bool CertificateRangesOverlap(const void* first, uint32_t firstSize, const void* second, uint32_t secondSize)
{
    if (first == nullptr || second == nullptr || firstSize == 0 || secondSize == 0)
    {
        return false;
    }
    const uintptr_t firstAddress = reinterpret_cast<uintptr_t>(first);
    const uintptr_t secondAddress = reinterpret_cast<uintptr_t>(second);
    return firstAddress <= secondAddress ? secondAddress - firstAddress < firstSize : firstAddress - secondAddress < secondSize;
}

NetInterface::NetInterface(uint32_t payloadLimit) :
m_payloadLimit(payloadLimit),
m_compressionAllowed(false),
m_compactAllowed(false),
m_headerReady(false),
m_compressed(false),
m_headerSize(0),
m_wireSize(0),
m_body(nullptr),
m_capacity(0),
m_bodySize(0),
m_previousCompactData(),
m_frame(),
m_status(payloadLimit == 0 || payloadLimit > NET_INTERFACE_MAX_PAYLOAD_LIMIT ? NI_INVALID_ARGUMENT : NI_NEED_MORE)
{

}

NetInterface::~NetInterface()
{
    delete[] m_body;
}

uint64_t NetInterface::readNumber(const uint8_t* bytes, uint32_t width)
{
    uint64_t value = 0;
    for (uint32_t index = 0; index < width; ++index)
    {
        value = (value << 8) | bytes[index];
    }
    return value;
}

void NetInterface::writeNumber(uint8_t* bytes, uint64_t value, uint32_t width)
{
    for (uint32_t index = width; index != 0; --index)
    {
        bytes[index - 1] = static_cast<uint8_t>(value);
        value >>= 8;
    }
}

int32_t NetInterface::prepareHeader()
{
    if (m_header[0] == 0xe3 || m_header[0] == 0xe4 || (m_header[0] & 0xc0) == 0x80)
    {
        // feed已完整验证变长头并保存字段；这里不重复扫描相同的整数前缀。
        m_compressed = false;
        m_wireSize = m_frame.m_payloadSize;
    }
    else
    {
        const uint32_t flags = static_cast<uint32_t>(readNumber(m_header + 28, 4));
        if (std::memcmp(m_header, "NIF2", 4) != 0 || readNumber(m_header + 4, 2) != 2 ||
            flags > 1 || (flags == 1 && !m_compressionAllowed))
        {
            return NI_PROTOCOL_ERROR;
        }
        m_frame.m_kind = static_cast<uint16_t>(readNumber(m_header + 6, 2));
        m_compressed = flags == 1;
        if (m_frame.m_kind < NI_HELLO || m_frame.m_kind > NI_DATA_ACK ||
            (m_compressed && m_frame.m_kind != NI_REQUEST && m_frame.m_kind != NI_RESPONSE))
        {
            return NI_PROTOCOL_ERROR;
        }
        m_frame.m_productId = static_cast<uint32_t>(readNumber(m_header + 8, 4));
        m_frame.m_requestId = readNumber(m_header + 12, 8);
        m_wireSize = static_cast<uint32_t>(readNumber(m_header + 20, 4));
        m_frame.m_messageType = static_cast<uint32_t>(readNumber(m_header + 24, 4));
    }
    if (m_wireSize > m_payloadLimit)
    {
        return NI_LIMIT_EXCEEDED;
    }
    if (m_compressed && m_wireSize == 0)
    {
        return NI_PROTOCOL_ERROR;
    }
    if (m_wireSize > m_capacity)
    {
        // 常用小消息按几何容量复用；大消息只占本条所需空间，consume立即释放。
        uint32_t capacity = m_wireSize;
        if (capacity <= 4096)
        {
            capacity = 256;
            while (capacity < m_wireSize)
            {
                capacity *= 2;
            }
        }
        delete[] m_body;
        m_body = nullptr;
        m_capacity = 0;
        m_body = new (std::nothrow) uint8_t[capacity];
        if (m_body == nullptr)
        {
            return NI_NO_MEMORY;
        }
        m_capacity = capacity;
    }
    return NI_NEED_MORE;
}

/** 复制已验证的独立正文，短片段使用定长访问，避免通用memcpy的逐帧调度
@param [out] output 至少size字节的解码器自有存储
@param [in] input 至少size字节的输入；不访问该区间外的字节
@param [in] size 实际正文长度，0时不访问输入和输出
*/
static inline void CopyFrameBody(uint8_t* output, const uint8_t* input, uint32_t size)
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

int32_t NetInterface::feed(const uint8_t* data, uint32_t size, uint32_t& consumed)
{
    consumed = 0;
    if (size != 0 && (data == nullptr || reinterpret_cast<uintptr_t>(data) > UINTPTR_MAX - size))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_status != NI_NEED_MORE)
    {
        return m_status;
    }
    if (!m_headerReady)
    {
        // 常见的单字节长度/编号增量直接取字段；其他宽度及分段头仍走完整规范解析。
        // 不借用输入正文，不减少长度、协商及64位溢出检查。
        if (m_headerSize == 0 && size != 0 && m_compactAllowed && (data[0] & 0xc0) == 0x80 &&
            m_previousCompactData.m_kind == NI_DATA)
        {
            if (m_previousCompactData.m_requestId == UINT64_MAX)
            {
                return fail(NI_PROTOCOL_ERROR);
            }
            const uint32_t length = data[0] & 63;
            m_frame = m_previousCompactData;
            ++m_frame.m_requestId;
            m_frame.m_payloadSize = length;
            m_header[0] = data[0];
            m_headerSize = 1;
            consumed = 1;
            // 完整短帧复用已分配正文；不再经过通用头准备、分段合并和压缩分支。
            // 仍拥有正文副本，接纳后调用方可以立即覆盖输入；分段和首次分配走原路径。
            if (length <= m_capacity && length <= size - 1)
            {
                if (length > m_payloadLimit)
                {
                    return fail(NI_LIMIT_EXCEEDED);
                }
                CopyFrameBody(m_body, data + 1, length);
                m_wireSize = m_bodySize = length;
                m_headerReady = true;
                m_compressed = false;
                m_frame.m_payload = length == 0 ? nullptr : m_body;
                consumed += length;
                m_status = NI_READY;
                return m_status;
            }
        }
        else if (m_headerSize == 0 && size >= 3 && m_compactAllowed && data[0] == 0xe4 &&
            data[1] < 128 && data[2] != 0 && data[2] < 128 && m_previousCompactData.m_kind == NI_DATA)
        {
            if (m_previousCompactData.m_requestId > UINT64_MAX - data[2])
            {
                return fail(NI_PROTOCOL_ERROR);
            }
            m_frame = m_previousCompactData;
            m_frame.m_requestId += data[2];
            m_frame.m_payloadSize = data[1];
            m_header[0] = 0xe4;
            m_headerSize = 3;
            consumed = 3;
        }
        else
        {
            const uint32_t previous = m_headerSize;
            const uint32_t count = (std::min)(size, NET_INTERFACE_HEADER_SIZE - previous);
            if (count != 0)
            {
                std::memcpy(m_header + previous, data, count);
                m_headerSize += count;
                consumed = count;
            }
            if (m_headerSize == 0)
            {
                return NI_NEED_MORE;
            }
            if (m_header[0] == 0xe3 || m_header[0] == 0xe4 || (m_header[0] & 0xc0) == 0x80)
            {
                if (!m_compactAllowed)
                {
                    return fail(NI_PROTOCOL_ERROR);
                }
                NetInterfaceFrame fields = {};
                uint32_t headerSize = 0;
                const int32_t parsed = NetInterfaceCompact::header(m_header, m_headerSize, fields, headerSize, &m_previousCompactData);
                if (parsed == NI_NEED_MORE)
                {
                    return parsed;
                }
                if (parsed < 0)
                {
                    return fail(parsed);
                }
                // 预读到缓存的正文不算消费；原输入仍从实际头末尾接纳，不能吞下一条帧。
                consumed = headerSize - previous;
                m_headerSize = headerSize;
                m_frame = fields;
            }
            else if (m_headerSize != NET_INTERFACE_HEADER_SIZE)
            {
                return NI_NEED_MORE;
            }
        }
        const int32_t checked = prepareHeader();
        if (checked < 0)
        {
            return fail(checked);
        }
        m_headerReady = true;
    }
    const uint32_t count = (std::min)(size - consumed, m_wireSize - m_bodySize);
    if (count != 0)
    {
        std::memcpy(m_body + m_bodySize, data + consumed, count);
        m_bodySize += count;
        consumed += count;
    }
    if (m_bodySize != m_wireSize)
    {
        return NI_NEED_MORE;
    }
    m_frame.m_payloadSize = m_wireSize;
    if (m_compressed)
    {
        uint8_t* decoded = nullptr;
        uint32_t length = 0;
        const int32_t result = NetInterfaceCompression::decode(m_body, m_wireSize, m_payloadLimit, decoded, length);
        if (result != NI_OK)
        {
            return fail(result);
        }
        delete[] m_body;
        m_body = decoded;
        m_capacity = length;
        m_frame.m_payloadSize = length;
    }
    m_frame.m_payload = m_frame.m_payloadSize == 0 ? nullptr : m_body;
    m_status = NI_READY;
    return m_status;
}

int32_t NetInterface::peek(NetInterfaceFrame& frame) const
{
    frame = m_status == NI_READY ? m_frame : NetInterfaceFrame();
    return m_status;
}

int32_t NetInterface::peek(const NetInterfaceFrame*& frame) const
{
    frame = m_status == NI_READY ? &m_frame : nullptr;
    return m_status;
}

void NetInterface::clear(bool release)
{
    if (release)
    {
        delete[] m_body;
        m_body = nullptr;
        m_capacity = 0;
        m_frame = NetInterfaceFrame();
    }
    m_headerSize = 0;
    m_headerReady = false;
    m_wireSize = 0;
    m_bodySize = 0;
    m_compressed = false;
    m_status = m_payloadLimit == 0 || m_payloadLimit > NET_INTERFACE_MAX_PAYLOAD_LIMIT ? NI_INVALID_ARGUMENT : NI_NEED_MORE;
}

int32_t NetInterface::consume()
{
    if (m_status != NI_READY)
    {
        return m_status < 0 ? m_status : NI_INVALID_STATE;
    }
    if (m_compactAllowed && !m_compressed && m_frame.m_kind == NI_DATA &&
        (m_header[0] == 0xe3 || m_header[0] == 0xe4 || (m_header[0] & 0xc0) == 0x80))
    {
        if (m_header[0] == 0xe3)
        {
            m_previousCompactData = m_frame;
            m_previousCompactData.m_payload = nullptr;
        }
        else
        {
            // 增量头已校验并继承产品和消息类型，只更新变化的字段。
            m_previousCompactData.m_requestId = m_frame.m_requestId;
            m_previousCompactData.m_payloadSize = m_frame.m_payloadSize;
        }
    }
    clear(m_capacity > 4096);
    return NI_OK;
}

int32_t NetInterface::finish()
{
    if (m_status != NI_NEED_MORE)
    {
        return m_status;
    }
    return m_headerSize == 0 ? NI_OK : fail(NI_TRUNCATED);
}

void NetInterface::reset()
{
    m_previousCompactData = NetInterfaceFrame();
    clear(true);
}

int32_t NetInterface::status() const
{
    return m_status;
}

uint32_t NetInterface::payloadLimit() const
{
    return m_payloadLimit;
}

int32_t NetInterface::fail(int32_t error)
{
    m_previousCompactData = NetInterfaceFrame();
    clear(true);
    m_status = error;
    return error;
}

void NetInterface::allowCompression(bool allowed)
{
    m_compressionAllowed = allowed;
}

void NetInterface::allowCompact(bool allowed)
{
    if (m_compactAllowed != allowed)
    {
        m_previousCompactData = NetInterfaceFrame();
    }
    m_compactAllowed = allowed;
}

int32_t NetInterface::encodeCompact(const NetInterfaceFrame& frame, uint8_t* output, uint32_t capacity, uint32_t& required)
{
    return NetInterfaceCompact::encode(frame, output, capacity, required);
}

int32_t NetInterface::encode(const NetInterfaceFrame& frame, uint8_t* output, uint32_t capacity, uint32_t& required)
{
    // 先固定元数据，避免输出写入调用方frame存储时覆盖后续读取的字段。
    const NetInterfaceFrame input = frame;
    required = 0;
    if (input.m_kind < NI_HELLO || input.m_kind > NI_DATA_ACK ||
        (input.m_payload == nullptr && input.m_payloadSize != 0) || (output == nullptr && capacity != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (input.m_payloadSize > NET_INTERFACE_MAX_PAYLOAD_LIMIT)
    {
        return NI_LIMIT_EXCEEDED;
    }
    const uintptr_t source = reinterpret_cast<uintptr_t>(input.m_payload);
    if (input.m_payloadSize != 0 && source > UINTPTR_MAX - input.m_payloadSize)
    {
        return NI_INVALID_ARGUMENT;
    }
    required = NET_INTERFACE_HEADER_SIZE + input.m_payloadSize;
    if (capacity < required)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    const uintptr_t destination = reinterpret_cast<uintptr_t>(output);
    if (destination > UINTPTR_MAX - required || (input.m_payloadSize != 0 &&
        source < destination + required && destination < source + input.m_payloadSize))
    {
        required = 0;
        return NI_INVALID_ARGUMENT;
    }
    std::memcpy(output, "NIF2", 4);
    writeNumber(output + 4, 2, 2);
    writeNumber(output + 6, input.m_kind, 2);
    writeNumber(output + 8, input.m_productId, 4);
    writeNumber(output + 12, input.m_requestId, 8);
    writeNumber(output + 20, input.m_payloadSize, 4);
    writeNumber(output + 24, input.m_messageType, 4);
    writeNumber(output + 28, 0, 4);
    if (input.m_payloadSize != 0)
    {
        std::memcpy(output + NET_INTERFACE_HEADER_SIZE, input.m_payload, input.m_payloadSize);
    }
    return NI_OK;
}

int32_t NetInterface::tunnelCreate(NetInterfaceTunnelHandle* handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    NetInterfaceTunnel* decoder = new (std::nothrow) NetInterfaceTunnel();
    if (decoder == nullptr)
    {
        return NI_NO_MEMORY;
    }
    *handle = reinterpret_cast<NetInterfaceTunnelHandle>(decoder);
    return NI_OK;
}

void NetInterface::tunnelDestroy(NetInterfaceTunnelHandle handle)
{
    delete reinterpret_cast<NetInterfaceTunnel*>(handle);
}

int32_t NetInterface::tunnelFeed(NetInterfaceTunnelHandle handle, const uint8_t* data, uint32_t size, uint32_t* consumed)
{
    if (consumed == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *consumed = 0;
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceTunnel*>(handle)->feed(data, size, *consumed);
}

int32_t NetInterface::tunnelPeek(NetInterfaceTunnelHandle handle, uint32_t* fields, const uint8_t** payload)
{
    if (fields != nullptr)
    {
        std::memset(fields, 0, 5 * sizeof(uint32_t));
    }
    if (payload != nullptr)
    {
        *payload = nullptr;
    }
    if (handle == nullptr || fields == nullptr || payload == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceTunnelRecord record = {};
    const int32_t result = reinterpret_cast<NetInterfaceTunnel*>(handle)->peek(record);
    if (result == NI_READY)
    {
        fields[0] = record.m_kind;
        fields[1] = record.m_product;
        fields[2] = static_cast<uint32_t>(record.m_channel);
        fields[3] = static_cast<uint32_t>(record.m_channel >> 32);
        fields[4] = record.m_size;
        *payload = record.m_data;
    }
    return result;
}

int32_t NetInterface::tunnelConsume(NetInterfaceTunnelHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceTunnel*>(handle)->consume();
}

int32_t NetInterface::tunnelFinish(NetInterfaceTunnelHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceTunnel*>(handle)->finish();
}

int32_t NetInterface::tunnelEncode(const uint32_t* fields, const uint8_t* payload, uint8_t* output,
    uint32_t capacity, uint32_t* required)
{
    if (required == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *required = 0;
    if (fields == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceTunnelRecord record = {};
    record.m_kind = fields[0];
    record.m_product = fields[1];
    record.m_channel = static_cast<uint64_t>(fields[3]) << 32 | fields[2];
    record.m_data = payload;
    record.m_size = fields[4];
    return NetInterfaceTunnel::encode(record, output, capacity, *required);
}

const uint8_t* NetInterface::trustAuthority(uint32_t* size)
{
    if (size != nullptr)
    {
        *size = static_cast<uint32_t>(sizeof(g_networkAuthority));
    }
    return reinterpret_cast<const uint8_t*>(g_networkAuthority);
}

int32_t NetInterface::tlsCreateClient(const uint8_t* authority, uint32_t authoritySize, const char* hostname,
                                    NetInterfaceEntropyFunction entropy, void* context, uint32_t timeoutMs, NetInterfaceTlsHandle* handle,
                                    int32_t* detail)
{
    NetInterfaceTlsOptions options = {};
    options.authority = authority;
    options.authoritySize = authoritySize;
    options.hostname = hostname;
    options.entropy = entropy;
    options.entropyContext = context;
    options.handshakeTimeoutMs = timeoutMs;
    return NetInterface::tlsCreate(&options, handle, detail);
}

int32_t NetInterface::tlsCreate(const NetInterfaceTlsOptions* options, NetInterfaceTlsHandle* handle, int32_t* detail)
{
    if (detail != nullptr)
    {
        *detail = 0;
    }
    if (handle == nullptr)
    {
        return NET_INTERFACE_TLS_ARGUMENT;
    }
    *handle = nullptr;
    if (options == nullptr)
    {
        return NET_INTERFACE_TLS_ARGUMENT;
    }
    NetInterfaceTls* instance = new (std::nothrow) NetInterfaceTls(*options);
    if (instance == nullptr)
    {
        return NET_INTERFACE_TLS_MEMORY;
    }
    int32_t result = instance->status();
    if (detail != nullptr)
    {
        *detail = instance->detail();
    }
    if (result < 0)
    {
        delete instance;
    }
    else
    {
        *handle = reinterpret_cast<NetInterfaceTlsHandle>(instance);
    }
    return result;
}

void NetInterface::tlsDestroy(NetInterfaceTlsHandle handle)
{
    delete reinterpret_cast<NetInterfaceTls*>(handle);
}

int32_t NetInterface::tlsFeed(NetInterfaceTlsHandle handle, const uint8_t* data, uint32_t size, uint32_t* consumed)
{
    if (consumed == nullptr)
    {
        return NET_INTERFACE_TLS_ARGUMENT;
    }
    *consumed = 0;
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->feed(data, size, *consumed);
}

int32_t NetInterface::tlsProgress(NetInterfaceTlsHandle handle, uint32_t timeLow, uint32_t timeHigh)
{
    uint64_t time = (static_cast<uint64_t>(timeHigh) << 32) | timeLow;
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->progress(time);
}

int32_t NetInterface::tlsPeekOutput(NetInterfaceTlsHandle handle, const uint8_t** data, uint32_t* size)
{
    if (data != nullptr)
    {
        *data = nullptr;
    }
    if (size != nullptr)
    {
        *size = 0;
    }
    if (handle == nullptr || data == nullptr || size == nullptr)
    {
        return NET_INTERFACE_TLS_ARGUMENT;
    }
    return reinterpret_cast<NetInterfaceTls*>(handle)->peekOutput(*data, *size);
}

int32_t NetInterface::tlsConsumeOutput(NetInterfaceTlsHandle handle, uint32_t size)
{
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->consumeOutput(size);
}

int32_t NetInterface::tlsWrite(NetInterfaceTlsHandle handle, const uint8_t* data, uint32_t size, uint32_t* accepted)
{
    if (accepted == nullptr)
    {
        return NET_INTERFACE_TLS_ARGUMENT;
    }
    *accepted = 0;
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->write(data, size, *accepted);
}

int32_t NetInterface::tlsRead(NetInterfaceTlsHandle handle, uint8_t* output, uint32_t capacity, uint32_t* received)
{
    if (received == nullptr)
    {
        return NET_INTERFACE_TLS_ARGUMENT;
    }
    *received = 0;
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->read(output, capacity, *received);
}

int32_t NetInterface::tlsFinishInput(NetInterfaceTlsHandle handle)
{
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->finishInput();
}

int32_t NetInterface::tlsClose(NetInterfaceTlsHandle handle)
{
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->close();
}

int32_t NetInterface::tlsGetStatus(NetInterfaceTlsHandle handle, int32_t* detail)
{
    if (detail != nullptr)
    {
        *detail = handle == nullptr ? 0 : reinterpret_cast<NetInterfaceTls*>(handle)->detail();
    }
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->status();
}

int32_t NetInterface::rpcEncode(uint32_t kind, uint32_t value, uint32_t parentLow, uint32_t parentHigh, const uint8_t* data, uint32_t size,
                              uint8_t* output, uint32_t capacity, uint32_t* required)
{
    if (required == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *required = 0;
    if (kind > 65535)
    {
        return NI_INVALID_ARGUMENT;
    }
    return NetInterfaceRpc::encode(static_cast<uint16_t>(kind), value, (static_cast<uint64_t>(parentHigh) << 32) | parentLow, data, size,
                                   output, capacity, *required);
}

int32_t NetInterface::rpcParse(const uint32_t* frameFields, uint32_t count, const uint8_t* data, uint32_t* fields)
{
    uint32_t snapshot[6] = {};
    if (frameFields != nullptr && count == 6)
    {
        std::memcpy(snapshot, frameFields, sizeof(snapshot));
    }
    if (fields != nullptr)
    {
        std::memset(fields, 0, 5 * sizeof(uint32_t));
    }
    if (frameFields == nullptr || count != 6 || fields == nullptr || snapshot[0] > 65535)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = static_cast<uint16_t>(snapshot[0]);
    frame.m_productId = snapshot[1];
    frame.m_requestId = (static_cast<uint64_t>(snapshot[3]) << 32) | snapshot[2];
    frame.m_messageType = snapshot[4];
    frame.m_payloadSize = snapshot[5];
    frame.m_payload = data;
    NetInterfaceRpcView view = {};
    const int32_t result = NetInterfaceRpc::parse(frame, view);
    if (result == NI_OK)
    {
        fields[0] = view.m_value;
        fields[1] = static_cast<uint32_t>(view.m_parent);
        fields[2] = static_cast<uint32_t>(view.m_parent >> 32);
        fields[3] = 16;
        fields[4] = view.m_size;
    }
    return result;
}

int32_t NetInterface::rpcControl(const uint32_t* fields, uint32_t count, uint32_t* reply)
{
    uint32_t values[6] = {};
    if (fields != nullptr && count == 6)
    {
        std::memcpy(values, fields, sizeof(values));
    }
    if (reply != nullptr)
    {
        std::memset(reply, 0, sizeof(values));
    }
    if (fields == nullptr || count != 6 || reply == nullptr || values[0] > 65535)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = static_cast<uint16_t>(values[0]);
    frame.m_productId = values[1];
    frame.m_requestId = (static_cast<uint64_t>(values[3]) << 32) | values[2];
    frame.m_messageType = values[4];
    frame.m_payloadSize = values[5];
    const int32_t result = NetInterfaceRpc::control(frame, frame);
    if (result == NI_READY)
    {
        reply[0] = frame.m_kind;
        reply[1] = frame.m_productId;
        reply[2] = static_cast<uint32_t>(frame.m_requestId);
        reply[3] = static_cast<uint32_t>(frame.m_requestId >> 32);
        reply[4] = frame.m_messageType;
        reply[5] = frame.m_payloadSize;
    }
    return result;
}

int32_t NetInterface::requestsCreate(uint32_t capacity, NetInterfaceRequestsHandle* handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    if (capacity == 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::unique_ptr<NetInterfaceRequests> requests(new NetInterfaceRequests(capacity));
        const int32_t initialized = requests->status();
        if (initialized != NI_OK)
        {
            return initialized;
        }
        *handle = reinterpret_cast<NetInterfaceRequestsHandle>(requests.release());
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

void NetInterface::requestsDestroy(NetInterfaceRequestsHandle handle)
{
    delete reinterpret_cast<NetInterfaceRequests*>(handle);
}

int32_t NetInterface::requestsAddEndpoint(NetInterfaceRequestsHandle handle, uint32_t low, uint32_t high, uint32_t service, uint32_t product,
                                        uint32_t limit, uint32_t internalOnly)
{
    if (handle == nullptr || service > 1 || internalOnly > 1)
    {
        return NI_INVALID_ARGUMENT;
    }
    return reinterpret_cast<NetInterfaceRequests*>(handle)->addEndpoint((static_cast<uint64_t>(high) << 32) | low, service != 0, product,
                                                                        limit, internalOnly != 0);
}

int32_t NetInterface::requestsRemoveEndpoint(NetInterfaceRequestsHandle handle, uint32_t low, uint32_t high)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT
                             : reinterpret_cast<NetInterfaceRequests*>(handle)->removeEndpoint((static_cast<uint64_t>(high) << 32) | low);
}

int32_t NetInterface::requestsBegin(NetInterfaceRequestsHandle handle, const uint32_t* input, uint32_t inputCount, uint32_t autoId,
                                  uint32_t* output)
{
    uint32_t snapshot[13] = {};
    if (input != nullptr && inputCount == 13)
    {
        std::memcpy(snapshot, input, sizeof(snapshot));
        input = snapshot;
    }
    if (output != nullptr)
    {
        std::memset(output, 0, 16 * sizeof(uint32_t));
    }
    if (handle == nullptr || input == nullptr || output == nullptr || inputCount != 13 || autoId > 1 ||
        (autoId != 0 && RequestPair(input + 2) != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceRequest request = {};
    NetInterfaceRequests* requests = reinterpret_cast<NetInterfaceRequests*>(handle);
    const int32_t result = autoId ? requests->beginNext(RequestPair(input), RequestPair(input + 4), input[6], input[7],
                                                        RequestPair(input + 8), RequestPair(input + 10), input[12], request)
                                  : requests->begin(RequestPair(input), RequestPair(input + 2), RequestPair(input + 4), input[6], input[7],
                                                    RequestPair(input + 8), RequestPair(input + 10), input[12], request);
    PackRequest(request, output);
    return result;
}

int32_t NetInterface::requestsComplete(NetInterfaceRequestsHandle handle, const uint32_t* input, uint32_t inputCount, uint32_t* output)
{
    uint32_t snapshot[10] = {};
    if (input != nullptr && (inputCount == 8 || inputCount == 10))
    {
        std::memcpy(snapshot, input, inputCount * sizeof(uint32_t));
        input = snapshot;
    }
    if (output != nullptr)
    {
        std::memset(output, 0, 16 * sizeof(uint32_t));
    }
    if (handle == nullptr || input == nullptr || output == nullptr || (inputCount != 8 && inputCount != 10))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceRequest request = {};
    NetInterfaceRequests* requests = reinterpret_cast<NetInterfaceRequests*>(handle);
    const int32_t result = inputCount == 8 ? requests->complete(RequestPair(input), RequestPair(input + 2), input[4], input[5],
                                                                RequestPair(input + 6), request)
                                           : requests->completeOriginal(RequestPair(input), RequestPair(input + 2), RequestPair(input + 4),
                                                                        input[6], input[7], RequestPair(input + 8), request);
    PackRequest(request, output);
    return result;
}

int32_t NetInterface::requestsCancel(NetInterfaceRequestsHandle handle, uint32_t sourceLow, uint32_t sourceHigh, uint32_t originalLow,
                                   uint32_t originalHigh, uint32_t* output)
{
    if (output != nullptr)
    {
        std::memset(output, 0, 16 * sizeof(uint32_t));
    }
    if (handle == nullptr || output == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceRequest request = {};
    const int32_t result = reinterpret_cast<NetInterfaceRequests*>(handle)->cancel(
        (static_cast<uint64_t>(sourceHigh) << 32) | sourceLow, (static_cast<uint64_t>(originalHigh) << 32) | originalLow, request);
    PackRequest(request, output);
    return result;
}

int32_t NetInterface::requestsTake(NetInterfaceRequestsHandle handle, uint32_t operation, uint32_t keyLow, uint32_t keyHigh, uint32_t* output)
{
    if (output != nullptr)
    {
        std::memset(output, 0, 16 * sizeof(uint32_t));
    }
    if (handle == nullptr || output == nullptr || operation > 2)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceRequest request = {};
    NetInterfaceRequests* requests = reinterpret_cast<NetInterfaceRequests*>(handle);
    const uint64_t key = (static_cast<uint64_t>(keyHigh) << 32) | keyLow;
    const int32_t result = operation == 0 ? requests->takeExpired(key, request)
                                          : (operation == 1 ? requests->takeEndpoint(key, request) : requests->takeChild(key, request));
    PackRequest(request, output);
    return result;
}

int32_t NetInterface::requestsCount(NetInterfaceRequestsHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : static_cast<int32_t>(reinterpret_cast<NetInterfaceRequests*>(handle)->count());
}

int32_t NetInterface::messageCreate(NetInterfaceMessageHandle* handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    try
    {
        *handle = reinterpret_cast<NetInterfaceMessageHandle>(new NetInterfaceMessage);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

void NetInterface::messageDestroy(NetInterfaceMessageHandle handle)
{
    delete reinterpret_cast<NetInterfaceMessage*>(handle);
}

int32_t NetInterface::messageClear(NetInterfaceMessageHandle handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    reinterpret_cast<NetInterfaceMessage*>(handle)->clear();
    return NI_OK;
}

int32_t NetInterface::messageParse(NetInterfaceMessageHandle handle, const uint8_t* input, uint32_t size)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceMessage* state = reinterpret_cast<NetInterfaceMessage*>(handle);
    state->clear();
    if (input == nullptr && size != 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (size > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT)
    {
        return NI_LIMIT_EXCEEDED;
    }
    try
    {
        std::string bytes;
        if (size != 0)
        {
            bytes.assign(reinterpret_cast<const char*>(input), size);
        }
        if (!state->m_message.from(bytes))
        {
            return NI_PROTOCOL_ERROR;
        }
        state->m_budget = size;
        return NI_OK;
    }
    catch (...)
    {
        state->clear();
        return NI_NO_MEMORY;
    }
}

int32_t NetInterface::messageEncode(NetInterfaceMessageHandle handle, uint8_t* output, uint32_t capacity, uint32_t* required)
{
    if (required != nullptr)
    {
        *required = 0;
    }
    if (handle == nullptr || required == nullptr || (output == nullptr && capacity != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::string bytes;
        if (!reinterpret_cast<NetInterfaceMessage*>(handle)->m_message.toString(bytes))
        {
            return NI_PROTOCOL_ERROR;
        }
        if (bytes.size() > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT)
        {
            return NI_LIMIT_EXCEEDED;
        }
        return CopyMessageOutput(bytes, output, capacity, required);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetInterface::messageSetValue(NetInterfaceMessageHandle handle, int32_t group, int32_t key, int32_t type, const uint8_t* value,
                                    uint32_t size)
{
    if (handle == nullptr || (value == nullptr && size != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceMessage* state = reinterpret_cast<NetInterfaceMessage*>(handle);
    if (size > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT - 64 || state->m_budget > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT - 64 - size)
    {
        return NI_LIMIT_EXCEEDED;
    }
    try
    {
        std::string bytes;
        if (size != 0)
        {
            bytes.assign(reinterpret_cast<const char*>(value), size);
        }
        if (!Variant::isValidBlob(bytes, type))
        {
            return NI_INVALID_ARGUMENT;
        }
        state->m_message.setKeyValue(key, Variant(bytes, static_cast<Variant::VariantType>(type), true), group);
        state->m_budget += 64 + size;
        return NI_OK;
    }
    catch (...)
    {
        state->clear();
        return NI_NO_MEMORY;
    }
}

int32_t NetInterface::messageGetValue(NetInterfaceMessageHandle handle, int32_t group, int32_t key, int32_t* type, uint8_t* output,
                                    uint32_t capacity, uint32_t* required)
{
    if (required != nullptr)
    {
        *required = 0;
    }
    if (type != nullptr)
    {
        *type = 0;
    }
    if (handle == nullptr || type == nullptr || required == nullptr || (output == nullptr && capacity != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        Variant value;
        if (!reinterpret_cast<NetInterfaceMessage*>(handle)->m_message.getKeyValue(key, value, group))
        {
            return NI_NOT_FOUND;
        }
        *type = static_cast<int32_t>(value.type());
        return CopyMessageOutput(value.toBlob(), output, capacity, required);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetInterface::messageSetCode(NetInterfaceMessageHandle handle, int32_t code, const uint8_t* error, uint32_t size)
{
    if (handle == nullptr || (error == nullptr && size != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceMessage* state = reinterpret_cast<NetInterfaceMessage*>(handle);
    if (size > 4096 || state->m_budget > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT - size - 32)
    {
        return NI_LIMIT_EXCEEDED;
    }
    try
    {
        state->m_message.setCode(code, size ? std::string(reinterpret_cast<const char*>(error), size) : std::string());
        state->m_budget += size + 32;
        return NI_OK;
    }
    catch (...)
    {
        state->clear();
        return NI_NO_MEMORY;
    }
}

int32_t NetInterface::messageGetCode(NetInterfaceMessageHandle handle, int32_t* code, uint8_t* error, uint32_t capacity, uint32_t* required)
{
    if (required != nullptr)
    {
        *required = 0;
    }
    if (code != nullptr)
    {
        *code = ProtoMessage::PROTO_ERROR;
    }
    if (handle == nullptr || code == nullptr || required == nullptr || (error == nullptr && capacity != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        ProtoMessage& message = reinterpret_cast<NetInterfaceMessage*>(handle)->m_message;
        *code = message.getCode();
        return CopyMessageOutput(message.getErrorMessage(), error, capacity, required);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetInterface::linkCreate(const NetInterfaceLinkOptions* options, NetInterfaceLinkHandle* handle)
{
    if (handle == nullptr) { return NI_INVALID_ARGUMENT; }
    *handle = nullptr;
    if (options == nullptr) { return NI_INVALID_ARGUMENT; }
    try
    {
        NetInterfaceLink* link = new (std::nothrow) NetInterfaceLink(*options);
        if (link == nullptr) { return NI_NO_MEMORY; }
        const int32_t status = link->status();
        if (status < 0) { delete link; return status; }
        *handle = reinterpret_cast<NetInterfaceLinkHandle>(link);
        return status;
    }
    catch (...) { return NI_NO_MEMORY; }
}

void NetInterface::linkDestroy(NetInterfaceLinkHandle handle)
{
    delete reinterpret_cast<NetInterfaceLink*>(handle);
}

int32_t NetInterface::linkAccept(NetInterfaceLinkHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    return handle == nullptr || fields == nullptr ? NI_INVALID_ARGUMENT :
        reinterpret_cast<NetInterfaceLink*>(handle)->accept(LinkApiRecord(fields, payload));
}

int32_t NetInterface::linkProgress(NetInterfaceLinkHandle handle, uint32_t low, uint32_t high)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT :
        reinterpret_cast<NetInterfaceLink*>(handle)->progress((static_cast<uint64_t>(high) << 32) | low);
}

int32_t NetInterface::linkPeekOutput(NetInterfaceLinkHandle handle, uint32_t* fields, const uint8_t** payload)
{
    if (fields != nullptr) { std::memset(fields, 0, 5 * sizeof(uint32_t)); }
    if (payload != nullptr) { *payload = nullptr; }
    if (handle == nullptr || fields == nullptr || payload == nullptr) { return NI_INVALID_ARGUMENT; }
    NetInterfaceTunnelRecord record = {};
    const int32_t result = reinterpret_cast<NetInterfaceLink*>(handle)->peekOutput(record);
    if (result == NI_READY) { LinkApiRecordFields(record, fields, payload); }
    return result;
}

int32_t NetInterface::linkConsumeOutput(NetInterfaceLinkHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceLink*>(handle)->consumeOutput();
}

int32_t NetInterface::linkSendRecord(NetInterfaceLinkHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    return handle == nullptr || fields == nullptr ? NI_INVALID_ARGUMENT :
        reinterpret_cast<NetInterfaceLink*>(handle)->sendRecord(LinkApiRecord(fields, payload));
}

int32_t NetInterface::linkPeekRecord(NetInterfaceLinkHandle handle, uint32_t* fields, const uint8_t** payload)
{
    if (fields != nullptr) { std::memset(fields, 0, 5 * sizeof(uint32_t)); }
    if (payload != nullptr) { *payload = nullptr; }
    if (handle == nullptr || fields == nullptr || payload == nullptr) { return NI_INVALID_ARGUMENT; }
    NetInterfaceTunnelRecord record = {};
    const int32_t result = reinterpret_cast<NetInterfaceLink*>(handle)->peekRecord(record);
    if (result == NI_READY) { LinkApiRecordFields(record, fields, payload); }
    return result;
}

int32_t NetInterface::linkConsumeRecord(NetInterfaceLinkHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceLink*>(handle)->consumeRecord();
}

int32_t NetInterface::linkSendControl(NetInterfaceLinkHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    if (handle == nullptr || fields == nullptr || fields[0] > UINT16_MAX) { return NI_INVALID_ARGUMENT; }
    NetInterfaceFrame frame = {};
    frame.m_kind = static_cast<uint16_t>(fields[0]);
    frame.m_productId = fields[1];
    frame.m_requestId = (static_cast<uint64_t>(fields[3]) << 32) | fields[2];
    frame.m_messageType = fields[4];
    frame.m_payloadSize = fields[5];
    frame.m_payload = payload;
    return reinterpret_cast<NetInterfaceLink*>(handle)->sendControl(frame);
}

int32_t NetInterface::linkPeekControl(NetInterfaceLinkHandle handle, uint32_t* fields, const uint8_t** payload)
{
    if (fields != nullptr) { std::memset(fields, 0, 6 * sizeof(uint32_t)); }
    if (payload != nullptr) { *payload = nullptr; }
    if (handle == nullptr || fields == nullptr || payload == nullptr) { return NI_INVALID_ARGUMENT; }
    NetInterfaceFrame frame = {};
    const int32_t result = reinterpret_cast<NetInterfaceLink*>(handle)->peekControl(frame);
    if (result == NI_READY)
    {
        fields[0] = frame.m_kind;
        fields[1] = frame.m_productId;
        fields[2] = static_cast<uint32_t>(frame.m_requestId);
        fields[3] = static_cast<uint32_t>(frame.m_requestId >> 32);
        fields[4] = frame.m_messageType;
        fields[5] = frame.m_payloadSize;
        *payload = frame.m_payload;
    }
    return result;
}

int32_t NetInterface::linkConsumeControl(NetInterfaceLinkHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceLink*>(handle)->consumeControl();
}

int32_t NetInterface::linkPeerInfo(NetInterfaceLinkHandle handle, uint32_t* fields,
    const uint8_t** name, const uint8_t** certificate, const uint8_t** claims)
{
    if (fields != nullptr) { std::memset(fields, 0, 4 * sizeof(uint32_t)); }
    if (name != nullptr) { *name = nullptr; }
    if (certificate != nullptr) { *certificate = nullptr; }
    if (claims != nullptr) { *claims = nullptr; }
    if (handle == nullptr || fields == nullptr || name == nullptr || certificate == nullptr || claims == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceLinkPeer peer = {};
    const int32_t result = reinterpret_cast<NetInterfaceLink*>(handle)->peerInfo(peer);
    if (result == NI_READY)
    {
        fields[0] = peer.m_role;
        fields[1] = static_cast<uint32_t>(std::strlen(peer.m_name));
        fields[2] = peer.m_certificateSize;
        fields[3] = peer.m_claimsSize;
        *name = reinterpret_cast<const uint8_t*>(peer.m_name);
        *certificate = peer.m_certificate;
        *claims = peer.m_claims;
    }
    return result;
}

int32_t NetInterface::linkFinishInput(NetInterfaceLinkHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceLink*>(handle)->finishInput();
}

int32_t NetInterface::linkStatus(NetInterfaceLinkHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceLink*>(handle)->status();
}

int32_t NetInterface::handshakeCreate(const uint32_t* options, uint32_t optionCount, const uint8_t* identities, uint32_t identitySize,
                                    uint32_t timeLow, uint32_t timeHigh, NetInterfaceHandshakeHandle* handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    if (options == nullptr || identities == nullptr || optionCount != 6 || identitySize != 32)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceHandshakeOptions configuration = {};
    configuration.m_server = options[0];
    configuration.m_platform = options[1];
    configuration.m_capabilities = options[2];
    configuration.m_maxRequests = options[3];
    configuration.m_maxChunk = options[4];
    configuration.m_timeoutMs = options[5];
    std::memcpy(configuration.m_nonce, identities, 16);
    std::memcpy(configuration.m_device, identities + 16, 16);
    const uint64_t now = (static_cast<uint64_t>(timeHigh) << 32) | timeLow;
    try
    {
        NetInterfaceHandshake* handshake = new NetInterfaceHandshake(configuration, now);
        const int32_t result = handshake->progress(now);
        if (result < 0)
        {
            delete handshake;
        }
        else
        {
            *handle = reinterpret_cast<NetInterfaceHandshakeHandle>(handshake);
        }
        return result;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

void NetInterface::handshakeDestroy(NetInterfaceHandshakeHandle handle)
{
    delete reinterpret_cast<NetInterfaceHandshake*>(handle);
}

int32_t NetInterface::handshakeProgress(NetInterfaceHandshakeHandle handle, uint32_t timeLow, uint32_t timeHigh)
{
    return handle == nullptr
               ? NI_INVALID_ARGUMENT
               : reinterpret_cast<NetInterfaceHandshake*>(handle)->progress((static_cast<uint64_t>(timeHigh) << 32) | timeLow);
}

int32_t NetInterface::handshakeAccept(NetInterfaceHandshakeHandle handle, const uint32_t* fields, const uint8_t* payload, uint32_t timeLow,
                                    uint32_t timeHigh)
{
    if (handle == nullptr || fields == nullptr || fields[0] > 65535 || (fields[5] != 0 && payload == nullptr))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = static_cast<uint16_t>(fields[0]);
    frame.m_productId = fields[1];
    frame.m_requestId = (static_cast<uint64_t>(fields[3]) << 32) | fields[2];
    frame.m_messageType = fields[4];
    frame.m_payloadSize = fields[5];
    frame.m_payload = payload;
    return reinterpret_cast<NetInterfaceHandshake*>(handle)->accept(frame, (static_cast<uint64_t>(timeHigh) << 32) | timeLow);
}

int32_t NetInterface::handshakePeekOutput(NetInterfaceHandshakeHandle handle, uint32_t* fields, const uint8_t** payload)
{
    if (fields != nullptr)
    {
        std::memset(fields, 0, 6 * sizeof(uint32_t));
    }
    if (payload != nullptr)
    {
        *payload = nullptr;
    }
    if (handle == nullptr || fields == nullptr || payload == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    const int32_t result = reinterpret_cast<NetInterfaceHandshake*>(handle)->peekOutput(frame);
    if (result == NI_READY)
    {
        fields[0] = frame.m_kind;
        fields[4] = frame.m_messageType;
        fields[5] = frame.m_payloadSize;
        *payload = frame.m_payload;
    }
    return result;
}

int32_t NetInterface::handshakeConsumeOutput(NetInterfaceHandshakeHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceHandshake*>(handle)->consumeOutput();
}

int32_t NetInterface::handshakePeerInfo(NetInterfaceHandshakeHandle handle, uint32_t* fields, uint8_t* identities)
{
    if (fields != nullptr)
    {
        std::memset(fields, 0, 4 * sizeof(uint32_t));
    }
    if (identities != nullptr)
    {
        std::memset(identities, 0, 32);
    }
    if (handle == nullptr || fields == nullptr || identities == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return reinterpret_cast<NetInterfaceHandshake*>(handle)->peerInfo(fields, identities, identities + 16);
}

int32_t NetInterface::fileParseInfo(const uint8_t* bytes, uint32_t size, NetInterfaceFileInfo* info)
{
    return info != nullptr ? NetInterfaceFile::parseInfo(bytes, size, *info) : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileInfoFields(const uint8_t* bytes, uint32_t size, uint32_t* fields, uint8_t* transfer, uint8_t* token, uint8_t* hash)
{
    if (NetInterfaceFileGuard::overlap(fields, 12 * sizeof(uint32_t), bytes, size) ||
        NetInterfaceFileGuard::overlap(transfer, 16, bytes, size) ||
        NetInterfaceFileGuard::overlap(token, 32, bytes, size) ||
        NetInterfaceFileGuard::overlap(hash, 32, bytes, size) ||
        NetInterfaceFileGuard::overlap(fields, 12 * sizeof(uint32_t), transfer, 16) ||
        NetInterfaceFileGuard::overlap(fields, 12 * sizeof(uint32_t), token, 32) ||
        NetInterfaceFileGuard::overlap(fields, 12 * sizeof(uint32_t), hash, 32) ||
        NetInterfaceFileGuard::overlap(transfer, 16, token, 32) ||
        NetInterfaceFileGuard::overlap(transfer, 16, hash, 32) ||
        NetInterfaceFileGuard::overlap(token, 32, hash, 32))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (fields != nullptr)
    {
        std::memset(fields, 0, sizeof(uint32_t) * 12);
    }
    if (transfer != nullptr)
    {
        std::memset(transfer, 0, 16);
    }
    if (token != nullptr)
    {
        std::memset(token, 0, 32);
    }
    if (hash != nullptr)
    {
        std::memset(hash, 0, 32);
    }
    if (fields == nullptr || transfer == nullptr || token == nullptr || hash == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFileInfo info = {};
    NetInterfaceFileGuard wipe(&info, sizeof(info));
    const int32_t result = NetInterfaceFile::parseInfo(bytes, size, info);
    if (result != NI_OK)
    {
        return result;
    }
    fields[0] = info.direction;
    fields[1] = info.state;
    fields[2] = info.chunk;
    fields[3] = info.window;
    fields[4] = static_cast<uint32_t>(info.total);
    fields[5] = static_cast<uint32_t>(info.total >> 32);
    fields[6] = static_cast<uint32_t>(info.offset);
    fields[7] = static_cast<uint32_t>(info.offset >> 32);
    fields[8] = static_cast<uint32_t>(info.stream);
    fields[9] = static_cast<uint32_t>(info.stream >> 32);
    fields[10] = static_cast<uint32_t>(info.expiresUtc);
    fields[11] = static_cast<uint32_t>(info.expiresUtc >> 32);
    std::memcpy(transfer, info.transfer, 16);
    std::memcpy(token, info.token, 32);
    std::memcpy(hash, info.hash, 32);
    return NI_OK;
}

int32_t NetInterface::fileFlowCreate(const NetInterfaceFileInfo* info, uint32_t product, uint32_t sender, uint32_t timeLow, uint32_t timeHigh,
                                   uint32_t timeoutMs, NetInterfaceFileFlowHandle* handle)
{
    if (NetInterfaceFileGuard::overlap(handle, sizeof(*handle), info, sizeof(*info)))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle != nullptr)
    {
        *handle = nullptr;
    }
    if (handle == nullptr || info == nullptr || sender > 1)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        const uint64_t now = (static_cast<uint64_t>(timeHigh) << 32) | timeLow;
        *handle = new NetInterfaceFileFlowOpaque(*info, product, sender != 0, now, timeoutMs);
        const int32_t status = (*handle)->m_flow.progress(now);
        if (status < 0)
        {
            delete *handle;
            *handle = nullptr;
            return status;
        }
        return NI_OK;
    }
    catch (...)
    {
        delete *handle;
        *handle = nullptr;
        return NI_NO_MEMORY;
    }
}

void NetInterface::fileFlowDestroy(NetInterfaceFileFlowHandle handle)
{
    delete handle;
}

int32_t NetInterface::fileFlowCreateWire(const uint8_t* bytes, uint32_t size, uint32_t product, uint32_t sender, uint32_t timeLow,
                                       uint32_t timeHigh, uint32_t timeoutMs, NetInterfaceFileFlowHandle* handle)
{
    if (NetInterfaceFileGuard::overlap(handle, sizeof(*handle), bytes, size))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle != nullptr)
    {
        *handle = nullptr;
    }
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFileInfo info = {};
    NetInterfaceFileGuard wipe(&info, sizeof(info));
    const int32_t parsed = NetInterfaceFile::parseInfo(bytes, size, info);
    if (parsed != NI_OK)
    {
        return parsed;
    }
    const int32_t result = NetInterface::fileFlowCreate(&info, product, sender, timeLow, timeHigh, timeoutMs, handle);
    return result;
}

int32_t NetInterface::fileFlowPrime(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh, const uint8_t* bytes,
                                  uint32_t size)
{
    return handle != nullptr ? handle->m_flow.prime((static_cast<uint64_t>(offsetHigh) << 32) | offsetLow, bytes, size)
                             : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowProgress(NetInterfaceFileFlowHandle handle, uint32_t timeLow, uint32_t timeHigh)
{
    return handle != nullptr ? handle->m_flow.progress((static_cast<uint64_t>(timeHigh) << 32) | timeLow) : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowRead(NetInterfaceFileFlowHandle handle, uint32_t* fields)
{
    if (fields != nullptr)
    {
        std::memset(fields, 0, sizeof(uint32_t) * 3);
    }
    if (handle == nullptr || fields == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    uint64_t offset = 0;
    uint32_t size = 0;
    const int32_t result = handle->m_flow.readRequest(offset, size);
    if (result == NI_READY)
    {
        fields[0] = static_cast<uint32_t>(offset);
        fields[1] = static_cast<uint32_t>(offset >> 32);
        fields[2] = size;
    }
    return result;
}

int32_t NetInterface::fileFlowProduced(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh, const uint8_t* bytes,
                                     uint32_t size)
{
    return handle != nullptr ? handle->m_flow.produced((static_cast<uint64_t>(offsetHigh) << 32) | offsetLow, bytes, size)
                             : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowAccept(NetInterfaceFileFlowHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    if (handle == nullptr || fields == nullptr || fields[0] > UINT16_MAX || (fields[5] != 0 && payload == nullptr))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = static_cast<uint16_t>(fields[0]);
    frame.m_productId = fields[1];
    frame.m_requestId = (static_cast<uint64_t>(fields[3]) << 32) | fields[2];
    frame.m_messageType = fields[4];
    frame.m_payloadSize = fields[5];
    frame.m_payload = payload;
    return handle->m_flow.accept(frame);
}

int32_t NetInterface::fileFlowWrite(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** bytes)
{
    if (NetInterfaceFileGuard::overlap(fields, 3 * sizeof(uint32_t), bytes, sizeof(*bytes)))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (fields != nullptr)
    {
        std::memset(fields, 0, sizeof(uint32_t) * 3);
    }
    if (bytes != nullptr)
    {
        *bytes = nullptr;
    }
    if (handle == nullptr || fields == nullptr || bytes == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    uint64_t offset = 0;
    uint32_t size = 0;
    const int32_t result = handle->m_flow.writeRequest(offset, *bytes, size);
    if (result == NI_READY)
    {
        fields[0] = static_cast<uint32_t>(offset);
        fields[1] = static_cast<uint32_t>(offset >> 32);
        fields[2] = size;
    }
    return result;
}

int32_t NetInterface::fileFlowWritten(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh, uint32_t size,
                                    int32_t result)
{
    return handle != nullptr ? handle->m_flow.written((static_cast<uint64_t>(offsetHigh) << 32) | offsetLow, size, result)
                             : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowTakeCommit(NetInterfaceFileFlowHandle handle)
{
    return handle != nullptr ? handle->m_flow.takeCommit() : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowCommitted(NetInterfaceFileFlowHandle handle, int32_t result)
{
    return handle != nullptr ? handle->m_flow.committed(result) : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowPeek(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** payload)
{
    if (NetInterfaceFileGuard::overlap(fields, 6 * sizeof(uint32_t), payload, sizeof(*payload)))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (fields != nullptr)
    {
        std::memset(fields, 0, sizeof(uint32_t) * 6);
    }
    if (payload != nullptr)
    {
        *payload = nullptr;
    }
    if (handle == nullptr || fields == nullptr || payload == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    const int32_t result = handle->m_flow.peekOutput(frame);
    if (result == NI_READY)
    {
        fields[0] = frame.m_kind;
        fields[1] = frame.m_productId;
        fields[2] = static_cast<uint32_t>(frame.m_requestId);
        fields[3] = static_cast<uint32_t>(frame.m_requestId >> 32);
        fields[4] = frame.m_messageType;
        fields[5] = frame.m_payloadSize;
        *payload = frame.m_payload;
    }
    return result;
}

int32_t NetInterface::fileFlowConsume(NetInterfaceFileFlowHandle handle)
{
    return handle != nullptr ? handle->m_flow.consumeOutput() : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowCancel(NetInterfaceFileFlowHandle handle, int32_t reason)
{
    if (handle == nullptr || reason >= 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    handle->m_flow.cancel(reason);
    return NI_OK;
}

int32_t NetInterface::fileFlowSetWriteBatchChunks(NetInterfaceFileFlowHandle handle, uint32_t chunks)
{
    return handle ? handle->m_flow.setWriteBatchChunks(chunks) : NI_INVALID_ARGUMENT;
}

int32_t NetInterface::fileFlowSnapshot(NetInterfaceFileFlowHandle handle, NetInterfaceFileInfo* info, int32_t* result)
{
    if (NetInterfaceFileGuard::overlap(info, sizeof(*info), result, sizeof(*result)))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (info != nullptr)
    {
        std::memset(info, 0, sizeof(*info));
    }
    if (result != nullptr)
    {
        *result = NI_INVALID_ARGUMENT;
    }
    if (handle == nullptr || info == nullptr || result == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    handle->m_flow.snapshot(*info, *result);
    return NI_OK;
}

int32_t NetInterface::fileFlowSnapshotWire(NetInterfaceFileFlowHandle handle, uint8_t* bytes, uint32_t capacity, int32_t* result)
{
    if (NetInterfaceFileGuard::overlap(bytes, capacity, result, sizeof(*result)))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (result)
    {
        *result = NI_INVALID_STATE;
    }
    if (!handle || !result || (!bytes && capacity))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (capacity < NET_INTERFACE_FILE_INFO_SIZE)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    NetInterfaceFileInfo info = {};
    NetInterfaceFileGuard wipe(&info, sizeof(info));
    handle->m_flow.snapshot(info, *result);
    return NetInterfaceFile::encodeInfo(info, bytes, capacity);
}

int32_t NetInterface::fileControlEncode(const uint32_t* fields, const uint8_t* transfer, const uint8_t* token, const uint8_t* hash,
                                      const uint8_t* key, uint32_t keySize, const uint8_t* authorization, uint32_t authorizationSize,
                                      uint8_t* output, uint32_t capacity, uint32_t* required)
{
    if (required != nullptr &&
        (NetInterfaceFileGuard::overlap(required, sizeof(*required), fields, 5 * sizeof(uint32_t)) ||
         NetInterfaceFileGuard::overlap(required, sizeof(*required), transfer, 16) ||
         NetInterfaceFileGuard::overlap(required, sizeof(*required), token, 32) ||
         NetInterfaceFileGuard::overlap(required, sizeof(*required), hash, 32) ||
         NetInterfaceFileGuard::overlap(required, sizeof(*required), key, keySize) ||
         NetInterfaceFileGuard::overlap(required, sizeof(*required), authorization, authorizationSize) ||
         NetInterfaceFileGuard::overlap(required, sizeof(*required), output, capacity)))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (required != nullptr)
    {
        *required = 0;
    }
    if (!fields || !transfer || !token || !hash || !required || (!output && capacity))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFileControl control = {};
    NetInterfaceFileGuard wipe(&control, sizeof(control));
    control.operation = fields[0];
    control.chunk = fields[1];
    control.window = fields[2];
    control.total = (static_cast<uint64_t>(fields[4]) << 32) | fields[3];
    std::memcpy(control.transfer, transfer, 16);
    std::memcpy(control.token, token, 32);
    std::memcpy(control.hash, hash, 32);
    control.key = key;
    control.keySize = keySize;
    control.authorization = authorization;
    control.authorizationSize = authorizationSize;
    return NetInterfaceFile::encodeControl(control, output, capacity, *required);
}

int32_t NetInterface::directoryCreateNodes(const NetInterfaceDirectoryNodeData* nodes, uint32_t count,
    NetInterfaceDirectoryHandle* handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (count <= UINT32_MAX / sizeof(NetInterfaceDirectoryNodeData) && NetInterfaceDirectoryHelper::directoryRangesOverlap(nodes,
        count * static_cast<uint32_t>(sizeof(NetInterfaceDirectoryNodeData)), handle, sizeof(*handle)))
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    if (nodes == nullptr || count == 0 || count > UINT32_MAX / sizeof(NetInterfaceDirectoryNodeData))
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::vector<NetInterfaceDirectoryPoint> points;
        points.reserve(count);
        for (uint32_t index = 0; index < count; ++index)
        {
            const NetInterfaceDirectoryNodeData& input = nodes[index];
            if (input.m_structSize != sizeof(NetInterfaceDirectoryNodeData) || input.m_version != 1 ||
                std::memchr(input.m_name, 0, sizeof(input.m_name)) == nullptr ||
                std::memchr(input.m_address, 0, sizeof(input.m_address)) == nullptr)
            {
                return NI_INVALID_ARGUMENT;
            }
            NetInterfaceDirectoryPoint point;
            point.m_name = input.m_name;
            point.m_address = input.m_address;
            point.m_port = input.m_port;
            point.m_capacity = input.m_capacity;
            point.m_boot = input.m_boot;
            points.push_back(point);
        }
        std::string encoded;
        const int32_t result = NetInterfaceDirectory::encode(points, encoded);
        if (result != NI_OK)
        {
            return result;
        }
        *handle = reinterpret_cast<NetInterfaceDirectoryHandle>(new std::vector<NetInterfaceDirectoryPoint>(std::move(points)));
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetInterface::directoryCreate(const uint8_t* data, uint32_t size, NetInterfaceDirectoryHandle* handle)
{
    return DirectoryCreate(data, size, 0, handle);
}

int32_t NetInterface::directoryCreateCache(const uint8_t* data, uint32_t size, uint32_t utcLow, uint32_t utcHigh,
    NetInterfaceDirectoryHandle* handle)
{
    if (handle == nullptr || NetInterfaceDirectoryHelper::directoryRangesOverlap(data, size, handle, sizeof(*handle)))
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    const uint64_t utc = (static_cast<uint64_t>(utcHigh) << 32) | utcLow;
    return utc == 0 ? NI_INVALID_ARGUMENT : DirectoryCreate(data, size, utc, handle);
}

void NetInterface::directoryDestroy(NetInterfaceDirectoryHandle handle)
{
    delete reinterpret_cast<std::vector<NetInterfaceDirectoryPoint>*>(handle);
}

int32_t NetInterface::directoryCount(NetInterfaceDirectoryHandle handle, uint32_t* count)
{
    if (count == nullptr) { return NI_INVALID_ARGUMENT; }
    *count = 0;
    if (handle == nullptr) { return NI_INVALID_ARGUMENT; }
    *count = static_cast<uint32_t>(reinterpret_cast<std::vector<NetInterfaceDirectoryPoint>*>(handle)->size());
    return NI_OK;
}

int32_t NetInterface::directoryCopyNodes(NetInterfaceDirectoryHandle handle,
    NetInterfaceDirectoryNodeData* nodes, uint32_t capacity, uint32_t* count)
{
    if (count == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (capacity <= UINT32_MAX / sizeof(NetInterfaceDirectoryNodeData) && NetInterfaceDirectoryHelper::directoryRangesOverlap(nodes,
        capacity * static_cast<uint32_t>(sizeof(NetInterfaceDirectoryNodeData)), count, sizeof(*count)))
    {
        return NI_INVALID_ARGUMENT;
    }
    *count = 0;
    if (handle == nullptr || (nodes == nullptr && capacity != 0) || capacity > UINT32_MAX / sizeof(NetInterfaceDirectoryNodeData))
    {
        return NI_INVALID_ARGUMENT;
    }
    const std::vector<NetInterfaceDirectoryPoint>& points = *reinterpret_cast<std::vector<NetInterfaceDirectoryPoint>*>(handle);
    if (capacity < points.size())
    {
        *count = static_cast<uint32_t>(points.size());
        return NI_BUFFER_TOO_SMALL;
    }
    for (uint32_t index = 0; index < points.size(); ++index)
    {
        if (nodes[index].m_structSize != sizeof(NetInterfaceDirectoryNodeData) || nodes[index].m_version != 1)
        {
            return NI_INVALID_ARGUMENT;
        }
    }
    for (uint32_t index = 0; index < points.size(); ++index)
    {
        const NetInterfaceDirectoryPoint& point = points[index];
        NetInterfaceDirectoryNodeData output;
        std::memset(&output, 0, sizeof(output));
        output.m_structSize = sizeof(output);
        output.m_version = 1;
        std::memcpy(output.m_name, point.m_name.c_str(), point.m_name.size() + 1);
        std::memcpy(output.m_address, point.m_address.c_str(), point.m_address.size() + 1);
        output.m_port = point.m_port;
        output.m_capacity = point.m_capacity;
        output.m_boot = point.m_boot;
        nodes[index] = output;
    }
    *count = static_cast<uint32_t>(points.size());
    return NI_OK;
}

int32_t NetInterface::directoryNode(NetInterfaceDirectoryHandle handle, uint32_t index, char* name, uint32_t nameCapacity,
    char* address, uint32_t addressCapacity, uint32_t* fields)
{
    if (fields == nullptr || NetInterfaceDirectoryHelper::directoryRangesOverlap(name, nameCapacity, address, addressCapacity) ||
        NetInterfaceDirectoryHelper::directoryRangesOverlap(name, nameCapacity, fields, 4 * sizeof(uint32_t)) ||
        NetInterfaceDirectoryHelper::directoryRangesOverlap(address, addressCapacity, fields, 4 * sizeof(uint32_t)))
    {
        return NI_INVALID_ARGUMENT;
    }
    std::memset(fields, 0, 4 * sizeof(uint32_t));
    if (handle == nullptr) { return NI_INVALID_ARGUMENT; }
    const std::vector<NetInterfaceDirectoryPoint>& points = *reinterpret_cast<std::vector<NetInterfaceDirectoryPoint>*>(handle);
    if (index >= points.size()) { return NI_NOT_FOUND; }
    const NetInterfaceDirectoryPoint& point = points[index];
    if (name == nullptr || address == nullptr || nameCapacity <= point.m_name.size() || addressCapacity <= point.m_address.size())
    {
        return NI_BUFFER_TOO_SMALL;
    }
    std::memcpy(name, point.m_name.c_str(), point.m_name.size() + 1);
    std::memcpy(address, point.m_address.c_str(), point.m_address.size() + 1);
    fields[0] = point.m_port;
    fields[1] = point.m_capacity;
    fields[2] = static_cast<uint32_t>(point.m_boot);
    fields[3] = static_cast<uint32_t>(point.m_boot >> 32);
    return NI_OK;
}

int32_t NetInterface::directoryCache(NetInterfaceDirectoryHandle handle, uint32_t utcLow, uint32_t utcHigh,
    uint8_t* output, uint32_t capacity, uint32_t* required)
{
    if (required == nullptr || NetInterfaceDirectoryHelper::directoryRangesOverlap(output, capacity, required, sizeof(*required)))
    {
        return NI_INVALID_ARGUMENT;
    }
    *required = 0;
    if (handle == nullptr) { return NI_INVALID_ARGUMENT; }
    try
    {
        std::string bytes;
        const int32_t result = NetInterfaceDirectory::encodeCache(*reinterpret_cast<std::vector<NetInterfaceDirectoryPoint>*>(handle),
            (static_cast<uint64_t>(utcHigh) << 32) | utcLow, bytes);
        if (result != NI_OK) { return result; }
        *required = static_cast<uint32_t>(bytes.size());
        if (output == nullptr || capacity < bytes.size()) { return NI_BUFFER_TOO_SMALL; }
        std::memcpy(output, bytes.data(), bytes.size());
        return NI_OK;
    }
    catch (...) { return NI_NO_MEMORY; }
}

/** 将密码运算结果转换为网关状态，不向调用方传播第三方错误值
@param [in] result CEncodeDecode通用运算结果
@return 网关状态；未知错误始终按失败处理
*/
static int32_t CryptoResult(int32_t result)
{
    switch (result)
    {
    case CENCODE_OK: return NI_OK;
    case CENCODE_ARGUMENT: return NI_INVALID_ARGUMENT;
    case CENCODE_MEMORY: return NI_NO_MEMORY;
    case CENCODE_LIMIT: return NI_LIMIT_EXCEEDED;
    case CENCODE_BUFFER: return NI_BUFFER_TOO_SMALL;
    default: return NI_INVALID_STATE;
    }
}

int32_t NetInterface::hashCreate(NetInterfaceHashHandle* handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    CEncodeDecodeHash* hash = nullptr;
    const int32_t result = CEncodeDecode::hashCreate(&hash);
    *handle = reinterpret_cast<NetInterfaceHashHandle>(hash);
    return CryptoResult(result);
}

int32_t NetInterface::hashUpdate(NetInterfaceHashHandle handle, const uint8_t* input, uint32_t size)
{
    return CryptoResult(CEncodeDecode::hashUpdate(reinterpret_cast<CEncodeDecodeHash*>(handle), input, size));
}

int32_t NetInterface::hashFinish(NetInterfaceHashHandle handle, uint8_t* output, uint32_t capacity)
{
    return CryptoResult(CEncodeDecode::hashFinish(reinterpret_cast<CEncodeDecodeHash*>(handle), output, capacity));
}

void NetInterface::hashDestroy(NetInterfaceHashHandle handle)
{
    CEncodeDecode::hashDestroy(reinterpret_cast<CEncodeDecodeHash*>(handle));
}

int32_t NetInterface::sha256(const uint8_t* input, uint32_t size, uint8_t* output, uint32_t capacity)
{
    return CryptoResult(CEncodeDecode::sha256(input, size, output, capacity));
}

int32_t NetInterface::pbkdf2Sha256(const uint8_t* password, uint32_t passwordSize, const uint8_t* salt, uint32_t saltSize,
    uint32_t iterations, uint8_t* output, uint32_t capacity)
{
    return CryptoResult(CEncodeDecode::pbkdf2Sha256(password, passwordSize, salt, saltSize, iterations, output, capacity));
}

void NetInterface::wipe(uint8_t* bytes, uint32_t size)
{
    CEncodeDecode::wipe(bytes, size);
}

int32_t NetInterface::entropyBytes(NetInterfaceEntropyFunction entropy, void* context, uint8_t* output, uint32_t size)
{
    if ((output == nullptr && size != 0) || size > 8192)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (size == 0)
    {
        return NI_OK;
    }
    try
    {
        if (entropy != nullptr && entropy(context, output, size) == 0)
        {
            return NI_OK;
        }
    }
    catch (...)
    {
        // 部分输出或异常不能作为握手随机数继续使用
    }
    NetInterface::wipe(output, size);
    return NI_INVALID_STATE;
}

int32_t NetInterface::creditConfigure(NetInterfaceCreditHandle handle, uint32_t bytes)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceCredit*>(handle)->configure(bytes);
}

int32_t NetInterface::creditConfigureMessage(NetInterfaceCreditHandle handle, const uint8_t* input, uint32_t size)
{
    if (handle == nullptr || input == nullptr || size == 0 || size > 16384) { return NI_INVALID_ARGUMENT; }
    try
    {
        ProtoMessage message;
        uint32_t bytes = 0;
        if (!message.from(std::string(reinterpret_cast<const char*>(input), size)) ||
            NetInterfaceCredit::window(message, bytes) != NI_OK)
        {
            return NI_PROTOCOL_ERROR;
        }
        return reinterpret_cast<NetInterfaceCredit*>(handle)->configure(bytes);
    }
    catch (...) { return NI_NO_MEMORY; }
}

int32_t NetInterface::creditCreate(NetInterfaceCreditHandle* handle)
{
    if (handle == nullptr) { return NI_INVALID_ARGUMENT; }
    *handle = nullptr;
    try { *handle = reinterpret_cast<NetInterfaceCreditHandle>(new NetInterfaceCredit()); return NI_OK; }
    catch (...) { return NI_NO_MEMORY; }
}

void NetInterface::creditDestroy(NetInterfaceCreditHandle handle)
{
    delete reinterpret_cast<NetInterfaceCredit*>(handle);
}

int32_t NetInterface::creditGet(NetInterfaceCreditHandle handle, uint32_t* fields)
{
    if (handle == nullptr || fields == nullptr) { return NI_INVALID_ARGUMENT; }
    NetInterfaceCredit* credit = reinterpret_cast<NetInterfaceCredit*>(handle);
    fields[0] = credit->available();
    fields[1] = credit->pending();
    return credit->status();
}

int32_t NetInterface::creditSent(NetInterfaceCreditHandle handle, uint32_t size)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceCredit*>(handle)->sent(size);
}

int32_t NetInterface::creditGrant(NetInterfaceCreditHandle handle, uint32_t size)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceCredit*>(handle)->grant(size);
}

int32_t NetInterface::creditReceived(NetInterfaceCreditHandle handle, uint32_t size)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceCredit*>(handle)->received(size);
}

int32_t NetInterface::creditConsume(NetInterfaceCreditHandle handle, uint32_t size)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceCredit*>(handle)->consume(size);
}

int32_t NetInterface::connectionCreate(const NetInterfaceTlsOptions* options, NetInterfaceConnectionHandle* handle, int32_t* detail)
{
    if (detail != nullptr)
    {
        *detail = 0;
    }
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    if (options == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        NetInterfaceConnection* connection = new NetInterfaceConnection(*options);
        const int32_t result = connection->status();
        if (detail != nullptr)
        {
            *detail = connection->detail();
        }
        if (result < 0)
        {
            delete connection;
        }
        else
        {
            *handle = reinterpret_cast<NetInterfaceConnectionHandle>(connection);
        }
        return result;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetInterface::connectionCreateServiceClient(uint32_t product, NetInterfaceEntropyFunction entropy, void* context,
    uint32_t timeoutMs, NetInterfaceConnectionHandle* handle, int32_t* detail)
{
    if (handle == nullptr) { return NI_INVALID_ARGUMENT; }
    *handle = nullptr;
    if (detail != nullptr) { *detail = 0; }
    try
    {
        std::ostringstream name;
        name << "service." << product;
        const std::string hostname = name.str();
        NetInterfaceTlsOptions options = {};
        options.authority = NetInterface::trustAuthority(&options.authoritySize);
        options.hostname = hostname.c_str();
        options.entropy = entropy;
        options.entropyContext = context;
        options.handshakeTimeoutMs = timeoutMs;
        return NetInterface::connectionCreate(&options, handle, detail);
    }
    catch (...) { return NI_NO_MEMORY; }
}

void NetInterface::connectionDestroy(NetInterfaceConnectionHandle handle)
{
    delete reinterpret_cast<NetInterfaceConnection*>(handle);
}

int32_t NetInterface::connectionFeed(NetInterfaceConnectionHandle handle, const uint8_t* data, uint32_t size, uint32_t* consumed)
{
    if (consumed != nullptr)
    {
        *consumed = 0;
    }
    if (handle == nullptr || consumed == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return reinterpret_cast<NetInterfaceConnection*>(handle)->feed(data, size, *consumed);
}

int32_t NetInterface::connectionProgress(NetInterfaceConnectionHandle handle, uint32_t timeLow, uint32_t timeHigh)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return reinterpret_cast<NetInterfaceConnection*>(handle)->progress((static_cast<uint64_t>(timeHigh) << 32) | timeLow);
}

int32_t NetInterface::connectionConfigureCompression(NetInterfaceConnectionHandle handle, const uint32_t* types, uint32_t count, int32_t negotiated)
{
    if (handle == nullptr || (negotiated != 0 && negotiated != 1)) { return NI_INVALID_ARGUMENT; }
    return reinterpret_cast<NetInterfaceConnection*>(handle)->configureCompression(types, count, negotiated != 0);
}

int32_t NetInterface::connectionSend(NetInterfaceConnectionHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    if (handle == nullptr || fields == nullptr || fields[0] > 65535)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = static_cast<uint16_t>(fields[0]);
    frame.m_productId = fields[1];
    frame.m_requestId = (static_cast<uint64_t>(fields[3]) << 32) | fields[2];
    frame.m_messageType = fields[4];
    frame.m_payloadSize = fields[5];
    frame.m_payload = payload;
    return reinterpret_cast<NetInterfaceConnection*>(handle)->send(frame);
}

int32_t NetInterface::connectionPeek(NetInterfaceConnectionHandle handle, uint32_t* fields, const uint8_t** payload)
{
    if (fields != nullptr)
    {
        std::memset(fields, 0, 6 * sizeof(uint32_t));
    }
    if (payload != nullptr)
    {
        *payload = nullptr;
    }
    if (handle == nullptr || fields == nullptr || payload == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    const int32_t result = reinterpret_cast<NetInterfaceConnection*>(handle)->peek(frame);
    if (result == NI_READY)
    {
        fields[0] = frame.m_kind;
        fields[1] = frame.m_productId;
        fields[2] = static_cast<uint32_t>(frame.m_requestId);
        fields[3] = static_cast<uint32_t>(frame.m_requestId >> 32);
        fields[4] = frame.m_messageType;
        fields[5] = frame.m_payloadSize;
        *payload = frame.m_payload;
    }
    return result;
}

int32_t NetInterface::connectionConsume(NetInterfaceConnectionHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceConnection*>(handle)->consume();
}

int32_t NetInterface::connectionPeekOutput(NetInterfaceConnectionHandle handle, const uint8_t** data, uint32_t* size)
{
    if (data != nullptr)
    {
        *data = nullptr;
    }
    if (size != nullptr)
    {
        *size = 0;
    }
    if (handle == nullptr || data == nullptr || size == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return reinterpret_cast<NetInterfaceConnection*>(handle)->peekOutput(*data, *size);
}

int32_t NetInterface::connectionConsumeOutput(NetInterfaceConnectionHandle handle, uint32_t size)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceConnection*>(handle)->consumeOutput(size);
}

int32_t NetInterface::connectionFinishInput(NetInterfaceConnectionHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceConnection*>(handle)->finishInput();
}

int32_t NetInterface::connectionGetStatus(NetInterfaceConnectionHandle handle, int32_t* detail)
{
    if (detail != nullptr)
    {
        *detail = handle == nullptr ? 0 : reinterpret_cast<NetInterfaceConnection*>(handle)->detail();
    }
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceConnection*>(handle)->status();
}

int32_t NetInterface::create(uint32_t payloadLimit, NetInterfaceHandle* handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    if (payloadLimit == 0 || payloadLimit > NET_INTERFACE_MAX_PAYLOAD_LIMIT)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterface* decoder = new (std::nothrow) NetInterface(payloadLimit);
    if (decoder == nullptr)
    {
        return NI_NO_MEMORY;
    }
    *handle = reinterpret_cast<NetInterfaceHandle>(decoder);
    return NI_OK;
}

void NetInterface::destroy(NetInterfaceHandle handle)
{
    delete reinterpret_cast<NetInterface*>(handle);
}

int32_t NetInterface::feed(NetInterfaceHandle handle, const uint8_t* data, uint32_t size, uint32_t* consumed)
{
    if (consumed == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *consumed = 0;
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return reinterpret_cast<NetInterface*>(handle)->feed(data, size, *consumed);
}

int32_t NetInterface::peek(NetInterfaceHandle handle, NetInterfaceFrame* frame)
{
    if (frame == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    std::memset(frame, 0, sizeof(*frame));
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return reinterpret_cast<NetInterface*>(handle)->peek(*frame);
}

int32_t NetInterface::consume(NetInterfaceHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterface*>(handle)->consume();
}

int32_t NetInterface::finish(NetInterfaceHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterface*>(handle)->finish();
}

int32_t NetInterface::reset(NetInterfaceHandle handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    reinterpret_cast<NetInterface*>(handle)->reset();
    return NI_OK;
}

int32_t NetInterface::encode(const NetInterfaceFrame* frame, uint8_t* output, uint32_t capacity, uint32_t* required)
{
    if (required == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *required = 0;
    if (frame == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return NetInterface::encode(*frame, output, capacity, *required);
}

int32_t NetInterface::peekFields(NetInterfaceHandle handle, uint32_t* fields, const uint8_t** payload)
{
    if (fields != nullptr)
    {
        std::memset(fields, 0, 6 * sizeof(uint32_t));
    }
    if (payload != nullptr)
    {
        *payload = nullptr;
    }
    if (fields == nullptr || payload == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    int32_t result = NetInterface::peek(handle, &frame);
    if (result == NI_READY)
    {
        fields[0] = frame.m_kind;
        fields[1] = frame.m_productId;
        fields[2] = static_cast<uint32_t>(frame.m_requestId);
        fields[3] = static_cast<uint32_t>(frame.m_requestId >> 32);
        fields[4] = frame.m_messageType;
        fields[5] = frame.m_payloadSize;
        *payload = frame.m_payload;
    }
    return result;
}

int32_t NetInterface::encodeFields(const uint32_t* fields, const uint8_t* payload, uint8_t* output, uint32_t capacity, uint32_t* required)
{
    if (required != nullptr)
    {
        *required = 0;
    }
    if (fields == nullptr || fields[0] > UINT16_MAX)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = static_cast<uint16_t>(fields[0]);
    frame.m_productId = fields[1];
    frame.m_requestId = (static_cast<uint64_t>(fields[3]) << 32) | fields[2];
    frame.m_messageType = fields[4];
    frame.m_payloadSize = fields[5];
    frame.m_payload = payload;
    return NetInterface::encode(&frame, output, capacity, required);
}

int32_t NetInterface::certificateCreateRequest(const uint8_t* name, uint32_t nameSize,
    NetInterfaceEntropyFunction entropy, void* context, uint8_t* privateKey, uint32_t keyCapacity,
    uint32_t* keySize, uint8_t* request, uint32_t requestCapacity, uint32_t* requestSize)
{
    if (CertificateRangesOverlap(keySize, sizeof(uint32_t), requestSize, sizeof(uint32_t)) ||
        CertificateRangesOverlap(keySize, sizeof(uint32_t), privateKey, keyCapacity) ||
        CertificateRangesOverlap(keySize, sizeof(uint32_t), request, requestCapacity) ||
        CertificateRangesOverlap(keySize, sizeof(uint32_t), name, nameSize) ||
        CertificateRangesOverlap(requestSize, sizeof(uint32_t), privateKey, keyCapacity) ||
        CertificateRangesOverlap(requestSize, sizeof(uint32_t), request, requestCapacity) ||
        CertificateRangesOverlap(requestSize, sizeof(uint32_t), name, nameSize))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (keySize != nullptr)
    {
        *keySize = 0;
    }
    if (requestSize != nullptr)
    {
        *requestSize = 0;
    }
    if (keySize == nullptr || requestSize == nullptr || privateKey == nullptr || request == nullptr ||
        keyCapacity == 0 || keyCapacity > NET_INTERFACE_IDENTITY_PEM_BYTES || requestCapacity == 0 ||
        requestCapacity > NET_INTERFACE_IDENTITY_PEM_BYTES || name == nullptr || nameSize == 0 || nameSize > 96 || entropy == nullptr ||
        CertificateRangesOverlap(privateKey, keyCapacity, request, requestCapacity) ||
        CertificateRangesOverlap(privateKey, keyCapacity, name, nameSize) ||
        CertificateRangesOverlap(request, requestCapacity, name, nameSize))
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        if (!NetInterfaceCertificate::validName(std::string(reinterpret_cast<const char*>(name), nameSize)))
        {
            return NI_INVALID_ARGUMENT;
        }
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
    const int32_t result = CEncodeDecode::p256CreateRequest(name, nameSize, entropy, context,
        privateKey, keyCapacity, keySize, request, requestCapacity, requestSize);
    return result == CENCODE_STATE ? NI_PROTOCOL_ERROR : CryptoResult(result);
}
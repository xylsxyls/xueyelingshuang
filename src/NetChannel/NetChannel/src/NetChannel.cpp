#include "NetChannel.h"
#include <algorithm>
#include <cstring>
#include <new>

/** 检查计数字段是否与调用者提供的字节范围共用存储，按地址差比较以避免末地址回卷
@param [in] count 四字节长度输出
@param [in] bytes 数据范围首地址，空指针或零长度没有交集
@param [in] size 数据范围长度，单位字节
@return 计数和字节范围重叠返回 true
*/
static bool OverlapsCount(const uint32_t& count, const void* bytes, uint32_t size)
{
    if (!bytes || !size) { return false; }
    const uintptr_t word = reinterpret_cast<uintptr_t>(&count);
    const uintptr_t start = reinterpret_cast<uintptr_t>(bytes);
    return word >= start ? word - start < size : start - word < sizeof(count);
}

NetChannel::NetChannel() :
m_headerBytes(0),
m_bodyBytes(0),
m_storage(nullptr),
m_capacity(0),
m_state(NC_NEED_MORE),
m_record()
{

}

NetChannel::~NetChannel()
{
    delete[] m_storage;
}

bool NetChannel::valid(const NetChannelRecord& record)
{
    if (record.m_size > NET_CHANNEL_PAYLOAD_LIMIT)
    {
        return false;
    }
    switch (record.m_kind)
    {
    case NC_HELLO:
    {
        return record.m_product == 0 && record.m_channel == 0 && record.m_size == 0;
    }
    case NC_CONTROL:
    {
        return record.m_product == 0 && record.m_channel == 0 && record.m_size != 0;
    }
    case NC_BYTES:
    {
        return record.m_channel != 0 && record.m_size != 0;
    }
    case NC_CLOSE:
    {
        return record.m_channel != 0 && record.m_size == 4;
    }
    default:
    {
        return false;
    }
    }
}

uint64_t NetChannel::readNumber(const uint8_t* bytes, uint32_t width)
{
    uint64_t value = 0;
    for (uint32_t index = 0; index < width; ++index)
    {
        value = (value << 8) | bytes[index];
    }
    return value;
}

void NetChannel::writeNumber(uint8_t* bytes, uint64_t value, uint32_t width)
{
    for (uint32_t index = width; index != 0; --index)
    {
        bytes[index - 1] = static_cast<uint8_t>(value);
        value >>= 8;
    }
}

int32_t NetChannel::acceptHeader()
{
    if (std::memcmp(m_header, "NTW3", 4) != 0 || m_header[4] != 3 || m_header[6] != 0 || m_header[7] != 0)
    {
        return NC_PROTOCOL_ERROR;
    }
    m_record.m_kind = m_header[5];
    m_record.m_product = static_cast<uint32_t>(readNumber(m_header + 8, 4));
    m_record.m_channel = readNumber(m_header + 12, 8);
    m_record.m_size = static_cast<uint32_t>(readNumber(m_header + 20, 4));
    if (m_record.m_size > NET_CHANNEL_PAYLOAD_LIMIT)
    {
        return NC_LIMIT_EXCEEDED;
    }
    if (!valid(m_record))
    {
        return NC_PROTOCOL_ERROR;
    }
    if (m_record.m_size > m_capacity)
    {
        // 此时上一记录已consume且新正文尚未接纳，可先释放再扩容，峰值也不超硬界
        uint32_t capacity = 256;
        while (capacity < m_record.m_size)
        {
            capacity *= 2;
        }
        delete[] m_storage;
        m_storage = nullptr;
        m_capacity = 0;
        m_storage = new (std::nothrow) uint8_t[capacity];
        if (m_storage == nullptr)
        {
            return NC_NO_MEMORY;
        }
        m_capacity = capacity;
    }
    return NC_NEED_MORE;
}

int32_t NetChannel::feed(const uint8_t* data, uint32_t size, uint32_t& consumed)
{
    if (OverlapsCount(consumed, data, size)) { return NC_INVALID_ARGUMENT; }
    consumed = 0;
    if (size != 0 && (data == nullptr || reinterpret_cast<uintptr_t>(data) > UINTPTR_MAX - size))
    {
        return NC_INVALID_ARGUMENT;
    }
    if (m_state != NC_NEED_MORE)
    {
        return m_state;
    }
    if (m_headerBytes != NET_CHANNEL_HEADER_SIZE)
    {
        const uint32_t count = (std::min)(size, NET_CHANNEL_HEADER_SIZE - m_headerBytes);
        if (count != 0)
        {
            std::memcpy(m_header + m_headerBytes, data, count);
            m_headerBytes += count;
            consumed = count;
        }
        if (m_headerBytes != NET_CHANNEL_HEADER_SIZE)
        {
            return NC_NEED_MORE;
        }
        m_state = acceptHeader();
        if (m_state < 0)
        {
            return m_state;
        }
    }
    const uint32_t count = (std::min)(size - consumed, m_record.m_size - m_bodyBytes);
    if (count != 0)
    {
        std::memcpy(m_storage + m_bodyBytes, data + consumed, count);
        m_bodyBytes += count;
        consumed += count;
    }
    if (m_bodyBytes == m_record.m_size)
    {
        m_record.m_data = m_bodyBytes == 0 ? nullptr : m_storage;
        m_state = NC_READY;
    }
    return m_state;
}

int32_t NetChannel::peek(NetChannelRecord& record) const
{
    record = m_state == NC_READY ? m_record : NetChannelRecord();
    return m_state;
}

int32_t NetChannel::consume()
{
    if (m_state != NC_READY)
    {
        return m_state < 0 ? m_state : NC_INVALID_STATE;
    }
    m_record = NetChannelRecord();
    m_headerBytes = 0;
    m_bodyBytes = 0;
    m_state = NC_NEED_MORE;
    return NC_OK;
}

int32_t NetChannel::finish()
{
    if (m_state != NC_NEED_MORE)
    {
        return m_state;
    }
    if (m_headerBytes == 0)
    {
        return NC_OK;
    }
    m_state = NC_TRUNCATED;
    return m_state;
}

int32_t NetChannel::encode(const NetChannelRecord& record, uint8_t* output, uint32_t capacity, uint32_t& required)
{
    // 输出可以覆盖调用方的record结构；先固定元数据，正文仍不得与输出重叠
    if (OverlapsCount(required, &record, sizeof(record)) || OverlapsCount(required, record.m_data, record.m_size) ||
        OverlapsCount(required, output, capacity))
    {
        return NC_INVALID_ARGUMENT;
    }
    const NetChannelRecord input = record;
    required = 0;
    if (!valid(input) || (input.m_size != 0 && (input.m_data == nullptr ||
        reinterpret_cast<uintptr_t>(input.m_data) > UINTPTR_MAX - input.m_size)))
    {
        return NC_INVALID_ARGUMENT;
    }
    required = NET_CHANNEL_HEADER_SIZE + input.m_size;
    if (capacity < required)
    {
        return NC_BUFFER_TOO_SMALL;
    }
    const uintptr_t destination = reinterpret_cast<uintptr_t>(output);
    const uintptr_t source = reinterpret_cast<uintptr_t>(input.m_data);
    if (output == nullptr || destination > UINTPTR_MAX - required ||
        (input.m_size != 0 && source < destination + required && destination < source + input.m_size))
    {
        required = 0;
        return NC_INVALID_ARGUMENT;
    }
    std::memcpy(output, "NTW3", 4);
    output[4] = 3;
    output[5] = static_cast<uint8_t>(input.m_kind);
    output[6] = 0;
    output[7] = 0;
    writeNumber(output + 8, input.m_product, 4);
    writeNumber(output + 12, input.m_channel, 8);
    writeNumber(output + 20, input.m_size, 4);
    if (input.m_size != 0)
    {
        std::memcpy(output + NET_CHANNEL_HEADER_SIZE, input.m_data, input.m_size);
    }
    return NC_OK;
}

// 使用示例（单独编译运行时取消下列行注释）
// #include "NetChannelCredit.h"
// #include <cstdio>
//
// /** 演示在同一逻辑通道中发送和接收一条二进制记录，并在下层接纳后登记或返还额度
// @return 0 表示完成一次交付，1 表示配置、编码、解码或额度操作失败
// */
// int main()
// {
//     NetChannelCredit sender;
//     NetChannelCredit receiver;
//     const uint32_t window = 128U * 1024U;
//     if (sender.configure(window) != NC_OK || receiver.configure(window) != NC_OK)
//     {
//         return 1;
//     }
//     const uint8_t body[] = {'A', 0, 'B'};
//     NetChannelRecord outgoing = {NC_BYTES, 7, 42, body, sizeof(body)};
//     uint8_t wire[NET_CHANNEL_HEADER_SIZE + sizeof(body)];
//     uint32_t written = 0;
//     if (sender.available() < outgoing.m_size ||
//         NetChannel::encode(outgoing, wire, sizeof(wire), written) != NC_OK)
//     {
//         return 1;
//     }
//
//     // 本例以 wire 模拟已接纳整条记录的下层，实际接入在运输层接纳成功后扣账。
//     if (sender.sent(outgoing.m_size) != NC_OK)
//     {
//         return 1;
//     }
//     NetChannel decoder;
//     uint32_t offset = 0;
//     while (offset < written)
//     {
//         uint32_t consumed = 0;
//         const int32_t state = decoder.feed(wire + offset, written - offset, consumed);
//         if (state < 0 || consumed == 0)
//         {
//             return 1;
//         }
//         offset += consumed;
//         if (state == NC_READY)
//         {
//             NetChannelRecord incoming = {};
//             if (decoder.peek(incoming) != NC_READY || receiver.received(incoming.m_size) != NC_OK)
//             {
//                 return 1;
//             }
//             // 正文可含零字节。处理完正文后结束借用，只保留待返还的字节数。
//             const uint32_t credit = incoming.m_size;
//             for (uint32_t index = 0; index < credit; ++index)
//             {
//                 std::printf("%02X ", static_cast<unsigned>(incoming.m_data[index]));
//             }
//             std::printf("\n");
//             if (decoder.consume() != NC_OK)
//             {
//                 return 1;
//             }
//             // 实际接入在额度消息入管理队列后 consume，对端收到该消息后 grant。
//             if (receiver.consume(credit) != NC_OK || sender.grant(credit) != NC_OK)
//             {
//                 return 1;
//             }
//         }
//     }
//     return decoder.finish() == NC_OK && receiver.pending() == 0 &&
//         sender.status() == NC_OK && sender.available() == sender.capacity() ? 0 : 1;
// }
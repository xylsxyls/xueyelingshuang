#include "NetSenderLocalHubTest.h"
#include "NetInboxTestAccess.h"
#include "NetSender/NetSenderLocalState.h"
#include "NetSenderTestAccess.h"
#include <cstdio>
#include <cstring>

int NetSenderLocalHubTest::run()
{
    uint32_t checks = 0;
#define HUB_CHECK(value) { ++checks; if (!(value)) { std::printf("INBOX_FAIL line=%u %s\n", static_cast<uint32_t>(__LINE__), #value); return 1; } }
    NetSenderLocalInbox inbox;
    NetSenderLocalInboxPools pools;
    // 只在本满池用例中将未知OPEN物理池设为64槽，生产配置没有64条OPEN门槛。
    pools.m_open.m_classes[2].initialCapacity = 0;
    pools.m_open.m_classes[2].maximumCapacity = 64;
    HUB_CHECK(inbox.configurePools(pools) == NI_OK);
    (inbox.*netInboxTestMember(NetInboxTestStart()))(false);
    HUB_CHECK(inbox.configurePools(pools) == NI_INVALID_STATE);
    NetSenderLocalInput input;
    uint8_t payload[16384] = {};
    uint8_t bytes[16440] = {};
    /** 构造独立的输入模式；调用返回后立即覆盖借用缓冲，验证收件箱已取得所有权
    @param [in] target 目标随机代次首字节，OPEN为0
    @param [in] pid 模拟回调的实际发送者
    @param [in] kind NIP2种类
    @param [in] sequence 序号
    @param [in] size 密文字节数
    @return 编码是否成功
    */
    const auto emit = [&](uint8_t target, int32_t pid, uint32_t kind, uint64_t sequence, uint32_t size) -> bool
    {
        NetSenderLocalPacketView packet = {};
        packet.m_kind = kind;
        packet.m_packet = sequence;
        packet.m_source[0] = 77;
        packet.m_target[0] = target;
        packet.m_data = size == 0 ? nullptr : payload;
        packet.m_size = size;
        for (uint32_t index = 0; index < size; ++index)
        {
            payload[index] = static_cast<uint8_t>(target * 19U + sequence * 7U + index);
        }
        uint32_t written = 0;
        if (NetSenderLocalPacket::encode(packet, bytes, sizeof(bytes), written) != NI_OK)
        {
            return false;
        }
        inbox.receive(pid, reinterpret_cast<const char*>(bytes), written, static_cast<MessageType>(NET_SENDER_LOCAL_MESSAGE));
        std::memset(bytes, 0xE7, sizeof(bytes));
        return true;
    };
    uint8_t generation[16] = {};
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestAllow()))(nullptr, 101, 1, nullptr) == NI_INVALID_ARGUMENT &&
        (inbox.*netInboxTestMember(NetInboxTestAllow()))(generation, 101, 1, nullptr) == NI_INVALID_ARGUMENT);
    for (uint32_t peer = 1; peer <= 64; ++peer)
    {
        generation[0] = static_cast<uint8_t>(peer);
        HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestAllow()))(generation, 100 + peer, peer, nullptr) == NI_OK);
        for (uint64_t sequence = 1; sequence <= 4; ++sequence)
        {
            HUB_CHECK(emit(generation[0], 100 + peer, NET_LOCAL_DATA, sequence, 16384));
        }
        for (uint64_t sequence = 1; sequence <= 4; ++sequence)
        {
            HUB_CHECK(emit(generation[0], 100 + peer, NET_LOCAL_ACK, sequence, 0));
        }
        HUB_CHECK(emit(generation[0], 100 + peer, NET_LOCAL_END, 0, 0));
    }
    generation[0] = 65;
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestAllow()))(generation, 165, 65, nullptr) == NI_OK);
    for (uint32_t round = 0; round < 9; ++round)
    {
        for (uint32_t peer = 1; peer <= 64; ++peer)
        {
            HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_reason == NI_OK && input.m_peer == peer && input.m_pid == static_cast<int32_t>(100 + peer));
            NetSenderLocalPacketView packet = {};
            HUB_CHECK(NetSenderLocalPacket::parse(input.m_bytes.data(), static_cast<uint32_t>(input.m_bytes.size()), packet) == NI_OK);
            if (round < 4)
            {
                HUB_CHECK(packet.m_kind == NET_LOCAL_DATA && packet.m_packet == round + 1 && packet.m_size == 16384);
                for (uint32_t index = 0; index < packet.m_size; ++index)
                {
                    HUB_CHECK(packet.m_data[index] == static_cast<uint8_t>(peer * 19U + (round + 1) * 7U + index));
                }
            }
            else
            {
                HUB_CHECK(packet.m_kind == static_cast<uint32_t>(round < 8 ? NET_LOCAL_ACK : NET_LOCAL_END) &&
                    packet.m_packet == (round < 8 ? round - 3 : 0) && packet.m_size == 0);
            }
        }
    }
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_NEED_MORE && input.m_bytes.empty() && input.m_peer == 0);
    // 同一通道不经消费连续接纳128片，跨越旧4片门槛；两轮复用同一物理池。
    for (uint32_t repeat = 0; repeat < 2; ++repeat)
    {
        for (uint64_t sequence = 1; sequence <= 128; ++sequence)
        {
            HUB_CHECK(emit(1, 101, NET_LOCAL_DATA, sequence, 16384));
        }
        HUB_CHECK(emit(2, 102, NET_LOCAL_DATA, 1, 16));
        HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_peer == 1 && input.m_reason == NI_OK);
        HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_peer == 2 && input.m_reason == NI_OK);
        for (uint64_t sequence = 2; sequence <= 128; ++sequence)
        {
            HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_peer == 1 && input.m_reason == NI_OK);
            NetSenderLocalPacketView packet = {};
            HUB_CHECK(NetSenderLocalPacket::parse(input.m_bytes.data(), input.m_bytes.size(), packet) == NI_OK && packet.m_packet == sequence);
        }
        HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_NEED_MORE);
    }
    generation[0] = 1;
    (inbox.*netInboxTestMember(NetInboxTestRemove()))(generation);
    for (uint32_t index = 0; index < 100; ++index)
    {
        HUB_CHECK(emit(2, 999, NET_LOCAL_DATA, 1, 16));
    }
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_NEED_MORE);
    // 控制消息跨越旧16条门槛，仍与其它通道轮转；不将测试数量当成容量。
    for (uint32_t index = 0; index < 128; ++index)
    {
        HUB_CHECK(emit(2, 102, NET_LOCAL_ACK, index + 1, 0));
    }
    HUB_CHECK(emit(65, 165, NET_LOCAL_DATA, 1, 16));
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_peer == 2 && input.m_reason == NI_OK);
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_peer == 65 && input.m_reason == NI_OK);
    for (uint32_t index = 1; index < 128; ++index)
    {
        HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_peer == 2 && input.m_reason == NI_OK);
    }
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_NEED_MORE);
    // 删除就绪链中间、首尾节点均不能留下借用指针；旧目标也不再接纳。
    HUB_CHECK(emit(3, 103, NET_LOCAL_DATA, 1, 16) && emit(4, 104, NET_LOCAL_DATA, 1, 16) && emit(5, 105, NET_LOCAL_DATA, 1, 16));
    generation[0] = 4;
    (inbox.*netInboxTestMember(NetInboxTestRemove()))(generation);
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_peer == 3 && (inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_peer == 5);
    HUB_CHECK(emit(6, 106, NET_LOCAL_DATA, 1, 16) && emit(7, 107, NET_LOCAL_DATA, 1, 16) && emit(8, 108, NET_LOCAL_DATA, 1, 16));
    generation[0] = 6;
    (inbox.*netInboxTestMember(NetInboxTestRemove()))(generation);
    generation[0] = 8;
    (inbox.*netInboxTestMember(NetInboxTestRemove()))(generation);
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_peer == 7 && (inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_NEED_MORE);
    HUB_CHECK(emit(8, 108, NET_LOCAL_DATA, 1, 16) && (inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_NEED_MORE);
    // 未识别OPEN满载时，已建立通道仍在下一次轮转被取出，不被OPEN占满额度。
    (inbox.*netInboxTestMember(NetInboxTestStart()))(true);
    generation[0] = 1;
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestAllow()))(generation, 101, 1000, nullptr) == NI_OK);
    for (uint32_t index = 0; index < 65; ++index)
    {
        HUB_CHECK(emit(0, 101, NET_LOCAL_OPEN, 1, 16384));
    }
    HUB_CHECK(emit(1, 101, NET_LOCAL_DATA, 1, 16));
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_kind == NET_LOCAL_OPEN && input.m_peer == 0);
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_peer == 1000 && input.m_kind == NET_LOCAL_DATA);
    for (uint32_t index = 1; index < 64; ++index)
    {
        HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_kind == NET_LOCAL_OPEN && input.m_peer == 0);
    }
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_NEED_MORE);
    HUB_CHECK(emit(1, 101, NET_LOCAL_DATA, 2, 16384));
    inbox.stop();
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_NEED_MORE && emit(1, 101, NET_LOCAL_DATA, 3, 16384) && (inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_NEED_MORE);
    (inbox.*netInboxTestMember(NetInboxTestStart()))(false);
    HUB_CHECK(emit(0, 101, NET_LOCAL_OPEN, 1, 16) && emit(1, 101, NET_LOCAL_DATA, 1, 16) && (inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_NEED_MORE);
    // 四个OPEN先到，建立本地目标后收到DATA；目标切换不能让DATA越过OPEN尾片。
    (inbox.*netInboxTestMember(NetInboxTestStart()))(true);
    for (uint64_t sequence = 1; sequence <= 4; ++sequence)
    {
        HUB_CHECK(emit(0, 101, NET_LOCAL_OPEN, sequence, 16));
    }
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_kind == NET_LOCAL_OPEN);
    generation[0] = 1;
    uint8_t remote[16] = {77};
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestAllow()))(generation, 101, 2000, remote) == NI_OK);
    HUB_CHECK(emit(1, 101, NET_LOCAL_DATA, 5, 16));
    for (uint64_t sequence = 2; sequence <= 5; ++sequence)
    {
        HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_peer == 2000 && input.m_reason == NI_OK);
        NetSenderLocalPacketView packet = {};
        HUB_CHECK(NetSenderLocalPacket::parse(input.m_bytes.data(), static_cast<uint32_t>(input.m_bytes.size()), packet) == NI_OK &&
            packet.m_packet == sequence && packet.m_kind == static_cast<uint32_t>(sequence == 5 ? NET_LOCAL_DATA : NET_LOCAL_OPEN));
    }
    // 已登记后的OPEN也直接排入此通道；其它来源号不得消耗本通道窗口。
    HUB_CHECK(emit(0, 101, NET_LOCAL_OPEN, 6, 16) && emit(1, 101, NET_LOCAL_DATA, 7, 16));
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_peer == 2000 && input.m_kind == NET_LOCAL_OPEN);
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_READY && input.m_peer == 2000 && input.m_kind == NET_LOCAL_DATA);
    HUB_CHECK((inbox.*netInboxTestMember(NetInboxTestTake()))(input) == NI_NEED_MORE);
    (inbox.*netInboxTestMember(NetInboxTestRemove()))(nullptr);
    // 缩小实际池，验证租约出队后仍占用存储、失败只通知一次、控制池独立。
    for (uint32_t repeat = 0; repeat < 2; ++repeat)
    {
        NetSenderLocalInbox boundedInbox;
        NetSenderLocalInboxPools bounded;
        bounded.m_data.m_classes[1].initialCapacity = 0;
        bounded.m_data.m_classes[1].maximumCapacity = 1;
        HUB_CHECK(boundedInbox.configurePools(bounded) == NI_OK);
        NetSenderLocalInboxPools bad = bounded;
        bad.m_data.m_classes[1].initialCapacity = 2;
        HUB_CHECK(boundedInbox.configurePools(bad) == NI_INVALID_ARGUMENT);
        (boundedInbox.*netInboxTestMember(NetInboxTestStart()))(false);
        generation[0] = 1;
        HUB_CHECK((boundedInbox.*netInboxTestMember(NetInboxTestAllow()))(generation, 101, 1, nullptr) == NI_OK);
        generation[0] = 2;
        HUB_CHECK((boundedInbox.*netInboxTestMember(NetInboxTestAllow()))(generation, 102, 2, nullptr) == NI_OK);
        NetSenderLocalPacketView packet = {};
        packet.m_kind = NET_LOCAL_DATA; packet.m_packet = 1; packet.m_source[0] = 77;
        packet.m_target[0] = 1; packet.m_data = payload; packet.m_size = 16;
        uint32_t size = 0;
        HUB_CHECK(NetSenderLocalPacket::encode(packet, bytes, sizeof(bytes), size) == NI_OK);
        boundedInbox.receive(101, reinterpret_cast<const char*>(bytes), size, static_cast<MessageType>(NET_SENDER_LOCAL_MESSAGE));
        NetSenderLocalInput retained;
        HUB_CHECK((boundedInbox.*netInboxTestMember(NetInboxTestTake()))(retained) == NI_READY && retained.m_reason == NI_OK);
        boundedInbox.receive(101, reinterpret_cast<const char*>(bytes), size, static_cast<MessageType>(NET_SENDER_LOCAL_MESSAGE));
        packet.m_kind = NET_LOCAL_ACK; packet.m_target[0] = 2; packet.m_data = nullptr; packet.m_size = 0;
        HUB_CHECK(NetSenderLocalPacket::encode(packet, bytes, sizeof(bytes), size) == NI_OK);
        boundedInbox.receive(102, reinterpret_cast<const char*>(bytes), size, static_cast<MessageType>(NET_SENDER_LOCAL_MESSAGE));
        NetSenderLocalInput event;
        bool failure = false, control = false;
        for (uint32_t index = 0; index < 2; ++index)
        {
            HUB_CHECK((boundedInbox.*netInboxTestMember(NetInboxTestTake()))(event) == NI_READY);
            if (event.m_peer == 1) { failure = event.m_reason == NI_LIMIT_EXCEEDED && event.m_bytes.empty(); }
            if (event.m_peer == 2) { control = event.m_reason == NI_OK && event.m_kind == NET_LOCAL_ACK; }
        }
        HUB_CHECK(failure && control && (boundedInbox.*netInboxTestMember(NetInboxTestTake()))(event) == NI_NEED_MORE);
        retained.m_bytes.clear();
        packet.m_kind = NET_LOCAL_DATA; packet.m_data = payload; packet.m_size = 16;
        HUB_CHECK(NetSenderLocalPacket::encode(packet, bytes, sizeof(bytes), size) == NI_OK);
        boundedInbox.receive(102, reinterpret_cast<const char*>(bytes), size, static_cast<MessageType>(NET_SENDER_LOCAL_MESSAGE));
        HUB_CHECK((boundedInbox.*netInboxTestMember(NetInboxTestTake()))(event) == NI_READY && event.m_peer == 2 && event.m_reason == NI_OK);
        event.m_bytes.clear();
        boundedInbox.stop();
        HUB_CHECK(boundedInbox.configurePools(bounded) == NI_INVALID_STATE);
    }
    std::printf("NetSenderTestClient<NetSender, NetSenderClient> inbox capacity, FIFO and isolation checks=%u PASS\n", checks);
    return 0;
#undef HUB_CHECK
}
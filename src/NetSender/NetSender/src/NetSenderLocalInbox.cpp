#include "NetSenderMacro.h"
#include "NetSenderLocalState.h"
#include <algorithm>
#include <cstring>
#include <utility>

// NET_SENDER_INBOX_UNIT_BEGIN
// #include "NetSenderLocalHubTest.h"
// #include <cstdio>
// #include <cstring>
// #if defined(_MSC_VER) && defined(_DEBUG)
// #include <crtdbg.h>
// #endif
//
// int NetSenderLocalHubTest::run()
// {
//     uint32_t checks = 0;
// #define HUB_CHECK(value) { ++checks; if (!(value)) { std::printf("INBOX_FAIL line=%u %s\n", static_cast<uint32_t>(__LINE__), #value); return 1; } }
//     NetSenderLocalInbox inbox;
//     inbox.start(false);
//     NetSenderLocalInput input;
//     uint8_t payload[16384] = {};
//     uint8_t bytes[16440] = {};
//     /** 构造独立的输入模式；调用返回后立即覆盖借用缓冲，验证收件箱已取得所有权
//     @param [in] target 目标随机代次首字节，OPEN为0
//     @param [in] pid 模拟回调的实际发送者
//     @param [in] kind NIP2种类
//     @param [in] sequence 序号
//     @param [in] size 密文字节数
//     @return 编码是否成功
//     */
//     const auto emit = [&](uint8_t target, int32_t pid, uint32_t kind, uint64_t sequence, uint32_t size) -> bool
//     {
//         NetSenderLocalPacketView packet = {};
//         packet.m_kind = kind;
//         packet.m_packet = sequence;
//         packet.m_source[0] = 77;
//         packet.m_target[0] = target;
//         packet.m_data = size == 0 ? nullptr : payload;
//         packet.m_size = size;
//         for (uint32_t index = 0; index < size; ++index)
//         {
//             payload[index] = static_cast<uint8_t>(target * 19U + sequence * 7U + index);
//         }
//         uint32_t written = 0;
//         if (NetSenderLocalPacket::encode(packet, bytes, sizeof(bytes), written) != NI_OK)
//         {
//             return false;
//         }
//         inbox.receive(pid, reinterpret_cast<const char*>(bytes), written, static_cast<MessageType>(NET_SENDER_LOCAL_MESSAGE));
//         std::memset(bytes, 0xE7, sizeof(bytes));
//         return true;
//     };
//     uint8_t generation[16] = {};
//     HUB_CHECK(inbox.allowGeneration(nullptr, 101, 1) == NI_INVALID_ARGUMENT &&
//         inbox.allowGeneration(generation, 101, 1) == NI_INVALID_ARGUMENT);
//     for (uint32_t peer = 1; peer <= 64; ++peer)
//     {
//         generation[0] = static_cast<uint8_t>(peer);
//         HUB_CHECK(inbox.allowGeneration(generation, 100 + peer, peer) == NI_OK);
//         for (uint64_t sequence = 1; sequence <= 4; ++sequence)
//         {
//             HUB_CHECK(emit(generation[0], 100 + peer, NET_LOCAL_DATA, sequence, 16384));
//         }
//         for (uint64_t sequence = 1; sequence <= 4; ++sequence)
//         {
//             HUB_CHECK(emit(generation[0], 100 + peer, NET_LOCAL_ACK, sequence, 0));
//         }
//         HUB_CHECK(emit(generation[0], 100 + peer, NET_LOCAL_END, 0, 0));
//     }
//     generation[0] = 65;
//     HUB_CHECK(inbox.allowGeneration(generation, 165, 65) == NI_INVALID_STATE);
//     for (uint32_t round = 0; round < 9; ++round)
//     {
//         for (uint32_t peer = 1; peer <= 64; ++peer)
//         {
//             HUB_CHECK(inbox.take(input) == NI_READY && input.m_reason == NI_OK && input.m_peer == peer && input.m_pid == static_cast<int32_t>(100 + peer));
//             NetSenderLocalPacketView packet = {};
//             HUB_CHECK(NetSenderLocalPacket::parse(input.m_bytes.data(), static_cast<uint32_t>(input.m_bytes.size()), packet) == NI_OK);
//             if (round < 4)
//             {
//                 HUB_CHECK(packet.m_kind == NET_LOCAL_DATA && packet.m_packet == round + 1 && packet.m_size == 16384);
//                 for (uint32_t index = 0; index < packet.m_size; ++index)
//                 {
//                     HUB_CHECK(packet.m_data[index] == static_cast<uint8_t>(peer * 19U + (round + 1) * 7U + index));
//                 }
//             }
//             else
//             {
//                 HUB_CHECK(packet.m_kind == static_cast<uint32_t>(round < 8 ? NET_LOCAL_ACK : NET_LOCAL_END) &&
//                     packet.m_packet == (round < 8 ? round - 3 : 0) && packet.m_size == 0);
//             }
//         }
//     }
//     HUB_CHECK(inbox.take(input) == NI_NEED_MORE && input.m_bytes.empty() && input.m_peer == 0);
//     // 单通道越过四包数据窗口，不影响其它通道；失败通知一次且不依赖正文分配。
//     for (uint64_t sequence = 1; sequence <= 5; ++sequence)
//     {
//         HUB_CHECK(emit(1, 101, NET_LOCAL_DATA, sequence, 16384));
//     }
//     HUB_CHECK(emit(2, 102, NET_LOCAL_DATA, 1, 16));
//     HUB_CHECK(inbox.take(input) == NI_READY && input.m_peer == 1 && input.m_reason == NI_LIMIT_EXCEEDED && input.m_bytes.empty());
//     HUB_CHECK(inbox.take(input) == NI_READY && input.m_peer == 2 && input.m_reason == NI_OK);
//     HUB_CHECK(emit(1, 101, NET_LOCAL_DATA, 1, 16) && inbox.take(input) == NI_NEED_MORE);
//     generation[0] = 1;
//     inbox.removeGeneration(generation);
//     generation[0] = 65;
//     HUB_CHECK(inbox.allowGeneration(generation, 165, 65) == NI_OK);
//     for (uint32_t index = 0; index < 100; ++index)
//     {
//         HUB_CHECK(emit(2, 999, NET_LOCAL_DATA, 1, 16));
//     }
//     HUB_CHECK(inbox.take(input) == NI_NEED_MORE);
//     // 独立控制槽位也有上限，异常控制洪泛不能把全箱关闭。
//     for (uint32_t index = 0; index < 17; ++index)
//     {
//         HUB_CHECK(emit(2, 102, NET_LOCAL_ACK, 1, 0));
//     }
//     HUB_CHECK(emit(65, 165, NET_LOCAL_DATA, 1, 16));
//     HUB_CHECK(inbox.take(input) == NI_READY && input.m_peer == 2 && input.m_reason == NI_LIMIT_EXCEEDED);
//     HUB_CHECK(inbox.take(input) == NI_READY && input.m_peer == 65 && input.m_reason == NI_OK && inbox.take(input) == NI_NEED_MORE);
//     // 删除就绪链中间、首尾节点均不能留下借用指针；旧目标也不再接纳。
//     HUB_CHECK(emit(3, 103, NET_LOCAL_DATA, 1, 16) && emit(4, 104, NET_LOCAL_DATA, 1, 16) && emit(5, 105, NET_LOCAL_DATA, 1, 16));
//     generation[0] = 4;
//     inbox.removeGeneration(generation);
//     HUB_CHECK(inbox.take(input) == NI_READY && input.m_peer == 3 && inbox.take(input) == NI_READY && input.m_peer == 5);
//     HUB_CHECK(emit(6, 106, NET_LOCAL_DATA, 1, 16) && emit(7, 107, NET_LOCAL_DATA, 1, 16) && emit(8, 108, NET_LOCAL_DATA, 1, 16));
//     generation[0] = 6;
//     inbox.removeGeneration(generation);
//     generation[0] = 8;
//     inbox.removeGeneration(generation);
//     HUB_CHECK(inbox.take(input) == NI_READY && input.m_peer == 7 && inbox.take(input) == NI_NEED_MORE);
//     HUB_CHECK(emit(8, 108, NET_LOCAL_DATA, 1, 16) && inbox.take(input) == NI_NEED_MORE);
//     // 未识别OPEN满载时，已建立通道仍在下一次轮转被取出，不被OPEN占满额度。
//     inbox.start(true);
//     generation[0] = 1;
//     HUB_CHECK(inbox.allowGeneration(generation, 101, 1000) == NI_OK);
//     for (uint32_t index = 0; index < 65; ++index)
//     {
//         HUB_CHECK(emit(0, 101, NET_LOCAL_OPEN, 1, 16384));
//     }
//     HUB_CHECK(emit(1, 101, NET_LOCAL_DATA, 1, 16));
//     HUB_CHECK(inbox.take(input) == NI_READY && input.m_kind == NET_LOCAL_OPEN && input.m_peer == 0);
//     HUB_CHECK(inbox.take(input) == NI_READY && input.m_peer == 1000 && input.m_kind == NET_LOCAL_DATA);
//     for (uint32_t index = 1; index < 64; ++index)
//     {
//         HUB_CHECK(inbox.take(input) == NI_READY && input.m_kind == NET_LOCAL_OPEN && input.m_peer == 0);
//     }
//     HUB_CHECK(inbox.take(input) == NI_NEED_MORE);
//     HUB_CHECK(emit(1, 101, NET_LOCAL_DATA, 2, 16384));
//     inbox.stop();
//     HUB_CHECK(inbox.take(input) == NI_NEED_MORE && emit(1, 101, NET_LOCAL_DATA, 3, 16384) && inbox.take(input) == NI_NEED_MORE);
//     inbox.start(false);
//     HUB_CHECK(emit(0, 101, NET_LOCAL_OPEN, 1, 16) && emit(1, 101, NET_LOCAL_DATA, 1, 16) && inbox.take(input) == NI_NEED_MORE);
//     // 四个OPEN先到，建立本地目标后收到DATA；目标切换不能让DATA越过OPEN尾片。
//     inbox.start(true);
//     for (uint64_t sequence = 1; sequence <= 4; ++sequence)
//     {
//         HUB_CHECK(emit(0, 101, NET_LOCAL_OPEN, sequence, 16));
//     }
//     HUB_CHECK(inbox.take(input) == NI_READY && input.m_kind == NET_LOCAL_OPEN);
//     generation[0] = 1;
//     uint8_t remote[16] = {77};
//     HUB_CHECK(inbox.allowGeneration(generation, 101, 2000, remote) == NI_OK);
//     HUB_CHECK(emit(1, 101, NET_LOCAL_DATA, 5, 16));
//     for (uint64_t sequence = 2; sequence <= 5; ++sequence)
//     {
//         HUB_CHECK(inbox.take(input) == NI_READY && input.m_peer == 2000 && input.m_reason == NI_OK);
//         NetSenderLocalPacketView packet = {};
//         HUB_CHECK(NetSenderLocalPacket::parse(input.m_bytes.data(), static_cast<uint32_t>(input.m_bytes.size()), packet) == NI_OK &&
//             packet.m_packet == sequence && packet.m_kind == static_cast<uint32_t>(sequence == 5 ? NET_LOCAL_DATA : NET_LOCAL_OPEN));
//     }
//     // 已登记后的OPEN也直接排入此通道；其它来源号不得消耗本通道窗口。
//     HUB_CHECK(emit(0, 101, NET_LOCAL_OPEN, 6, 16) && emit(1, 101, NET_LOCAL_DATA, 7, 16));
//     HUB_CHECK(inbox.take(input) == NI_READY && input.m_peer == 2000 && input.m_kind == NET_LOCAL_OPEN);
//     HUB_CHECK(inbox.take(input) == NI_READY && input.m_peer == 2000 && input.m_kind == NET_LOCAL_DATA);
//     HUB_CHECK(inbox.take(input) == NI_NEED_MORE);
//     inbox.removeGeneration(nullptr);
// #if defined(_MSC_VER) && defined(_DEBUG)
//     // 每次只拒绝一次普通分配；回调内和OPEN归并阶段都须保持其它目标可用。
//     static int32_t s_failure = -1;
//     static bool s_denied = false;
//     /** 拒绝指定次普通分配，不影响CRT管理块
//     @param [in] operation CRT操作
//     @param [in] block CRT块类型
//     @return 是否允许分配
//     */
//     const _CRT_ALLOC_HOOK hook = [](int operation, void*, size_t, int block, long, const unsigned char*, int) -> int
//     {
//         if (operation == _HOOK_ALLOC && block != _CRT_BLOCK && s_failure >= 0 && s_failure-- == 0)
//         {
//             s_denied = true;
//             return 0;
//         }
//         return 1;
//     };
//     // 单包接纳的每个分配点：失败事件无分配交付，停止及重新启用均可用。
//     uint32_t receiveFailures = 0;
//     bool receiveComplete = false;
//     for (int32_t failure = 0; failure < 16 && !receiveComplete; ++failure)
//     {
//         inbox.start(false);
//         generation[0] = 1;
//         HUB_CHECK(inbox.allowGeneration(generation, 101, 1) == NI_OK);
//         generation[0] = 2;
//         HUB_CHECK(inbox.allowGeneration(generation, 102, 2) == NI_OK);
//         const _CRT_ALLOC_HOOK previousHook = _CrtSetAllocHook(hook);
//         s_failure = failure;
//         s_denied = false;
//         const bool encoded = emit(1, 101, NET_LOCAL_DATA, 1, 16384);
//         s_failure = -1;
//         _CrtSetAllocHook(previousHook);
//         HUB_CHECK(encoded && inbox.take(input) == NI_READY && input.m_peer == 1);
//         if (s_denied)
//         {
//             ++receiveFailures;
//             HUB_CHECK(input.m_reason == NI_NO_MEMORY && input.m_bytes.empty());
//         }
//         else
//         {
//             HUB_CHECK(input.m_reason == NI_OK && input.m_bytes.size() == 16440);
//             receiveComplete = true;
//         }
//         HUB_CHECK(emit(2, 102, NET_LOCAL_DATA, 1, 16384) && inbox.take(input) == NI_READY && input.m_peer == 2 && input.m_reason == NI_OK);
//         HUB_CHECK(inbox.take(input) == NI_NEED_MORE);
//     }
//     HUB_CHECK(receiveComplete && receiveFailures >= 1);
//     uint32_t registrationFailures = 0;
//     uint32_t adoptionFailures = 0;
//     bool adoptionComplete = false;
//     for (int32_t failure = 0; failure < 24 && !adoptionComplete; ++failure)
//     {
//         inbox.start(true);
//         for (uint64_t sequence = 1; sequence <= 4; ++sequence)
//         {
//             HUB_CHECK(emit(0, 101, NET_LOCAL_OPEN, sequence, 16384));
//         }
//         HUB_CHECK(inbox.take(input) == NI_READY && input.m_kind == NET_LOCAL_OPEN);
//         generation[0] = 1;
//         const _CRT_ALLOC_HOOK previousHook = _CrtSetAllocHook(hook);
//         s_failure = failure;
//         s_denied = false;
//         const int32_t accepted = inbox.allowGeneration(generation, 101, 1, remote);
//         s_failure = -1;
//         _CrtSetAllocHook(previousHook);
//         if (accepted == NI_NO_MEMORY)
//         {
//             ++registrationFailures;
//             HUB_CHECK(s_denied);
//             // 登记未提交时，原OPEN仍由未识别入口拥有，没有失去消息或错误路由到新号。
//             for (uint32_t index = 0; index < 3; ++index)
//             {
//                 HUB_CHECK(inbox.take(input) == NI_READY && input.m_peer == 0 && input.m_kind == NET_LOCAL_OPEN);
//             }
//         }
//         else
//         {
//             HUB_CHECK(accepted == NI_OK);
//             if (s_denied)
//             {
//                 ++adoptionFailures;
//                 HUB_CHECK(inbox.take(input) == NI_READY && input.m_peer == 1 && input.m_reason == NI_NO_MEMORY);
//             }
//             else
//             {
//                 for (uint32_t index = 0; index < 3; ++index)
//                 {
//                     HUB_CHECK(inbox.take(input) == NI_READY && input.m_peer == 1 && input.m_kind == NET_LOCAL_OPEN && input.m_reason == NI_OK);
//                 }
//                 adoptionComplete = true;
//             }
//         }
//         HUB_CHECK(inbox.take(input) == NI_NEED_MORE);
//     }
//     HUB_CHECK(adoptionComplete && registrationFailures >= 1 && adoptionFailures >= 1);
//     _CrtMemState before = {};
//     _CrtMemState after = {};
//     _CrtMemCheckpoint(&before);
//     // 不提前排空队列，直接销毁有已登记数据、控制和未识别OPEN的实例。
//     for (uint32_t round = 0; round < 128; ++round)
//     {
//         NetSenderLocalInbox abandoned;
//         abandoned.start(true);
//         generation[0] = 1;
//         HUB_CHECK(abandoned.allowGeneration(generation, 101, 1, remote) == NI_OK);
//         NetSenderLocalPacketView packet = {};
//         packet.m_kind = NET_LOCAL_DATA;
//         packet.m_packet = 1;
//         packet.m_source[0] = 77;
//         packet.m_target[0] = 1;
//         packet.m_data = payload;
//         packet.m_size = sizeof(payload);
//         uint32_t written = 0;
//         HUB_CHECK(NetSenderLocalPacket::encode(packet, bytes, sizeof(bytes), written) == NI_OK);
//         abandoned.receive(101, reinterpret_cast<const char*>(bytes), written, static_cast<MessageType>(NET_SENDER_LOCAL_MESSAGE));
//         packet.m_kind = NET_LOCAL_END;
//         packet.m_packet = 0;
//         packet.m_data = nullptr;
//         packet.m_size = 0;
//         HUB_CHECK(NetSenderLocalPacket::encode(packet, bytes, sizeof(bytes), written) == NI_OK);
//         abandoned.receive(101, reinterpret_cast<const char*>(bytes), written, static_cast<MessageType>(NET_SENDER_LOCAL_MESSAGE));
//         packet.m_kind = NET_LOCAL_OPEN;
//         packet.m_packet = 1;
//         packet.m_target[0] = 0;
//         packet.m_data = payload;
//         packet.m_size = sizeof(payload);
//         HUB_CHECK(NetSenderLocalPacket::encode(packet, bytes, sizeof(bytes), written) == NI_OK);
//         abandoned.receive(102, reinterpret_cast<const char*>(bytes), written, static_cast<MessageType>(NET_SENDER_LOCAL_MESSAGE));
//     }
//     _CrtMemCheckpoint(&after);
//     HUB_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
// #endif
//     std::printf("NetSender inbox capacity, FIFO and isolation checks=%u PASS\n", checks);
//     return 0;
// #undef HUB_CHECK
// }
//
// int main()
// {
//     return NetSenderLocalHubTest::run();
// }
// NET_SENDER_INBOX_UNIT_END
#include "NetSubmissionTestCases.h"
#include "NetGatewayTest.h"
#include "NetInterfaceTestAccess.h"
#include <cstring>
#include <stdexcept>
#include <cstdio>
#include <thread>
#include <vector>

#include <chrono>
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

/** 运行从正式库迁出的完整提交边界及吞吐用例
@param [in] benchmark true运行原吞吐分支，false运行完整断言
@return 0通过，非0为原始检查失败
*/
static int NetSubmissionRun(bool benchmark)
{
    if (benchmark)
    {
        NetSenderSubmission queue;
        if (queue.start(1, 101) != NI_OK || queue.ready(1, 128, 4096) != NI_OK)
        {
            return 1;
        }
        uint8_t bytes[128] = {};
        std::memset(bytes, 0x6a, sizeof(bytes));
        NetInterfaceFrame frame = {};
        frame.m_kind = NI_DATA;
        frame.m_productId = 101;
        frame.m_requestId = 1;
        frame.m_payload = bytes;
        frame.m_payloadSize = sizeof(bytes);
        NetSenderSubmissionCommand command = {}, metadata = {};
        const uint32_t rounds = 20000;
        uint64_t received = 0;
        const auto started = std::chrono::steady_clock::now();
        for (uint32_t round = 0; round < rounds; ++round)
        {
            uint64_t ids[8] = {};
            for (uint32_t index = 0; index < 8; ++index)
            {
                if (queue.begin(101, 77, 5000, ids[index]) != NI_OK ||
                    queue.send(ids[index], bytes, sizeof(bytes)) != NI_OK || queue.data(frame) != NI_OK)
                {
                    return 2;
                }
            }
            if (queue.beginPoll() != NI_OK)
            {
                return 3;
            }
            for (uint32_t index = 0; index < 8; ++index)
            {
                if (queue.next(command) != NI_READY || command.m_kind != NET_SUBMIT_BEGIN || command.m_id != ids[index])
                {
                    return 4;
                }
                queue.finish(NI_OK);
                if (queue.next(command) != NI_READY || command.m_kind != NET_SUBMIT_SEND || command.m_id != ids[index] ||
                    command.m_size != sizeof(bytes) || std::memcmp(command.m_bytes, bytes, sizeof(bytes)) != 0)
                {
                    return 5;
                }
                queue.finish(NI_OK);
                if (!queue.complete(1, ids[index], metadata) || queue.next(command) != NI_READY || command.m_kind != NET_SUBMIT_DATA ||
                    command.m_frame.m_payloadSize != sizeof(bytes) || std::memcmp(command.m_frame.m_payload, bytes, sizeof(bytes)) != 0)
                {
                    return 6;
                }
                queue.finish(NI_OK);
                received += 2;
            }
            if (!queue.idle() || queue.queuedBytes() != 0)
            {
                return 7;
            }
        }
        const int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started).count();
        if (received != static_cast<uint64_t>(rounds) * 16)
        {
            return 8;
        }
        std::printf("SUBMISSION_BENCHMARK messages=%llu microseconds=%lld PASS\n",
            static_cast<unsigned long long>(received), static_cast<long long>(elapsed));
        return 0;
    }
    uint32_t checks = 0;
#define SUBMIT_CHECK(value) { netTestCheckpoint(); ++checks; if (!(value)) { std::printf("FAIL line=%u %s\n", static_cast<uint32_t>(__LINE__), #value); return 1; } }
    NetSenderSubmission queue;
    NetSenderSubmissionPools pools;
    pools.m_requests.initialCapacity = 3;
    pools.m_requests.maximumCapacity = 3;
    pools.m_frames.initialCapacity = 64;
    pools.m_frames.maximumCapacity = 64;
    pools.m_controls.initialCapacity = 16;
    pools.m_controls.maximumCapacity = 16;
    SUBMIT_CHECK(queue.configurePools(pools) == NI_OK);
    uint64_t low = 0, high = 0, cancelled = 0;
    SUBMIT_CHECK(queue.start(1, 101) == NI_OK && queue.ready(1, 3, 65536) == NI_OK);
    SUBMIT_CHECK(queue.begin(0, 700, 1000, low) == NI_INVALID_ARGUMENT && low == 0);
    SUBMIT_CHECK(queue.begin(101, 700, 10000, low) == NI_OK);
    SUBMIT_CHECK(queue.begin(101, 700, 10000, high) == NI_OK && high > low);
    SUBMIT_CHECK(queue.begin(101, 700, 10000, cancelled) == NI_OK);
    uint64_t rejected = 1;
    SUBMIT_CHECK(queue.begin(101, 700, 1000, rejected) == NI_LIMIT_EXCEEDED && rejected == 0);
    uint8_t byte = 7;
    SUBMIT_CHECK(queue.send(high, &byte, 1) == NI_OK);
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_DATA; frame.m_productId = 101; frame.m_requestId = 7001; frame.m_payload = &byte; frame.m_payloadSize = 1;
    SUBMIT_CHECK(queue.data(frame) == NI_OK);
    SUBMIT_CHECK(queue.send(low, &byte, 1) == NI_OK && queue.send(low, &byte, 1) == NI_INVALID_STATE);
    byte = 99;
    SUBMIT_CHECK(queue.cancel(cancelled) == NI_OK && queue.cancel(cancelled) == NI_NOT_FOUND);
    NetSenderSubmissionCommand command = {}, terminal = {};
    SUBMIT_CHECK(queue.beginPoll() == NI_OK && queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_TERMINAL && command.m_id == cancelled);
    SUBMIT_CHECK(queue.complete(1, cancelled, terminal)); queue.finish(NI_OK);
    SUBMIT_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_BEGIN && command.m_id == high); queue.finish(NI_OK);
    SUBMIT_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND && command.m_id == high && command.m_bytes[0] == 7);
    queue.finish(NI_LIMIT_EXCEEDED);
    SUBMIT_CHECK(queue.next(command) == NI_NEED_MORE);
    SUBMIT_CHECK(queue.beginPoll() == NI_OK && queue.next(command) == NI_READY && command.m_id == high);
    queue.finish(NI_OK);
    SUBMIT_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_DATA && command.m_frame.m_payload[0] == 7); queue.finish(NI_OK);
    SUBMIT_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_BEGIN && command.m_id == low); queue.finish(NI_OK);
    SUBMIT_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND && command.m_id == low && command.m_bytes[0] == 7);
    SUBMIT_CHECK(queue.complete(1, low, terminal) && command.m_bytes[0] == 7); queue.finish(NI_OK);
    SUBMIT_CHECK(queue.complete(1, high, terminal) && queue.idle() && queue.queuedBytes() == 0);
    // 大于内联长度，明确验证帧对象池的64个实际槽位；短帧的入口池另测。
    uint8_t poolBody[65] = {};
    frame.m_payload = poolBody;
    frame.m_payloadSize = sizeof(poolBody);
    for (uint32_t index = 0; index < 64; ++index) { SUBMIT_CHECK(queue.data(frame) == NI_OK); }
    SUBMIT_CHECK(queue.data(frame) == NI_LIMIT_EXCEEDED);
    frame.m_kind = NI_DATA_ACK;
    frame.m_payload = &byte;
    frame.m_payloadSize = 1;
    for (uint32_t index = 0; index < 16; ++index) { SUBMIT_CHECK(queue.data(frame) == NI_OK); }
    SUBMIT_CHECK(queue.data(frame) == NI_LIMIT_EXCEEDED);
    queue.close(0); queue.disconnected(1);
    SUBMIT_CHECK(queue.idle() && queue.queuedBytes() == 0);
    SUBMIT_CHECK(queue.start(2, 101) == NI_OK && queue.ready(2, 3, 65536) == NI_OK);
    SUBMIT_CHECK(queue.begin(101, 700, 10000, low) == NI_OK);
    uint64_t ticket = 0;
    SUBMIT_CHECK(queue.sendSynchronous(low, &byte, 1, ticket) == NI_OK);
    SUBMIT_CHECK(queue.beginPoll() == NI_OK && queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_BEGIN); queue.finish(NI_OK);
    SUBMIT_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND); queue.finish(NI_OK);
    queue.advanceSynchronous(NI_READY);
    SUBMIT_CHECK(queue.synchronousResult(ticket, false) == NI_READY && queue.next(command) == NI_NEED_MORE);
    queue.advanceSynchronous(NI_OK);
    SUBMIT_CHECK(queue.synchronousResult(ticket, true) == NI_OK && queue.complete(2, low, terminal) && queue.idle());
    NetSenderSubmission bytes;
    pools = NetSenderSubmissionPools();
    pools.m_bodies.m_classes[4].initialCapacity = 2;
    pools.m_bodies.m_classes[4].maximumCapacity = 2;
    SUBMIT_CHECK(bytes.configurePools(pools) == NI_OK);
    SUBMIT_CHECK(bytes.start(1, 101) == NI_OK && bytes.ready(1, 4, 65536) == NI_OK);
    std::vector<uint8_t> big(1048560, 7);
    uint64_t ids[3] = {};
    for (uint32_t index = 0; index < 3; ++index) { SUBMIT_CHECK(bytes.begin(101, 700, 10000, ids[index]) == NI_OK); }
    SUBMIT_CHECK(bytes.send(ids[0], big.data(), static_cast<uint32_t>(big.size())) == NI_OK);
    SUBMIT_CHECK(bytes.send(ids[1], big.data(), static_cast<uint32_t>(big.size())) == NI_OK);
    SUBMIT_CHECK(bytes.send(ids[2], big.data(), static_cast<uint32_t>(big.size())) == NI_LIMIT_EXCEEDED);
    SUBMIT_CHECK(bytes.cancel(ids[0]) == NI_OK && bytes.beginPoll() == NI_OK);
    SUBMIT_CHECK(bytes.next(command) == NI_READY && command.m_kind == NET_SUBMIT_TERMINAL && bytes.complete(1, ids[0], terminal)); bytes.finish(NI_OK);
    SUBMIT_CHECK(bytes.send(ids[2], big.data(), static_cast<uint32_t>(big.size())) == NI_OK);
    bytes.close(0); bytes.disconnected(1);
    while (bytes.takeClosed(terminal)) {}
    SUBMIT_CHECK(bytes.idle() && bytes.queuedBytes() == 0);

    NetSenderSubmission receipts;
    pools = NetSenderSubmissionPools();
    pools.m_receipts.initialCapacity = 128;
    pools.m_receipts.maximumCapacity = 128;
    SUBMIT_CHECK(receipts.configurePools(pools) == NI_OK);
    SUBMIT_CHECK(receipts.start(1, 101) == NI_OK && receipts.ready(1, 128, 65536) == NI_OK);
    uint64_t saved[128] = {};
    frame.m_kind = NI_DATA;
    for (uint32_t index = 0; index < 128; ++index)
    {
        SUBMIT_CHECK(receipts.dataSynchronous(frame, saved[index]) == NI_OK);
        SUBMIT_CHECK(receipts.beginPoll() == NI_OK && receipts.next(command) == NI_READY);
        receipts.finish(NI_OK);
        receipts.advanceSynchronous(NI_OK);
        SUBMIT_CHECK(receipts.synchronousResult(saved[index], false) == NI_OK && receipts.queuedBytes() == 0);
    }
    SUBMIT_CHECK(receipts.dataSynchronous(frame, ticket) == NI_LIMIT_EXCEEDED && ticket == 0);
    SUBMIT_CHECK(receipts.start(2, 101) == NI_INVALID_STATE);
    // 自动恢复允许保留已完成回执；慢等待者不能阻止其他生产者继续连接。
    SUBMIT_CHECK(receipts.ready(2, 128, 65536) == NI_OK);
    SUBMIT_CHECK(receipts.synchronousResult(saved[127], false) == NI_OK);
    for (uint32_t index = 128; index != 0; --index)
    {
        SUBMIT_CHECK(receipts.synchronousResult(saved[index - 1], true) == NI_OK);
        SUBMIT_CHECK(receipts.synchronousResult(saved[index - 1], true) == NI_NOT_FOUND);
    }
    SUBMIT_CHECK(receipts.beginPoll() == NI_OK);
    SUBMIT_CHECK(receipts.idle() && receipts.dataSynchronous(frame, ticket) == NI_OK && ticket > saved[127]);
    // 先取得DATA借用，再触发非法代次切换，才能验证断线不会提前释放借用正文。
    SUBMIT_CHECK(receipts.beginPoll() == NI_OK && receipts.next(command) == NI_READY && command.m_kind == NET_SUBMIT_DATA);
    SUBMIT_CHECK(receipts.ready(3, 128, 65536) == NI_PROTOCOL_ERROR);
    uint64_t retainedGeneration = 0;
    bool retainedReady = false;
    receipts.state(retainedGeneration, retainedReady);
    SUBMIT_CHECK(retainedGeneration == 2 && retainedReady);
    // 提交表拒绝切代不代替会话层关连接；真实握手失败由Dispatch显式close，再报告disconnected。
    receipts.close(NI_PROTOCOL_ERROR);
    receipts.disconnected(2);
    SUBMIT_CHECK(command.m_frame.m_payloadSize == 1 && command.m_frame.m_payload[0] == 99);
    SUBMIT_CHECK(receipts.synchronousResult(ticket, true) == NI_INVALID_STATE);
    receipts.finish(NI_OK);
    SUBMIT_CHECK(receipts.idle() && receipts.queuedBytes() == 0);
    SUBMIT_CHECK(receipts.next(command) == NI_READY && command.m_kind == NET_SUBMIT_CLOSE && command.m_status == NI_PROTOCOL_ERROR);
    receipts.finish(NI_OK);
    SUBMIT_CHECK(receipts.next(command) == NI_NEED_MORE);

    NetSenderSubmission cancellation;
    SUBMIT_CHECK(cancellation.start(1, 101) == NI_OK && cancellation.ready(1, 128, 4096) == NI_OK);
    SUBMIT_CHECK(cancellation.begin(101, 0, 5000, low) == NI_OK);
    SUBMIT_CHECK(cancellation.send(low, &byte, 1) == NI_OK && cancellation.data(frame) == NI_OK);
    SUBMIT_CHECK(cancellation.beginPoll() == NI_OK && cancellation.next(command) == NI_READY && command.m_kind == NET_SUBMIT_BEGIN);
    cancellation.finish(NI_OK);
    SUBMIT_CHECK(cancellation.cancel(low) == NI_OK);
    SUBMIT_CHECK(cancellation.next(command) == NI_READY && command.m_kind == NET_SUBMIT_CANCEL);
    cancellation.finish(NI_OK);
    SUBMIT_CHECK(cancellation.next(command) == NI_READY && command.m_kind == NET_SUBMIT_DATA);
    cancellation.finish(NI_OK);
    SUBMIT_CHECK(cancellation.complete(1, low, terminal) && cancellation.idle());
    SUBMIT_CHECK(cancellation.begin(101, 0, 5000, low) == NI_OK);
    const uint8_t* wrapped = reinterpret_cast<const uint8_t*>(UINTPTR_MAX - 3);
    SUBMIT_CHECK(cancellation.send(low, wrapped, 8) == NI_INVALID_ARGUMENT);
    frame.m_payload = wrapped;
    frame.m_payloadSize = 8;
    SUBMIT_CHECK(cancellation.data(frame) == NI_INVALID_ARGUMENT);
    SUBMIT_CHECK(cancellation.sendSynchronous(low, &byte, 1, ticket) == NI_OK);
    SUBMIT_CHECK(cancellation.beginPoll() == NI_OK && cancellation.next(command) == NI_READY);
    cancellation.finish(NI_OK);
    SUBMIT_CHECK(cancellation.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND);
    cancellation.finish(NI_OK);
    cancellation.advanceSynchronous(NI_NEED_MORE);
    SUBMIT_CHECK(cancellation.synchronousResult(ticket, false) == NI_READY);
    cancellation.advanceSynchronous(NI_OK);
    SUBMIT_CHECK(cancellation.synchronousResult(ticket, true) == NI_OK && cancellation.complete(1, low, terminal));

    // 反向预留正文，与DATA和ACK混排；独立预期只取成功接纳的调用顺序。
    NetSenderSubmission mixed;
    SUBMIT_CHECK(mixed.start(1, 101) == NI_OK && mixed.ready(1, 128, 4096) == NI_OK);
    for (uint32_t round = 0; round < 512; ++round)
    {
        uint64_t requests[8] = {};
        uint8_t content[32] = {};
        for (uint32_t index = 0; index < 8; ++index)
        {
            SUBMIT_CHECK(mixed.begin(101, round, 5000, requests[index]) == NI_OK);
        }
        for (uint32_t index = 0; index < 8; ++index)
        {
            std::memset(content, static_cast<int>(index + 1), sizeof(content));
            SUBMIT_CHECK(mixed.send(requests[7 - index], content, sizeof(content)) == NI_OK);
            frame.m_kind = index % 2 == 0 ? NI_DATA : NI_DATA_ACK;
            frame.m_requestId = index + 1;
            frame.m_payload = content;
            frame.m_payloadSize = sizeof(content);
            SUBMIT_CHECK(mixed.data(frame) == NI_OK);
        }
        std::memset(content, 0, sizeof(content));
        SUBMIT_CHECK(mixed.beginPoll() == NI_OK);
        for (uint32_t index = 0; index < 8; ++index)
        {
            SUBMIT_CHECK(mixed.next(command) == NI_READY && command.m_kind == NET_SUBMIT_BEGIN && command.m_id == requests[7 - index]);
            mixed.finish(NI_OK);
            SUBMIT_CHECK(mixed.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND);
            if ((round + index) % 3 == 0)
            {
                mixed.finish(NI_LIMIT_EXCEEDED);
                SUBMIT_CHECK(mixed.next(command) == NI_NEED_MORE);
                SUBMIT_CHECK(mixed.beginPoll() == NI_OK && mixed.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND);
            }
            std::memset(content, static_cast<int>(index + 1), sizeof(content));
            SUBMIT_CHECK(command.m_id == requests[7 - index] && command.m_size == sizeof(content));
            SUBMIT_CHECK(std::memcmp(command.m_bytes, content, sizeof(content)) == 0);
            // 模拟协议调用尚未返回时先回报唯一终态。
            SUBMIT_CHECK(mixed.complete(1, command.m_id, terminal));
            SUBMIT_CHECK(std::memcmp(command.m_bytes, content, sizeof(content)) == 0);
            mixed.finish(NI_OK);
            SUBMIT_CHECK(mixed.next(command) == NI_READY && command.m_kind == NET_SUBMIT_DATA);
            SUBMIT_CHECK(command.m_frame.m_kind == (index % 2 == 0 ? NI_DATA : NI_DATA_ACK));
            SUBMIT_CHECK(command.m_frame.m_requestId == index + 1 && command.m_frame.m_payloadSize == sizeof(content));
            SUBMIT_CHECK(std::memcmp(command.m_frame.m_payload, content, sizeof(content)) == 0);
            mixed.finish(NI_OK);
        }
        SUBMIT_CHECK(mixed.next(command) == NI_NEED_MORE && mixed.idle() && mixed.queuedBytes() == 0);
    }

// 在每个借用阶段退出，独立验证请求、流帧、未读回执均由所属模块回收。
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemState heapBefore = {}, heapAfter = {};
    _CrtMemCheckpoint(&heapBefore);
#endif
    for (uint32_t round = 0; round < 1024; ++round)
    {
        NetSenderSubmission ending;
        uint8_t owned[256] = {};
        std::memset(owned, 0x3d, sizeof(owned));
        SUBMIT_CHECK(ending.start(1, 101) == NI_OK && ending.ready(1, 128, 4096) == NI_OK);
        SUBMIT_CHECK(ending.begin(101, 11, 5000, low) == NI_OK);
        const uint32_t size = round % 4 == 0 ? 0 : sizeof(owned);
        SUBMIT_CHECK(ending.sendSynchronous(low, size == 0 ? nullptr : owned, size, ticket) == NI_OK);
        frame.m_kind = round % 2 == 0 ? NI_DATA : NI_DATA_ACK;
        frame.m_payload = owned;
        frame.m_payloadSize = sizeof(owned);
        SUBMIT_CHECK(ending.data(frame) == NI_OK);
        SUBMIT_CHECK(ending.beginPoll() == NI_OK && ending.next(command) == NI_READY && command.m_kind == NET_SUBMIT_BEGIN);
        ending.finish(NI_OK);
        SUBMIT_CHECK(ending.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND);
        SUBMIT_CHECK(command.m_size == size && (size == 0 ? command.m_bytes == nullptr : std::memcmp(command.m_bytes, owned, size) == 0));
        if (round % 4 == 0)
        {
            ending.disconnected(1);
            SUBMIT_CHECK(ending.complete(1, low, terminal));
            // 模拟外层异常兜底释放，故意保留当前借用和未读失败回执。
        }
        else if (round % 4 == 1)
        {
            SUBMIT_CHECK(ending.complete(1, low, terminal));
            ending.finish(NI_OK);
            ending.advanceSynchronous(NI_OK);
            SUBMIT_CHECK(ending.synchronousResult(ticket, false) == NI_OK);
        }
        else if (round % 4 == 2)
        {
            ending.close(NI_CANCELLED);
            ending.finish(NI_LIMIT_EXCEEDED);
        }
        else
        {
            ending.finish(NI_OK);
            ending.advanceSynchronous(NI_OK);
            SUBMIT_CHECK(ending.synchronousResult(ticket, true) == NI_OK);
            SUBMIT_CHECK(ending.next(command) == NI_READY && command.m_kind == NET_SUBMIT_DATA);
            ending.finish(NI_LIMIT_EXCEEDED);
        }
    }
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemCheckpoint(&heapAfter);
    SUBMIT_CHECK(heapAfter.lCounts[_NORMAL_BLOCK] == heapBefore.lCounts[_NORMAL_BLOCK]);
    SUBMIT_CHECK(heapAfter.lSizes[_NORMAL_BLOCK] == heapBefore.lSizes[_NORMAL_BLOCK]);
    SUBMIT_CHECK(heapAfter.lCounts[_CLIENT_BLOCK] == heapBefore.lCounts[_CLIENT_BLOCK]);
    SUBMIT_CHECK(heapAfter.lSizes[_CLIENT_BLOCK] == heapBefore.lSizes[_CLIENT_BLOCK]);
    std::printf("SUBMISSION_CLEANUP cycles=1024 live_blocks_delta=0 live_bytes_delta=0\n");
#endif
    std::printf("NetSenderSubmission FIFO and lifetime checks=%u PASS\n", checks);
    return 0;
#undef SUBMIT_CHECK
}

void NetSubmissionTestCases::boundary()
{
    if (NetSubmissionRun(false) != 0)
    {
        throw std::runtime_error("submission boundary failure");
    }
}

void NetSubmissionTestCases::benchmark()
{
    if (NetSubmissionRun(true) != 0)
    {
        throw std::runtime_error("submission benchmark validation failure");
    }
}

void NetSubmissionTestCases::orderedBatch()
{
    /** 比较接纳表的实际输出与独立命令序列
    @param [in] value 必须成立的预期
    @param [in] message 失败诊断
    */
    const auto Check = [](bool value, const char* message)
    {
        if (!value)
        {
            throw std::runtime_error(message);
        }
    };
    NetSenderSubmission queue;
    Check(queue.start(1, 101) == NI_OK && queue.ready(1, 128, 4096) == NI_OK, "batch submission start");
    uint8_t bytes[6] = {'1', '2', '3', '4', '5', '6'};
    uint64_t ids[4] = {};
    for (uint32_t index = 0; index < 4; ++index)
    {
        Check(queue.begin(101, 77, 5000, ids[index]) == NI_OK && queue.send(ids[index], bytes, 6) == NI_OK,
            "batch request admission");
    }
    NetSenderSubmissionCommand command = {};
    Check(queue.beginPoll() == NI_OK, "batch poll start");
    for (uint32_t index = 0; index < 4; ++index)
    {
        Check(queue.next(command, true) == NI_READY && command.m_kind == NET_SUBMIT_BEGIN && command.m_id == ids[index],
            "batch begin order");
        queue.finish(NI_OK);
        Check(queue.next(command, true) == NI_READY && command.m_kind == NET_SUBMIT_SEND && command.m_id == ids[index] &&
            command.m_size == 6 && std::memcmp(command.m_bytes, bytes, 6) == 0, "batch send order");
        queue.finish(NI_OK);
    }
    Check(queue.cancel(ids[0]) == NI_OK && queue.next(command, true) == NI_NEED_MORE,
        "cancel overtook pending transport bytes");
    Check(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_CANCEL && command.m_id == ids[0],
        "cancel lost at barrier");
    queue.finish(NI_OK);
    for (uint32_t index = 0; index < 4; ++index)
    {
        Check(queue.complete(1, ids[index], command), "batch request completion");
    }
    NetInterfaceFrame frame = {};
    frame.m_productId = 101;
    frame.m_messageType = NI_FILE_DATA_TYPE;
    frame.m_requestId = 9;
    frame.m_payload = bytes;
    frame.m_payloadSize = 6;
    for (uint32_t round = 0; round < 2; ++round)
    {
        frame.m_kind = NI_DATA;
        Check(queue.data(frame) == NI_OK, "batch stream prefix");
        frame.m_kind = round == 0 ? NI_DATA_ACK : NI_CANCEL;
        frame.m_payloadSize = round == 0 ? 6 : 0;
        Check(queue.data(frame) == NI_OK, "batch stream control");
        frame.m_kind = NI_DATA;
        frame.m_payloadSize = 6;
        Check(queue.data(frame) == NI_OK && queue.beginPoll() == NI_OK, "batch stream suffix");
        Check(queue.next(command, true) == NI_READY && command.m_frame.m_kind == NI_DATA, "batch stream prefix order");
        queue.finish(NI_OK);
        Check(queue.next(command, true) == NI_NEED_MORE && queue.next(command, true) == NI_NEED_MORE,
            "control barrier was skipped or consumed");
        Check(queue.next(command) == NI_READY && command.m_frame.m_kind == (round == 0 ? NI_DATA_ACK : NI_CANCEL),
            "batch stream control order");
        queue.finish(NI_OK);
        Check(queue.next(command, true) == NI_READY && command.m_frame.m_kind == NI_DATA, "batch stream suffix order");
        queue.finish(NI_OK);
    }
    uint64_t ticket = 0;
    Check(queue.dataSynchronous(frame, ticket) == NI_OK && queue.data(frame) == NI_OK && queue.beginPoll() == NI_OK,
        "batch synchronous admission");
    Check(queue.next(command, true) == NI_READY && command.m_kind == NET_SUBMIT_DATA, "batch synchronous staging");
    queue.finish(NI_OK);
    queue.advanceSynchronous(NI_READY);
    Check(queue.synchronousResult(ticket, false) == NI_READY && queue.next(command, true) == NI_NEED_MORE,
        "batch weakened synchronous delivery");
    queue.advanceSynchronous(NI_OK);
    Check(queue.synchronousResult(ticket, true) == NI_OK && queue.next(command, true) == NI_READY,
        "batch synchronous completion blocked suffix");
    queue.finish(NI_LIMIT_EXCEEDED);
    Check(queue.next(command, true) == NI_NEED_MORE && queue.beginPoll() == NI_OK && queue.next(command, true) == NI_READY,
        "batch full retried in same round or lost borrowed bytes");
    queue.finish(NI_OK);
    Check(queue.idle() && queue.queuedBytes() == 0, "batch retained request or bytes");
    // 纯DATA批量快路径不能推迟跨线程发布的关闭意图。
    for (uint32_t index = 0; index < 128; ++index)
    {
        frame.m_requestId = index + 1;
        Check(queue.data(frame) == NI_OK, "close-prefix admission");
    }
    Check(queue.beginPoll() == NI_OK && queue.next(command, true) == NI_READY, "close-prefix import");
    queue.finish(NI_OK);
    // 无参线程任务仅发布关闭意图；owner继续检查已排队前缀，join后可观察该意图。
    std::thread closer([&queue]() { queue.close(NI_CANCELLED); });
    closer.join();
    Check(queue.next(command, true) == NI_READY && command.m_kind == NET_SUBMIT_CLOSE &&
        command.m_status == NI_CANCELLED, "DATA fast path delayed close behind pending frames");
    queue.finish(NI_OK);
    queue.disconnected(2);
    Check(queue.idle() && queue.queuedBytes() == 0, "close-prefix leaked queued bodies");
}
#include "NetSubmissionPoolTest.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetInterface/NetBufferPool.h"
#include <cstring>
#include <stdexcept>

/** 验证条件并报告具体边界
@param [in] condition 预期成立的条件
@param [in] detail 静态错误信息
*/
static void SubmissionPoolCheck(bool condition, const char* detail)
{
    if (!condition)
    {
        throw std::runtime_error(detail);
    }
}

void NetSubmissionPoolTest::run()
{
    {
        NetBufferPoolConfig config;
        NetBufferPool gradual(config);
        const uint32_t lengths[] = {65, 1025, 60000, 1048576};
        const uint32_t capacities[] = {1024, 16448, 131072, 1048704};
        SubmissionPoolCheck(gradual.status() == NI_OK, "Gradual default pool initialization");
        for (uint32_t index = 1; index < 5; ++index)
        {
            SubmissionPoolCheck(gradual.statistics(index).capacity == 0, "Unused large class must remain lazy");
            NetBufferLease first, second;
            SubmissionPoolCheck(gradual.allocate(lengths[index - 1], first) == NI_OK, "Default first allocation");
            const size_t slots = gradual.statistics(index).capacity;
            SubmissionPoolCheck(slots > 0 && (capacities[index - 1] <= 65536 ?
                slots * capacities[index - 1] <= 65536 : slots == 1), "First body must not reserve an entire 8 MiB class");
            SubmissionPoolCheck(gradual.allocate(lengths[index - 1], second) == NI_OK, "Actual demand must grow the class");
            const size_t grown = gradual.statistics(index).capacity;
            first.reset();
            second.reset();
            SubmissionPoolCheck(gradual.allocate(lengths[index - 1], first) == NI_OK &&
                gradual.statistics(index).capacity == grown, "Released growth must be reused without allocation");
            first.reset();
            SubmissionPoolCheck(gradual.statistics(index).occupied == 0, "Default body ownership returned");
        }
    }
    NetBufferPoolConfig buffers;
    for (uint32_t index = 0; index < 5; ++index)
    {
        buffers.m_classes[index].initialCapacity = 0;
        buffers.m_classes[index].maximumCapacity = 2;
    }
    NetBufferPool pool(buffers);
    const uint32_t sizes[] = {64, 1024, 16448, 131072, 1048704};
    SubmissionPoolCheck(pool.status() == NI_OK, "Buffer pool initialization");
    for (uint32_t index = 0; index < 5; ++index)
    {
        NetBufferLease first, second, exhausted;
        const uint32_t size = index == 0 ? 1 : sizes[index - 1] + 1;
        SubmissionPoolCheck(pool.allocate(size, first) == NI_OK && first.capacity() == sizes[index] &&
            pool.allocate(sizes[index], second) == NI_OK && second.capacity() == sizes[index], "Buffer class rounding");
        std::memset(first.data(), 0x51, first.capacity());
        SubmissionPoolCheck(pool.allocate(size, exhausted) == NI_LIMIT_EXCEEDED && exhausted.data() == nullptr &&
            pool.allocate(size, first) == NI_INVALID_ARGUMENT, "Buffer pool exhaustion or ownership overwrite");
        first.reset();
        SubmissionPoolCheck(pool.allocate(size, exhausted) == NI_OK, "Buffer pool reuse");
        second.reset();
        exhausted.reset();
        SubmissionPoolCheck(pool.statistics(index).occupied == 0, "Buffer pool final lease leak");
    }
    NetBufferLease empty;
    SubmissionPoolCheck(pool.allocate(0, empty) == NI_OK && empty.data() == nullptr &&
        pool.allocate(1048705, empty) == NI_INVALID_ARGUMENT, "Buffer empty or oversized input");

    NetSenderSubmission queue;
    NetSenderSubmissionPools invalid;
    invalid.m_frames.initialCapacity = 2;
    invalid.m_frames.maximumCapacity = 1;
    SubmissionPoolCheck(queue.configurePools(invalid) == NI_INVALID_ARGUMENT, "Submission invalid record pool status");
    invalid = NetSenderSubmissionPools();
    invalid.m_queue.maximumProducers = 0;
    SubmissionPoolCheck(queue.configurePools(invalid) == NI_INVALID_ARGUMENT, "Submission invalid queue pool status");
    SubmissionPoolCheck(queue.start(1, 101) == NI_OK && queue.ready(1, 128, 4096) == NI_OK, "Submission start");
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_DATA;
    frame.m_productId = 101;
    frame.m_requestId = 1;
    frame.m_payload = reinterpret_cast<const uint8_t*>("123456");
    frame.m_payloadSize = 6;
    for (uint32_t index = 0; index < 2048; ++index)
    {
        frame.m_requestId = index + 1;
        SubmissionPoolCheck(queue.data(frame) == NI_OK, "Submission retained old 64 or 512 limit");
    }
    SubmissionPoolCheck(queue.queuedBytes() == 2048 * 6, "Submission payload ownership accounting");
    NetSenderSubmissionCommand command = {};
    SubmissionPoolCheck(queue.beginPoll() == NI_OK, "Submission begin drain");
    for (uint32_t index = 0; index < 2048; ++index)
    {
        SubmissionPoolCheck(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_DATA &&
            command.m_frame.m_requestId == index + 1 && command.m_frame.m_payloadSize == 6 &&
            std::memcmp(command.m_frame.m_payload, "123456", 6) == 0, "Submission FIFO or body corruption");
        queue.finish(NI_OK);
    }
    SubmissionPoolCheck(queue.idle() && queue.queuedBytes() == 0, "Submission ownership not returned");

    NetSenderSubmissionPools config;
    config.m_frames.initialCapacity = 2;
    config.m_frames.maximumCapacity = 2;
    uint8_t pooledBody[65] = {'1','2','3','4','5','6'};
    frame.m_payload = pooledBody;
    frame.m_payloadSize = sizeof(pooledBody);
    NetSenderSubmission tiny;
    SubmissionPoolCheck(tiny.configurePools(config) == NI_OK && tiny.start(1, 101) == NI_OK &&
        tiny.ready(1, 128, 4096) == NI_OK && tiny.configurePools(config) == NI_INVALID_STATE, "Submission pool configuration lifecycle");
    SubmissionPoolCheck(tiny.data(frame) == NI_OK && tiny.data(frame) == NI_OK &&
        tiny.data(frame) == NI_LIMIT_EXCEEDED && tiny.queuedBytes() == 2 * sizeof(pooledBody), "Submission actual record capacity");
    SubmissionPoolCheck(tiny.beginPoll() == NI_OK && tiny.next(command) == NI_READY, "Submission borrowed record");
    tiny.close(NI_CANCELLED);
    tiny.disconnected(1);
    SubmissionPoolCheck(std::memcmp(command.m_frame.m_payload, "123456", 6) == 0 && tiny.queuedBytes() == sizeof(pooledBody),
        "Submission close freed borrowed pool storage");
    tiny.finish(NI_OK);
    SubmissionPoolCheck(tiny.queuedBytes() == 0, "Submission final borrowed release");

    frame.m_payload = reinterpret_cast<const uint8_t*>("123456");
    frame.m_payloadSize = 6;
    // 析构时入口项超过单轮512导入预算，验证必须完整排空后才能销毁底层池
    {
        NetSenderSubmission unfinished;
        SubmissionPoolCheck(unfinished.start(1, 101) == NI_OK && unfinished.ready(1, 128, 4096) == NI_OK,
            "Submission pending destruction start");
        for (uint32_t index = 0; index < 2048; ++index)
        {
            SubmissionPoolCheck(unfinished.data(frame) == NI_OK, "Submission pending destruction prepare");
        }
    }
}
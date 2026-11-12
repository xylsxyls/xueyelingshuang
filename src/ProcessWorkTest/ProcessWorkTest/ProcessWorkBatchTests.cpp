#include "ProcessWorkCoreTests.h"
#include "ProcessWorkTest.h"
#include "ProcessWork/ProcessWorkReceiveQueue.h"
#include <cstring>
#include <thread>
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

/** 构造真实12字节头，不在正文增加测试元数据。
@param [in] sequence 消息类型字段中的序号
@param [in] body 完整正文
@return 包头及正文
*/
static std::string BatchPacket(int32_t sequence, const std::string& body)
{
    const int32_t header[3] = { bbPid(), sequence, static_cast<int32_t>(body.size()) };
    std::string packet(reinterpret_cast<const char*>(header), sizeof(header));
    packet += body;
    return packet;
}

void testProcessWorkReceiveBatches()
{
    ProcessWorkReceiveQueue queue;
    ProcessWorkMemoryConfig config;
    config.m_localQueueBytes = 600;
    bbCheck(queue.configure(config), "receive batch budget configuration failed");
    for (int32_t round = 0; round < 1000; ++round)
    {
        std::string first = BatchPacket(round * 2, "123456");
        const std::string second = BatchPacket(round * 2 + 1, "123456");
        bbCheck(queue.append(first.data(), first.size()) && queue.append(second.data(), second.size()), "batch failed within budget");
        first.assign(first.size(), '!');
        bbCheck(queue.used() == 566 && !queue.append(second.data(), second.size()), "batch budget or failed-append contract wrong");
        bbCheck(!queue.configure(config), "in-flight batch reconfigured");
        bbCheck(queue.flush(), "batch publication failed");
        for (int32_t i = 0; i < 2; ++i)
        {
            char* packet = nullptr;
            bbCheck(queue.take(&packet, 0) && packet != nullptr, "batch packet missing");
            int32_t header[3] = {};
            std::memcpy(header, packet, sizeof(header));
            bbCheck(header[0] == bbPid() && header[1] == round * 2 + i && header[2] == 6 &&
                std::memcmp(packet + 12, "123456", 6) == 0 && packet[18] == '\0', "batch source ownership or FIFO corrupt");
            bbCheck(queue.used() == static_cast<uint64_t>(2 - i) * 283, "executing batch packet not charged");
            queue.release(packet);
        }
        char* packet = nullptr;
        bbCheck(queue.used() == 0 && !queue.take(&packet, 0), "batch leaked charge or duplicated packet");
    }
    const std::string large = BatchPacket(0, std::string(65536, 'x'));
    bbCheck(!queue.append(large.data(), large.size()) && queue.used() == 0, "fixed receive limit bypassed");
    config.m_localQueueBytes = static_cast<int64_t>(large.size()) + 265;
    bbCheck(queue.configure(config) && queue.append(large.data(), large.size()) && !queue.append(large.data(), large.size()),
        "large packet exact quota boundary");
    bbCheck(queue.flush(), "oversized singleton not published");
    char* packet = nullptr;
    bbCheck(queue.take(&packet, 0) && std::memcmp(packet, large.data(), large.size()) == 0, "large batch corrupted");
    queue.release(packet);
    bbCheck(queue.used() == 0 && queue.configure(config), "large batch not released");
    const std::string smallPacket = BatchPacket(1, "123456");
    bbCheck(queue.append(smallPacket.data(), smallPacket.size()), "unpublished cleanup setup failed");
    queue.clear();
    bbCheck(queue.used() == 0 && queue.configure(config), "unpublished batch not cleared");
    config.m_reserveBytes = UINT64_MAX;
    bbCheck(queue.configure(config) && !queue.append(smallPacket.data(), smallPacket.size()) && queue.used() == 0,
        "batch ignored system memory reserve or leaked failed reservation");
}

void testProcessWorkReceiveCopyBoundary()
{
#ifdef _WIN32
    SYSTEM_INFO info = {};
    GetSystemInfo(&info);
    const size_t pageBytes = info.dwPageSize;
    void* pages = VirtualAlloc(nullptr, pageBytes * 3, MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
    bbCheck(pages != nullptr, "guarded source allocation failed");
    // 释放整个测试映射；不会读取或写入两侧保护页。
    auto release = [](void* value)
    {
        VirtualFree(value, 0, MEM_RELEASE);
    };
    std::unique_ptr<void, decltype(release)> owner(pages, release);
    DWORD previous = 0;
    bbCheck(VirtualProtect(static_cast<char*>(pages) + pageBytes, pageBytes, PAGE_READWRITE, &previous) != FALSE,
        "guarded source protection failed");
#else
    const long queriedPageBytes = sysconf(_SC_PAGESIZE);
    bbCheck(queriedPageBytes > 0, "system page size unavailable");
    const size_t pageBytes = static_cast<size_t>(queriedPageBytes);
    void* pages = mmap(nullptr, pageBytes * 3, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    bbCheck(pages != MAP_FAILED, "guarded source allocation failed");
    // 释放整个测试映射；长度来自本次系统页大小查询。
    auto release = [pageBytes](void* value)
    {
        munmap(value, pageBytes * 3);
    };
    std::unique_ptr<void, decltype(release)> owner(pages, release);
    bbCheck(mprotect(static_cast<char*>(pages) + pageBytes, pageBytes, PROT_READ | PROT_WRITE) == 0,
        "guarded source protection failed");
#endif
    ProcessWorkReceiveQueue queue;
    ProcessWorkMemoryConfig config;
    bbCheck(queue.configure(config), "copy boundary queue setup failed");
    for (int32_t edge = 0; edge < 2; ++edge)
    {
        for (int32_t length = 0; length <= 96; ++length)
        {
            const int32_t sequence = edge * 97 + length;
            const std::string expected = BatchPacket(sequence, packetBody(sequence, length));
            char* source = static_cast<char*>(pages) + pageBytes;
            if (edge != 0)
            {
                source += pageBytes - expected.size();
            }
            std::memcpy(source, expected.data(), expected.size());
            bbCheck(queue.append(source, expected.size()), "guarded source copy rejected");
            std::memset(source, '!', expected.size());
            bbCheck(queue.flush(), "guarded source publication failed");
            char* packet = nullptr;
            bbCheck(queue.take(&packet, 0) && std::memcmp(packet, expected.data(), expected.size()) == 0 &&
                packet[expected.size()] == '\0', "guarded source copy corrupted data or ownership");
            queue.release(packet);
            bbCheck(queue.used() == 0, "copy boundary budget leaked");
        }
    }
}

void testProcessWorkReceiveBatchStress()
{
    ProcessWorkReceiveQueue queue;
    ProcessWorkMemoryConfig config;
    config.m_localQueueBytes = 200000;
    bbCheck(queue.configure(config), "batch stress configuration failed");
    std::atomic<bool> cancel(false);
    IpcJobs jobs;
    const int32_t count = 20000;
    const int32_t sizes[4] = { 0, 6, 64, 65536 };
    jobs.add([&queue, &cancel, &sizes]()
    {
        for (int32_t sequence = 0; sequence < 20000 && !cancel.load(); ++sequence)
        {
            const std::string packet = BatchPacket(sequence, packetBody(sequence, sizes[sequence % 4]));
            while (!queue.append(packet.data(), packet.size()) && !cancel.load())
            {
                queue.flush();
                std::this_thread::yield();
            }
            if (sequence % 7 == 0)
            {
                queue.flush();
            }
        }
        while (!queue.flush() && !cancel.load())
        {
            std::this_thread::yield();
        }
    });
    try
    {
        for (int32_t sequence = 0; sequence < count; ++sequence)
        {
            char* packet = nullptr;
            bbWait([&queue, &packet]() { return queue.take(&packet, 1); }, "batch stress next packet");
            const std::string expected = BatchPacket(sequence, packetBody(sequence, sizes[sequence % 4]));
            bbCheck(std::memcmp(packet, expected.data(), expected.size()) == 0 && packet[expected.size()] == '\0',
                "batch stress missing/duplicate/reordered/corrupt packet");
            queue.release(packet);
        }
        jobs.drain();
        bbCheck(queue.used() == 0, "batch stress retained budget");
    }
    catch (...)
    {
        cancel.store(true);
        throw;
    }
}

void testProcessWorkInlineBoundary()
{
    ProcessWork& ipc = ProcessWork::instance();
    PacketLog log;
    IpcScope scope;
    scope.start(&log, receiveName("inline-boundary"), 80, 1);
    const int32_t sizes[5] = { 36, 37, 6, 0, 68 };
    for (int32_t sequence = 0; sequence < 1000; ++sequence)
    {
        const std::string body = packetBody(sequence, sizes[sequence % 5]);
        sendPacket(0, bbPid(), "", body, sequence);
    }
    for (int32_t length = 0; length <= 36; ++length)
    {
        const std::string body = packetBody(length + 1000, length);
        std::unique_ptr<char[]> exact(new char[length + 1]);
        std::memcpy(exact.get() + 1, body.data(), body.size());
        bbCheck(ipc.send(bbPid(), length == 0 ? nullptr : exact.get() + 1, length,
            static_cast<MessageType>(length + 1000)), "short unaligned payload rejected");
    }
    waitPackets(log, 1037);
    const std::vector<ReceivedPacket> packets = log.snapshot();
    for (int32_t sequence = 0; sequence < 1000; ++sequence)
    {
        bbCheck(packets[sequence].type == sequence && packets[sequence].pid == bbPid() &&
            packets[sequence].body == packetBody(sequence, sizes[sequence % 5]), "inline/mapped slot transition corrupted packet");
    }
    for (int32_t length = 0; length <= 36; ++length)
    {
        bbCheck(packets[length + 1000].type == length + 1000 &&
            packets[length + 1000].body == packetBody(length + 1000, length), "short unaligned copy corrupted payload");
    }
}

void testProcessWorkSendCache()
{
    ProcessWorkSendCache cache;
    std::shared_ptr<ProcessWorkChannel> channel(new ProcessWorkChannel);
    std::weak_ptr<ProcessWorkChannel> lifetime(channel);
    const uint64_t original = cache.epoch();
    cache.remember(42, original, channel);
    bbCheck(channel.use_count() == 2, "cache must own exactly one reference");
    std::shared_ptr<ProcessWorkChannel> colliding(new ProcessWorkChannel);
    for (int32_t i = 0; i < 1000; ++i)
    {
        cache.remember(106, original, colliding);
    }
    bbCheck(colliding.use_count() == 1 && channel.use_count() == 2,
        "colliding target churned the admitted cache ownership");
    {
        ProcessWorkSendCacheLease borrowed;
        bbCheck(cache.find(42, borrowed) == channel.get() && channel.use_count() == 2,
            "hot cache borrow copied ownership or returned wrong target");
        ProcessWorkSendCacheLease nested;
        bbCheck(cache.find(42, nested) == nullptr, "nested borrow overwrote outer protection");
        channel.reset();
        cache.clear();
        bbCheck(!lifetime.expired(), "clear freed an in-flight channel");
        borrowed.reset();
        borrowed.reset();
        bbCheck(lifetime.expired(), "explicit lease reset retained cleared channel");
    }
    bbCheck(lifetime.expired(), "last borrow did not reclaim removed channel");
    channel.reset(new ProcessWorkChannel);
    cache.remember(42, original, channel);
    {
        ProcessWorkSendCacheLease stale;
        bbCheck(cache.find(42, stale) == nullptr, "lookup from an old clear epoch was cached");
    }
    std::atomic<bool> failed(false);
    IpcJobs jobs;
    for (int32_t index = 0; index < 8; ++index)
    {
        jobs.add([&cache, &channel, &failed, index]()
        {
            for (int32_t i = 0; i < 5000; ++i)
            {
                const int32_t pid = 42 + (index & 1) * 64;
                const uint64_t epoch = cache.epoch();
                ProcessWorkSendCacheLease lease;
                ProcessWorkChannel* found = cache.find(pid, lease);
                if (found != nullptr && found != channel.get())
                {
                    failed.store(true);
                }
                if (found == nullptr)
                {
                    cache.remember(pid, epoch, channel);
                }
                if ((i & 63) == 0)
                {
                    cache.clear();
                }
            }
        });
    }
    jobs.drain();
    cache.clear();
    bbCheck(!failed.load() && channel.use_count() == 1, "concurrent cache collision/clear leaked or corrupted ownership");
}

void testProcessWorkReceiveReserve()
{
    ProcessWorkReceiveQueue queue;
    ProcessWorkMemoryConfig config;
    config.m_receiveBatchReserve = 16;
    config.m_localQueueInitialCapacity = 256;
    config.m_localQueueBytes = 1024;
    bbCheck(queue.configure(config), "idle receive reserve must not consume message backlog quota");
    config.m_localQueueBytes = 2 * 1024 * 1024;
    bbCheck(queue.configure(config), "retrying a valid batch reservation failed");
    for (int32_t round = 0; round < 20; ++round)
    {
        for (int32_t i = 0; i < 16; ++i)
        {
            const std::string packet = BatchPacket(round * 16 + i, "123456");
            bbCheck(queue.append(packet.data(), packet.size()) && queue.flush(), "preallocated batch admission failed");
        }
        for (int32_t i = 0; i < 16; ++i)
        {
            char* packet = nullptr;
            bbCheck(queue.take(&packet, 0), "preallocated batch lost message");
            const std::string expected = BatchPacket(round * 16 + i, "123456");
            bbCheck(std::memcmp(packet, expected.data(), expected.size()) == 0, "preallocated batch corrupted FIFO");
            queue.release(packet);
        }
        bbCheck(queue.used() == 0, "preallocated batches retained message charges");
    }
    // 已预留的空闲存储不消耗消息额度；大包按实际计费恰好达到上限时仍能接纳。
    const std::string large = BatchPacket(320, std::string(2 * 1024 * 1024 - 277, 'x'));
    bbCheck(queue.append(large.data(), large.size()) && queue.flush(), "reserve blocked oversized singleton");
    char* packet = nullptr;
    bbCheck(queue.take(&packet, 0) && std::memcmp(packet, large.data(), large.size()) == 0,
        "oversized reserve transition corrupted data");
    queue.release(packet);
    bbCheck(queue.used() == 0, "oversized reserve transition leaked charges");
    config.m_localQueueInitialCapacity = 0;
    bbCheck(!queue.configure(config), "zero queue pool was accepted");
    queue.clear();
}
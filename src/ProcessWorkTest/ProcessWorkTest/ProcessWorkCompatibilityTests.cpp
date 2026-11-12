#include "ProcessWorkCompatibilityTests.h"
#include <algorithm>
#include <climits>
#include <cstring>
#include <type_traits>
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

ProcessWorkInputPages::ProcessWorkInputPages() :
m_allocation(nullptr), m_page(0), m_accessible(0)
{
#ifdef _WIN32
    SYSTEM_INFO info = {};
    GetSystemInfo(&info);
    m_page = info.dwPageSize;
    m_accessible = ((65536 + m_page - 1) / m_page) * m_page;
    m_allocation = VirtualAlloc(nullptr, m_accessible + 2 * m_page, MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
#else
    const long page = sysconf(_SC_PAGESIZE);
    bbCheck(page > 0, "invalid system page size");
    m_page = static_cast<size_t>(page);
    m_accessible = ((65536 + m_page - 1) / m_page) * m_page;
    m_allocation = mmap(nullptr, m_accessible + 2 * m_page, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (m_allocation == MAP_FAILED)
    {
        m_allocation = nullptr;
    }
#endif
    bbCheck(m_allocation != nullptr, "guarded source allocation failed");
}

ProcessWorkInputPages::~ProcessWorkInputPages()
{
#ifdef _WIN32
    if (!VirtualFree(m_allocation, 0, MEM_RELEASE))
#else
    if (munmap(m_allocation, m_accessible + 2 * m_page) != 0)
#endif
    {
        bbCleanupFailure("guarded source release failed");
    }
}

void ProcessWorkInputPages::protect(bool enabled)
{
    char* begin = static_cast<char*>(m_allocation) + m_page;
#ifdef _WIN32
    DWORD previous = 0;
    bbCheck(VirtualProtect(begin, m_accessible, enabled ? PAGE_READWRITE : PAGE_NOACCESS, &previous) != FALSE,
        "guarded source protection failed");
#else
    bbCheck(mprotect(begin, m_accessible, enabled ? PROT_READ | PROT_WRITE : PROT_NONE) == 0,
        "guarded source protection failed");
#endif
}

char* ProcessWorkInputPages::data(size_t length, bool tail)
{
    bbCheck(length <= 65536, "guarded input length");
    return length == 0 ? nullptr : static_cast<char*>(m_allocation) + m_page + (tail ? m_accessible - length : 0);
}

void ProcessWorkCompatibilityTests::payloadRestart()
{
    ProcessWork& ipc = ProcessWork::instance();
    ProcessWorkChannel stale;
    PacketLog log;
    IpcScope scope;
    const std::string name = receiveName("payload-restart");
    bbCheck(stale.payloadCapacity() == 0 && ipc.receivePayloadCapacity() == 0, "unopened capacity");
    scope.start(&log, name, 65548, 2);
    bbCheck(ipc.receivePayloadCapacity() == 65536 && stale.openSender(bbPid()) && stale.payloadCapacity() == 65536,
        "declared capacity differs from body capacity");
    const std::string large = packetBody(0, 65536);
    bbCheck(ipc.send(bbPid(), large, static_cast<MessageType>(65536)), "max body send");
    waitPackets(log, 1);
    scope.close();
    bbCheck(stale.payloadCapacity() == 0 && ipc.receivePayloadCapacity() == 0, "revoked capacity");
    scope.start(&log, name, 13, 2);
    bbCheck(ipc.receivePayloadCapacity() == 1 && stale.payloadCapacity() == 0, "old mapping regained capacity");
    bbCheck(stale.trySend(large.data(), 65536, 0) == PROCESS_WORK_UNAVAILABLE, "stale sender reused old large mapping");
    bbCheck(ipc.trySend(bbPid(), large.data(), 2) == PROCESS_WORK_TOO_LARGE, "smaller generation allowed excess");
    bbCheck(ipc.send(name, large.data(), 1, static_cast<MessageType>(1)), "small named send");
    waitPackets(log, 2);
    scope.close();
    scope.start(&log, name, 12, 1);
    bbCheck(ipc.receivePayloadCapacity() == 0 && ipc.send(bbPid(), nullptr, 0, static_cast<MessageType>(0)), "empty-only send");
    bbCheck(ipc.trySend(bbPid(), large.data(), 1) == PROCESS_WORK_TOO_LARGE, "empty-only excess");
    waitPackets(log, 3);
    scope.close();
    const std::vector<ReceivedPacket> rows = log.snapshot();
    bbCheck(rows.size() == 3 && rows[0].body == large && rows[1].body == large.substr(0, 1) &&
        rows[2].body.empty() && rows[2].nullData && rows[0].type == 65536 && rows[1].type == 1 && rows[2].type == 0,
        "restart payload/type/ownership mismatch");
}

void ProcessWorkCompatibilityTests::typeDomain()
{
    bbCheck(sizeof(MessageType) == sizeof(int32_t) && std::is_same<std::underlying_type<MessageType>::type, int32_t>::value,
        "message type must be signed 32bit");
    bbCheck(INIT == 0 && MESSAGE == 10 && HEAD == 20 && LOGTEST_MESSAGE == 1000 && LOGTEST_SEND_MESSAGE == 1001 &&
        LOGTEST_LOCAL_MESSAGE == 1002 && LOGTEST_FILE_MESSAGE == 1003, "message enum values changed");
    const int32_t types[] = { 0, 1, 1000, 1003, 1023, 1024, 65535, 65536, 0x3fffffff, INT32_MAX - 1, INT32_MAX };
    const int32_t routes[] = { 0, 1, 2, 3, 5 };
    PacketLog log;
    IpcScope scope;
    const std::string name = receiveName("type-domain");
    scope.start(&log, name, 139, 2);
    ProcessWork& ipc = ProcessWork::instance();
    bbCheck(ipc.initPostThread(), "post init");
    size_t count = 0;
    for (size_t route = 0; route < 5; ++route)
    {
        for (size_t t = 0; t < sizeof(types) / sizeof(types[0]); ++t)
        {
            std::string body = packetBody(static_cast<int32_t>(count), 127);
            sendPacket(routes[route], bbPid(), name, body, types[t]);
            std::fill(body.begin(), body.end(), 'x');
            waitPackets(log, ++count);
        }
    }
    const int32_t negative[] = { INT32_MIN, -2, -1 };
    for (size_t i = 0; i < 3; ++i)
    {
        const MessageType type = static_cast<MessageType>(negative[i]);
        bbCheck(ipc.trySend(bbPid(), "123456", 6, type) == PROCESS_WORK_INVALID &&
            !ipc.send(bbPid(), "123456", 6, type) && !ipc.send(name, "123456", 6, type) &&
            !ipc.post(bbPid(), "123456", 6, type) && !ipc.post(name, "123456", 6, type), "negative type accepted");
    }
    ipc.waitForPostEnd();
    scope.close();
    const std::vector<ReceivedPacket> rows = log.snapshot();
    bbCheck(rows.size() == 55, "type domain missing/extra messages");
    for (size_t i = 0; i < rows.size(); ++i)
    {
        bbCheck(rows[i].pid == bbPid() && rows[i].type == types[i % 11] && rows[i].body == packetBody(static_cast<int32_t>(i), 127),
            "type domain payload or FIFO mismatch");
    }
}

void ProcessWorkCompatibilityTests::guardedInput()
{
    ProcessWorkInputPages pages;
    PacketLog log;
    IpcScope scope;
    const std::string name = receiveName("guarded-input");
    scope.start(&log, name, 65548, 2);
    ProcessWork& ipc = ProcessWork::instance();
    const int32_t sizes[] = { 0, 1, 2, 3, 6, 7, 15, 16, 31, 32, 63, 64, 127, 128, 255, 256, 4095, 4096, 4097, 65535, 65536 };
    size_t count = 0;
    for (int32_t route = 0; route < 5; ++route)
    {
        for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); ++s)
        {
            for (int32_t side = 0; side < 2; ++side)
            {
                pages.protect(true);
                const int32_t length = sizes[s];
                const std::string expected = packetBody(static_cast<int32_t>(count), length);
                char* data = pages.data(length, side != 0);
                if (length)
                {
                    std::memcpy(data, expected.data(), length);
                }
                const MessageType type = static_cast<MessageType>(count);
                bool sent = false;
                if (route == 0 || route == 1)
                {
                    sent = route == 0 ? ipc.send(bbPid(), data, length, type) : ipc.send(name, data, length, type);
                }
                else if (route == 2 || route == 3)
                {
                    // 消费者尚未启动时只做非等待接纳；撤销源地址后才启动后台，避免时序碰巧通过。
                    sent = (route == 2 ? ipc.tryPost(bbPid(), data, length, type, true, false) :
                        ipc.tryPost(name, data, length, type, true, false)) == PROCESS_WORK_POSTED;
                }
                else
                {
                    sent = ipc.trySend(bbPid(), data, length, type, true, true) == PROCESS_WORK_SENT;
                }
                bbCheck(sent, "guarded source admission");
                pages.protect(false);
                if (route == 2 || route == 3)
                {
                    bbCheck(ipc.initPostThread(), "guarded post start");
                }
                waitPackets(log, ++count);
                if (route == 2 || route == 3)
                {
                    ipc.waitForPostEnd();
                    ipc.uninitPostThread();
                }
                const std::vector<ReceivedPacket> rows = log.snapshot();
                bbCheck(rows.size() == count && rows.back().body == expected && rows.back().pid == bbPid() &&
                    rows.back().type == static_cast<int32_t>(count - 1) && rows.back().nullData == (length == 0),
                    "guarded source copy/FIFO mismatch");
            }
        }
    }
    scope.close();
    bbCheck(count == 210 && log.snapshot().size() == count, "guarded input case loss");
}

void ProcessWorkCompatibilityTests::overloadRejection()
{
    const int32_t capacities[] = { 0, 1, 6, 65536 };
    for (int32_t mode = 0; mode < 2; ++mode)
    {
        for (int32_t c = 0; c < 4; ++c)
        {
            const int32_t capacity = capacities[c];
            const std::string name(1023, 'n');
            const std::string missing = "missing_" + bbNumber(bbPid());
            PacketLog log;
            IpcScope scope;
            scope.start(&log, name, capacity + 12, 2, 1, 2, 1, 2);
            ProcessWork& ipc = ProcessWork::instance();
            if (mode)
            {
                bbCheck(ipc.initPostThread(), "overload post start");
            }
            IpcJobs jobs;
            for (int32_t producer = 0; producer < 4; ++producer)
            {
                // 每个生产者交替四个重载；独立序号不依赖跨生产者调度顺序。
                jobs.add([&, producer]()
                {
                    const std::string oversized(capacity + 1, 'x');
                    for (int32_t sequence = 0; sequence < 512; ++sequence)
                    {
                        if (sequence % 16 == 0)
                        {
                            if (mode == 0)
                            {
                                bbCheck(!ipc.send(bbPid(), oversized) && !ipc.send(name, oversized) &&
                                    !ipc.send(bbPid(), nullptr, 1) && !ipc.send(name, "x", -1) && !ipc.send(missing, "x", 1),
                                    "invalid direct overload accepted");
                            }
                            else
                            {
                                bbCheck(ipc.post(bbPid(), oversized) && ipc.post(name, oversized) && ipc.post(missing, "x", 1),
                                    "post must allow local acceptance before remote validation");
                                bbCheck(!ipc.post(bbPid(), nullptr, 1) && !ipc.post(name, "x", -1), "invalid post parameter accepted");
                            }
                        }
                        const int32_t length = sequence % 3 == 0 ? 0 : (sequence % 3 == 1 ? (std::min)(1, capacity) : capacity);
                        std::string body = packetBody(producer * 100000 + sequence, length);
                        const char* data = length ? body.data() : nullptr;
                        const MessageType type = static_cast<MessageType>(producer * 100000 + sequence);
                        bool ok = false;
                        switch (sequence % 4)
                        {
                        case 0:
                            ok = mode ? ipc.post(bbPid(), data, length, type) : ipc.send(bbPid(), data, length, type);
                            break;
                        case 1:
                            ok = mode ? ipc.post(name, body, type) : ipc.send(name, body, type);
                            break;
                        case 2:
                            ok = mode ? ipc.post(bbPid(), body, type) : ipc.send(bbPid(), body, type);
                            break;
                        default:
                            ok = mode ? ipc.post(name, data, length, type) : ipc.send(name, data, length, type);
                            break;
                        }
                        bbCheck(ok, "valid overload failed");
                        std::fill(body.begin(), body.end(), 'x');
                    }
                });
            }
            jobs.drain();
            if (mode)
            {
                ipc.waitForPostEnd();
            }
            waitPackets(log, 2048);
            scope.close();
            const std::vector<ReceivedPacket> rows = log.snapshot();
            bbCheck(rows.size() == 2048, "invalid overload delivered or valid message lost");
            int32_t next[4] = {};
            for (size_t i = 0; i < rows.size(); ++i)
            {
                const int32_t producer = rows[i].type / 100000, sequence = rows[i].type % 100000;
                bbCheck(producer >= 0 && producer < 4 && sequence == next[producer]++, "overload per-stream FIFO");
                const int32_t length = sequence % 3 == 0 ? 0 : (sequence % 3 == 1 ? (std::min)(1, capacity) : capacity);
                bbCheck(rows[i].pid == bbPid() && rows[i].body == packetBody(rows[i].type, length) && rows[i].nullData == (length == 0),
                    "overload content mismatch");
            }
            for (int32_t i = 0; i < 4; ++i)
            {
                bbCheck(next[i] == 512, "overload producer missing messages");
            }
        }
    }
}
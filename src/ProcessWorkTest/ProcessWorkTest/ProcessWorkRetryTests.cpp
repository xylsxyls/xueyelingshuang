#include "ProcessWorkRetryTests.h"
#include "ProcessWorkTest.h"
#include "ProcessWork/ProcessWorkTime.h"
#include "ProcessWork/ProcessWorkReceiveQueue.h"
#include <cstring>
#include <atomic>

void ProcessWorkRetryTests::sendCapacity(int32_t mode)
{
    ProcessWorkChannel receiver, sender;
    ProcessWorkReceiveQueue received;
    std::atomic<int32_t> full(0);
    bbCheck(receiver.openReceiver(76, 1, 0, 0, 0, 0), "retry receiver setup");
    receiver.publish();
    bbCheck(sender.openSender(bbPid()), "retry sender setup");
    bbCheck(sender.trySend("123456", 6, 1) == PROCESS_WORK_RESOURCE, "first registration must be pending");
    receiver.pump(received, full);
    bbCheck(sender.trySend("123456", 6, 2) == PROCESS_WORK_SENT, "fill one slot");
    const bool waitCapacity = mode >= 2;
    if (!waitCapacity)
    {
        bbCheck(sender.trySend("123456", 6, 3, (mode & 1) != 0, false) == PROCESS_WORK_CAPACITY,
            "full trySend must return capacity instead of Busy");
        bbCheck(receiver.pump(received, full) == 1, "rejected send changed queue");
        char* packet = nullptr;
        bbCheck(received.take(&packet, 0), "first accepted packet absent");
        received.release(packet);
        bbCheck(!received.take(&packet, 0), "rejected send delivered");
        return;
    }
    std::atomic<bool> started(false), done(false);
    int32_t result = PROCESS_WORK_INVALID;
    IpcJobs jobs;
    try
    {
        jobs.add([&]()
        {
            started.store(true);
            result = mode == 4 ? sender.send("123456", 6, 3) :
                sender.trySend("123456", 6, 3, mode == 5 || (mode & 1) != 0, true);
            done.store(true);
        });
        bbWait([&]() { return started.load(); }, "sender reached retry call");
        bbSleep(20);
        bbCheck(!done.load(), "capacity wait returned before capacity released");
        if (mode == 5)
        {
            receiver.stop();
            bbWait([&]() { return done.load(); }, "stopped receiver releases capacity waiter");
            jobs.drain();
            bbCheck(result == PROCESS_WORK_UNAVAILABLE, "stopped send reported success");
            return;
        }
        bbWait([&]() { receiver.pump(received, full); return done.load(); }, "send resumes after pump");
        jobs.drain();
        receiver.pump(received, full);
        bbCheck(result == PROCESS_WORK_SENT, "waiting send did not commit");
        for (int32_t expected = 2; expected <= 3; ++expected)
        {
            char* packet = nullptr;
            bbCheck(received.take(&packet, 0), "accepted retry packet missing");
            int32_t header[3] = {};
            std::memcpy(header, packet, sizeof(header));
            const bool valid = header[1] == expected && header[2] == 6 && std::memcmp(packet + 12, "123456", 6) == 0;
            received.release(packet);
            bbCheck(valid, "capacity retry reordered or corrupted a message");
        }
        char* duplicate = nullptr;
        bbCheck(!received.take(&duplicate, 0), "capacity retry committed duplicate");
    }
    catch (...)
    {
        receiver.stop();
        jobs.drain();
        throw;
    }
}

void ProcessWorkRetryTests::postCapacity(int32_t mode)
{
    ProcessWork& ipc = ProcessWork::instance();
    ProcessWorkChannel receiver;
    ProcessWorkReceiveQueue received;
    std::atomic<int32_t> full(0);
    IpcScope scope;
    ProcessWorkMemoryConfig config;
    config.m_localQueueBytes = 300;
    bbCheck(ipc.configurePostMemory(config) && receiver.openReceiver(76, 1, 0, 0, 0, 0), "post retry setup");
    receiver.publish();
    bbCheck(ipc.initPostThread(), "start post consumer");
    bbCheck(ipc.tryPost(bbPid(), "123456", 6, static_cast<MessageType>(1)) == PROCESS_WORK_POSTED, "fill local post budget");
    const bool waitCapacity = mode >= 2;
    std::atomic<bool> started(false), done(false);
    int32_t result = PROCESS_WORK_POST_INVALID;
    IpcJobs jobs;
    try
    {
        jobs.add([&]()
        {
            started.store(true);
            result = mode == 4 ? (ipc.post(bbPid(), "123456", 6, static_cast<MessageType>(2)) ?
                PROCESS_WORK_POSTED : PROCESS_WORK_POST_UNAVAILABLE) :
                ipc.tryPost(bbPid(), "123456", 6, static_cast<MessageType>(2), (mode & 1) != 0, waitCapacity);
            done.store(true);
        });
        bbWait([&]() { return started.load(); }, "post producer started");
        bbSleep(20);
        bbCheck(waitCapacity ? !done.load() : done.load(), "post capacity switch ignored");
        const int32_t expected = waitCapacity ? 2 : 1;
        int32_t count = 0;
        bbWait([&]()
        {
            receiver.pump(received, full);
            char* packet = nullptr;
            while (received.take(&packet, 0))
            {
                int32_t header[3] = {};
                std::memcpy(header, packet, sizeof(header));
                const bool valid = header[1] == count + 1 && header[2] == 6 && std::memcmp(packet + 12, "123456", 6) == 0;
                received.release(packet);
                bbCheck(valid, "post capacity retry FIFO/body mismatch");
                ++count;
            }
            return done.load() && count == expected && ipc.postPendingBytes() == 0;
        }, "post retries finish exactly once");
        jobs.drain();
        bbCheck(result == (waitCapacity ? PROCESS_WORK_POSTED : PROCESS_WORK_POST_CAPACITY), "post result classification");
    }
    catch (...)
    {
        receiver.stop();
        jobs.drain();
        throw;
    }
    ipc.uninitPostThread();
    bbCheck(ipc.configurePostMemory(ProcessWorkMemoryConfig()), "restore post memory budget");
}

void ProcessWorkRetryTests::run()
{
    uint32_t attempts = 0;
    for (uint32_t i = 0; i < 4096; ++i)
    {
        bbCheck(ProcessWorkTime::waitCapacity(attempts) && attempts == i + 1, "capacity fast retry count");
    }
    const uint64_t begin = bbNow();
    for (int32_t i = 0; i < 8; ++i)
    {
        bbCheck(ProcessWorkTime::waitCapacity(attempts) && attempts == 4096, "capacity backoff must stay saturated");
    }
    bbCheck(bbNow() - begin >= 7, "capacity retries did not sleep after threshold");
    for (int32_t mode = 0; mode < 6; ++mode)
    {
        sendCapacity(mode);
    }
    for (int32_t mode = 0; mode < 5; ++mode)
    {
        postCapacity(mode);
    }
}
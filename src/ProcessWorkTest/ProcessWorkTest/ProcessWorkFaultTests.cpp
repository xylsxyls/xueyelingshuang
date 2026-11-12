#include "ProcessWorkCoreTests.h"
#include "ProcessWorkTest.h"
#include "ProcessWorkCrashStreamTests.h"
#include "ProcessWork/ProcessWorkStorage.h"
#include "ProcessWork/ProcessWorkSharedQueue.h"
#include "ProcessWork/ProcessWorkAtomic.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#endif

/** 强制终止本用例创建的子进程，跳过所有析构和库清理以模拟崩溃。
@param [in,out] child 已启动且仍由用例持有的子进程
*/
void CrashOwnedChild(BlackBoxChild& child)
{
    bbCheck(child.poll() < 0, "crash target already exited");
#ifdef _WIN32
    HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(child.pid()));
    bbCheck(process != nullptr, "cannot open owned crash target");
    const BOOL stopped = TerminateProcess(process, 127);
    CloseHandle(process);
    bbCheck(stopped != FALSE, "cannot terminate owned crash target");
#else
    bbCheck(kill(child.pid(), SIGKILL) == 0, "cannot kill owned crash target");
#endif
    bbCheck(child.wait(5000) != 0, "abruptly terminated child exited normally");
}

int runProcessWorkFaultChild(int argc, char** argv)
{
    bbCheck(argc == 5 || argc == 6, "fault child arguments");
    const bool abrupt = argc == 6;
    const int32_t pid = std::atoi(argv[2]);
    const int32_t phase = std::atoi(argv[3]);
    const std::string prefix = argv[4];
    ProcessWorkChannel sender;
    bbCheck(sender.openSender(pid), "fault child cannot open sender");
    SharedMemory directoryMemory("P3D" + bbNumber(pid));
    ProcessWorkSharedDirectory* directory = static_cast<ProcessWorkSharedDirectory*>(directoryMemory.writeWithoutLock());
    bbCheck(directory != nullptr, "fault child directory missing");
    const uint32_t session = static_cast<uint32_t>(ProcessWorkAtomic::load(&directory->m_session));
    const uint64_t identity = directory->m_identity;
    SharedMemory controlMemory(ProcessWorkChannel::objectName('C', pid, identity, session));
    ProcessWorkSharedControl* control = static_cast<ProcessWorkSharedControl*>(controlMemory.writeWithoutLock());
    bbCheck(control != nullptr, "fault child control missing");
    int64_t token = 0;
    bbWait([&token, control]()
    {
        for (size_t i = 0; i < 256; ++i)
        {
            if (ProcessWorkAtomic::load(&control->m_peers[i].m_pid) == bbPid())
            {
                token = ProcessWorkAtomic::load(&control->m_peers[i].m_token);
                if (token > 0)
                {
                    return true;
                }
            }
        }
        return false;
    }, "fault child approved");
    ProcessWorkStorage storage(pid, identity, session, control->m_blockBytes, control->m_initial, false, ProcessWorkMemoryConfig());
    if (phase == 3)
    {
        // 先通过真实发送填满首段；父进程持续消费，正文可复用但已提交单元不会清空。
        for (int32_t i = 0; i < kProcessWorkSegmentCells; ++i)
        {
            bbWait([&sender, i]() { return sender.trySend("x", 1, 1000 + i) == PROCESS_WORK_SENT; }, "tail fixture prelude send");
        }
        bbWait([&storage]()
        {
            return ProcessWorkAtomic::load(&storage.block(1)->m_owner) == 0 &&
                ProcessWorkAtomic::load(&storage.block(2)->m_owner) == 0;
        }, "tail fixture bodies drained");
    }
    ProcessWorkSharedBlock* block = storage.block(1);
    bbCheck(block != nullptr && ProcessWorkAtomic::replace(&block->m_owner, int64_t(0), token), "fault child body reservation failed");
    std::unique_ptr<SharedMemory> body;
    char* data = storage.data(1, static_cast<uint32_t>(ProcessWorkAtomic::load(&block->m_generation)), body);
    bbCheck(data != nullptr, "fault child body map failed");
    const int32_t header[3] = { bbPid(), 7, 1 };
    std::memcpy(data, header, sizeof(header));
    data[12] = 'x';
    ProcessWorkSharedQueue queue(storage, *control);
    uint32_t index = 0;
    if (phase == 2)
    {
        uint32_t hint = 1;
        bbCheck(queue.enqueue(token, 1, control->m_blockBytes <= 48, 2, hint), "fault child publish failed");
    }
    else if (phase != 0)
    {
        bbCheck(queue.reserve(token, 2, 1, index), "fault child node reservation failed");
        if (phase == 3)
        {
            // 模拟本流链接提交后立刻退出，不推进写游标；注入全部留在测试工程。
            ProcessWorkProducerState* producer = queue.producer(token);
            bbCheck(producer != nullptr, "fault child stream missing");
            const int64_t generation = ProcessWorkAtomic::load(&queue.node(index)->m_link);
            ProcessWorkAtomic::store(&queue.node(index)->m_cells[0], generation | 1 | (control->m_blockBytes <= 48 ? 0x1000000 : 0));
            const uint32_t tail = static_cast<uint32_t>(ProcessWorkAtomic::load(&producer->m_shared->m_writeNode));
            ProcessWorkSharedNode* last = queue.node(tail - 1);
            bbCheck(last != nullptr, "fault child last segment missing");
            const int64_t link = ProcessWorkAtomic::load(&last->m_link);
            bbCheck(static_cast<uint32_t>(link) == 0 && ProcessWorkAtomic::replace(&last->m_link, link, link | (index + 1)), "partial publish failed");
        }
    }
    bbWrite(prefix + ".reserved", "1");
    if (phase == 4 || abrupt)
    {
        bbWait([&prefix]() { return bbRead(prefix + ".exit") == "1"; }, "allowed exit", 10000);
    }
    // 正常退出同样留下进程拥有的共享占位；本地映射析构不清除它。
    return 0;
}

void testProcessWorkDeadProducer(int32_t phase, bool abrupt)
{
    ProcessWorkChannel receiver;
    bbCheck(receiver.openReceiver(abrupt ? 32 : 64, 2, 0, 0, 0, 0), "receiver open failed");
    receiver.publish();
    LockFreeQueue<char*> queue;
    Semaphore notification;
    std::atomic<int32_t> full(0);
    const std::string prefix = bbTempPath("-fault-producer");
    BlackBoxChild child;
    std::vector<std::string> arguments;
    arguments.push_back("--fault-producer");
    arguments.push_back(bbNumber(bbPid()));
    arguments.push_back(bbNumber(phase));
    arguments.push_back(prefix);
    if (abrupt)
    {
        arguments.push_back("crash");
    }
    child.start(arguments, prefix + ".log");
    bbWait([&receiver, &queue, &notification, &full, &prefix, &child]()
    {
        receiver.pump(queue, notification, full);
        bbCheck(child.poll() < 0 || bbRead(prefix + ".reserved") == "1", "fault child exited too early");
        return bbRead(prefix + ".reserved") == "1";
    }, "fault child reservation");
    ProcessWorkChannel survivor;
    bbCheck(survivor.openSender(bbPid()), "survivor open failed");
    if (phase == 4)
    {
        bbWait([&receiver, &survivor, &queue, &notification, &full]()
        {
            receiver.pump(queue, notification, full);
            return survivor.trySend("x", 1, 10) == PROCESS_WORK_SENT;
        }, "progress beside paused producer");
        bbCheck(survivor.trySend("x", 1, 11) == PROCESS_WORK_CAPACITY, "live reservation reclaimed");
        bbSleep(100);
        receiver.pump(queue, notification, full);
        char* packet = nullptr;
        bbCheck(queue.pop(&packet) && packet != nullptr, "live producer blocked unrelated message");
        std::unique_ptr<char[]> held(packet);
        int32_t header[3] = {};
        std::memcpy(header, packet, sizeof(header));
        bbCheck(header[0] == bbPid() && header[1] == 10, "wrong message beside paused producer");
        if (!abrupt)
        {
            bbWrite(prefix + ".exit", "1");
        }
    }
    if (abrupt)
    {
        CrashOwnedChild(child);
    }
    else
    {
        bbCheck(child.wait(10000) == 0, "fault child failed: " + bbRead(prefix + ".log"));
    }
    int32_t childDelivered = 0;
    const uint64_t deadline = bbNow() + 5000;
    bool first = false;
    bool second = false;
    while (!first || !second)
    {
        bbCheck(bbNow() < deadline, "dead reservation did not recover");
        receiver.pump(queue, notification, full);
        char* packet = nullptr;
        while (queue.pop(&packet))
        {
            std::unique_ptr<char[]> held(packet);
            int32_t header[3] = {};
            std::memcpy(header, packet, sizeof(header));
            if (header[0] == child.pid() && header[1] == 7)
            {
                bbCheck(header[1] == 7 && header[2] == 1 && packet[12] == 'x', "dead-sender body corrupt");
                ++childDelivered;
            }
            else if (header[0] == child.pid())
            {
                bbCheck(phase == 3 && header[1] >= 1000 && header[1] < 1000 + kProcessWorkSegmentCells && header[2] == 1 && packet[12] == 'x', "tail prelude corrupt");
            }
        }
        // 同轮占满两槽，证明死进程占位已回收，不是仅剩一槽循环使用。
        first = survivor.trySend("x", 1, 20) == PROCESS_WORK_SENT;
        second = first && survivor.trySend("x", 1, 21) == PROCESS_WORK_SENT;
        if (!second)
        {
            bbSleep(20);
        }
    }
    bbCheck(childDelivered == ((phase == 2 || phase == 3) ? 1 : 0), "dead-sender delivery count wrong");
    receiver.pump(queue, notification, full);
    int32_t finalCount = 0;
    char* packet = nullptr;
    while (queue.pop(&packet))
    {
        ++finalCount;
        delete[] packet;
    }
    bbCheck(finalCount == 2, "recovery final capacity/delivery wrong");
    std::remove((prefix + ".reserved").c_str());
    std::remove((prefix + ".exit").c_str());
    std::remove((prefix + ".log").c_str());
}

void testProcessWorkDeadSenderBackpressure()
{
    ProcessWork& ipc = ProcessWork::instance();
    std::atomic<bool> entered(false), release(false);
    PacketLog log;
    log.action = [&entered, &release](const ReceivedPacket& packet)
    {
        if (packet.type == 1)
        {
            entered.store(true);
            bbWait([&release]() { return release.load(); }, "release held callback", 15000);
        }
    };
    IpcScope scope;
    ProcessWorkMemoryConfig memory;
    memory.m_localQueueBytes = 283;
    ipc.addProcessReceiveCallback(&log);
    bbCheck(ipc.setReceiveName(receiveName("dead-busy")) && ipc.initReceive(32, 2, 0, 0, 0, 0, memory), "backpressure crash setup");
    const std::string prefix = bbTempPath("-dead-busy");
    BlackBoxChild child;
    try
    {
        ipc.send(bbPid(), "123456", 6, static_cast<MessageType>(1));
        bbWait([&entered]() { return entered.load(); }, "callback entered");
        std::vector<std::string> arguments;
        arguments.push_back("--fault-producer");
        arguments.push_back(bbNumber(bbPid()));
        arguments.push_back("2");
        arguments.push_back(prefix);
        arguments.push_back("crash");
        child.start(arguments, prefix + ".log");
        bbWait([&child, &prefix]()
        {
            bbCheck(child.poll() < 0, "fault sender exited early");
            return bbRead(prefix + ".reserved") == "1";
        }, "sender committed while local queue full");
        CrashOwnedChild(child);
        SharedMemory directoryMemory("P3D" + bbNumber(bbPid()));
        ProcessWorkSharedDirectory* directory = static_cast<ProcessWorkSharedDirectory*>(directoryMemory.writeWithoutLock());
        bbCheck(directory != nullptr, "directory missing");
        const uint32_t session = static_cast<uint32_t>(ProcessWorkAtomic::load(&directory->m_session));
        ProcessWorkStorage storage(bbPid(), directory->m_identity, session, 32, 2, false, memory);
        ProcessWorkSharedBlock* retained = storage.block(1);
        bbCheck(retained != nullptr, "committed body mapping missing");
        const uint64_t until = bbNow() + 750;
        while (bbNow() < until)
        {
            bbCheck(ProcessWorkAtomic::load(&retained->m_owner) != 0, "dead committed body reclaimed under local backpressure");
            bbSleep(5);
        }
        release.store(true);
        waitPackets(log, 2);
        const std::vector<ReceivedPacket> packets = log.snapshot();
        bbCheck(packets[0].body == "123456" && packets[1].pid == child.pid() && packets[1].type == 7 && packets[1].body == "x",
            "retained message lost, duplicated or corrupted");
        bbWait([&storage]()
        {
            return ProcessWorkAtomic::load(&storage.block(1)->m_owner) == 0 && ProcessWorkAtomic::load(&storage.block(2)->m_owner) == 0;
        }, "both body slots returned");
        scope.close();
    }
    catch (...)
    {
        release.store(true);
        throw;
    }
    std::remove((prefix + ".reserved").c_str());
    std::remove((prefix + ".log").c_str());
}

int runProcessWorkCrashReceiver(int argc, char** argv)
{
    bbCheck(argc == 6, "crash receiver arguments");
    const std::string prefix = argv[2];
    const std::string name = argv[3];
    const bool stalled = std::atoi(argv[4]) != 0;
    const int32_t parent = std::atoi(argv[5]);
    ProcessWork& ipc = ProcessWork::instance();
    PacketLog log;
    StreamReceiver ledger(8, 500, true);
    ledger.configure(std::vector<int>(1, parent));
    IpcScope scope;
    if (stalled)
    {
        log.action = [&prefix](const ReceivedPacket& packet)
        {
            bbCheck(packet.body == "123456", "crash receiver initial body mismatch");
            bbWrite(prefix + ".entered", "1");
            // 故意不释放正在执行的正文，父进程在共享区也占满后直接终止本进程。
            bbWait([&prefix]() { return bbRead(prefix + ".release") == "1"; }, "crash receiver held callback", 30000);
        };
        ProcessWorkMemoryConfig memory;
        memory.m_localQueueBytes = 300;
        ipc.addProcessReceiveCallback(&log);
        bbCheck(ipc.setReceiveName(name) && ipc.initReceive(64, 2, 0, 0, 0, 0, memory), "crash receiver setup failed");
    }
    else
    {
        scope.start(&ledger, name, 64, 128);
    }
    bbWrite(prefix + ".ready", "1");
    if (stalled)
    {
        bbWait([&prefix]() { return bbRead(prefix + ".release") == "1"; }, "crash receiver held main", 30000);
    }
    else
    {
        bbWait([&ledger]() { return ledger.bad.load() != 0 || ledger.received.load() == 4000; }, "restarted receiver deliveries", 20000);
        bbCheck(ledger.bad.load() == 0, "restarted receiver corrupt/duplicate/out of order");
        bbWrite(prefix + ".verified", "4000");
    }
    return 0;
}

void testProcessWorkReceiverCrash()
{
    ProcessWork& ipc = ProcessWork::instance();
    IpcScope scope;
    const std::string name = receiveName("abrupt-receiver");
    const std::string prefix = bbTempPath("-abrupt-receiver");
    BlackBoxChild receiver;
    std::vector<std::string> arguments;
    arguments.push_back("--crash-receiver");
    arguments.push_back(prefix);
    arguments.push_back(name);
    arguments.push_back("1");
    arguments.push_back(bbNumber(bbPid()));
    receiver.start(arguments, prefix + ".log");
    bbWait([&receiver, &prefix]()
    {
        bbCheck(receiver.poll() < 0, "crash receiver exited during setup");
        return bbRead(prefix + ".ready") == "1";
    }, "crash receiver ready");
    sendPacket(1, receiver.pid(), name, "123456", 0);
    bbWait([&prefix]() { return bbRead(prefix + ".entered") == "1"; }, "receiver callback occupied");
    for (int32_t i = 1; i <= 2; ++i)
    {
        sendPacket(0, receiver.pid(), name, "123456", i);
    }
    bbCheck(ipc.trySend(receiver.pid(), "123456", 6) == PROCESS_WORK_CAPACITY, "receiver shared buffer not full before crash");
    std::atomic<int32_t> entered(0), finished(0);
    IpcJobs jobs;
    try
    {
        for (int32_t thread = 0; thread < 8; ++thread)
        {
            jobs.add([&ipc, &receiver, &name, &entered, &finished, thread]()
            {
                int32_t result = PROCESS_WORK_UNAVAILABLE;
                if (thread < 4)
                {
                    ++entered;
                    result = thread % 2 == 0 ? ipc.sendResult(receiver.pid(), "123456", 6, static_cast<MessageType>(10 + thread)) :
                        ipc.sendResult(name, "123456", 6, static_cast<MessageType>(10 + thread));
                }
                else
                {
                    ProcessWorkChannel waiting;
                    bbCheck(waiting.openSender(receiver.pid()), "background waiting channel open failed");
                    std::atomic<bool> stop(false);
                    ++entered;
                    result = waiting.send("123456", 6, 10 + thread, &stop);
                }
                bbCheck(result == PROCESS_WORK_UNAVAILABLE, "receiver crash did not end the waiting transmission");
                ++finished;
            });
        }
        bbWait([&entered]() { return entered.load() == 8; }, "ordinary and background waits entered");
        bbSleep(50);
        bbCheck(finished.load() == 0, "background transmissions did not retain full-buffer wait");
        CrashOwnedChild(receiver);
        bbWait([&finished]() { return finished.load() == 8; }, "receiver crash released all synchronous senders", 5000);
        jobs.drain();
    }
    catch (...)
    {
        if (receiver.poll() < 0)
        {
            CrashOwnedChild(receiver);
        }
        throw;
    }
    bbCheck(ipc.trySend(receiver.pid(), "123456", 6) == PROCESS_WORK_UNAVAILABLE, "dead receiver cache remains usable");
    // 保留名称缓存和旧通道映射，用同名新进程验证身份隔离，不通过clear掩盖问题。
    const std::string restartedPrefix = prefix + "-restart";
    arguments[1] = restartedPrefix;
    arguments[3] = "0";
    BlackBoxChild restarted;
    restarted.start(arguments, restartedPrefix + ".log");
    bbWait([&restarted, &restartedPrefix]()
    {
        bbCheck(restarted.poll() < 0, "restarted receiver exited during setup");
        return bbRead(restartedPrefix + ".ready") == "1";
    }, "restarted same-name receiver ready");
    for (int32_t thread = 0; thread < 8; ++thread)
    {
        jobs.add([&ipc, &name, &restarted, thread]()
        {
            for (int32_t sequence = 0; sequence < 500; ++sequence)
            {
                sendPacket(thread % 2 == 0 ? 1 : 0, restarted.pid(), name, "123456", thread * 500 + sequence);
            }
        });
    }
    jobs.drain();
    bbCheck(restarted.wait(25000) == 0 && bbRead(restartedPrefix + ".verified") == "4000",
        "same-name receiver failed after crash: " + bbRead(restartedPrefix + ".log"));
}
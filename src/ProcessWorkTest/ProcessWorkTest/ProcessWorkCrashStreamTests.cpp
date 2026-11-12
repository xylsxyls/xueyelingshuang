#include "ProcessWorkCrashStreamTests.h"
#include "ProcessWork/ProcessWorkStorage.h"
#include "ProcessWork/ProcessWorkAtomic.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <set>
#include <algorithm>

ProcessWorkCrashLedger::ProcessWorkCrashLedger() : ownCount(0), childCount(0), errors(0)
{

}

void ProcessWorkCrashLedger::receive(int32_t pid, const char* data, int32_t length, MessageType type)
{
    try
    {
        const int32_t value = static_cast<int32_t>(type);
        const int32_t stream = value / 1000000;
        const int32_t sequence = value % 1000000;
        bbCheck(value >= 0 && stream < 8 && length == 6 && data != nullptr && std::memcmp(data, "123456", 6) == 0,
            "crash stream packet fields corrupt");
        int32_t& next = m_next[std::make_pair(pid, stream)];
        bbCheck(sequence == next, "crash stream lost, duplicated or reordered a committed prefix");
        ++next;
    }
    catch (...)
    {
        ++errors;
    }
    if (pid == bbPid())
    {
        ++ownCount;
    }
    else
    {
        ++childCount;
    }
}

int runProcessWorkCrashStream(int argc, char** argv)
{
    bbCheck(argc == 4, "crash stream arguments");
    const int32_t destination = std::atoi(argv[2]);
    const std::string prefix = argv[3];
    IpcJobs jobs;
    for (int32_t stream = 0; stream < 8; ++stream)
    {
        jobs.add([destination, stream]()
        {
            for (int32_t sequence = 0; sequence < 200000; ++sequence)
            {
                sendPacket(0, destination, "", "123456", stream * 1000000 + sequence);
            }
        });
    }
    bbWrite(prefix + ".ready", "1");
    jobs.drain();
    // 正常情况下父用例早已强制终止；留出观察窗口，避免慢父进程误以为自然退出。
    bbWait([&prefix]() { return bbRead(prefix + ".exit") == "1"; }, "await forced sender death", 30000);
    return 0;
}

void testProcessWorkCrashStream()
{
    ProcessWorkCrashLedger ledger;
    IpcScope scope;
    scope.start(&ledger, receiveName("crash-stream"), 32, 16);
    for (int32_t cycle = 0; cycle < 6; ++cycle)
    {
        const int32_t childBefore = ledger.childCount.load();
        const std::string prefix = bbTempPath("-crash-stream-" + bbNumber(cycle));
        BlackBoxChild child;
        std::vector<std::string> arguments;
        arguments.push_back("--crash-stream");
        arguments.push_back(bbNumber(bbPid()));
        arguments.push_back(prefix);
        child.start(arguments, prefix + ".log");
        bbWait([&ledger, &child, &prefix, childBefore]()
        {
            bbCheck(child.poll() < 0 && ledger.errors.load() == 0, "stream child failed before crash");
            return bbRead(prefix + ".ready") == "1" && ledger.childCount.load() >= childBefore + 256;
        }, "child actively sending");
        IpcJobs survivors;
        for (int32_t stream = 0; stream < 8; ++stream)
        {
            survivors.add([stream, cycle]()
            {
                for (int32_t sequence = cycle * 2000; sequence < (cycle + 1) * 2000; ++sequence)
                {
                    sendPacket(0, bbPid(), "", "123456", stream * 1000000 + sequence);
                }
            });
        }
        CrashOwnedChild(child);
        survivors.drain();
        bbWait([&ledger, cycle]()
        {
            bbCheck(ledger.errors.load() == 0, "crash damaged a survivor or sender prefix");
            return ledger.ownCount.load() == (cycle + 1) * 16000;
        }, "all survivor packets delivered", 15000);

        // 读取真实共享协议证明完整资源归还，而不只证明剩余一个槽还能收发。
        SharedMemory directoryMemory("P3D" + bbNumber(bbPid()));
        ProcessWorkSharedDirectory* directory = static_cast<ProcessWorkSharedDirectory*>(directoryMemory.writeWithoutLock());
        bbCheck(directory != nullptr, "crash stream directory missing");
        const uint32_t session = static_cast<uint32_t>(ProcessWorkAtomic::load(&directory->m_session));
        SharedMemory controlMemory(ProcessWorkChannel::objectName('C', bbPid(), directory->m_identity, session));
        ProcessWorkSharedControl* control = static_cast<ProcessWorkSharedControl*>(controlMemory.writeWithoutLock());
        bbCheck(control != nullptr, "crash stream control missing");
        ProcessWorkStorage storage(bbPid(), directory->m_identity, session, 32, 16, false, ProcessWorkMemoryConfig());
        bbWait([&storage, control]()
        {
            std::set<uint32_t> heads;
            const uint32_t count = static_cast<uint32_t>((std::min)(ProcessWorkAtomic::load(&control->m_nextStream), int64_t(0xffffff)));
            for (uint32_t index = 1; index <= count; ++index)
            {
                ProcessWorkSharedStream* stream = storage.stream(index);
                if (stream != nullptr && ProcessWorkAtomic::load(&stream->m_owner) > 0)
                {
                    const uint32_t head = static_cast<uint32_t>(ProcessWorkAtomic::load(&stream->m_head));
                    if (head != 0)
                    {
                        heads.insert(head - 1);
                    }
                }
            }
            for (uint32_t i = 0; i <= 16; ++i)
            {
                if ((i != 0 && ProcessWorkAtomic::load(&storage.block(i)->m_owner) != 0) ||
                    (heads.count(i) == 0 && ProcessWorkAtomic::load(&storage.node(i)->m_owner) != 0))
                {
                    return false;
                }
            }
            for (uint32_t i = 1; i <= count; ++i)
            {
                ProcessWorkSharedStream* stream = storage.stream(i);
                if (stream != nullptr && heads.count(0x1000000U + i) == 0)
                {
                    const int64_t owner = ProcessWorkAtomic::load(&stream->m_first.m_owner);
                    if (owner != 0 && owner != -1)
                    {
                        return false;
                    }
                }
            }
            return true;
        }, "all sixteen bodies and unused segments reclaimed", 10000);
        std::remove((prefix + ".ready").c_str());
        std::remove((prefix + ".log").c_str());
    }
    bbCheck(ledger.ownCount.load() == 96000 && ledger.childCount.load() >= 1536 && ledger.errors.load() == 0,
        "repeated sender crash final ledger mismatch");
    scope.close();
}
#include "ProcessWorkTest.h"
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>

PacketLog::PacketLog() :
m_errors(0), m_fullEvents(0)
{

}

void PacketLog::receive(int32_t pid, const char* data, int32_t length, MessageType type)
{
    try
    {
        bbCheck(length >= 0 && (length == 0 ? data == nullptr : data != nullptr), "callback pointer/length invalid");
        ReceivedPacket packet = {
            pid, static_cast<int>(type), data == nullptr, length ? std::string(data, length) : std::string()};
        if (m_action)
        {
            m_action(packet);
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        m_packets.push_back(packet);
    }
    catch (...)
    {
        ++m_errors;
    }
}

void PacketLog::bufferFull(int32_t count)
{
    if (count <= 0)
    {
        ++m_errors;
    }
    ++m_fullEvents;
}

std::vector<ReceivedPacket> PacketLog::snapshot()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_packets;
}

void ThrowingReceiver::receive(int32_t, const char*, int32_t, MessageType)
{
    throw std::runtime_error("intentional observer exception");
}

IpcScope::IpcScope()
{

}

IpcScope::~IpcScope()
{
    try
    {
        close();
    }
    catch (...)
    {
        bbCleanupFailure("IPC fixture");
    }
}

void IpcScope::start(ProcessReceiveCallback* receiver, const std::string& name, int bytes, int blocks, int threshold,
    int increment, int idle, int rounds)
{
    ProcessWork& ipc = ProcessWork::instance();
    bbCheck(ipc.setReceiveName(name), "setReceiveName failed");
    bbCheck(ipc.addProcessReceiveCallback(receiver), "callback registration failed");
    bbCheck(ipc.initReceive(bytes, blocks, threshold, increment, idle, rounds), "initReceive failed");
    bbCheck(ipc.receivePayloadCapacity() == bytes - 12, "declared payload capacity wrong");
}

void IpcScope::close()
{
    ProcessWork::instance().uninitReceive();
}

IpcJob::IpcJob(const std::function<void()>& body) :
m_failed(false), m_stopped(false), m_body(body)
{

}

void IpcJob::doTask()
{
    try
    {
        m_body();
    }
    catch (const std::exception& error)
    {
        m_failed = true;
        std::cerr << "producer: " << error.what() << "\n";
    }
    catch (...)
    {
        m_failed = true;
    }
}

void IpcJob::stopTask()
{
    m_stopped = true;
}

IpcJobs::~IpcJobs()
{
    for (size_t i = 0; i < m_ids.size(); ++i)
    {
        if (m_ids[i] == 0)
        {
            continue;
        }
        try
        {
            bbCheck(
                CTaskThreadManager::instance().uninit(m_ids[i]).m_status == CTaskThreadOk, "producer cleanup failed");
        }
        catch (...)
        {
            bbCleanupFailure("IPC producer");
        }
    }
}

void IpcJobs::add(const std::function<void()>& body)
{
    std::shared_ptr<IpcJob> task = std::make_shared<IpcJob>(body);
    // 容器分配先完成，随后创建的线程一定能记入清理清单。
    m_ids.reserve(m_ids.size() + 1);
    m_tasks.reserve(m_tasks.size() + 1);
    uint32_t id = CTaskThreadManager::instance().init();
    bbCheck(id != 0, "producer init failed");
    m_ids.push_back(id);
    m_tasks.push_back(task);
    std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
    bbCheck(worker != nullptr, "producer has no interface");
    bbCheck(worker->postTask(task), "producer task not admitted");
}

void IpcJobs::drain()
{
    for (size_t i = 0; i < m_ids.size(); ++i)
    {
        if (m_ids[i] != 0)
        {
            bbCheck(
                CTaskThreadManager::instance().waitForEnd(m_ids[i]).m_status == CTaskThreadOk, "producer join failed");
            m_ids[i] = 0;
        }
    }
    m_ids.clear();
    for (size_t i = 0; i < m_tasks.size(); ++i)
    {
        bbCheck(!m_tasks[i]->m_failed && !m_tasks[i]->m_stopped, "producer failed/stopped");
    }
}

std::string receiveName(const std::string& suffix)
{
    return "pwtest_" + bbNumber(bbPid()) + "_" + suffix;
}

std::string packetBody(int value, int length, bool fixed)
{
    std::string body(length, '\0');
    for (int i = 0; i < length; ++i)
    {
        body[i] = fixed
            ? static_cast<char>('1' + i)
            : static_cast<char>((static_cast<uint32_t>(value) * 17U + static_cast<uint32_t>(i) * 31U + i / 256) % 251U);
    }
    return body;
}

void sendPacket(int mode, int pid, const std::string& name, const std::string& body, int type)
{
    ProcessWork& ipc = ProcessWork::instance();
    MessageType messageType = static_cast<MessageType>(type);
    // 覆盖PID/名称、buffer/string全部重载。mode=4统一post流混用两种寻址。
    if (mode == 0 || mode == 1)
    {
        const bool accepted = mode == 0 ? (type % 2 ? ipc.send(pid, body, messageType)
                                                    : ipc.send(pid, body.empty() ? nullptr : body.data(),
                                                          static_cast<int32_t>(body.size()), messageType))
                                        : (type % 2 ? ipc.send(name, body, messageType)
                                                    : ipc.send(name, body.empty() ? nullptr : body.data(),
                                                          static_cast<int32_t>(body.size()), messageType));
        bbCheck(accepted, "ordinary send must retry contention and capacity internally");
    }
    else if (mode == 2 || (mode == 4 && type % 2 == 0))
    {
        const bool accepted = type % 2
            ? ipc.post(pid, body, messageType)
            : ipc.post(pid, body.empty() ? nullptr : body.data(), static_cast<int32_t>(body.size()), messageType);
        bbCheck(accepted, "ordinary PID post not admitted");
    }
    else if (mode == 3 || mode == 4)
    {
        const bool accepted = type % 2
            ? ipc.post(name, body, messageType)
            : ipc.post(name, body.empty() ? nullptr : body.data(), static_cast<int32_t>(body.size()), messageType);
        bbCheck(accepted, "ordinary name post not admitted");
    }
    else if (mode == 6)
    {
        // 只重试明确未接纳的竞争、容量或可恢复资源结果；POSTED后不重复提交。
        bbWait(
            [&ipc, pid, &name, &body, messageType, type]()
            {
                const int32_t result = type % 2 == 0 ? ipc.tryPost(pid, body.empty() ? nullptr : body.data(),
                                                           static_cast<int32_t>(body.size()), messageType)
                                                     : ipc.tryPost(name, body.empty() ? nullptr : body.data(),
                                                           static_cast<int32_t>(body.size()), messageType);
                bbCheck(result == PROCESS_WORK_POSTED || result == PROCESS_WORK_POST_BUSY ||
                        result == PROCESS_WORK_POST_CAPACITY || result == PROCESS_WORK_POST_RESOURCE,
                    "tryPost permanent failure");
                return result == PROCESS_WORK_POSTED;
            },
            "tryPost bounded retry", 20000);
    }
    else
    {
        bbWait(
            [&ipc, pid, &body, messageType]()
            {
                int result = ipc.trySend(
                    pid, body.empty() ? nullptr : body.data(), static_cast<int32_t>(body.size()), messageType);
                bbCheck(result == PROCESS_WORK_SENT || result == PROCESS_WORK_BUSY || result == PROCESS_WORK_CAPACITY ||
                        result == PROCESS_WORK_RESOURCE,
                    "trySend failed: " + bbNumber(result));
                return result == PROCESS_WORK_SENT;
            },
            "trySend retry");
    }
}

void waitPackets(PacketLog& log, size_t count)
{
    bbWait(
        [&log, count]()
        {
            return log.m_errors != 0 || log.snapshot().size() >= count;
        },
        "receive expected packets", 10000);
    bbCheck(log.m_errors == 0 && log.snapshot().size() == count, "callback failed or unexpected packet count");
}

StreamReceiver::StreamReceiver(int threads, int packets, bool fixed) :
m_received(0), m_bad(0), m_threads(threads), m_packets(packets), m_fixed(fixed), m_excludedPid(0), m_configured(false)
{

}

void StreamReceiver::configure(const std::vector<int>& pids, int32_t excludedPid)
{
    m_pids = pids;
    m_next.assign(pids.size() * m_threads, 0);
    m_excludedPid = excludedPid;
    m_configured.store(true, std::memory_order_release);
}

void StreamReceiver::receive(int32_t pid, const char* data, int32_t length, MessageType type)
{
    try
    {
        bbCheck(m_configured.load(std::memory_order_acquire), "ledger used before publication");
        const int value = static_cast<int>(type), stream = value / m_packets, sequence = value % m_packets;
        const int sizes[] = {0, 1, 6, 64, 4096, 65536};
        bbCheck(value >= 0 && stream < static_cast<int>(m_next.size()), "unknown stream");
        bbCheck(pid != m_excludedPid && pid == m_pids[stream / m_threads] && m_next[stream] == sequence,
            "PID/sequence/duplicate mismatch");
        const int expected = m_fixed ? 6 : sizes[sequence % 6];
        bbCheck(length == expected && (length ? data != nullptr : data == nullptr), "length/null mismatch");
        const std::string body = packetBody(value, expected, m_fixed);
        bbCheck(!length || std::memcmp(data, body.data(), length) == 0, "payload corrupted");
        ++m_next[stream];
    }
    catch (...)
    {
        ++m_bad;
    }
    ++m_received;
}

void testProcessWorkStreamLedger()
{
    // 模拟两个来源；不创建真实PID，直接验证账本自身的拒绝能力。
    const std::vector<int> pids = { 101, 202 };
    for (int32_t scenario = 0; scenario < 5; ++scenario)
    {
        StreamReceiver ledger(1, 2, true);
        ledger.configure(pids, 101);
        const int32_t pid = scenario == 1 ? 101 : (scenario == 2 ? 303 : 202);
        const int32_t first = scenario == 1 ? 0 : (scenario == 4 ? 3 : 2);
        ledger.receive(pid, "123456", 6, static_cast<MessageType>(first));
        ledger.receive(pid, "123456", 6, static_cast<MessageType>(scenario == 3 ? first : first + 1));
        bbCheck(ledger.m_received == 2 && (ledger.m_bad == 0) == (scenario == 0),
            "mesh ledger accepted misrouting, duplicate or out-of-order delivery");
    }
    StreamReceiver self(1, 2, true);
    self.configure(std::vector<int>(1, 101));
    self.receive(101, "123456", 6, static_cast<MessageType>(0));
    self.receive(101, "123456", 6, static_cast<MessageType>(1));
    bbCheck(self.m_received == 2 && self.m_bad == 0, "explicit self-send ledger was rejected");
}

PublicChannelScope::PublicChannelScope(int initial, int threshold, int increment, int idle, int rounds,
    const ProcessWorkMemoryConfig& memory) :
m_full(0)
{
    bbCheck(m_receiver.openReceiver(76, initial, threshold, increment, idle, rounds, memory), "open m_receiver failed");
    m_receiver.publish();
    bbCheck(m_sender.openSender(bbPid()), "open m_sender failed");
    pump(); // 公共接收维护批准发送方身份。
}

PublicChannelScope::~PublicChannelScope()
{
    m_sender.close();
    m_receiver.stop();
    m_receiver.close();
    char* packet = nullptr;
    while (m_queue.pop(&packet))
    {
        delete[] packet;
    }
}

int PublicChannelScope::pump()
{
    int count = m_receiver.pump(m_queue, m_notification, m_full);
    char* packet = nullptr;
    while (m_queue.pop(&packet))
    {
        std::unique_ptr<char[]> owner(packet);
        int32_t header[3] = {};
        std::memcpy(header, packet, sizeof(header));
        bbCheck(header[0] == bbPid() && header[2] == 1 && packet[12] == 'x', "channel packet damaged");
        m_delivered.push_back(header[1]);
    }
    return count;
}

int runIpcNode(int argc, char** argv)
{
    try
    {
        bbCheck(argc == 10, "node argument count");
        const std::string prefix = argv[2];
        const int node = std::atoi(argv[3]), nodes = std::atoi(argv[4]), threads = std::atoi(argv[5]);
        const int packets = std::atoi(argv[6]), mode = std::atoi(argv[7]), initial = std::atoi(argv[9]);
        const bool fixed = std::atoi(argv[8]) != 0;
        bbCheck(node >= 0 && node < nodes && nodes <= 8 && threads > 0 && threads <= 32 && packets > 0 &&
                packets <= 1000000,
            "invalid node configuration");
        StreamReceiver ledger(threads, packets, fixed);
        IpcScope scope;
        const std::string name = "mesh_" + prefix.substr(prefix.find_last_of("/\\") + 1) + "_" + bbNumber(node);
        scope.start(&ledger, name, 65548, initial, initial == 8 ? 2 : 0, initial == 8 ? 4 : 0, 1, initial == 8 ? 3 : 0);
        bbWrite(prefix + ".ready" + bbNumber(node), bbNumber(bbPid()));
        bbWait(
            [&prefix]()
            {
                return !bbRead(prefix + ".pids").empty();
            },
            "mesh PID barrier", 20000);
        std::istringstream input(bbRead(prefix + ".pids"));
        std::vector<int> pids(nodes, 0);
        for (int i = 0; i < nodes; ++i)
        {
            bbCheck(static_cast<bool>(input >> pids[i]) && pids[i] > 0, "invalid PID list");
        }
        ledger.configure(pids, bbPid());
        bbWrite(prefix + ".armed" + bbNumber(node), bbNumber(sizeof(void*) * 8));
        bbWait(
            [&prefix]()
            {
                return bbRead(prefix + ".go") == "1";
            },
            "mesh start barrier", 20000);
        ProcessWork& ipc = ProcessWork::instance();
        if ((mode >= 2 && mode <= 4) || mode == 6)
        {
            bbCheck(ipc.initPostThread(), "post worker initialization failed");
        }
        IpcJobs producers;
        for (int thread = 0; thread < threads; ++thread)
        {
            producers.add(
                [=]()
                {
                    const int sizes[] = {0, 1, 6, 64, 4096, 65536};
                    for (int sequence = 0; sequence < packets; ++sequence)
                    {
                        int value = (node * threads + thread) * packets + sequence;
                        std::string body = packetBody(value, fixed ? 6 : sizes[sequence % 6], fixed);
                        for (int peer = 0; peer < nodes; ++peer)
                        {
                            if (peer == node)
                            {
                                continue;
                            }
                            const std::string peerName =
                                "mesh_" + prefix.substr(prefix.find_last_of("/\\") + 1) + "_" + bbNumber(peer);
                            sendPacket(mode, pids[peer], peerName, body, value);
                        }
                    }
                });
        }
        producers.drain();
        if ((mode >= 2 && mode <= 4) || mode == 6)
        {
            ipc.waitForPostEnd();
        }
        const int expected = (nodes - 1) * threads * packets;
        bbWait(
            [&ledger, expected]()
            {
                return ledger.m_received >= expected || ledger.m_bad > 0;
            },
            "mesh deliveries", 20000);
        bbCheck(ledger.m_received == expected && ledger.m_bad == 0, "mesh ledger mismatch");
        bbWrite(prefix + ".done" + bbNumber(node), "1");
        bbWait(
            [&prefix]()
            {
                return bbRead(prefix + ".finish") == "1";
            },
            "mesh finish barrier", 20000);
        scope.close();
        bbCheck(ledger.m_received == expected && ledger.m_bad == 0, "extra late packets");
        bbCheck(ipc.receiveBufferCount() == 0 && ipc.receivePayloadCapacity() == 0, "shutdown snapshot invalid");
        bbCheckCleanup();
        std::cout << "m_received=" << ledger.m_received << " expected=" << expected << " m_bad=" << ledger.m_bad
                  << "\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "node: " << error.what() << "\n";
        return 1;
    }
    catch (...)
    {
        std::cerr << "node: unknown exception\n";
        return 1;
    }
}

void meshTest(int mode, bool fixed, int packets, int nodes, int threads, int initial)
{
    const std::string prefix = bbTempPath("-mesh");
    std::vector<std::shared_ptr<BlackBoxChild>> children;
    for (int node = 0; node < nodes; ++node)
    {
        std::vector<std::string> args;
        args.push_back("--node");
        args.push_back(prefix);
        args.push_back(bbNumber(node));
        args.push_back(bbNumber(nodes));
        args.push_back(bbNumber(threads));
        args.push_back(bbNumber(packets));
        args.push_back(bbNumber(mode));
        args.push_back(fixed ? "1" : "0");
        args.push_back(bbNumber(initial));
        children.push_back(std::make_shared<BlackBoxChild>());
        const char* peerExecutable = std::getenv("PROCESS_WORK_TEST_PEER");
        if (node % 2 != 0 && peerExecutable != nullptr && peerExecutable[0] != '\0')
        {
            // 测试进程显式指定另一位数的同版Test，交替创建真实通信节点。
            children.back()->startProgram(peerExecutable, args, prefix + ".log" + bbNumber(node));
        }
        else
        {
            children.back()->start(args, prefix + ".log" + bbNumber(node));
        }
    }
    try
    {
        std::string pids;
        for (int node = 0; node < nodes; ++node)
        {
            bbWait(
                [&prefix, &children, node]()
                {
                    bbCheck(children[node]->poll() < 0, "peer exited before READY");
                    return bbRead(prefix + ".ready" + bbNumber(node)) == bbNumber(children[node]->pid());
                },
                "peer READY", 20000);
            pids += bbNumber(children[node]->pid()) + " ";
        }
        // 完整文件先落盘，再通过单独标志发布，避免节点看到半截PID列表。
        bbWrite(prefix + ".pids.tmp", pids);
        bbCheck(
            std::rename((prefix + ".pids.tmp").c_str(), (prefix + ".pids").c_str()) == 0, "publish PID file failed");
        for (int node = 0; node < nodes; ++node)
        {
            bbWait(
                [&prefix, &children, node]()
                {
                    bbCheck(children[node]->poll() < 0, "peer exited before ARMED");
                    const std::string bits = bbRead(prefix + ".armed" + bbNumber(node));
                    if (bits != "32" && bits != "64")
                    {
                        return false;
                    }
                    const char* peer = std::getenv("PROCESS_WORK_TEST_PEER");
                    const bool mixedPeer = node % 2 != 0 && peer != nullptr && peer[0] != '\0';
                    bbCheck((bits != bbNumber(sizeof(void*) * 8)) == mixedPeer, "peer pointer width mismatch");
                    return true;
                },
                "peer ARMED", 20000);
        }
        bbWrite(prefix + ".go", "1");
        for (int node = 0; node < nodes; ++node)
        {
            bbWait(
                [&prefix, &children, node]()
                {
                    bbCheck(children[node]->poll() < 0, "peer exited before DELIVERED");
                    return bbRead(prefix + ".done" + bbNumber(node)) == "1";
                },
                "peer DELIVERED", 45000);
        }
        bbWrite(prefix + ".finish", "1");
        for (int node = 0; node < nodes; ++node)
        {
            bbCheck(children[node]->wait(10000) == 0, "peer verification failed");
        }
        for (int node = 0; node < nodes; ++node)
        {
            std::cout << "peer " << node << " " << bbRead(prefix + ".log" + bbNumber(node));
            const char* suffixes[] = {".ready", ".armed", ".done", ".log"};
            for (int i = 0; i < 4; ++i)
            {
                std::remove((prefix + suffixes[i] + bbNumber(node)).c_str());
            }
        }
        std::remove((prefix + ".pids").c_str());
        std::remove((prefix + ".go").c_str());
        std::remove((prefix + ".finish").c_str());
    }
    catch (...)
    {
        for (int node = 0; node < nodes; ++node)
        {
            std::cerr << "peer log " << prefix << ".log" << node << "\n" << bbRead(prefix + ".log" + bbNumber(node));
        }
        throw;
    }
}
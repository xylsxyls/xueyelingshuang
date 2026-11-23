#include "ProcessWorkFairnessTests.h"
#include "ProcessWorkCrashStreamTests.h"
#include "ProcessWorkRetryTests.h"
#include "ProcessWorkTest.h"
#include "ProcessWorkTestAutomation.h"
#include "ProcessWorkCompatibilityTests.h"
#include "ProcessWorkBenchmark.h"
#include "ProcessWorkCoreTests.h"
#include "ProcessWorkMemoryTests.h"
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <Windows.h>

/** 输出测试进程未处理的原生异常，不改变生产库的异常处理
@param [in] exception Windows提供的异常记录和寄存器上下文
@return EXCEPTION_EXECUTE_HANDLER，保留异常退出状态并终止当前测试子进程
*/
static LONG WINAPI ReportNativeTestFailure(EXCEPTION_POINTERS* exception)
{
    if (exception != nullptr && exception->ExceptionRecord != nullptr)
    {
        char message[256] = {};
        const int32_t length = wsprintfA(message, "native exception code=0x%08X address=%p image=%p\r\n",
            exception->ExceptionRecord->ExceptionCode, exception->ExceptionRecord->ExceptionAddress,
            GetModuleHandleW(nullptr));
        DWORD written = 0;
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), message, static_cast<DWORD>(length), &written, nullptr);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

/** 用合成异常记录验证诊断文本，并验证父进程识别子进程失败退出状态
*/
static void TestNativeFailureDiagnostic()
{
#ifdef _WIN32
    const std::string log = bbTempPath("-native-diagnostic.log");
    BlackBoxChild child;
    child.start(std::vector<std::string>(1, "--native-diagnostic-check"), log);
    bbCheck(child.wait(10000) == 125 && bbRead(log).find("native exception code=0xE0420001") != std::string::npos,
        "native crash exit or diagnostic lost");

#endif
}

/** 验证空对象重复停止、排空及清理均不产生接收状态
*/
static void EmptyLifecycle()
{
    ProcessWork& ipc = ProcessWork::instance();
    IpcScope scope;
    bbCheck(ipc.receiveBufferCount() == 0 && ipc.receivePayloadCapacity() == 0, "fresh snapshot not empty");
    ipc.uninitPostThread();
    ipc.waitForPostEnd();
    ipc.clear();
    ipc.clearUseless();
    ipc.clearDestPid(0);
    scope.close();
    scope.close();
    bbCheck(ipc.receiveBufferCount() == 0, "empty lifecycle changed capacity");
}

/** 验证非法初始化逐项拒绝且不损坏随后合法初始化
*/
static void InvalidInit()
{
    PacketLog log;
    IpcScope scope;
    ProcessWork& ipc = ProcessWork::instance();
    ipc.addProcessReceiveCallback(&log);
    const int invalid[][6] = {{11, 1, 0, 0, 0, 0}, {-1, 1, 0, 0, 0, 0}, {INT_MAX, 1, 0, 0, 0, 0}, {64, 0, 0, 0, 0, 0},
        {64, 0x1000000, 0, 0, 0, 0}, {64, 1, -1, 0, 0, 0}, {64, 1, 0, -1, 0, 0}, {64, 1, 0, 0, -1, 0},
        {64, 1, 0, 0, 0, -1}, {64, 1, 0, 1, 0, 0}, {64, 1, 0, 0, 0, 1}, {64, 1, 0, 1, 0, -2},
        {64, 1, 0, INT_MAX, 0, INT_MAX}};
    bbCheck(ipc.initPostThread(), "post worker initialization failed");
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        const int* p = invalid[i];
        bbCheck(!ipc.initReceive(p[0], p[1], p[2], p[3], p[4], p[5]), "invalid init accepted row " + bbNumber(i));
        bbCheck(ipc.receiveBufferCount() == 0 && ipc.receivePayloadCapacity() == 0, "invalid init leaked readiness");
    }
    bbCheck(ipc.setReceiveName(receiveName("retry")) && ipc.initReceive(64, 2), "valid retry failed");
    bbCheck(ipc.post(bbPid(), std::string("retry"), static_cast<MessageType>(1)), "retry post rejected");
    ipc.waitForPostEnd();
    waitPackets(log, 1);
    bbCheck(log.snapshot()[0].m_body == "retry", "failed init lost callback or existing post state");
}

/** 验证名称长度、NUL及接收期间重命名边界
*/
static void NameBounds()
{
    PacketLog log;
    IpcScope scope;
    ProcessWork& ipc = ProcessWork::instance();
    bbCheck(!ipc.setReceiveName(std::string(1024, 'a')), "1024-byte name accepted");
    bbCheck(!ipc.setReceiveName(std::string("a\0b", 3)), "embedded NUL name accepted");
    bbCheck(ipc.setReceiveName(""), "empty default-name reset rejected");
    const std::string name = receiveName("long") + std::string(1023 - receiveName("long").size(), 'z');
    scope.start(&log, name, 64, 2);
    bbCheck(!ipc.setReceiveName("renamed"), "live receiver renamed");
    sendPacket(1, bbPid(), name, "name", 1);
    waitPackets(log, 1);
    bbCheck(log.snapshot()[0].m_body == "name", "long-name lookup corrupted");
}

/** 验证重复初始化拒绝且保留原通道容量
*/
static void DuplicateInit()
{
    PacketLog log;
    IpcScope scope;
    scope.start(&log, receiveName("duplicate"), 128, 2);
    ProcessWork& ipc = ProcessWork::instance();
    bbCheck(!ipc.initReceive(1024, 16), "duplicate init accepted");
    bbCheck(ipc.receiveBufferCount() == 2 && ipc.receivePayloadCapacity() == 116, "duplicate init damaged receiver");
    sendPacket(0, bbPid(), "", "still-alive", 1);
    waitPackets(log, 1);
}

/** 验证接口重载、正文副本及FIFO
@param [in] mode PID/名称send或post模式
*/
static void LocalTransfer(int mode)
{
    PacketLog log;
    IpcScope scope;
    const std::string name = receiveName("transfer");
    scope.start(&log, name);
    ProcessWork& ipc = ProcessWork::instance();
    if (mode >= 2 && mode <= 4)
    {
        bbCheck(ipc.initPostThread(), "post worker initialization failed");
    }
    const int sizes[] = {0, 1, 6, 64, 4096, 65536, 0, 65536};
    for (int i = 0; i < 8; ++i)
    {
        std::string body = packetBody(i, sizes[i]);
        sendPacket(mode, bbPid(), name, body, i);
        body.assign(body.size(), '!'); // post必须拥有副本；调用者立即复用原内存。
    }
    if (mode >= 2 && mode <= 4)
    {
        ipc.waitForPostEnd();
    }
    waitPackets(log, 8);
    scope.close();
    const std::vector<ReceivedPacket> packets = log.snapshot();
    bbCheck(packets.size() == 8 && log.m_errors == 0, "late extra packet");
    for (int i = 0; i < 8; ++i)
    {
        bbCheck(packets[i].m_pid == bbPid() && packets[i].m_type == i && packets[i].m_body == packetBody(i, sizes[i]) &&
                packets[i].m_nullData == (sizes[i] == 0),
            "FIFO/PID/type/body mismatch at " + bbNumber(i));
    }
}

/** 验证trySend非法参数及精确容量边界
*/
static void InvalidSend()
{
    PacketLog log;
    IpcScope scope;
    scope.start(&log, receiveName("invalid"), 76, 2);
    ProcessWork& ipc = ProcessWork::instance();
    bbCheck(ipc.trySend(0, "x", 1) == PROCESS_WORK_INVALID, "zero PID accepted");
    bbCheck(ipc.trySend(-1, "x", 1) == PROCESS_WORK_INVALID, "negative PID accepted");
    bbCheck(ipc.trySend(bbPid(), nullptr, 1) == PROCESS_WORK_INVALID, "null body accepted");
    bbCheck(ipc.trySend(bbPid(), "x", -1) == PROCESS_WORK_INVALID, "negative length accepted");
    bbCheck(
        ipc.trySend(bbPid(), "x", 1, static_cast<MessageType>(-1)) == PROCESS_WORK_INVALID, "negative type accepted");
    const std::string oversized(65, 'x');
    bbCheck(ipc.trySend(bbPid(), oversized.data(), 65) == PROCESS_WORK_TOO_LARGE, "capacity+1 not rejected");
    sendPacket(5, bbPid(), "", std::string(64, 'x'), INT_MAX);
    waitPackets(log, 1);
    bbCheck(log.snapshot()[0].m_type == INT_MAX && log.snapshot()[0].m_body.size() == 64,
        "exact capacity/highest type failed");
}

/** 验证普通接口拒绝非法请求，异步远端拒绝项不产生交付
*/
static void InvalidVoidSend()
{
    PacketLog log;
    IpcScope scope;
    const std::string name = receiveName("invalid_void");
    scope.start(&log, name, 64, 2);
    ProcessWork& ipc = ProcessWork::instance();
    bbCheck(ipc.initPostThread(), "post worker initialization failed");
    bbCheck(!ipc.send(bbPid(), nullptr, 1) && !ipc.send(name, "x", -1) &&
            !ipc.send(bbPid(), "x", 1, static_cast<MessageType>(-1)),
        "invalid send accepted");
    bbCheck(!ipc.send(0, "x") && !ipc.send(std::string(), "x"), "invalid send destination accepted");
    bbCheck(!ipc.post(bbPid(), nullptr, 1) && !ipc.post(name, "x", -1) &&
            !ipc.post(bbPid(), "x", 1, static_cast<MessageType>(-1)),
        "invalid post accepted");
    bbCheck(!ipc.post(0, "x") && !ipc.post(std::string(), "x"), "invalid post destination accepted");
    bbCheck(!ipc.send(bbPid(), std::string(53, 'x')), "remote oversized send accepted");
    // Post只确认本地接纳；对端包长限制由后台发送检查，不能错误断言本地投递失败。
    bbCheck(ipc.post(name, std::string(53, 'x')), "locally valid post rejected");
    bbCheck(ipc.post(bbPid(), std::string("barrier"), static_cast<MessageType>(77)), "barrier rejected");
    ipc.waitForPostEnd();
    waitPackets(log, 1);
    bbSleep(30);
    scope.close();
    bbCheck(
        log.snapshot().size() == 1 && log.snapshot()[0].m_body == "barrier", "invalid void call delivered a packet");
}

/** 验证仅空正文通道及空指针回调契约
*/
static void ZeroCapacity()
{
    PacketLog log;
    IpcScope scope;
    scope.start(&log, receiveName("zero"), 12, 1);
    ProcessWork& ipc = ProcessWork::instance();
    bbCheck(ipc.receivePayloadCapacity() == 0 && ipc.receiveBufferCount() == 1, "empty-only receiver not ready");
    bbCheck(ipc.trySend(bbPid(), "x", 1) == PROCESS_WORK_TOO_LARGE, "empty-only receiver accepted body");
    sendPacket(5, bbPid(), "", "", 0);
    waitPackets(log, 1);
    bbCheck(log.snapshot()[0].m_nullData, "zero body was not nullptr");
}

/** 验证多个回调顺序及抛异常观察者不阻止其他回调
*/
static void Callbacks()
{
    PacketLog first, last;
    ThrowingReceiver throwing;
    std::atomic<int> phase(0);
    first.m_action = [&phase](const ReceivedPacket& p)
    {
        bbCheck(phase == p.m_type * 2, "first observer order");
        ++phase;
    };
    last.m_action = [&phase](const ReceivedPacket& p)
    {
        bbCheck(phase == p.m_type * 2 + 1, "last observer order");
        ++phase;
    };
    IpcScope scope;
    ProcessWork& ipc = ProcessWork::instance();
    ipc.addProcessReceiveCallback(&first);
    ipc.addProcessReceiveCallback(nullptr);
    ipc.addProcessReceiveCallback(&throwing);
    scope.start(&last, receiveName("Callbacks"), 64, 2);
    for (int i = 0; i < 5; ++i)
    {
        sendPacket(0, bbPid(), "", "x", i);
    }
    waitPackets(first, 5);
    waitPackets(last, 5);
    bbCheck(phase == 10 && first.m_errors == 0 && last.m_errors == 0, "observer exception stopped later observers");
}

/** 验证回调内异步投递随后完整交付
*/
static void CallbackReentry()
{
    PacketLog log;
    log.m_action = [](const ReceivedPacket& packet)
    {
        if (packet.m_type == 1)
        {
            ProcessWork::instance().post(bbPid(), std::string("response"), static_cast<MessageType>(2));
        }
    };
    IpcScope scope;
    scope.start(&log, receiveName("reentry"), 64, 2);
    bbCheck(ProcessWork::instance().initPostThread(), "post worker initialization failed");
    sendPacket(0, bbPid(), "", "request", 1);
    waitPackets(log, 2);
    ProcessWork::instance().waitForPostEnd();
    bbCheck(log.snapshot()[1].m_type == 2 && log.snapshot()[1].m_body == "response", "callback post failed");
}

/** 验证不同缓存清理入口后仍可发送完整消息
*/
static void ClearCaches()
{
    PacketLog log;
    IpcScope scope;
    const std::string name = receiveName("cache");
    scope.start(&log, name, 128, 4);
    ProcessWork& ipc = ProcessWork::instance();
    for (int i = 0; i < 8; ++i)
    {
        sendPacket(i % 2, bbPid(), name, packetBody(i, 64), i);
        waitPackets(log, i + 1);
        if (i % 3 == 0)
        {
            ipc.clearDestPid(bbPid());
        }
        else if (i % 3 == 1)
        {
            ipc.clearUseless();
        }
        else
        {
            ipc.clear();
        }
    }
    bbCheck(log.snapshot().size() == 8 && ipc.receiveBufferCount() == 4, "cache clearing damaged receiver");
}

/** 验证同PID新接收代次隔离旧通道和旧回调
*/
static void RestartReceiver()
{
    PacketLog first, second;
    IpcScope scope;
    const std::string name = receiveName("restart");
    scope.start(&first, name, 64, 2);
    sendPacket(1, bbPid(), name, "old", 1);
    waitPackets(first, 1);
    ProcessWorkChannel oldSender;
    bbCheck(oldSender.openSender(bbPid()), "old channel open failed");
    scope.close();
    bbCheck(!oldSender.ready() && oldSender.trySend("x", 1, 1) == PROCESS_WORK_UNAVAILABLE,
        "old generation remained writable");
    scope.start(&second, name, 128, 3);
    sendPacket(1, bbPid(), name, std::string(116, 'n'), 2);
    waitPackets(second, 1);
    bbCheck(first.snapshot().size() == 1 && second.snapshot()[0].m_body.size() == 116,
        "restart retained Callbacks or stale capacity");
    bbCheck(!oldSender.ready(), "old channel silently rebound to new generation");
}

/** 验证异步发送排空、重启及跨阶段FIFO
*/
static void PostDrainRestart()
{
    PacketLog log;
    IpcScope scope;
    const std::string name = receiveName("post_restart");
    scope.start(&log, name, 128, 4);
    ProcessWork& ipc = ProcessWork::instance();
    bbCheck(ipc.initPostThread(), "post worker initialization failed");
    bbCheck(ipc.initPostThread(), "post worker initialization failed");
    for (int round = 0; round < 3; ++round)
    {
        for (int i = 0; i < 20; ++i)
        {
            sendPacket(4, bbPid(), name, packetBody(round * 20 + i, 64), round * 20 + i);
        }
        ipc.waitForPostEnd();
        waitPackets(log, (round + 1) * 20);
    }
    ipc.uninitPostThread();
    ipc.uninitPostThread();
    bbCheck(ipc.initPostThread(), "post worker initialization failed");
    sendPacket(2, bbPid(), name, "new", 60);
    ipc.waitForPostEnd();
    waitPackets(log, 61);
    const std::vector<ReceivedPacket> packets = log.snapshot();
    for (int i = 0; i < 61; ++i)
    {
        bbCheck(packets[i].m_type == i, "post drain/restart reordered messages");
    }
}

/** 验证接收器独占所有权、发布和撤销可见性
*/
static void ChannelOwnership()
{
    ProcessWorkChannel first, second, sender;
    bbCheck(first.openReceiver(76, 2, 0, 0, 0, 0), "prepare receiver failed");
    bbCheck(!sender.openSender(bbPid()), "unpublished receiver visible");
    first.publish();
    bbCheck(!second.openReceiver(76, 2, 0, 0, 0, 0), "second owner accepted");
    bbCheck(sender.openSender(bbPid()) && sender.ready() && sender.payloadCapacity() == 64,
        "published receiver not available");
    second.close();
    bbCheck(sender.ready(), "failed owner close destroyed first owner");
    first.stop();
    bbCheck(!sender.ready(), "stop did not revoke readiness");
    sender.close();
    first.close();
    first.close();
    bbCheck(second.openReceiver(76, 2, 0, 0, 0, 0), "ownership not released after close");
    second.close();
}

/** 验证满载拒绝不提交及槽位归还后的FIFO
*/
static void ChannelFull()
{
    PublicChannelScope channel(2, 0, 0, 0, 0);
    for (int i = 0; i < 2; ++i)
    {
        bbCheck(channel.m_sender.trySend("x", 1, i) == PROCESS_WORK_SENT, "initial free slot unavailable");
    }
    for (int i = 0; i < 100; ++i)
    {
        bbCheck(channel.m_sender.trySend("x", 1, 2) == PROCESS_WORK_CAPACITY, "full channel did not return CAPACITY");
    }
    channel.pump();
    bbCheck(channel.m_delivered.size() == 2 && channel.m_delivered[0] == 0 && channel.m_delivered[1] == 1,
        "BUSY accidentally committed / FIFO damaged");
    bbCheck(channel.m_sender.trySend("x", 1, 2) == PROCESS_WORK_SENT, "freed capacity not reusable");
    channel.pump();
    bbCheck(channel.m_delivered.size() == 3 && channel.m_delivered[2] == 2, "retry was lost or duplicated");
    bbCheck(channel.m_receiver.bufferCount() == 2, "fixed capacity changed");
}

/** 验证扩容、最大容量及空闲缩容
@param [in] shrink true允许空闲缩容
@param [in] threshold 剩余块扩容阈值
*/
static void ChannelResize(bool shrink, int threshold)
{
    PublicChannelScope channel(2, threshold, 2, shrink ? 1 : 0, 2);
    int sequence = 0;
    // 暂停pump填满；BUSY只表示本次未提交，先维护再原序重试。
    for (int round = 0; round < 3; ++round)
    {
        const int capacity = channel.m_receiver.bufferCount();
        bbWait(
            [&channel, capacity, &sequence]()
            {
                // pump可能先释放报文、下次才做维护；持续制造真实满载，而非假定一次pump必扩容。
                for (int i = 0; i < capacity; ++i)
                {
                    bbCheck(channel.m_sender.trySend("x", 1, sequence++) == PROCESS_WORK_SENT, "fill capacity failed");
                }
                bbCheck(channel.m_sender.trySend("x", 1, sequence) == PROCESS_WORK_CAPACITY,
                    "full resize channel accepted extra packet");
                channel.pump();
                return channel.m_receiver.bufferCount() > capacity || capacity == 6;
            },
            "growth maintenance", 4000);
    }
    bbCheck(channel.m_receiver.bufferCount() == 6, "growth did not respect configured maximum");
    for (size_t i = 0; i < channel.m_delivered.size(); ++i)
    {
        bbCheck(channel.m_delivered[i] == static_cast<int>(i), "resize lost/reordered message");
    }
    bbCheck(channel.m_delivered.size() == static_cast<size_t>(sequence), "resize lost submitted messages");
    if (shrink)
    {
        bbWait(
            [&channel]()
            {
                channel.pump();
                return channel.m_receiver.bufferCount() == 2;
            },
            "idle shrink to initial", 6000);
    }
    else
    {
        const uint64_t until = bbNow() + 1300;
        while (bbNow() < until)
        {
            channel.pump();
            bbSleep(10);
        }
        bbCheck(channel.m_receiver.bufferCount() == 6, "idleSeconds=0 unexpectedly shrank");
    }
    bbCheck(channel.m_sender.trySend("x", 1, sequence) == PROCESS_WORK_SENT, "channel unusable after resize");
    channel.pump();
    bbCheck(channel.m_delivered.back() == sequence, "post-resize payload missing");
}

/** 重复初始化、通信与关闭，检查托管线程和共享状态回收
*/
static void LifecycleStress()
{
    for (int round = 0; round < 30; ++round)
    {
        PacketLog log;
        IpcScope scope;
        scope.start(&log, receiveName("life"), 128, 4);
        ProcessWork& ipc = ProcessWork::instance();
        bbCheck(ipc.initPostThread(), "post worker initialization failed");
        for (int i = 0; i < 20; ++i)
        {
            sendPacket(2, bbPid(), "", packetBody(i, 64), i);
        }
        ipc.waitForPostEnd();
        waitPackets(log, 20);
        scope.close();
        bbCheck(CTaskThreadManager::instance().count() == 0 && ipc.receiveBufferCount() == 0,
            "IPC left managed threads/resources ready");
    }
}

/** 验证目标退出后的同步失败及异步清理
*/
static void DeadPeer()
{
    BlackBoxChild child;
    std::vector<std::string> args(1, "--exit-peer");
    child.start(args, bbTempPath("-dead-peer.log"));
    bbCheck(child.wait(5000) == 0, "dead-peer setup failed");
    ProcessWork& ipc = ProcessWork::instance();
    IpcScope scope;
    bbCheck(ipc.trySend(child.pid(), "x", 1) == PROCESS_WORK_UNAVAILABLE, "exited peer considered available");
    ipc.send(child.pid(), "x");
    bbCheck(ipc.initPostThread(), "post worker initialization failed");
    ipc.post(child.pid(), "x");
    ipc.waitForPostEnd();
}

/** 验证清缓存与同步发送或后台前缀借用并发时的完整交付
@param [in] post true使用异步PID/名称混合投递，false保留同步发送场景
*/
static void ConcurrentClear(bool post)
{
    const int threads = 4, packets = 300;
    StreamReceiver ledger(threads, packets, false);
    ledger.configure(std::vector<int>(1, bbPid()));
    IpcScope scope;
    const std::string name = receiveName("concurrent_clear");
    scope.start(&ledger, name);
    if (post)
    {
        bbCheck(ProcessWork::instance().initPostThread(), "post worker initialization failed");
    }
    IpcJobs jobs;
    for (int thread = 0; thread < threads; ++thread)
    {
        jobs.add(
            [thread, name, post]()
            {
                const int sizes[] = {0, 1, 6, 64, 4096, 65536};
                for (int i = 0; i < 300; ++i)
                {
                    const int value = thread * 300 + i;
                    sendPacket((post ? 2 : 0) + i % 2, bbPid(), name, packetBody(value, sizes[i % 6]), value);
                }
            });
    }
    jobs.add(
        []()
        {
            for (int i = 0; i < 300; ++i)
            {
                ProcessWork::instance().clearDestPid(bbPid());
                ProcessWork::instance().clear();
                ProcessWork::instance().clearUseless();
            }
        });
    jobs.drain();
    if (post)
    {
        ProcessWork::instance().waitForPostEnd();
    }
    bbWait(
        [&ledger]()
        {
            return ledger.m_received >= 1200 || ledger.m_bad > 0;
        },
        "concurrent cache clear deliveries", 10000);
    scope.close();
    bbCheck(ledger.m_received == 1200 && ledger.m_bad == 0, "cache clear raced with send: loss/duplicate/corruption");
}

/** 参与同名接收器同时启动，校验定向正文以及仅可能交付一次的名称正文
@param [in] name 四个进程共用的名称
@param [in] prefix 本子进程独有的屏障路径前缀
@param [in] gate 启动前等待的公共屏障路径
@param [in] parent 预期的发送进程号
@return 0表示核对完成，其他错误由外层测试入口报告
*/
static int RunConcurrentName(const std::string& name, const std::string& prefix,
    const std::string& gate, int32_t parent)
{
    PacketLog log;
    IpcScope scope;
    bbWrite(prefix + ".armed", "1");
    bbWait([&gate]() { return bbRead(gate) == "1"; }, "release concurrent receivers");
    scope.start(&log, name, 32, 4);
    bbWrite(prefix + ".ready", "1");
    bbWait([&log]()
    {
        const std::vector<ReceivedPacket> messages = log.snapshot();
        return !messages.empty() && messages.back().m_type == 3;
    }, "concurrent name end marker");
    scope.close();
    const std::vector<ReceivedPacket> messages = log.snapshot();
    bbCheck((messages.size() == 2 || messages.size() == 3) && messages.front().m_body == "pid" &&
        messages.front().m_type == 1 && messages.back().m_body == "end", "incorrect PID delivery");
    for (size_t i = 0; i < messages.size(); ++i)
    {
        bbCheck(messages[i].m_pid == parent, "unexpected concurrent name sender");
    }
    if (messages.size() == 3)
    {
        bbCheck(messages[1].m_type == 2 && messages[1].m_body == "name", "wrong name delivery");
    }
    bbWrite(prefix + ".owner", messages.size() == 3 ? "1" : "0");
    return 0;
}

/** 四个已就绪进程同时初始化同名接收器，所有PID须可达且名称只交付给其中一个
*/
static void ConcurrentName()
{
    const std::string name = receiveName("concurrent-name");
    const std::string path = bbTempPath("-name");
    const std::string gate = path + ".go";
    std::vector<std::shared_ptr<BlackBoxChild>> peers;
    for (int32_t i = 0; i < 4; ++i)
    {
        const std::string prefix = path + bbNumber(i);
        std::shared_ptr<BlackBoxChild> peer(new BlackBoxChild);
        peer->start(std::vector<std::string>{ "--concurrent-name", name, prefix, gate, bbNumber(bbPid()) }, prefix + ".log");
        peers.push_back(peer);
    }
    // 等待每个子进程发布当前阶段，提前退出时立即报告失败。
    const auto waitAll = [&](const std::string& stage)
    {
        for (size_t i = 0; i < peers.size(); ++i)
        {
            const std::string file = path + bbNumber(i) + stage;
            const std::string description = "concurrent name barrier " + stage;
            bbWait([&]()
            {
                bbCheck(peers[i]->poll() < 0, "receiver exited before " + stage);
                return bbRead(file) == "1";
            }, description.c_str());
        }
    };
    waitAll(".armed");
    bbWrite(gate, "1");
    waitAll(".ready");
    for (size_t i = 0; i < peers.size(); ++i)
    {
        sendPacket(0, peers[i]->pid(), "", "pid", 1);
    }
    sendPacket(1, 0, name, "name", 2);
    for (size_t i = 0; i < peers.size(); ++i)
    {
        sendPacket(0, peers[i]->pid(), "", "end", 3);
    }
    size_t owners = 0;
    for (size_t i = 0; i < peers.size(); ++i)
    {
        const std::string prefix = path + bbNumber(i);
        bbCheck(peers[i]->wait(10000) == 0, "concurrent name peer failed: " + prefix + ".log");
        const std::string owner = bbRead(prefix + ".owner");
        bbCheck(owner == "0" || owner == "1", "missing name delivery ledger");
        owners += owner == "1" ? 1 : 0;
        std::remove((prefix + ".owner").c_str());
        std::remove((prefix + ".armed").c_str());
        std::remove((prefix + ".ready").c_str());
        std::remove((prefix + ".log").c_str());
    }
    std::remove(gate.c_str());
    bbCheck(owners == 1, "name delivery missing or sent to multiple receivers");
}

/** 验证同名接收器冲突不夺取活跃名称且仍可按PID通信
*/
static void NameCollision()
{
    PacketLog log;
    IpcScope scope;
    const std::string name = receiveName("exclusive");
    scope.start(&log, name, 64, 2);
    BlackBoxChild child;
    std::vector<std::string> args;
    const std::string ready = bbTempPath("-same-name.ready");
    args.push_back("--share-name");
    args.push_back(name);
    args.push_back(bbNumber(bbPid()));
    args.push_back(ready);
    const std::string output = bbTempPath("-name-collision.log");
    child.start(args, output);
    bbWait(
        [&child, &ready]()
        {
            bbCheck(child.poll() < 0, "same-name peer exited before ready");
            return bbRead(ready) == "1";
        },
        "same-name peer ready");
    waitPackets(log, 1);
    bbCheck(log.snapshot()[0].m_pid == child.pid() && log.snapshot()[0].m_body == "name-owner",
        "name moved away from first live owner");
    sendPacket(0, child.pid(), "", "by-pid", 2);
    const int result = child.wait(10000);
    bbCheck(result == 0, "same-name peer failed; log=" + output + ": " + bbRead(output));
    sendPacket(1, bbPid(), name, "owner", 3);
    waitPackets(log, 2);
    bbCheck(log.snapshot()[1].m_body == "owner", "second receiver close damaged first owner's name");
    std::remove(ready.c_str());
    scope.close();
    ConcurrentName();
}

/** 验证1024个初始块配置与空正文收发
*/
static void MaximumBlocks()
{
    PacketLog log;
    IpcScope scope;
    scope.start(&log, receiveName("max_blocks"), 12, 1024);
    bbCheck(ProcessWork::instance().receiveBufferCount() == 1024, "maximum block configuration rejected/truncated");
    sendPacket(5, bbPid(), "", "", 1);
    waitPackets(log, 1);
}

/** 验证满载接收器停止后等待退出且后续发送失败
*/
static void CloseFullChannel()
{
    PublicChannelScope channel(1, 0, 0, 0, 0);
    bbCheck(channel.m_sender.trySend("x", 1, 0) == PROCESS_WORK_SENT, "fill single slot failed");
    bbCheck(channel.m_sender.trySend("x", 1, 1) == PROCESS_WORK_CAPACITY, "single slot not full");
    channel.m_receiver.stop();
    bbCheck(!channel.m_sender.waitForSpace(), "closed receiver left sender waiting for space");
    bbCheck(channel.m_sender.trySend("x", 1, 1) == PROCESS_WORK_UNAVAILABLE, "closed full receiver accepted send");
}

std::vector<BlackBoxCase> processCases()
{
    std::vector<BlackBoxCase> cases;
    const std::function<void(const char*, const char*, bool, std::function<void()>)> add =
        [&cases](const char* name, const char* expected, bool stress, std::function<void()> run)
    {
        BlackBoxCase c = {static_cast<int>(cases.size() + 1), name, expected, stress, stress ? 150 : 75, [run]()
            {
                run();
                bbCheck(CTaskThreadManager::instance().count() == 0, "case retained managed threads");
            }};
        cases.push_back(c);
    };
    add("empty-lifecycle", "empty/repeated cleanup safe; snapshots zero", false, EmptyLifecycle);
    add("invalid-init-retry", "13 invalid configurations rejected; callback/post preserved", false, InvalidInit);
    add("name-bounds", "NUL/1024 rejected; 1023 accepted; live rename rejected", false, NameBounds);
    add("duplicate-init", "repeat init rejected without damaging live receiver", false, DuplicateInit);
    add("pid-send", "all buffer/string/empty/binary/exact-capacity packets in FIFO", false,
        []()
        {
            LocalTransfer(0);
        });
    add("name-send", "name resolution plus FIFO and full byte verification", false,
        []()
        {
            LocalTransfer(1);
        });
    add("pid-post-copy", "post owns buffer; source mutation cannot change delivery", false,
        []()
        {
            LocalTransfer(2);
        });
    add("name-post-copy", "name post owns buffer; all lengths delivered", false,
        []()
        {
            LocalTransfer(3);
        });
    add("mixed-post-fifo", "name/PID post share one ordered stream", false,
        []()
        {
            LocalTransfer(4);
        });
    add("try-send-contract", "INVALID/TOO_LARGE exact; boundary payload/type accepted", false, InvalidSend);
    add("invalid-void-send", "invalid sync/async calls produce no packets", false, InvalidVoidSend);
    add("zero-payload-capacity", "12-byte block accepts only empty packet", false, ZeroCapacity);
    add("callback-order-isolation", "registration order; null skipped; exception isolated", false, Callbacks);
    add("callback-reentry", "callback can post a response without deadlock", false, CallbackReentry);
    add("clear-caches", "clear/clearDestPid/clearUseless retain receiver usability", false, ClearCaches);
    add("receiver-restart", "old generation unavailable; new capacity/callback/name valid", false, RestartReceiver);
    add("post-drain-restart", "drain/recreate three times; no loss or reordering", false, PostDrainRestart);
    add("channel-publication-ownership", "not visible before publish; one owner; safe failed close", false,
        ChannelOwnership);
    add("channel-busy-retry", "full Capacity commits nothing; retry delivered once", false, ChannelFull);
    add("grow-shrink-zero-threshold", "grow on full demand; cap6; idle returns to initial2", false,
        []()
        {
            ChannelResize(true, 0);
        });
    add("grow-no-shrink", "idleSeconds=0 retains expanded capacity", false,
        []()
        {
            ChannelResize(false, 0);
        });
    add("grow-positive-threshold", "positive threshold supported; shrink floor is initial", false,
        []()
        {
            ChannelResize(true, 1);
        });
    add("mesh-pid-send", "3 real processes x 2 streams; ledger PID/order/body exact", false,
        []()
        {
            meshTest(0, false, 48, 3, 2);
        });
    add("mesh-name-send", "3 real processes; all-to-all name send", false,
        []()
        {
            meshTest(1, false, 48, 3, 2);
        });
    add("mesh-pid-post", "3 real processes; all-to-all PID post", false,
        []()
        {
            meshTest(2, false, 48, 3, 2);
        });
    add("mesh-name-post", "3 real processes; all-to-all name post", false,
        []()
        {
            meshTest(3, false, 48, 3, 2);
        });
    add("mesh-mixed-post", "mixed name/PID post FIFO across real processes", false,
        []()
        {
            meshTest(4, false, 48, 3, 2);
        });
    add("mesh-fixed-six", "exact 123456 payload, 128 initial shared blocks", false,
        []()
        {
            meshTest(4, true, 100, 3, 2, 128);
        });
    add("dead-target", "known exited child unavailable; send/post do not hang", false, DeadPeer);
    add("lifecycle-stress", "30 init/drain/close cycles; no managed threads retained", true, LifecycleStress);
    add("mesh-send-stress", "3x4 producers; 1000 variable packets per stream/peer", true,
        []()
        {
            meshTest(0, false, 1000, 3, 4);
        });
    add("mesh-post-stress", "3x4 producers; 1000 mixed post packets per stream/peer", true,
        []()
        {
            meshTest(4, false, 1000, 3, 4);
        });
    add("mesh-try-send-stress", "BUSY retry with 3x4 streams and byte ledger", true,
        []()
        {
            meshTest(5, false, 1000, 3, 4);
        });
    add("concurrent-cache-clear", "4 senders plus cache clearer; all1200 messages verified", true,
        []()
        {
            ConcurrentClear(false);
        });
    add("name-collision", "same-name peer works by PID; name stays first owner after peer close", false, NameCollision);
    add("maximum-blocks", "1024-block protocol boundary accepts empty payload", false, MaximumBlocks);
    add("close-full-channel", "stopped receiver returns unavailable; no stranded space wait", false, CloseFullChannel);
    add("callback-self-close", "self wait retains borrowed callback data until owner closes", false,
        testProcessWorkSelfClose);
    add("shared-atomic-publication", "10000 handoffs preserve full 64-bit payload; failed CAS leaves target", false,
        testProcessWorkAtomics);
    add("memory-pressure-guard", "invalid budgets and overflowing/reserve-exhausted requests rejected", false,
        testProcessWorkMemoryGuard);
    add("paged-storage", "concurrent page installation; 256/65536 boundaries; retired mapping generation isolation",
        false, testProcessWorkStorage);
    add("shared-fifo-interruption", "unpublished nodes do not block; stale CAS fails; consumer helps producer tail",
        false, testProcessWorkSharedQueue);
    add("shared-fifo-reuse-stress", "8 producers x 50000 entries; independent FIFO ledger without duplicates", true,
        testProcessWorkSharedQueueStress);
    add("unlimited-growth", "threshold0 waits for failed request; capacity1024 to1152 to1280; full FIFO ledger", false,
        testProcessWorkUnlimited);
    add("growth-memory-budget", "impossible reserve rejected; one-block maintenance budget; eventual configured cap",
        false, testProcessWorkGrowthBudget);
    add("dead-producer-body", "dead unpublished body reclaimed; both slots remain usable", false,
        []()
        {
            testProcessWorkDeadProducer(0);
        });
    add("dead-producer-node", "dead unpublished body and queue node reclaimed", false,
        []()
        {
            testProcessWorkDeadProducer(1);
        });
    add("dead-producer-committed", "committed body delivered once after sender exit", false,
        []()
        {
            testProcessWorkDeadProducer(2);
        });
    add("dead-producer-tail", "linked body delivered despite sender exit before tail advance", false,
        []()
        {
            testProcessWorkDeadProducer(3);
        });
    add("live-producer-paused",
        "paused reservation retained; other producer progresses; dead reservation then reclaimed", false,
        []()
        {
            testProcessWorkDeadProducer(4);
        });
    add("local-backlog-budget", "in-flight fixed budget, oversized rejection, failure rollback and pressure rejection",
        false, testProcessWorkLocalBudget);
    add("local-notification-stress",
        "8x5000 messages through a three-packet budget, FIFO ledger and zero retained charge", true,
        testProcessWorkLocalQueueStress);
    add("post-admission-budget",
        "unstarted queue returns Capacity for over-budget copies; failed calls commit nothing; drain returns charges",
        false, testProcessWorkPostBudget);
    add("slow-callback-backpressure",
        "blocked callback retains one local packet; shared capacity bounds backlog; FIFO recovery", false,
        testProcessWorkSlowCallback);
    add("corrupt-channel-closed",
        "invalid committed index or header closes channel without corrupt delivery or retry duplication", false,
        testProcessWorkCorruption);
    add("resource-plateau",
        "20 warmup plus100 lifecycle cycles; no charged packets/workers; Windows handles and private memory plateau",
        true, testProcessWorkResourcePlateau);
    add("mesh-16-producers", "3x16 producers,960000 verified deliveries,per-stream FIFO and fixed payload", true,
        []()
        {
            meshTest(0, true, 10000, 3, 16, 128);
        });
    add("mesh-32-producers", "2x32 producers,960000 verified deliveries,per-stream FIFO and fixed payload", true,
        []()
        {
            meshTest(0, true, 15000, 2, 32, 128);
        });
    add("mesh-post-backpressure",
        "72000 variable packets through bounded tryPost retry;accepted messages never retried or lost", true,
        []()
        {
            meshTest(6, false, 3000, 3, 4, 128);
        });
    add("ordinary-post-contention",
        "32x5000 ordinary posts with literal123456, no caller retry or exception, exact FIFO ledger", true,
        testProcessWorkPostContention);
    add("crash-producer-body", "abrupt process death after body reservation; full capacity recovered", false,
        []()
        {
            testProcessWorkDeadProducer(0, true);
        });
    add("crash-producer-node", "abrupt process death after node reservation; full capacity recovered", false,
        []()
        {
            testProcessWorkDeadProducer(1, true);
        });
    add("crash-producer-committed", "abrupt process death after commit; exactly one delivery and full recovery", false,
        []()
        {
            testProcessWorkDeadProducer(2, true);
        });
    add("crash-producer-tail", "abrupt process death between link and tail; committed message preserved", false,
        []()
        {
            testProcessWorkDeadProducer(3, true);
        });
    add("crash-paused-producer", "live paused owner retained; later killed owner reclaimed", false,
        []()
        {
            testProcessWorkDeadProducer(4, true);
        });
    add("crash-receiver-restart",
        "full trySend returns Capacity; eight ordinary/background transmissions released by receiver death; same-name "
        "restart verifies4000",
        false, testProcessWorkReceiverCrash);
    add("receive-batch-budget",
        "1000 pool reuses; exact per-packet charges, source independence, large exact-limit packet and pressure "
        "failure",
        false, testProcessWorkReceiveBatches);
    add("receive-batch-stress",
        "20000 variable0/6/64/65536-byte packets, concurrent producer/consumer, full byte FIFO ledger", true,
        testProcessWorkReceiveBatchStress);
    add("inline-boundary-reuse", "single shared slot alternates36/37/6/0/68-byte payloads without stale reads", false,
        testProcessWorkInlineBoundary);
    add("repeated-stream-sender-crash",
        "six forced process deaths during ordinary send,96000 survivor packets,exact123456,FIFO and all slots "
        "reclaimed",
        true, testProcessWorkCrashStream);
    add("borrowed-channel-cache",
        "no shared_ptr copy,reentrant deletion retains borrowed value,callback invoked at most once", false,
        testProcessWorkBorrowedCache);
    add("dead-sender-local-backpressure",
        "killed sender committed body retained through blocked callback and maintenance,then delivered exactly once",
        false, testProcessWorkDeadSenderBackpressure);
    add("fair-single-slot-progress",
        "32 simultaneous six-byte senders,one shared slot,six active observation windows,per-stream progress and exact "
        "final FIFO",
        true,
        []()
        {
            testProcessWorkFairProgress(false);
        });
    add("fair-mixed-late-resume",
        "32 small/64KiB streams,eight shared slots,late join and pause/resume,every active stream progresses and final "
        "FIFO matches",
        true,
        []()
        {
            testProcessWorkFairProgress(true);
        });
    add("result-contracts",
        "ordinary admission never reports Busy; capacity/resource distinguished; successful send/post preserve payload "
        "and FIFO",
        false, testProcessWorkResultContracts);
    add("new-stream-round", "new ready stream is found in first poll after an old empty/dead stream round boundary",
        false, testProcessWorkNewStreamRound);
    add("mapping-failure-return",
        "missing body mapping returns resource failure, frees reservation and permits exactly one restored send", false,
        testProcessWorkMappingFailure);
    add("send-failure-kinds",
        "pending registration is Resource; full buffer is Capacity; neither is CAS Busy; failed attempts do not commit",
        false, testProcessWorkSendFailureKinds);
    add("retry-options-capacity",
        "four retry combinations, ordinary equivalence, cancellation, exact delivery and4096 then1ms", false,
        ProcessWorkRetryTests::run);
    add("send-cache-lifetime",
        "in-flight clear, nested fallback, stale epoch and eight colliding cache writers reclaim all ownership", false,
        testProcessWorkSendCache);
    add("receive-batch-reserve", "idle preallocation independent from backlog quota, exact boundaries and FIFO reuse",
        false, testProcessWorkReceiveReserve);
    add("post-packet-reserve",
        "small packet ownership, full pool fallback, boundary sizes, concurrent reuse and budget rollback", true,
        testProcessWorkPostPacketPool);
    add("free-slots-with-waiters",
        "unused node capacity stays available; returned bodies hand off in FIFO and cannot be stolen", false,
        testProcessWorkFreeSlotsWithWaiters);
    add("post-prefix-cache-clear",
        "4 mixed PID/name post producers plus cache clearer preserve1200 messages and FIFO during prefix borrow", true,
        []()
        {
            ConcurrentClear(true);
        });
    add("sender-address-cache",
        "two channels alternate on one physical sender across pages,segments,reuse and reopen;28000 exact FIFO "
        "messages",
        false, testProcessWorkSenderAddressCache);
    add("receiver-page-reuse",
        "same receiver object reopens513/257/3 slots;3092 exact FIFO messages cross pages and reused mappings", false,
        testProcessWorkReceiverPageReuse);
    add("receive-copy-boundary-guard",
        "0..129-byte bodies placed at both inaccessible page boundaries;260 exact copies,NUL,ownership and budget",
        false, testProcessWorkReceiveCopyBoundary);
    add("dynamic-local-budget",
        "fixed quota boundaries,unlimited byte-accounting bypass,lifetime checks and impossible-request termination",
        false, testProcessWorkFixedBudget);
    add("component-pool-configuration",
        "default unlimited,actual post/receive growth,native configuration rejection and task startup rollback", false,
        testProcessWorkMemoryConfiguration);
    add("native-growth-notification", "actual Queue/Map/Hash growth,initial exclusion and observer exception isolation",
        false, testProcessWorkNativeGrowth);
    add("packet-pool-dynamic-growth", "finite cache cap,heap fallback,32 producers progress and exact ownership", true,
        testProcessWorkPacketGrowth);
    add("shared-growth-notification", "shared strategy clipping,actual growth notification and exact delivery", false,
        testProcessWorkSharedGrowth);
    add("reservation-window-capacity",
        "1/63/64/65/129/255/256/257/511/512/513 slots,exact capacity then rejection,4 reuse cycles,10504 FIFO messages",
        false, testProcessWorkReservationWindow);
    add("automation-self-test", "strict parsing,23 workload preservation,median and failed child detection", false,
        ProcessWorkTestAutomation::selfTest);
    add("payload-capacity-restart", "65536 to1 to0 body capacity,stale mapping revoked,exact delivery", false,
        ProcessWorkCompatibilityTests::payloadRestart);
    add("message-type-domain", "five routes,55 valid boundary types and15 rejected negative calls,exact bytes", false,
        ProcessWorkCompatibilityTests::typeDomain);
    add("guarded-source-input", "five routes,28 lengths,two guard boundaries,280 copies survive source revocation",
        false, ProcessWorkCompatibilityTests::guardedInput);
    add("overload-rejection-fifo",
        "eight overloads,four capacities,1023-byte name,four producers,16384 valid deliveries amid rejected requests",
        true, ProcessWorkCompatibilityTests::overloadRejection);
    add("configuration-copy-failure",
        "configuration copy exception returns false; same channel retries and delivers exact payload", false,
        testProcessWorkConfigurationCopyFailure);
    add("prepare-send-mappings",
        "metadata/body preparation,invalid prefixes,no implicit messages,send/post payload fidelity", false,
        testProcessWorkPrepareSend);
    add("granted-body-page", "513x64B and64x64KiB,eight producers,32000 messages,full-pool resume preserves FIFO/body",
        true, testProcessWorkGrantedPage);
    add("prepared-sender-crash",
        "100 successful public sends remain deliverable after producer is killed before pumping", false,
        testProcessWorkPreparedSenderCrash);
    add("segment-locality",
        "30000 immediate consumes reuse one metadata page;16000 backlog entries cross pages without FIFO loss", false,
        testProcessWorkSegmentLocality);
    add("idle-grant-notification",
        "late capacity request merged with old notification must be granted before receiver waits;exact64KiB delivery",
        false, testProcessWorkIdleGrant);
    add("sender-token-reuse",
        "three sessions with distinct approvals;cached sender must use current token and deliver exactly24 messages",
        false, testProcessWorkSenderTokenReuse);
#ifdef _WIN32
    add("native-failure-diagnostic", "synthetic exception record preserves its code and the child exits unsuccessfully", false,
        TestNativeFailureDiagnostic);
    add("post-limited-cpu", "single CPU,eight producers,three progress windows,exact FIFO ledger and complete drain",
        true, testProcessWorkPostLimitedCpu);
#else
    add("native-failure-diagnostic", "Windows exception diagnostics", false, TestNativeFailureDiagnostic);
    cases.back().m_unavailableReason = "requires Windows exception records";
    add("post-limited-cpu", "Windows process affinity on one CPU", true, std::function<void()>());
    cases.back().m_unavailableReason = "requires Windows process affinity";
#endif
    add("stream-ledger-negative-control", "misrouted self delivery,wrong PID,duplicate and out-of-order messages rejected",
        false, testProcessWorkStreamLedger);
    add("dependency-options", "independent native queue and semaphore settings; rejection and recovery with verified delivery",
        false, testProcessWorkDependencyOptions);
    add("sender-mapping-reserve", "custom sender protection rejects pre-mapping only; reconfigure restores it and both sends arrive",
        false, testProcessWorkMappingReserve);
    add("receive-batch-pool", "record size, storage ceiling, idle reuse, large record recovery and public send boundary",
        false, testProcessWorkReceiveBatchPool);
    add("independent-local-limits", "frozen receive and Post limits independently control admission, charge and delivery",
        false, testProcessWorkIndependentLimits);
#ifdef CTTM_TEST_SINGLE_PROCESS
    // 签名应用不能自行启动测试子进程。保留原编号，由设备宿主记录未执行。
    const int childCases[] = { 23, 24, 25, 26, 27, 28, 29, 31, 32, 33, 35, 46, 47, 48, 49, 50,
        57, 58, 59, 61, 62, 63, 64, 65, 66, 70, 72, 88, 94, 102 };
    for (size_t i = 0; i < sizeof(childCases) / sizeof(childCases[0]); ++i)
    {
        cases[childCases[i] - 1].m_unavailableReason = "requires a separately launchable IPC peer";
    }
#endif
    return cases;
}

/** 分派测试子进程、诊断或完整回归入口
@param [in] argc 命令行参数数量
@param [in] argv 命令行参数，子进程模式均由测试驱动生成
@return 0通过，非0表示用例或测试设施失败
*/
int main(int argc, char** argv)
{
#ifdef _WIN32
    SetUnhandledExceptionFilter(ReportNativeTestFailure);
    if (argc == 2 && std::string(argv[1]) == "--native-diagnostic-check")
    {
        EXCEPTION_RECORD record = {};
        record.ExceptionCode = 0xE0420001UL;
        EXCEPTION_POINTERS diagnostic = { &record, nullptr };
        ReportNativeTestFailure(&diagnostic);
        return static_cast<int>(record.ExceptionCode);
    }
#endif
    try
    {
        bbCheck(PrepareProcessWorkTestDirectory(), "mobile IPC directory is missing or inaccessible");
        if (argc == 4 && std::string(argv[1]) == "--prepared-sender")
        {
            return runProcessWorkPreparedSender(static_cast<int32_t>(bbUnsigned(argv[2], INT32_MAX)), argv[3]);
        }
        if (argc > 1 && std::string(argv[1]) == "--tool")
        {
            return ProcessWorkTestAutomation::run(argc - 2, argv + 2);
        }
        if (argc > 1 && std::string(argv[1]) == "--benchmark")
        {
            return RunProcessWorkBenchmark(argc - 1, argv + 1);
        }
        if ((argc == 11 && std::string(argv[1]) == "--producer") ||
            (argc == 7 && argv[1][0] >= '0' && argv[1][0] <= '9'))
        {
            return RunProcessWorkBenchmark(argc, argv);
        }
        if (argc == 2 && std::string(argv[1]) == "--automation-fail")
        {
            return 17;
        }
        if (argc == 2 && std::string(argv[1]) == "--notification-failure")
        {
            return runProcessWorkNotificationFailure();
        }
        if (argc == 3 && std::string(argv[1]) == "--quota-probe")
        {
            return runProcessWorkQuotaProbe(static_cast<int32_t>(bbUnsigned(argv[2], INT32_MAX)));
        }
        if (argc == 2 && std::string(argv[1]) == "--runtime-handle-control")
        {
            runProcessWorkRuntimeControl();
            return 0;
        }
        if (argc > 1 && std::string(argv[1]) == "--node")
        {
            return runIpcNode(argc, argv);
        }
        if (argc > 1 && std::string(argv[1]) == "--fault-producer")
        {
            return runProcessWorkFaultChild(argc, argv);
        }
        if (argc > 1 && std::string(argv[1]) == "--crash-receiver")
        {
            return runProcessWorkCrashReceiver(argc, argv);
        }
        if (argc > 1 && std::string(argv[1]) == "--crash-stream")
        {
            return runProcessWorkCrashStream(argc, argv);
        }
        if (argc == 2 && std::string(argv[1]) == "--exit-peer")
        {
            return 0;
        }
        if (argc == 6 && std::string(argv[1]) == "--concurrent-name")
        {
            return RunConcurrentName(argv[2], argv[3], argv[4], static_cast<int32_t>(bbUnsigned(argv[5], INT32_MAX)));
        }
        if (argc == 5 && std::string(argv[1]) == "--share-name")
        {
            PacketLog log;
            IpcScope scope;
            scope.start(&log, argv[2], 64, 2);
            sendPacket(1, 0, argv[2], "name-owner", 1);
            bbWrite(argv[4], "1");
            waitPackets(log, 1);
            bbCheck(log.snapshot()[0].m_pid == static_cast<int32_t>(bbUnsigned(argv[3], INT32_MAX)) && log.snapshot()[0].m_body == "by-pid",
                "same-name peer not independently addressable");
            scope.close();
            return 0;
        }
        if (argc > 1 && std::string(argv[1]) == "--cases")
        {
            char selection[] = "--tests";
            argv[1] = selection;
            return bbRun(argc, argv, "ProcessWorkTest", processCases());
        }
        return bbRun(argc, argv, "ProcessWorkTest", processCases());
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << "\n";
        return 1;
    }
    catch (...)
    {
        std::cerr << "unknown setup exception\n";
        return 1;
    }
}
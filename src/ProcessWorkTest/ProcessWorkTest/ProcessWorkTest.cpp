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

static void emptyLifecycle()
{
    ProcessWork& ipc = ProcessWork::instance();
    IpcScope scope;
    bbCheck(ipc.receiveBufferCount() == 0 && ipc.receivePayloadCapacity() == 0, "fresh snapshot not empty");
    ipc.uninitPostThread(); ipc.waitForPostEnd(); ipc.clear(); ipc.clearUseless(); ipc.clearDestPid(0);
    scope.close(); scope.close();
    bbCheck(ipc.receiveBufferCount() == 0, "empty lifecycle changed capacity");
}
static void invalidInit()
{
    PacketLog log;
    IpcScope scope;
    ProcessWork& ipc = ProcessWork::instance();
    ipc.addProcessReceiveCallback(&log);
    const int invalid[][6] = {
        { 11, 1, 0, 0, 0, 0 }, { -1, 1, 0, 0, 0, 0 }, { INT_MAX, 1, 0, 0, 0, 0 },
        { 64, 0, 0, 0, 0, 0 }, { 64, 0x1000000, 0, 0, 0, 0 }, { 64, 1, -1, 0, 0, 0 },
        { 64, 1, 0, -1, 0, 0 }, { 64, 1, 0, 0, -1, 0 }, { 64, 1, 0, 0, 0, -1 },
        { 64, 1, 0, 1, 0, 0 }, { 64, 1, 0, 0, 0, 1 }, { 64, 1, 0, 1, 0, -2 },
        { 64, 1, 0, INT_MAX, 0, INT_MAX }
    };
    ipc.initPostThread();
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        const int* p = invalid[i];
        bbCheck(!ipc.initReceive(p[0], p[1], p[2], p[3], p[4], p[5]), "invalid init accepted row " + bbNumber(i));
        bbCheck(ipc.receiveBufferCount() == 0 && ipc.receivePayloadCapacity() == 0, "invalid init leaked readiness");
    }
    bbCheck(ipc.setReceiveName(receiveName("retry")) && ipc.initReceive(64, 2), "valid retry failed");
    ipc.post(bbPid(), std::string("retry"), static_cast<MessageType>(1)); ipc.waitForPostEnd();
    waitPackets(log, 1);
    bbCheck(log.snapshot()[0].body == "retry", "failed init lost callback or existing post state");
}
static void nameBounds()
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
    sendPacket(1, bbPid(), name, "name", 1); waitPackets(log, 1);
    bbCheck(log.snapshot()[0].body == "name", "long-name lookup corrupted");
}
static void duplicateInit()
{
    PacketLog log;
    IpcScope scope;
    scope.start(&log, receiveName("duplicate"), 128, 2);
    ProcessWork& ipc = ProcessWork::instance();
    bbCheck(!ipc.initReceive(1024, 16), "duplicate init accepted");
    bbCheck(ipc.receiveBufferCount() == 2 && ipc.receivePayloadCapacity() == 116, "duplicate init damaged receiver");
    sendPacket(0, bbPid(), "", "still-alive", 1); waitPackets(log, 1);
}
static void localTransfer(int mode)
{
    PacketLog log;
    IpcScope scope;
    const std::string name = receiveName("transfer");
    scope.start(&log, name);
    ProcessWork& ipc = ProcessWork::instance();
    if (mode >= 2 && mode <= 4) ipc.initPostThread();
    const int sizes[] = { 0, 1, 6, 64, 4096, 65536, 0, 65536 };
    for (int i = 0; i < 8; ++i)
    {
        std::string body = packetBody(i, sizes[i]);
        sendPacket(mode, bbPid(), name, body, i);
        body.assign(body.size(), '!'); // post必须拥有副本；调用者立即复用原内存。
    }
    if (mode >= 2 && mode <= 4) ipc.waitForPostEnd();
    waitPackets(log, 8);
    scope.close();
    const std::vector<ReceivedPacket> packets = log.snapshot();
    bbCheck(packets.size() == 8 && log.errors == 0, "late extra packet");
    for (int i = 0; i < 8; ++i)
        bbCheck(packets[i].pid == bbPid() && packets[i].type == i && packets[i].body == packetBody(i, sizes[i]) &&
            packets[i].nullData == (sizes[i] == 0), "FIFO/PID/type/body mismatch at " + bbNumber(i));
}
static void invalidSend()
{
    PacketLog log;
    IpcScope scope;
    scope.start(&log, receiveName("invalid"), 76, 2);
    ProcessWork& ipc = ProcessWork::instance();
    bbCheck(ipc.trySend(0, "x", 1) == PROCESS_WORK_INVALID, "zero PID accepted");
    bbCheck(ipc.trySend(-1, "x", 1) == PROCESS_WORK_INVALID, "negative PID accepted");
    bbCheck(ipc.trySend(bbPid(), nullptr, 1) == PROCESS_WORK_INVALID, "null body accepted");
    bbCheck(ipc.trySend(bbPid(), "x", -1) == PROCESS_WORK_INVALID, "negative length accepted");
    bbCheck(ipc.trySend(bbPid(), "x", 1, static_cast<MessageType>(-1)) == PROCESS_WORK_INVALID, "negative type accepted");
    const std::string oversized(65, 'x');
    bbCheck(ipc.trySend(bbPid(), oversized.data(), 65) == PROCESS_WORK_TOO_LARGE, "capacity+1 not rejected");
    sendPacket(5, bbPid(), "", std::string(64, 'x'), INT_MAX);
    waitPackets(log, 1);
    bbCheck(log.snapshot()[0].type == INT_MAX && log.snapshot()[0].body.size() == 64, "exact capacity/highest type failed");
}
static void invalidVoidSend()
{
    PacketLog log;
    IpcScope scope;
    const std::string name = receiveName("invalid_void");
    scope.start(&log, name, 64, 2);
    ProcessWork& ipc = ProcessWork::instance();
    ipc.initPostThread();
    ipc.send(bbPid(), nullptr, 1); ipc.send(name, "x", -1); ipc.send(bbPid(), "x", 1, static_cast<MessageType>(-1));
    ipc.send(0, "x"); ipc.send(std::string(), "x");
    ipc.post(bbPid(), nullptr, 1); ipc.post(name, "x", -1); ipc.post(bbPid(), "x", 1, static_cast<MessageType>(-1));
    ipc.post(0, "x"); ipc.post(std::string(), "x");
    ipc.send(bbPid(), std::string(53, 'x')); ipc.post(name, std::string(53, 'x'));
    ipc.post(bbPid(), std::string("barrier"), static_cast<MessageType>(77)); ipc.waitForPostEnd();
    waitPackets(log, 1); bbSleep(30); scope.close();
    bbCheck(log.snapshot().size() == 1 && log.snapshot()[0].body == "barrier", "invalid void call delivered a packet");
}
static void zeroCapacity()
{
    PacketLog log;
    IpcScope scope;
    scope.start(&log, receiveName("zero"), 12, 1);
    ProcessWork& ipc = ProcessWork::instance();
    bbCheck(ipc.receivePayloadCapacity() == 0 && ipc.receiveBufferCount() == 1, "empty-only receiver not ready");
    bbCheck(ipc.trySend(bbPid(), "x", 1) == PROCESS_WORK_TOO_LARGE, "empty-only receiver accepted body");
    sendPacket(5, bbPid(), "", "", 0); waitPackets(log, 1);
    bbCheck(log.snapshot()[0].nullData, "zero body was not nullptr");
}
static void callbacks()
{
    PacketLog first, last;
    ThrowingReceiver throwing;
    std::atomic<int> phase(0);
    first.action = [&phase](const ReceivedPacket& p) { bbCheck(phase == p.type * 2, "first observer order"); ++phase; };
    last.action = [&phase](const ReceivedPacket& p) { bbCheck(phase == p.type * 2 + 1, "last observer order"); ++phase; };
    IpcScope scope;
    ProcessWork& ipc = ProcessWork::instance();
    ipc.addProcessReceiveCallback(&first); ipc.addProcessReceiveCallback(nullptr); ipc.addProcessReceiveCallback(&throwing);
    scope.start(&last, receiveName("callbacks"), 64, 2);
    for (int i = 0; i < 5; ++i) sendPacket(0, bbPid(), "", "x", i);
    waitPackets(first, 5); waitPackets(last, 5);
    bbCheck(phase == 10 && first.errors == 0 && last.errors == 0, "observer exception stopped later observers");
}
static void callbackReentry()
{
    PacketLog log;
    log.action = [](const ReceivedPacket& packet)
    {
        if (packet.type == 1) ProcessWork::instance().post(bbPid(), std::string("response"), static_cast<MessageType>(2));
    };
    IpcScope scope;
    scope.start(&log, receiveName("reentry"), 64, 2);
    ProcessWork::instance().initPostThread();
    sendPacket(0, bbPid(), "", "request", 1); waitPackets(log, 2);
    ProcessWork::instance().waitForPostEnd();
    bbCheck(log.snapshot()[1].type == 2 && log.snapshot()[1].body == "response", "callback post failed");
}
static void clearCaches()
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
        if (i % 3 == 0) ipc.clearDestPid(bbPid());
        else if (i % 3 == 1) ipc.clearUseless();
        else ipc.clear();
    }
    bbCheck(log.snapshot().size() == 8 && ipc.receiveBufferCount() == 4, "cache clearing damaged receiver");
}
static void restartReceiver()
{
    PacketLog first, second;
    IpcScope scope;
    const std::string name = receiveName("restart");
    scope.start(&first, name, 64, 2);
    sendPacket(1, bbPid(), name, "old", 1); waitPackets(first, 1);
    ProcessWorkChannel oldSender;
    bbCheck(oldSender.openSender(bbPid()), "old channel open failed");
    scope.close();
    bbCheck(!oldSender.ready() && oldSender.trySend("x", 1, 1) == PROCESS_WORK_UNAVAILABLE, "old generation remained writable");
    scope.start(&second, name, 128, 3);
    sendPacket(1, bbPid(), name, std::string(116, 'n'), 2); waitPackets(second, 1);
    bbCheck(first.snapshot().size() == 1 && second.snapshot()[0].body.size() == 116, "restart retained callbacks or stale capacity");
    bbCheck(!oldSender.ready(), "old channel silently rebound to new generation");
}
static void postDrainRestart()
{
    PacketLog log;
    IpcScope scope;
    const std::string name = receiveName("post_restart");
    scope.start(&log, name, 128, 4);
    ProcessWork& ipc = ProcessWork::instance();
    ipc.initPostThread(); ipc.initPostThread();
    for (int round = 0; round < 3; ++round)
    {
        for (int i = 0; i < 20; ++i) sendPacket(4, bbPid(), name, packetBody(round * 20 + i, 64), round * 20 + i);
        ipc.waitForPostEnd(); waitPackets(log, (round + 1) * 20);
    }
    ipc.uninitPostThread(); ipc.uninitPostThread(); ipc.initPostThread();
    sendPacket(2, bbPid(), name, "new", 60); ipc.waitForPostEnd(); waitPackets(log, 61);
    const std::vector<ReceivedPacket> packets = log.snapshot();
    for (int i = 0; i < 61; ++i) bbCheck(packets[i].type == i, "post drain/restart reordered messages");
}
static void channelOwnership()
{
    ProcessWorkChannel first, second, sender;
    bbCheck(first.openReceiver(76, 2, 0, 0, 0, 0), "prepare receiver failed");
    bbCheck(!sender.openSender(bbPid()), "unpublished receiver visible");
    first.publish();
    bbCheck(!second.openReceiver(76, 2, 0, 0, 0, 0), "second owner accepted");
    bbCheck(sender.openSender(bbPid()) && sender.ready() && sender.payloadCapacity() == 64, "published receiver not available");
    second.close(); bbCheck(sender.ready(), "failed owner close destroyed first owner");
    first.stop(); bbCheck(!sender.ready(), "stop did not revoke readiness");
    sender.close(); first.close(); first.close();
    bbCheck(second.openReceiver(76, 2, 0, 0, 0, 0), "ownership not released after close");
    second.close();
}
static void channelFull()
{
    PublicChannelScope channel(2, 0, 0, 0, 0);
    for (int i = 0; i < 2; ++i) bbCheck(channel.sender.trySend("x", 1, i) == PROCESS_WORK_SENT, "initial free slot unavailable");
    for (int i = 0; i < 100; ++i) bbCheck(channel.sender.trySend("x", 1, 2) == PROCESS_WORK_CAPACITY, "full channel did not return CAPACITY");
    channel.pump();
    bbCheck(channel.delivered.size() == 2 && channel.delivered[0] == 0 && channel.delivered[1] == 1, "BUSY accidentally committed / FIFO damaged");
    bbCheck(channel.sender.trySend("x", 1, 2) == PROCESS_WORK_SENT, "freed capacity not reusable");
    channel.pump();
    bbCheck(channel.delivered.size() == 3 && channel.delivered[2] == 2, "retry was lost or duplicated");
    bbCheck(channel.receiver.bufferCount() == 2, "fixed capacity changed");
}
static void channelResize(bool shrink, int threshold)
{
    PublicChannelScope channel(2, threshold, 2, shrink ? 1 : 0, 2);
    int sequence = 0;
    // 暂停pump填满；BUSY只表示本次未提交，先维护再原序重试。
    for (int round = 0; round < 3; ++round)
    {
        const int capacity = channel.receiver.bufferCount();
        bbWait([&channel, capacity, &sequence]()
        {
            // pump可能先释放报文、下次才做维护；持续制造真实满载，而非假定一次pump必扩容。
            for (int i = 0; i < capacity; ++i)
                bbCheck(channel.sender.trySend("x", 1, sequence++) == PROCESS_WORK_SENT, "fill capacity failed");
            bbCheck(channel.sender.trySend("x", 1, sequence) == PROCESS_WORK_CAPACITY, "full resize channel accepted extra packet");
            channel.pump();
            return channel.receiver.bufferCount() > capacity || capacity == 6;
        }, "growth maintenance", 4000);
    }
    bbCheck(channel.receiver.bufferCount() == 6, "growth did not respect configured maximum");
    for (size_t i = 0; i < channel.delivered.size(); ++i) bbCheck(channel.delivered[i] == static_cast<int>(i), "resize lost/reordered message");
    bbCheck(channel.delivered.size() == static_cast<size_t>(sequence), "resize lost submitted messages");
    if (shrink)
        bbWait([&channel]() { channel.pump(); return channel.receiver.bufferCount() == 2; }, "idle shrink to initial", 6000);
    else
    {
        const uint64_t until = bbNow() + 1300;
        while (bbNow() < until) { channel.pump(); bbSleep(10); }
        bbCheck(channel.receiver.bufferCount() == 6, "idleSeconds=0 unexpectedly shrank");
    }
    bbCheck(channel.sender.trySend("x", 1, sequence) == PROCESS_WORK_SENT, "channel unusable after resize");
    channel.pump();
    bbCheck(channel.delivered.back() == sequence, "post-resize payload missing");
}
static void lifecycleStress()
{
    for (int round = 0; round < 30; ++round)
    {
        PacketLog log;
        IpcScope scope;
        scope.start(&log, receiveName("life"), 128, 4);
        ProcessWork& ipc = ProcessWork::instance();
        ipc.initPostThread();
        for (int i = 0; i < 20; ++i) sendPacket(2, bbPid(), "", packetBody(i, 64), i);
        ipc.waitForPostEnd(); waitPackets(log, 20); scope.close();
        bbCheck(CTaskThreadManager::instance().count() == 0 && ipc.receiveBufferCount() == 0, "IPC left managed threads/resources ready");
    }
}
static void deadPeer()
{
    BlackBoxChild child;
    std::vector<std::string> args(1, "--exit-peer");
    child.start(args, bbTempPath("-dead-peer.log"));
    bbCheck(child.wait(5000) == 0, "dead-peer setup failed");
    ProcessWork& ipc = ProcessWork::instance();
    IpcScope scope;
    bbCheck(ipc.trySend(child.pid(), "x", 1) == PROCESS_WORK_UNAVAILABLE, "exited peer considered available");
    ipc.send(child.pid(), "x"); ipc.initPostThread(); ipc.post(child.pid(), "x"); ipc.waitForPostEnd();
}
/** 验证清缓存与同步发送或后台前缀借用并发时的完整交付
@param [in] post true使用异步PID/名称混合投递，false保留同步发送场景
*/
static void concurrentClear(bool post)
{
    const int threads = 4, packets = 300;
    StreamReceiver ledger(threads, packets, false);
    ledger.configure(std::vector<int>(1, bbPid()));
    IpcScope scope;
    const std::string name = receiveName("concurrent_clear");
    scope.start(&ledger, name);
    if (post)
    {
        ProcessWork::instance().initPostThread();
    }
    IpcJobs jobs;
    for (int thread = 0; thread < threads; ++thread)
    {
        jobs.add([thread, name, post]()
        {
            const int sizes[] = { 0, 1, 6, 64, 4096, 65536 };
            for (int i = 0; i < 300; ++i)
            {
                const int value = thread * 300 + i;
                sendPacket((post ? 2 : 0) + i % 2, bbPid(), name, packetBody(value, sizes[i % 6]), value);
            }
        });
    }
    jobs.add([]()
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
    bbWait([&ledger]() { return ledger.received >= 1200 || ledger.bad > 0; }, "concurrent cache clear deliveries", 10000);
    scope.close();
    bbCheck(ledger.received == 1200 && ledger.bad == 0, "cache clear raced with send: loss/duplicate/corruption");
}
static void nameCollision()
{
    PacketLog log;
    IpcScope scope;
    const std::string name = receiveName("exclusive");
    scope.start(&log, name, 64, 2);
    BlackBoxChild child;
    std::vector<std::string> args;
    const std::string ready = bbTempPath("-same-name.ready");
    args.push_back("--share-name"); args.push_back(name); args.push_back(bbNumber(bbPid())); args.push_back(ready);
    const std::string output = bbTempPath("-name-collision.log");
    child.start(args, output);
    bbWait([&child, &ready]()
    {
        bbCheck(child.poll() < 0, "same-name peer exited before ready");
        return bbRead(ready) == "1";
    }, "same-name peer ready");
    waitPackets(log, 1);
    bbCheck(log.snapshot()[0].pid == child.pid() && log.snapshot()[0].body == "name-owner", "name moved away from first live owner");
    sendPacket(0, child.pid(), "", "by-pid", 2);
    const int result = child.wait(10000);
    bbCheck(result == 0, "same-name peer failed; log=" + output + ": " + bbRead(output));
    sendPacket(1, bbPid(), name, "owner", 3); waitPackets(log, 2);
    bbCheck(log.snapshot()[1].body == "owner", "second receiver close damaged first owner's name");
    std::remove(ready.c_str());
}
static void maximumBlocks()
{
    PacketLog log;
    IpcScope scope;
    scope.start(&log, receiveName("max_blocks"), 12, 1024);
    bbCheck(ProcessWork::instance().receiveBufferCount() == 1024, "maximum block configuration rejected/truncated");
    sendPacket(5, bbPid(), "", "", 1); waitPackets(log, 1);
}
static void closeFullChannel()
{
    PublicChannelScope channel(1, 0, 0, 0, 0);
    bbCheck(channel.sender.trySend("x", 1, 0) == PROCESS_WORK_SENT, "fill single slot failed");
    bbCheck(channel.sender.trySend("x", 1, 1) == PROCESS_WORK_CAPACITY, "single slot not full");
    channel.receiver.stop();
    bbCheck(!channel.sender.waitForSpace(), "closed receiver left sender waiting for space");
    bbCheck(channel.sender.trySend("x", 1, 1) == PROCESS_WORK_UNAVAILABLE, "closed full receiver accepted send");
}
std::vector<BlackBoxCase> processCases()
{
    std::vector<BlackBoxCase> cases;
    const std::function<void(const char*, const char*, bool, std::function<void()>)> add =
        [&cases](const char* name, const char* expected, bool stress, std::function<void()> run)
    {
        BlackBoxCase c = { static_cast<int>(cases.size() + 1), name, expected, stress, stress ? 150 : 75, [run]()
        {
            run();
            bbCheck(CTaskThreadManager::instance().count() == 0, "case retained managed threads");
        } };
        cases.push_back(c);
    };
    add("empty-lifecycle", "empty/repeated cleanup safe; snapshots zero", false, emptyLifecycle);
    add("invalid-init-retry", "13 invalid configurations rejected; callback/post preserved", false, invalidInit);
    add("name-bounds", "NUL/1024 rejected; 1023 accepted; live rename rejected", false, nameBounds);
    add("duplicate-init", "repeat init rejected without damaging live receiver", false, duplicateInit);
    add("pid-send", "all buffer/string/empty/binary/exact-capacity packets in FIFO", false, []() { localTransfer(0); });
    add("name-send", "name resolution plus FIFO and full byte verification", false, []() { localTransfer(1); });
    add("pid-post-copy", "post owns buffer; source mutation cannot change delivery", false, []() { localTransfer(2); });
    add("name-post-copy", "name post owns buffer; all lengths delivered", false, []() { localTransfer(3); });
    add("mixed-post-fifo", "name/PID post share one ordered stream", false, []() { localTransfer(4); });
    add("try-send-contract", "INVALID/TOO_LARGE exact; boundary payload/type accepted", false, invalidSend);
    add("invalid-void-send", "invalid sync/async calls produce no packets", false, invalidVoidSend);
    add("zero-payload-capacity", "12-byte block accepts only empty packet", false, zeroCapacity);
    add("callback-order-isolation", "registration order; null skipped; exception isolated", false, callbacks);
    add("callback-reentry", "callback can post a response without deadlock", false, callbackReentry);
    add("clear-caches", "clear/clearDestPid/clearUseless retain receiver usability", false, clearCaches);
    add("receiver-restart", "old generation unavailable; new capacity/callback/name valid", false, restartReceiver);
    add("post-drain-restart", "drain/recreate three times; no loss or reordering", false, postDrainRestart);
    add("channel-publication-ownership", "not visible before publish; one owner; safe failed close", false, channelOwnership);
    add("channel-busy-retry", "full Capacity commits nothing; retry delivered once", false, channelFull);
    add("grow-shrink-zero-threshold", "grow on full demand; cap6; idle returns to initial2", false, []() { channelResize(true, 0); });
    add("grow-no-shrink", "idleSeconds=0 retains expanded capacity", false, []() { channelResize(false, 0); });
    add("grow-positive-threshold", "positive threshold supported; shrink floor is initial", false, []() { channelResize(true, 1); });
    add("mesh-pid-send", "3 real processes x 2 streams; ledger PID/order/body exact", false, []() { meshTest(0, false, 48, 3, 2); });
    add("mesh-name-send", "3 real processes; all-to-all name send", false, []() { meshTest(1, false, 48, 3, 2); });
    add("mesh-pid-post", "3 real processes; all-to-all PID post", false, []() { meshTest(2, false, 48, 3, 2); });
    add("mesh-name-post", "3 real processes; all-to-all name post", false, []() { meshTest(3, false, 48, 3, 2); });
    add("mesh-mixed-post", "mixed name/PID post FIFO across real processes", false, []() { meshTest(4, false, 48, 3, 2); });
    add("mesh-fixed-six", "legacy 123456 payload; fixed128 block configuration", false, []() { meshTest(4, true, 100, 3, 2, 128); });
    add("dead-target", "known exited child unavailable; send/post do not hang", false, deadPeer);
    add("lifecycle-stress", "30 init/drain/close cycles; no managed threads retained", true, lifecycleStress);
    add("mesh-send-stress", "3x4 producers; 1000 variable packets per stream/peer", true, []() { meshTest(0, false, 1000, 3, 4); });
    add("mesh-post-stress", "3x4 producers; 1000 mixed post packets per stream/peer", true, []() { meshTest(4, false, 1000, 3, 4); });
    add("mesh-try-send-stress", "BUSY retry with 3x4 streams and byte ledger", true, []() { meshTest(5, false, 1000, 3, 4); });
    add("concurrent-cache-clear", "4 senders plus cache clearer; all1200 messages verified", true, []() { concurrentClear(false); });
    add("name-collision", "same-name peer works by PID; name stays first owner after peer close", false, nameCollision);
    add("maximum-blocks", "1024-block protocol boundary accepts empty payload", false, maximumBlocks);
    add("close-full-channel", "stopped receiver returns unavailable; no stranded space wait", false, closeFullChannel);
    add("callback-self-close", "self wait retains borrowed callback data until owner closes", false, testProcessWorkSelfClose);
    add("shared-atomic-publication", "10000 handoffs preserve full 64-bit payload; failed CAS leaves target", false, testProcessWorkAtomics);
    add("memory-pressure-guard", "invalid budgets and overflowing/reserve-exhausted requests rejected", false, testProcessWorkMemoryGuard);
    add("paged-storage", "concurrent page installation; 256/65536 boundaries; retired mapping generation isolation", false, testProcessWorkStorage);
    add("shared-fifo-interruption", "unpublished nodes do not block; stale CAS fails; consumer helps producer tail", false, testProcessWorkSharedQueue);
    add("shared-fifo-reuse-stress", "8 producers x 50000 entries; independent FIFO ledger without duplicates", true, testProcessWorkSharedQueueStress);
    add("unlimited-growth", "threshold0 waits for failed request; capacity1024 to1152 to1280; full FIFO ledger", false, testProcessWorkUnlimited);
    add("growth-memory-budget", "impossible reserve rejected; one-block maintenance budget; eventual configured cap", false, testProcessWorkGrowthBudget);
    add("dead-producer-body", "dead unpublished body reclaimed; both slots remain usable", false, []() { testProcessWorkDeadProducer(0); });
    add("dead-producer-node", "dead unpublished body and queue node reclaimed", false, []() { testProcessWorkDeadProducer(1); });
    add("dead-producer-committed", "committed body delivered once after sender exit", false, []() { testProcessWorkDeadProducer(2); });
    add("dead-producer-tail", "linked body delivered despite sender exit before tail advance", false, []() { testProcessWorkDeadProducer(3); });
    add("live-producer-paused", "paused reservation retained; other producer progresses; dead reservation then reclaimed", false, []() { testProcessWorkDeadProducer(4); });
    add("local-backlog-budget", "in-flight fixed budget, oversized rejection, failure rollback and pressure rejection", false, testProcessWorkLocalBudget);
    add("local-notification-stress", "8x5000 messages through a three-packet budget, FIFO ledger and zero retained charge", true, testProcessWorkLocalQueueStress);
    add("post-admission-budget", "unstarted queue returns Capacity for over-budget copies; failed calls commit nothing; drain returns charges", false, testProcessWorkPostBudget);
    add("slow-callback-backpressure", "blocked callback retains one local packet; shared capacity bounds backlog; FIFO recovery", false, testProcessWorkSlowCallback);
    add("corrupt-channel-closed", "invalid committed body index closes channel without delivery or retry duplication", false, testProcessWorkCorruption);
    add("resource-plateau", "20 warmup plus100 lifecycle cycles; no charged packets/workers; Windows handles and private memory plateau", true, testProcessWorkResourcePlateau);
    add("mesh-16-producers", "3x16 producers,960000 verified deliveries,per-stream FIFO and fixed payload", true, []() { meshTest(0, true, 10000, 3, 16, 128); });
    add("mesh-32-producers", "2x32 producers,960000 verified deliveries,per-stream FIFO and fixed payload", true, []() { meshTest(0, true, 15000, 2, 32, 128); });
    add("mesh-post-backpressure", "72000 variable packets through bounded tryPost retry;accepted messages never retried or lost", true, []() { meshTest(6, false, 3000, 3, 4, 128); });
    add("ordinary-post-contention", "32x5000 ordinary posts with literal123456, no caller retry or exception, exact FIFO ledger", true, testProcessWorkPostContention);
    add("crash-producer-body", "abrupt process death after body reservation; full capacity recovered", false, []() { testProcessWorkDeadProducer(0, true); });
    add("crash-producer-node", "abrupt process death after node reservation; full capacity recovered", false, []() { testProcessWorkDeadProducer(1, true); });
    add("crash-producer-committed", "abrupt process death after commit; exactly one delivery and full recovery", false, []() { testProcessWorkDeadProducer(2, true); });
    add("crash-producer-tail", "abrupt process death between link and tail; committed message preserved", false, []() { testProcessWorkDeadProducer(3, true); });
    add("crash-paused-producer", "live paused owner retained; later killed owner reclaimed", false, []() { testProcessWorkDeadProducer(4, true); });
    add("crash-receiver-restart", "full trySend returns Capacity; eight ordinary/background transmissions released by receiver death; same-name restart verifies4000", false, testProcessWorkReceiverCrash);
    add("receive-batch-budget", "1000 pool reuses; exact per-packet charges, source independence, large exact-limit packet and pressure failure", false, testProcessWorkReceiveBatches);
    add("receive-batch-stress", "20000 variable0/6/64/65536-byte packets, concurrent producer/consumer, full byte FIFO ledger", true, testProcessWorkReceiveBatchStress);
    add("inline-boundary-reuse", "single shared slot alternates36/37/6/0/68-byte payloads without stale reads", false, testProcessWorkInlineBoundary);
    add("repeated-stream-sender-crash", "six forced process deaths during ordinary send,96000 survivor packets,exact123456,FIFO and all slots reclaimed", true, testProcessWorkCrashStream);
    add("borrowed-channel-cache", "no shared_ptr copy,reentrant deletion retains borrowed value,callback invoked at most once", false, testProcessWorkBorrowedCache);
    add("dead-sender-local-backpressure", "killed sender committed body retained through blocked callback and maintenance,then delivered exactly once", false, testProcessWorkDeadSenderBackpressure);
    add("fair-single-slot-progress", "32 simultaneous six-byte senders,one shared slot,six active observation windows,per-stream progress and exact final FIFO", true, []() { testProcessWorkFairProgress(false); });
    add("fair-mixed-late-resume", "32 small/64KiB streams,eight shared slots,late join and pause/resume,every active stream progresses and final FIFO matches", true, []() { testProcessWorkFairProgress(true); });
    add("result-contracts", "ordinary admission never reports Busy; capacity/resource distinguished; successful send/post preserve payload and FIFO", false, testProcessWorkResultContracts);
    add("new-stream-round", "new ready stream is found in first poll after an old empty/dead stream round boundary", false, testProcessWorkNewStreamRound);
    add("mapping-failure-return", "missing body mapping returns resource failure, frees reservation and permits exactly one restored send", false, testProcessWorkMappingFailure);
    add("send-failure-kinds", "pending registration is Resource; full buffer is Capacity; neither is CAS Busy; failed attempts do not commit", false, testProcessWorkSendFailureKinds);
    add("retry-options-capacity", "four retry combinations, ordinary equivalence, cancellation, exact delivery and4096 then1ms", false, ProcessWorkRetryTests::run);
    add("send-cache-lifetime", "in-flight clear, nested fallback, stale epoch and eight colliding cache writers reclaim all ownership", false, testProcessWorkSendCache);
    add("receive-batch-reserve", "idle preallocation independent from backlog quota, exact boundaries and FIFO reuse", false, testProcessWorkReceiveReserve);
    add("post-packet-reserve", "small packet ownership, full pool fallback, boundary sizes, concurrent reuse and budget rollback", true, testProcessWorkPostPacketPool);
    add("free-slots-with-waiters", "unused node capacity stays available; returned bodies hand off in FIFO and cannot be stolen", false, testProcessWorkFreeSlotsWithWaiters);
    add("post-prefix-cache-clear", "4 mixed PID/name post producers plus cache clearer preserve1200 messages and FIFO during prefix borrow", true, []() { concurrentClear(true); });
    add("sender-address-cache", "two channels alternate on one physical sender across pages,segments,reuse and reopen;28000 exact FIFO messages", false, testProcessWorkSenderAddressCache);
    add("receiver-page-reuse", "same receiver object reopens513/257/3 slots;3092 exact FIFO messages cross pages and reused mappings", false, testProcessWorkReceiverPageReuse);
    add("receive-copy-boundary-guard", "0..96-byte bodies placed at both inaccessible page boundaries;194 exact copies,NUL,ownership and budget", false, testProcessWorkReceiveCopyBoundary);
    add("dynamic-local-budget", "fixed quota boundaries,unlimited byte-accounting bypass,lifetime checks and impossible-request termination", false, testProcessWorkFixedBudget);
    add("component-pool-configuration", "default unlimited,actual post/receive growth,native configuration rejection and task startup rollback", false, testProcessWorkMemoryConfiguration);
    add("native-growth-notification", "actual Queue/Map/Hash growth,initial exclusion and observer exception isolation", false, testProcessWorkNativeGrowth);
    add("packet-pool-dynamic-growth", "finite cache cap,heap fallback,32 producers progress and exact ownership", true, testProcessWorkPacketGrowth);
    add("shared-growth-notification", "shared strategy clipping,actual growth notification and exact delivery", false, testProcessWorkSharedGrowth);
    add("reservation-window-capacity", "1/63/64/65/129/255/256/257/511/512/513 slots,exact capacity then rejection,4 reuse cycles,10504 FIFO messages", false, testProcessWorkReservationWindow);
    add("automation-self-test", "strict parsing,23 workload preservation,median and failed child detection", false, ProcessWorkTestAutomation::selfTest);
    add("payload-capacity-restart", "65536 to1 to0 body capacity,stale mapping revoked,exact delivery", false, ProcessWorkCompatibilityTests::payloadRestart);
    add("message-type-domain", "five routes,55 valid boundary types and15 rejected negative calls,exact bytes", false, ProcessWorkCompatibilityTests::typeDomain);
    add("guarded-source-input", "five routes,21 lengths,two guard boundaries,210 copies survive source revocation", false, ProcessWorkCompatibilityTests::guardedInput);
    add("overload-rejection-fifo", "eight overloads,four capacities,1023-byte name,four producers,16384 valid deliveries amid rejected requests", true, ProcessWorkCompatibilityTests::overloadRejection);
    return cases;
}

int main(int argc, char** argv)
{
    try
    {
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
        if (argc == 3 && std::string(argv[1]) == "--quota-probe")
        {
            return runProcessWorkQuotaProbe(std::atoi(argv[2]));
        }
        if (argc == 2 && std::string(argv[1]) == "--runtime-handle-control")
        {
            runProcessWorkRuntimeControl();
            return 0;
        }
        if (argc > 1 && std::string(argv[1]) == "--node") return runIpcNode(argc, argv);
        if (argc > 1 && std::string(argv[1]) == "--fault-producer") return runProcessWorkFaultChild(argc, argv);
        if (argc > 1 && std::string(argv[1]) == "--crash-receiver") return runProcessWorkCrashReceiver(argc, argv);
        if (argc > 1 && std::string(argv[1]) == "--crash-stream") return runProcessWorkCrashStream(argc, argv);
        if (argc == 2 && std::string(argv[1]) == "--exit-peer") return 0;
        if (argc == 5 && std::string(argv[1]) == "--share-name")
        {
            PacketLog log;
            IpcScope scope;
            scope.start(&log, argv[2], 64, 2);
            sendPacket(1, 0, argv[2], "name-owner", 1);
            bbWrite(argv[4], "1");
            waitPackets(log, 1);
            bbCheck(log.snapshot()[0].pid == std::atoi(argv[3]) && log.snapshot()[0].body == "by-pid", "same-name peer not independently addressable");
            scope.close();
            return 0;
        }
        return bbRun(argc, argv, "ProcessWorkTest", processCases());
    }
    catch (const std::exception& error) { std::cerr << error.what() << "\n"; return 1; }
    catch (...) { std::cerr << "unknown setup exception\n"; return 1; }
}
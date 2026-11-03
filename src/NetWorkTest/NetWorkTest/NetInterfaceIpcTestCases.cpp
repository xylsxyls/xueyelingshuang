#include "NetInterfaceIpcTestCases.h"
#include "NetSenderTestClient.h"
#include "NetSender/NetSenderAPI.h"
#include <cstdio>
#include "NetWorkTestCases.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "NetLocalTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetRawLocalTestCases.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "AtomicMath/AtomicMathAPI.h"
#include "CSystem/CSystemAPI.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <chrono>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <memory>
#ifdef _WIN32
#include <windows.h>
#endif

/** 带统一取消/时限的独立断言
@param [in] expected 预期条件
@param [in] detail 失败说明
*/
static void CheckIpc(bool expected, const char* detail)
{
    netTestCheckpoint();
    if (!expected)
    {
        throw std::runtime_error(detail);
    }
}

void NetInterfaceIpcTestCases::localPackets()
{
    const uint8_t body[] = { 0, 0xff, 0x80 };
    NetSenderLocalPacketView packet = {};
    packet.m_kind = NET_LOCAL_OPEN;
    packet.m_packet = UINT64_C(0xfedcba9876543210);
    packet.m_source[0] = 1;
    packet.m_data = body;
    packet.m_size = sizeof(body);
    uint8_t encoded[60] = {};
    uint32_t required = 0;
    CheckIpc(NetSenderLocalPacket::encode(packet, encoded + 1, 59, required) == NI_OK && required == 59,
        "local TLS packet encode");
    const uint8_t prefix[] = { 'N', 'I', 'P', '2', 0, 0, 0, 1,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10, 1 };
    CheckIpc(std::memcmp(encoded + 1, prefix, sizeof(prefix)) == 0 && encoded[52] == 3 &&
        encoded[57] == 0 && encoded[58] == 0xff && encoded[59] == 0x80,
        "independent local header/body vector");
    NetSenderLocalPacketView view = {};
    CheckIpc(NetSenderLocalPacket::parse(encoded + 1, 59, view) == NI_OK &&
        view.m_packet == packet.m_packet && view.m_size == sizeof(body) &&
        std::memcmp(view.m_data, body, sizeof(body)) == 0 &&
        NetSenderLocalPacket::sameGeneration(view.m_source, packet.m_source), "unaligned local packet parse");
    for (uint32_t length = 0; length < 59; ++length)
    {
        CheckIpc(NetSenderLocalPacket::parse(encoded + 1, length, view) == NI_PROTOCOL_ERROR &&
            view.m_packet == 0 && view.m_data == nullptr, "truncated local packet accepted");
    }
    encoded[56] = 1;
    CheckIpc(NetSenderLocalPacket::parse(encoded + 1, 59, view) == NI_PROTOCOL_ERROR, "local reserved bit accepted");
    encoded[56] = 0;
    encoded[33] = 1;
    CheckIpc(NetSenderLocalPacket::parse(encoded + 1, 59, view) == NI_PROTOCOL_ERROR, "OPEN named an old target");
    encoded[33] = 0;
    packet.m_kind = NET_LOCAL_DATA;
    CheckIpc(NetSenderLocalPacket::encode(packet, encoded, sizeof(encoded), required) == NI_INVALID_ARGUMENT,
        "DATA missing destination generation");
    packet.m_target[0] = 2;
    packet.m_size = 16385;
    CheckIpc(NetSenderLocalPacket::encode(packet, encoded, sizeof(encoded), required) == NI_INVALID_ARGUMENT &&
        required == 0, "oversized IPC ciphertext");
    packet.m_size = 3;
    encoded[0] = 0x55;
    CheckIpc(NetSenderLocalPacket::encode(packet, encoded, 58, required) == NI_BUFFER_TOO_SMALL &&
        required == 59 && encoded[0] == 0x55, "short local output mutated");
    packet.m_kind = NET_LOCAL_ACK;
    packet.m_size = 0;
    packet.m_data = nullptr;
    CheckIpc(NetSenderLocalPacket::encode(packet, encoded, sizeof(encoded), required) == NI_OK &&
        required == 56 && NetSenderLocalPacket::parse(encoded, required, view) == NI_OK,
        "local ACK frame");
    packet.m_kind = NET_LOCAL_END;
    CheckIpc(NetSenderLocalPacket::encode(packet, encoded, sizeof(encoded), required) == NI_INVALID_ARGUMENT,
        "END with packet number");
    packet.m_packet = 0;
    CheckIpc(NetSenderLocalPacket::encode(packet, encoded, sizeof(encoded), required) == NI_OK,
        "local transport EOF frame");
    NetSenderLocalWindow window;
    for (uint64_t number = 1; number <= 4; ++number)
    {
        CheckIpc(window.next() == number && window.published(number) == NI_OK, "local send window admission");
    }
    CheckIpc(window.outstanding() == 4 && window.next() == 0 && window.published(5) == NI_INVALID_STATE &&
        window.acknowledge(5) == NI_PROTOCOL_ERROR && window.outstanding() == 4, "local window overflow");
    CheckIpc(window.acknowledge(2) == NI_OK && window.published(5) == NI_OK && window.published(6) == NI_OK &&
        window.acknowledge(2) == NI_OK && window.outstanding() == 4 && window.acknowledge(6) == NI_OK &&
        window.outstanding() == 0 && window.next() == 7, "cumulative/duplicate ACK released wrong quota");
    CheckIpc(window.received(0) == NI_PROTOCOL_ERROR && window.received(1) == NI_OK &&
        window.received(1) == NI_NOT_FOUND && window.received(3) == NI_PROTOCOL_ERROR &&
        window.received(2) == NI_OK && window.receipt() == 2, "local receive sequence replay/gap");
}

/** 构造当前测试拥有的对象名
@param [in] prefix 固定前缀
@param [in] pid 对应进程号
@param [in] block 可选缓存号
@return 对象名
*/
static std::string TestIpcName(const char* prefix, int32_t pid, int32_t block = 0)
{
    std::ostringstream result;
    result << prefix << pid;
    if (block != 0)
    {
        result << '_' << block;
    }
    return result.str();
}

void NetInterfaceIpcTestCases::notifications()
{
    const std::string name = TestIpcName("NetWorkTest_notify_", CSystem::currentProcessPid());
    Semaphore owner;
    Semaphore peer;
    CheckIpc(!peer.tryOpenProcessSemaphore(name) && peer.tryProcessSignal() < 0,
        "missing semaphore must not be created");
    owner.createProcessSemaphore(name, 1);
    CheckIpc(peer.tryOpenProcessSemaphore(name), "open existing semaphore");
    CheckIpc(peer.tryProcessSignal() == 0, "publish notification");
#ifdef _WIN32
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    for (int index = 0; index < 256; ++index)
    {
        CheckIpc(peer.tryProcessSignal() == 1, "full count must return busy without duplicate");
    }
    CheckIpc(std::chrono::steady_clock::now() - start < std::chrono::seconds(1), "full semaphore blocked");
#endif
    CheckIpc(owner.processWait(0) && !owner.processWait(0), "exactly one notification");
    peer.processSignal();
    CheckIpc(owner.processWait(0), "legacy signal compatibility");
    CheckIpc(peer.tryProcessSignal() == 0 && owner.processWait(0), "notification after draining");
    peer.closeProcessSemaphore();
    CheckIpc(peer.tryProcessSignal() < 0, "signal on closed object");
}

void NetInterfaceIpcTestCases::stalledReceiver()
{
    const int32_t pid = CSystem::currentProcessPid();
    ProcessWorkChannel receiver;
    CheckIpc(receiver.openReceiver(4096, 2, 0, 0, 0, 0), "create bounded v2 receiver");
    receiver.publish();
    LockFreeQueue<char*> packets;
    Semaphore delivered;
    std::atomic<int32_t> full(0);
    NetSenderIpcChannel sender;
    CheckIpc(sender.open(pid, pid, 8192) == NET_SENDER_IPC_TOO_LARGE, "open refuses unsupported declared payload");
    CheckIpc(sender.open(pid, pid, 4084) == NET_SENDER_IPC_ACCEPTED, "open stalled v2 receiver");
    receiver.pump(packets, delivered, full);
    CheckIpc(sender.trySend(nullptr, 1, 0) == NET_SENDER_IPC_INVALID &&
        sender.trySend(nullptr, 0, -1) == NET_SENDER_IPC_INVALID, "IPC input validation");
    std::vector<uint8_t> tooLarge(8192, 17);
    CheckIpc(sender.trySend(tooLarge.data(), 8192, 0) == NET_SENDER_IPC_TOO_LARGE,
        "declared capacity rejection does not reserve a block");
    CheckIpc(sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_ACCEPTED &&
        sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_ACCEPTED, "both available blocks committed");
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    for (int32_t index = 0; index < 512; ++index)
    {
        CheckIpc(sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_BUSY, "full receiver remains bounded");
    }
    CheckIpc(std::chrono::steady_clock::now() - start < std::chrono::seconds(1), "stalled receiver blocked sender");
    CheckIpc(receiver.pump(packets, delivered, full) == 2, "drain two committed packets");
    for (int32_t index = 0; index < 2; ++index)
    {
        char* packet = nullptr;
        CheckIpc(packets.pop(&packet) && packet != nullptr, "copied empty packet exists");
        std::unique_ptr<char[]> owned(packet);
        int32_t header[3] = {};
        std::memcpy(header, packet, sizeof(header));
        CheckIpc(header[0] == pid && header[1] == 0 && header[2] == 0, "empty packet header exact");
    }
    CheckIpc(packets.empty(), "no duplicate from busy attempts");
    sender.close();
    receiver.close();
    // 只有旧协议对象存在时不能假装已打开v2接收器。
    SharedMemory legacy(TestIpcName("ProcessAreaAssign_", pid), 20, false);
    CheckIpc(sender.open(pid, pid, 100) == NET_SENDER_IPC_UNAVAILABLE, "legacy protocol rejected explicitly");
}

void NetInterfaceIpcTestCases::publication()
{
    const int32_t pid = CSystem::currentProcessPid();
    ProcessWorkChannel receiver;
    CheckIpc(receiver.openReceiver(4096, 1, 0, 0, 0, 0), "create publication receiver");
    receiver.publish();
    LockFreeQueue<char*> packets;
    Semaphore delivered;
    std::atomic<int32_t> full(0);
    NetSenderIpcChannel sender;
    CheckIpc(sender.open(pid, pid, 4084) == NET_SENDER_IPC_ACCEPTED, "open v2 publication receiver");
    receiver.pump(packets, delivered, full);
    const uint8_t payload[] = { 0, 255, 128, 7, 0 };
    CheckIpc(sender.trySend(payload, sizeof(payload), 42) == NET_SENDER_IPC_ACCEPTED, "publish binary payload");
    CheckIpc(sender.pendingNotifications() == 0 && sender.flushNotifications() == NET_SENDER_IPC_ACCEPTED,
        "v2 notification hint has no duplicate payload retry");
    CheckIpc(sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_BUSY, "published block cannot be overwritten");
    sender.close();
    CheckIpc(receiver.pump(packets, delivered, full) == 1, "sender close preserves committed message");
    char* packet = nullptr;
    CheckIpc(packets.pop(&packet) && packet != nullptr, "published packet delivered");
    std::unique_ptr<char[]> owned(packet);
    const int32_t expected[] = { pid, 42, sizeof(payload) };
    CheckIpc(std::memcmp(packet, expected, 12) == 0 && std::memcmp(packet + 12, payload, sizeof(payload)) == 0,
        "v2 local packet header and binary body exact");
    CheckIpc(packets.empty(), "one publication delivered exactly once");
    CheckIpc(sender.open(pid, pid, 4084) == NET_SENDER_IPC_ACCEPTED &&
        sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_ACCEPTED, "sender resumes after receiver releases block");
    CheckIpc(receiver.pump(packets, delivered, full) == 1, "empty body delivered");
    packet = nullptr;
    CheckIpc(packets.pop(&packet) && packet != nullptr, "empty body copied");
    owned.reset(packet);
    int32_t header[3] = {};
    std::memcpy(header, packet, sizeof(header));
    CheckIpc(header[0] == pid && header[1] == 0 && header[2] == 0 && packets.empty(), "empty body exact and no duplicates");
}

/** 可独立重建的测试长度，覆盖空消息和128KiB跨页消息
@param [in] index 消息序号0至39
@return 消息字节数
*/
static int32_t IpcPayloadSize(int32_t index)
{
    return index == 0 ? 0 : (index == 39 ? 131072 : index * 97 + 1);
}

NetInterfaceIpcReceiver::NetInterfaceIpcReceiver(int32_t parentPid) :
m_count(0), m_failed(false), m_parentPid(parentPid)
{

}

void NetInterfaceIpcReceiver::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
{
    const int32_t index = m_count.load();
    if (sendPid != m_parentPid || index >= 40 || length != IpcPayloadSize(index) ||
        static_cast<int32_t>(type) != 42 || (length != 0 && buffer == nullptr))
    {
        m_failed.store(true);
    }
    else
    {
        for (int32_t offset = 0; offset < length; ++offset)
        {
            if (static_cast<uint8_t>(buffer[offset]) != static_cast<uint8_t>(index + offset * 13))
            {
                m_failed.store(true);
                break;
            }
        }
    }
    if (m_count.fetch_add(1) + 1 >= 40 || m_failed.load())
    {
        m_done.signal();
    }
}

#ifdef __APPLE__
void NetInterfaceIpcTestCases::appleResources()
{
    const int32_t pid = CSystem::currentProcessPid();
    const std::string name = TestIpcName("PTA", pid);
    CheckIpc(!CSystem::processName(pid).empty(), "macOS live PID lookup");
    {
        SharedMemory owner(name, 8192, false);
        CheckIpc(!owner.isFailed() && owner.realSize() == 8192, "Apple create exact capacity");
        SharedMemory exclusive(name, 8192, false);
        SharedMemory oversized(name, 16384, true);
        CheckIpc(exclusive.isFailed() && oversized.isFailed(), "active mapping replaced or resized");
        uint8_t* writer = static_cast<uint8_t*>(owner.writeWithoutLock());
        CheckIpc(writer != nullptr, "map Apple writer");
        writer[8191] = 193;
        SharedMemory peer(name);
        uint8_t* reader = static_cast<uint8_t*>(peer.readWithoutLock());
        CheckIpc(reader && reader[8191] == 193 && peer.realSize() == 8192, "shared mapping contents");
        peer.close();
        reader = static_cast<uint8_t*>(peer.writeWithoutLock());
        CheckIpc(reader && reader[8191] == 193, "unmap/remap retained native shared object");
        reader[0] = 91;
        CheckIpc(writer[0] == 91, "bidirectional shared visibility");
    }
    SharedMemory absent(name);
    CheckIpc(absent.writeWithoutLock() == nullptr, "last mapping left an attachable stale object");
    SharedMemory replacement(name, 4096, false);
    CheckIpc(!replacement.isFailed() && replacement.realSize() == 4096, "new mapping generation");
    SharedMemory bad("../outside", 4096, false);
    CheckIpc(bad.isFailed(), "mapping escaped scope");
    SharedMemory anonymous("", 4096, false);
    CheckIpc(!anonymous.isFailed() && anonymous.writeWithoutLock(), "anonymous mapping");
    int32_t value = 3;
    int64_t wide = INT64_C(0x100000001);
    CheckIpc(AtomicMath::selfAddOne(&value) == 4 && AtomicMath::selfSubOne(&value) == 3 &&
        AtomicMath::selfAdd(&value, 7) == 10 && AtomicMath::selfSub(&value, 2) == 8 &&
        !AtomicMath::compareAndSwap(&value, 7, 20) && AtomicMath::compareAndSwap(&value, 8, 20), "Apple int32 atomics");
    CheckIpc(AtomicMath::selfAddOne(&wide) == INT64_C(0x100000002) &&
        AtomicMath::selfSubOne(&wide) == INT64_C(0x100000001) &&
        AtomicMath::selfAdd(&wide, INT64_C(9)) == INT64_C(0x10000000A) &&
        AtomicMath::selfSub(&wide, INT64_C(2)) == INT64_C(0x100000008) &&
        AtomicMath::compareAndSwap(&wide, INT64_C(0x100000008), INT64_C(0x200000000)), "Apple int64 atomics");
}

#endif

#ifdef _WIN32
int NetInterfaceIpcTestCases::childMain(int32_t parentPid)
{
    NetInterfaceIpcReceiver receiver(parentPid);
    ProcessWork& work = ProcessWork::instance();
    work.addProcessReceiveCallback(&receiver);
    if (!work.initReceive(131072 + 12, 4))
    {
        work.uninitReceive();
        return 10;
    }
    Semaphore ready;
    const bool opened = ready.tryOpenProcessSemaphore(TestIpcName("NetWorkTest_child_ready_", parentPid));
    const bool announced = opened && ready.tryProcessSignal() == 0;
    const bool done = announced && receiver.m_done.wait(10000);
    work.uninitReceive();
    return done && receiver.m_count.load() == 40 && !receiver.m_failed.load() ? 0 : 11;
}

void NetInterfaceIpcTestCases::childTransfer()
{
    const int32_t pid = CSystem::currentProcessPid();
    Semaphore ready;
    ready.createProcessSemaphore(TestIpcName("NetWorkTest_child_ready_", pid), 1);
    wchar_t path[32768] = {};
    const DWORD length = GetModuleFileNameW(nullptr, path, 32768);
    CheckIpc(length > 0 && length < 32768, "test executable path");
    std::wostringstream arguments;
    arguments << L'"' << path << L"\" --ipc-child=" << pid;
    std::wstring command = arguments.str();
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION child = {};
    netTestCheckpoint();
    if (!CreateProcessW(path, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
        nullptr, nullptr, &startup, &child))
    {
        throw std::runtime_error("start owned IPC child");
    }
    CloseHandle(child.hThread);
    try
    {
        while (!ready.processWait(10))
        {
            CheckIpc(WaitForSingleObject(child.hProcess, 0) == WAIT_TIMEOUT, "IPC child exited during startup");
        }
        NetSenderIpcChannel sender;
        CheckIpc(sender.open(static_cast<int32_t>(child.dwProcessId), pid, 131072) == NET_SENDER_IPC_ACCEPTED,
            "open real child ProcessWork");
        for (int32_t index = 0; index < 40; ++index)
        {
            std::vector<uint8_t> payload(IpcPayloadSize(index));
            for (size_t offset = 0; offset < payload.size(); ++offset)
            {
                payload[offset] = static_cast<uint8_t>(index + offset * 13);
            }
            int32_t result;
            do
            {
                netTestCheckpoint();
                result = sender.trySend(payload.empty() ? nullptr : &payload[0],
                    static_cast<uint32_t>(payload.size()), 42);
                if (result == NET_SENDER_IPC_BUSY)
                {
                    Sleep(1);
                }
            }
            while (result == NET_SENDER_IPC_BUSY);
            CheckIpc(result == NET_SENDER_IPC_ACCEPTED, "real child send failed");
        }
        while (WaitForSingleObject(child.hProcess, 10) == WAIT_TIMEOUT)
        {
            netTestCheckpoint();
            // 接收者收到40包后可以先撤销共享入口再退出；不把正常收尾当作通知失败。
            sender.flushNotifications();
        }
        DWORD exitCode = 0;
        CheckIpc(GetExitCodeProcess(child.hProcess, &exitCode) && exitCode == 0, "real ProcessWork child rejected transfer");
    }
    catch (...)
    {
        // 只终止本用例创建的子进程；绝不按名字查找/结束外部程序。
        TerminateProcess(child.hProcess, 12);
        WaitForSingleObject(child.hProcess, 2000);
        CloseHandle(child.hProcess);
        throw;
    }
    CloseHandle(child.hProcess);
}

#endif

NetInterfaceIpcAuditReceiver::NetInterfaceIpcAuditReceiver(bool fail) :
m_throw(fail), m_count(0), m_bad(false)
{

}

void NetInterfaceIpcAuditReceiver::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
{
    if (sendPid != static_cast<int32_t>(CSystem::currentProcessPid()) || length != 4 || buffer == nullptr ||
        std::memcmp(buffer, "TEST", 4) != 0 || static_cast<int32_t>(type) != 42)
    {
        m_bad = true;
    }
    ++m_count;
    if (m_throw)
    {
        throw std::runtime_error("controlled IPC callback failure");
    }
}

#ifdef _WIN32
void NetInterfaceIpcTestCases::runAuditChild(const std::wstring& mode)
{
    wchar_t path[32768] = {};
    const DWORD size = GetModuleFileNameW(nullptr, path, 32768);
    CheckIpc(size > 0 && size < 32768, "audit executable path");
    std::wstring command = L"\"" + std::wstring(path) + L"\" --ipc-audit=" + mode;
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION child = {};
    CheckIpc(CreateProcessW(path, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
        nullptr, nullptr, &startup, &child) != FALSE, "start owned IPC audit child");
    CloseHandle(child.hThread);
    const DWORD waited = WaitForSingleObject(child.hProcess, mode == L"read" ? 10000 : 5000);
    DWORD code = 0;
    const bool exited = waited == WAIT_OBJECT_0 && GetExitCodeProcess(child.hProcess, &code) != FALSE;
    if (!exited)
    {
        TerminateProcess(child.hProcess, 98);
        WaitForSingleObject(child.hProcess, 2000);
    }
    CloseHandle(child.hProcess);
    std::printf("IPC_AUDIT mode=%ls exited=%u code=%u\n", mode.c_str(), exited ? 1 : 0, static_cast<uint32_t>(code));
    CheckIpc(exited && code == 0, "owned ProcessWork audit child failed or timed out");
}

void NetInterfaceIpcTestCases::callbackIsolation()
{
    runAuditChild(L"callback");
}

void NetInterfaceIpcTestCases::readSlotIsolation()
{
    runAuditChild(L"read");
    runAuditChild(L"read-crash");
}

void NetInterfaceIpcTestCases::initializationBounds()
{
    runAuditChild(L"init");
}

void NetInterfaceIpcTestCases::notificationCreationFailure()
{
    runAuditChild(L"semaphore");
}

void NetInterfaceIpcTestCases::invalidSendBounds()
{
    runAuditChild(L"send-bounds");
}

void NetInterfaceIpcTestCases::ipcCapacity()
{
    runAuditChild(L"gateway-capacity-open");
}

void NetInterfaceIpcTestCases::receiverOwnership()
{
    runAuditChild(L"gateway-receiver-owner");
}

void NetInterfaceIpcTestCases::externalReceiverCapacity()
{
    runAuditChild(L"gateway-external-capacity");
}

void NetInterfaceIpcTestCases::localReceiverCapacity()
{
    runAuditChild(L"gateway-local-capacity");
}

void NetInterfaceIpcTestCases::managedReceiverRollback()
{
    runAuditChild(L"gateway-managed-rollback");
}

int NetInterfaceIpcTestCases::auditChild(const std::string& mode)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    const int32_t pid = static_cast<int32_t>(CSystem::currentProcessPid());
    if (mode == "gateway-managed-rollback")
    {
        ProcessWork& work = ProcessWork::instance();
        NetInterfaceIpcAuditReceiver receiver(false);
        work.initPostThread();
        work.addProcessReceiveCallback(&receiver);
        const int32_t threads = CTaskThreadManager::Instance().Count();
        uint64_t identity = 0;
        CheckIpc(CSystem::processIdentity(pid, identity), "managed rollback process identity");
        SharedMemory directoryMemory("P2D" + std::to_string(pid), sizeof(ProcessWorkDirectory), true);
        ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
        CheckIpc(directory != nullptr, "managed rollback owns discovery reference");
        const uint32_t next = static_cast<uint32_t>(AtomicMath::selfAdd(&directory->m_nextSession, static_cast<int64_t>(0)) + 1);
        const std::string name = ProcessWorkChannel::objectName('R', pid, identity, next);
        HANDLE incompatible = CreateEventA(nullptr, TRUE, FALSE, name.c_str());
        CheckIpc(incompatible != nullptr, "managed rollback controlled notification collision");
        bool valid = true;
        {
            NetSenderTestClient<NetSender, NetSenderClient> sender;
            valid = sender.prepareGateway() == NI_INVALID_STATE;
            CloseHandle(incompatible);
            valid = work.receiveBufferCount() == 0 && CTaskThreadManager::Instance().Count() == threads && valid;
            const int32_t retried = sender.prepareGateway();
            valid = retried == NI_OK && valid;
            if (retried == NI_OK)
            {
                work.post(pid, "TEST", 4, static_cast<MessageType>(42));
                work.post(pid, "TEST", 4, static_cast<MessageType>(42));
                const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
                while (receiver.m_count.load() < 2 && std::chrono::steady_clock::now() < deadline)
                {
                    CSystem::sleepPrecise(1);
                }
                valid = receiver.m_count.load() == 2 && !receiver.m_bad.load() && valid;
            }
            sender.releaseGateway();
        }
        work.uninitReceive();
        CheckIpc(valid, "failed Hub init preserved the existing Post task and callback for a real retry");
        return 0;
    }
    if (mode == "gateway-capacity-open")
    {
        const int32_t sizes[] = { 16384, 16451, 16452, 65548 };
        for (size_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index)
        {
            ProcessWorkChannel receiver;
            CheckIpc(receiver.openReceiver(sizes[index], 2, 0, 0, 0, 0), "capacity receiver initialized");
            receiver.publish();
            NetSenderIpcChannel sender;
            const int32_t opened = sender.open(pid, pid, 16440);
            if (sizes[index] < 16452)
            {
                CheckIpc(opened == NET_SENDER_IPC_TOO_LARGE, "undersized declared receiver rejected at open");
                CheckIpc(sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_UNAVAILABLE,
                    "failed open did not keep a usable half-channel");
                CheckIpc(sender.open(pid, pid, static_cast<uint32_t>(sizes[index] - 12)) == NET_SENDER_IPC_ACCEPTED,
                    "low-level sender can explicitly use a smaller declared payload limit");
            }
            else
            {
                CheckIpc(opened == NET_SENDER_IPC_ACCEPTED, "exact or larger gateway receiver accepted");
            }
            LockFreeQueue<char*> packets;
            Semaphore delivered;
            std::atomic<int32_t> full(0);
            receiver.pump(packets, delivered, full);
            const uint32_t limit = static_cast<uint32_t>(sizes[index] < 16452 ? sizes[index] - 12 : 16440);
            std::vector<uint8_t> bytes(limit + 1, 0);
            for (uint32_t at = 0; at < bytes.size(); ++at)
            {
                bytes[at] = static_cast<uint8_t>(at * 29 + 7);
            }
            CheckIpc(sender.trySend(bytes.data(), limit + 1, 42) == NET_SENDER_IPC_TOO_LARGE,
                "one byte above configured body rejected without consuming quota");
            CheckIpc(sender.trySend(bytes.data(), limit, 42) == NET_SENDER_IPC_ACCEPTED,
                "exact configured body committed");
            CheckIpc(receiver.pump(packets, delivered, full) == 1, "exact body copied once");
            char* packet = nullptr;
            CheckIpc(packets.pop(&packet) && packet != nullptr, "complete body observable");
            std::unique_ptr<char[]> owned(packet);
            int32_t fields[3] = {};
            std::memcpy(fields, packet, sizeof(fields));
            CheckIpc(fields[0] == pid && fields[1] == 42 && static_cast<uint32_t>(fields[2]) == limit &&
                std::memcmp(packet + 12, bytes.data(), limit) == 0 && packets.empty(), "all body bytes and metadata exact");
            receiver.stop();
            CheckIpc(sender.trySend(bytes.data(), limit, 42) == NET_SENDER_IPC_UNAVAILABLE, "stopped generation rejected");
        }
        return 0;
    }
    if (mode == "gateway-receiver-owner")
    {
        ProcessWork& work = ProcessWork::instance();
        NetInterfaceIpcAuditReceiver receiver(false);
        work.addProcessReceiveCallback(&receiver);
        CheckIpc(work.initReceive(256, 4), "foreign receiver initialized");
        const int32_t threads = CTaskThreadManager::Instance().Count();
        bool valid = true;
        {
            NetSenderTestClient<NetSender, NetSenderClient> sender;
            valid = sender.prepareGateway() == NI_INVALID_STATE;
            sender.releaseGateway();
        }
        valid = valid && work.receiveBufferCount() == 4 && CTaskThreadManager::Instance().Count() == threads;
        const int32_t external = NetSenderDesktopBackend::prepareExternalReceiver();
        valid = external == NI_INVALID_STATE && valid;
        int32_t sent = PROCESS_WORK_UNAVAILABLE;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        if (work.receiveBufferCount() == 4)
        {
            do
            {
                sent = work.trySend(pid, "TEST", 4, static_cast<MessageType>(42));
                if (sent != PROCESS_WORK_BUSY)
                {
                    break;
                }
                CSystem::sleepPrecise(1);
            } while (std::chrono::steady_clock::now() < deadline);
            while (receiver.m_count.load() == 0 && std::chrono::steady_clock::now() < deadline)
            {
                CSystem::sleepPrecise(1);
            }
        }
        valid = sent == PROCESS_WORK_SENT && receiver.m_count.load() == 1 && !receiver.m_bad.load() && valid;
        work.uninitReceive();
        if (external == NI_OK)
        {
            NetSenderDesktopBackend::externalReceiverStopped();
        }
        CheckIpc(valid, "gateway refusal preserved the foreign receiver, callbacks and actual self delivery");
        return 0;
    }
    if (mode == "gateway-external-capacity")
    {
        ProcessWork& work = ProcessWork::instance();
        const int32_t threads = CTaskThreadManager::Instance().Count();
        bool valid = NetSenderDesktopBackend::prepareExternalReceiver() == NI_OK;
        {
            NetSenderTestClient<NetSender, NetSenderClient> sender;
            valid = sender.prepareGateway() == NI_INVALID_STATE && valid;
            sender.releaseGateway();
        }
        const int32_t sizes[] = { 16384, 16451, 16452, 65548 };
        for (size_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index)
        {
            valid = NetSenderDesktopBackend::prepareExternalReceiver() == NI_OK && valid;
            CheckIpc(work.initReceive(sizes[index], 4), "external receiver started");
            const int32_t expected = sizes[index] < 16452 ? NI_BUFFER_TOO_SMALL : NI_OK;
            {
                NetSenderTestClient<NetSender, NetSenderClient> sender;
                valid = sender.prepareGateway() == expected && valid;
                NetSenderClient* handle = nullptr;
                const int32_t created = NetSender::instance().createClient(handle);
                valid = created == NI_OK && handle != nullptr && valid;
                if (handle != nullptr)
                {
                    valid = NetSender::instance().prepareGateway(handle) == expected && valid;
                }
                sender.releaseGateway();
                if (handle != nullptr)
                {
                    valid = NetSender::instance().destroyClient(handle) == NI_OK && valid;
                }
            }
            valid = work.receiveBufferCount() == 4 && NetSenderDesktopBackend::externalReceiverStopped() == NI_INVALID_STATE && valid;
            work.uninitReceive();
            valid = NetSenderDesktopBackend::externalReceiverStopped() == NI_OK && valid;
            valid = CTaskThreadManager::Instance().Count() == threads && valid;
        }
        CheckIpc(valid, "external C++ facade setup rejects small capacity, preserves ownership and supports retry");
        return 0;
    }
    if (mode == "gateway-local-capacity")
    {
        ProcessWork& work = ProcessWork::instance();
        bool valid = true;
        const int32_t sizes[] = { 0, 16384, 16451, 16452 };
        for (size_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index)
        {
            NetSenderLocalInbox inbox;
            work.addProcessReceiveCallback(&inbox);
            if (sizes[index] != 0)
            {
                CheckIpc(work.initReceive(sizes[index], 4), "local receiver initialized");
            }
            NetLocalTestProbe secureListener(true);
            NetRawLocalTestProbe rawListener(true);
            {
                NetSenderLocal secure(inbox), raw(inbox);
                const int32_t expected = sizes[index] == 0 ? NI_INVALID_STATE :
                    (sizes[index] < 16452 ? NI_BUFFER_TOO_SMALL : NI_OK);
                valid = secure.listen(NetInterfaceTlsTestPair::options(true, "localhost"), &secureListener) == expected && valid;
                valid = raw.listenRaw(NetInterfaceTlsTestPair::strongRandom, nullptr, &rawListener) == expected && valid;
                secure.shutdown();
                raw.shutdown();
            }
            work.uninitReceive();
        }
        CheckIpc(valid, "TLS and opaque local entry points reject missing or undersized receivers before ready");
        return 0;
    }
    if (mode == "read" || mode == "read-crash")
    {
        PROCESS_INFORMATION child = {};
        uint64_t childIdentity = 0;
        if (mode == "read-crash")
        {
            wchar_t command[] = L"cmd.exe /d /c exit 17";
            STARTUPINFOW startup = {};
            startup.cb = sizeof(startup);
            if (!CreateProcessW(nullptr, command, nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                nullptr, nullptr, &startup, &child)) { return 32; }
            CloseHandle(child.hThread);
            const DWORD waited = WaitForSingleObject(child.hProcess, 2000);
            FILETIME created = {}, exited = {}, kernel = {}, user = {};
            if (waited != WAIT_OBJECT_0 || !GetProcessTimes(child.hProcess, &created, &exited, &kernel, &user))
            {
                if (waited != WAIT_OBJECT_0) { TerminateProcess(child.hProcess, 98); WaitForSingleObject(child.hProcess, 2000); }
                CloseHandle(child.hProcess);
                return 33;
            }
            childIdentity = (static_cast<uint64_t>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
        }
        // 保留退出进程句柄，验证“创建标识仍可查但已经退出”的回收分支。
        auto closeChild = [](void* handle) { if (handle != nullptr) { CloseHandle(handle); } };
        std::unique_ptr<void, decltype(closeChild)> ownedChild(child.hProcess, closeChild);
        const int32_t phases = mode == "read" ? 1 : 4;
        for (int32_t phase = 0; phase < phases; ++phase)
        {
            ProcessWorkChannel receiver;
            if (!receiver.openReceiver(64, 3, 0, 0, 0, 0)) { return 34; }
            receiver.publish();
            SharedMemory directoryMemory("P2D" + std::to_string(pid));
            ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
            if (directory == nullptr) { return 34; }
            SharedMemory controlMemory(ProcessWorkChannel::objectName('C', pid, directory->m_identity, static_cast<uint32_t>(directory->m_session)));
            ProcessWorkControl* control = static_cast<ProcessWorkControl*>(controlMemory.writeWithoutLock());
            if (control == nullptr) { return 34; }
            LockFreeQueue<char*> packets;
            Semaphore delivered;
            std::atomic<int32_t> full(0);
            if (mode == "read")
            {
                NetSenderIpcChannel sender;
                if (sender.open(pid, pid, 52) != NET_SENDER_IPC_ACCEPTED) { return 35; }
                receiver.pump(packets, delivered, full);
                const int64_t token = control->m_peers[0].m_token;
                if (token <= 0 || !AtomicMath::compareAndSwap(&control->m_blocks[0].m_owner, static_cast<int64_t>(0), token))
                {
                    return 35;
                }
                // 未提交写者没有占读取环；其他线程已提交正文可以交付，自己的占用仍须保留。
                if (sender.trySend(reinterpret_cast<const uint8_t*>("TEST"), 4, 11) != NET_SENDER_IPC_ACCEPTED ||
                    receiver.pump(packets, delivered, full) != 1 || control->m_blocks[0].m_owner != token) { return 36; }
                char* packet = nullptr;
                if (!packets.pop(&packet) || packet == nullptr) { return 36; }
                std::unique_ptr<char[]> owned(packet);
                int32_t header[3] = {};
                std::memcpy(header, packet, sizeof(header));
                if (header[1] != 11 || header[2] != 4 || std::memcmp(packet + 12, "TEST", 4) != 0)
                {
                    return 36;
                }
                AtomicMath::compareAndSwap(&control->m_blocks[0].m_owner, token, static_cast<int64_t>(0));
                const int32_t expected[] = { 17, 19 };
                for (int32_t i = 0; i < 2; ++i)
                {
                    if (sender.trySend(reinterpret_cast<const uint8_t*>("TEST"), 4, expected[i]) != NET_SENDER_IPC_ACCEPTED) { return 36; }
                }
                if (receiver.pump(packets, delivered, full) != 2) { return 36; }
                for (int32_t i = 0; i < 2; ++i)
                {
                    packet = nullptr;
                    if (!packets.pop(&packet) || packet == nullptr) { return 36; }
                    owned.reset(packet);
                    std::memcpy(header, packet, sizeof(header));
                    if (header[1] != expected[i] || header[2] != 4 || std::memcmp(packet + 12, "TEST", 4) != 0)
                    {
                        return 36;
                    }
                }
                if (!packets.empty()) { return 36; }
            }
            else
            {
                const int32_t dead = static_cast<int32_t>(child.dwProcessId);
                control->m_peers[0].m_pid = dead;
                control->m_peers[0].m_identity = childIdentity;
                control->m_peers[0].m_token = control->m_nextToken = 7;
                control->m_blocks[0].m_owner = 7;
                SharedMemory memory(ProcessWorkChannel::objectName('B', pid, directory->m_identity,
                    static_cast<uint32_t>(directory->m_session), 0, 1));
                char* data = static_cast<char*>(memory.writeWithoutLock());
                if (data == nullptr) { return 34; }
                const int32_t header[] = { dead, 42, 4 };
                if (phase >= 1) { std::memcpy(data, header, 12); std::memcpy(data + 12, "TEST", 4); }
                if (phase >= 2) { control->m_ring[0] = 1; }
                if (phase == 3)
                {
                    uint64_t identity = 0;
                    if (!CSystem::processIdentity(pid, identity))
                    {
                        return 35;
                    }
                    control->m_peers[1].m_pid = pid;
                    control->m_peers[1].m_identity = identity;
                    control->m_peers[1].m_token = control->m_nextToken = 8;
                    control->m_blocks[1].m_owner = 8;
                }
                const int32_t copied = receiver.pump(packets, delivered, full);
                if (copied != (phase >= 2 ? 1 : 0) || control->m_blocks[0].m_owner != 0 ||
                    control->m_head != control->m_tail || (phase == 3 && control->m_blocks[1].m_owner != 8)) { return 36; }
                if (phase >= 2)
                {
                    char* packet = nullptr;
                    if (!packets.pop(&packet) || packet == nullptr) { return 36; }
                    std::unique_ptr<char[]> owned(packet);
                    if (std::memcmp(packet, header, 12) != 0 || std::memcmp(packet + 12, "TEST", 4) != 0)
                    {
                        return 36;
                    }
                }
                if (!packets.empty()) { return 36; }
            }
        }
        return 0;
    }
    if (mode == "send-bounds")
    {
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
        ProcessWork& work = ProcessWork::instance();
        const MessageType type = static_cast<MessageType>(42);
        const std::string name = CSystem::GetCurrentExeName();
        // 原实现在第一个负长度post中发生实际越界复制；父runner有总期限并核对退出码。
        work.post(pid, "TEST", -1, type);
        work.post(name, "TEST", -1, type);
        work.post(pid, "TEST", INT32_MAX, type);
        work.post(name, "TEST", INT32_MAX, type);
        NetInterfaceIpcAuditReceiver receiver(false);
        work.addProcessReceiveCallback(&receiver);
        if (!work.initReceive(64, 4)) { work.uninitReceive(); return 37; }
        work.initPostThread();
        work.send(pid, "TEST", -1, type);
        work.send(name, "TEST", -1, type);
        // v2按声明块容量校验，不把Windows页尾余量当成有效正文容量。
        const std::vector<char> oversized(53, 'x');
        if (work.trySend(pid, oversized.data(), static_cast<int32_t>(oversized.size()), type) != PROCESS_WORK_TOO_LARGE)
        {
            work.uninitReceive();
            return 38;
        }
        for (uint32_t index = 0; index < 8; ++index)
        {
            work.send(pid, oversized.data(), static_cast<int32_t>(oversized.size()), type);
        }
        work.post(pid, "TEST", 4, type);
        work.post(name, "TEST", 4, type);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (receiver.m_count.load() < 2 && std::chrono::steady_clock::now() < deadline)
        {
            CSystem::sleepPrecise(1);
        }
        const bool passed = receiver.m_count.load() == 2 && !receiver.m_bad.load();
        work.uninitReceive();
        return passed ? 0 : 38;
    }
    if (mode == "semaphore")
    {
        ProcessWork& work = ProcessWork::instance();
        NetInterfaceIpcAuditReceiver receiver(false);
        work.initPostThread();
        work.addProcessReceiveCallback(&receiver);
        uint64_t identity = 0;
        if (!CSystem::processIdentity(pid, identity))
        {
            work.uninitReceive(); return 28;
        }
        SharedMemory directoryMemory("P2D" + std::to_string(pid), sizeof(ProcessWorkDirectory), true);
        ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
        if (directory == nullptr) { work.uninitReceive(); return 28; }
        const uint32_t next = static_cast<uint32_t>(AtomicMath::selfAdd(&directory->m_nextSession, static_cast<int64_t>(0)) + 1);
        const std::string name = ProcessWorkChannel::objectName('R', pid, identity, next);
        HANDLE incompatible = CreateEventA(nullptr, TRUE, FALSE, name.c_str());
        if (incompatible == nullptr)
        {
            work.uninitReceive();
            return 28;
        }
        const bool initialized = work.initReceive(64, 4);
        CloseHandle(incompatible);
        if (initialized)
        {
            work.uninitReceive();
            return 29;
        }
        // 不重建发送线程、不重登回调、不显式uninit；失败初始化必须只撤销自己的资源。
        if (!work.initReceive(64, 4))
        {
            work.uninitReceive();
            return 30;
        }
        work.post(pid, "TEST", 4, static_cast<MessageType>(42));
        work.post(CSystem::GetCurrentExeName(), "TEST", 4, static_cast<MessageType>(42));
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (receiver.m_count.load() < 2 && std::chrono::steady_clock::now() < deadline)
        {
            CSystem::sleepPrecise(1);
        }
        const bool passed = receiver.m_count.load() == 2 && !receiver.m_bad.load();
        work.uninitReceive();
        return passed ? 0 : 31;
    }
    if (mode != "callback" && mode != "init" && mode != "semaphore")
    {
        return 23;
    }
    ProcessWork& work = ProcessWork::instance();
    NetInterfaceIpcAuditReceiver throwing(true);
    NetInterfaceIpcAuditReceiver good(false);
    if (mode == "callback")
    {
        work.addProcessReceiveCallback(&throwing);
        work.addProcessReceiveCallback(nullptr);
    }
    work.addProcessReceiveCallback(&good);
    if (mode == "init" && (work.initReceive(0, 1) || work.initReceive(16, 0) || work.initReceive(16, -1)))
    {
        work.uninitReceive();
        return 24;
    }
    if (!work.initReceive(64, 4))
    {
        work.uninitReceive();
        return 25;
    }
    if (mode == "init" && work.initReceive(64, 4))
    {
        work.uninitReceive();
        return 26;
    }
    NetSenderIpcChannel sender;
    bool passed = sender.open(pid, pid, 52) == NET_SENDER_IPC_ACCEPTED;
    for (uint32_t i = 0; passed && i < 2; ++i)
    {
        const auto sendDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        int32_t result = NET_SENDER_IPC_BUSY;
        while (result == NET_SENDER_IPC_BUSY && std::chrono::steady_clock::now() < sendDeadline)
        {
            result = sender.trySend(reinterpret_cast<const uint8_t*>("TEST"), 4, 42);
            if (result == NET_SENDER_IPC_BUSY) { CSystem::sleepPrecise(1); }
        }
        passed = result == NET_SENDER_IPC_ACCEPTED;
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (passed && good.m_count.load() < 2 && std::chrono::steady_clock::now() < deadline)
    {
        passed = sender.flushNotifications() >= 0;
        CSystem::sleepPrecise(1);
    }
    passed = passed && good.m_count.load() == 2 && !good.m_bad.load() &&
        (mode != "callback" || (throwing.m_count.load() == 2 && !throwing.m_bad.load()));
    sender.close();
    work.uninitReceive();
    return passed ? 0 : 27;
}

#endif

void NetInterfaceIpcTestCases::queueLifecycle()
{
    LockFreeQueue<int32_t> queue;
    int32_t value = 99;
    CheckIpc(!queue.pop(&value) && value == 99, "empty queue changed output");
    queue.push(7);
    CheckIpc(queue.pop(&value) && value == 7 && queue.empty(), "normal queue lifecycle");
    queue.destroy();
    queue.push(9);
    CheckIpc(!queue.pop(&value) && queue.empty() && queue.size() == 0 && value == 7,
        "destroyed queue must reject publication without abort or leaked nodes");
    queue.init();
    queue.push(11);
    CheckIpc(queue.pop(&value) && value == 11 && queue.empty(), "queue failed to recover after init");
}

// PROCESS_WORK_WAKE_UNIT_TEST_BEGIN
//#include "ProcessReceiveCallback.h"
//#include "Semaphore/SemaphoreAPI.h"
//#include <atomic>
//#include "CopyTask.h"
//#include "ReceiveTask.h"
//#include "SendTask.h"
//#include "SendNameTask.h"
//#include <cstdio>
//#include <cstring>
//
///** 对四类真实任务预置无数据通知，再校验合法数据确实经过同一任务
//@param [in] argc 参数个数
//@param [in] argv 模式：copy/receive/send/name
//@return 0通过，非零失败；旧实现必须在空通知处出现访问冲突
//*/
//int main(int argc, char** argv)
//{
//    /** 仅测试main使用的接收探针，不保存回调借用内存
//    */
//    class ProcessWorkWakeProbe : public ProcessReceiveCallback
//    {
//    public:
//        /** 初始化接收计数和校验结果
//        */
//        ProcessWorkWakeProbe() :
//        m_count(0),
//        m_failed(false)
//        {
//
//        }
//
//        /** 校验有效载荷及type0消息仍到达
//        @param [in] sendPid 正发送方
//        @param [in] buffer 借入正文
//        @param [in] length 正文长度
//        @param [in] type 原始类型
//        */
//        void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
//        {
//            if (sendPid <= 0 || (type != INIT && type != MESSAGE) || length != 4 ||
//                buffer == nullptr || std::memcmp(buffer, "ABCD", 4) != 0)
//            {
//                m_failed.store(true);
//            }
//            ++m_count;
//            m_received.signal();
//        }
//
//        // 已收到的合法或非法消息数
//        std::atomic<uint32_t> m_count;
//        // 是否观察到不符合预期的消息
//        std::atomic<bool> m_failed;
//        // 每次实际回调均通知等待方
//        Semaphore m_received;
//    };
//    /** 构造真实队列封包，长度和类型独立，内存交给消费任务
//    @param [in] first 发送方或目标PID
//    @param [in] type 原始消息类型
//    @return 拥有的16字节封包
//    */
//    const auto WakePacket = [](int32_t first, MessageType type) -> char*
//    {
//        char* packet = new char[16];
//        const int32_t fields[3] = { first, static_cast<int32_t>(type), 4 };
//        std::memcpy(packet, fields, 12);
//        std::memcpy(packet + 12, "ABCD", 4);
//        return packet;
//    };
//
//    if (argc != 2) { return 2; }
//    const std::string mode(argv[1]);
//    Semaphore notification;
//    Semaphore delivered;
//    LockFreeQueue<int32_t> assigned;
//    LockFreeQueue<char*> incoming;
//    LockFreeQueue<char*> outgoing;
//    ProcessWorkWakeProbe probe;
//    std::vector<ProcessReceiveCallback*> callbacks(1, &probe);
//    std::atomic<bool> draining(false);
//    std::shared_ptr<CTask> task;
//    std::map<int32_t, std::shared_ptr<SharedMemory>> memories;
//    SharedMemory area("PW-Wake-Area-" + std::to_string(CSystem::currentProcessPid()), 36, false);
//    if (area.isFailed()) { return 3; }
//    int32_t* slots = static_cast<int32_t*>(area.writeWithoutLock());
//    if (slots == nullptr) { return 3; }
//    slots[0] = 4;
//    for (int32_t id = 1; id <= 4; ++id)
//    {
//        slots[id * 2 - 1] = id;
//        slots[id * 2] = 123;
//        memories[id].reset(new SharedMemory("PW-Wake-Block-" + std::to_string(CSystem::currentProcessPid()) +
//            "-" + std::to_string(id), id == 4 ? 8 : 64, false));
//        if (memories[id]->isFailed()) { return 3; }
//        char* memory = static_cast<char*>(memories[id]->writeWithoutLock());
//        if (memory == nullptr) { return 3; }
//        if (id != 4)
//        {
//            std::unique_ptr<char[]> packet(WakePacket(static_cast<int32_t>(CSystem::currentProcessPid()), MESSAGE));
//            std::memcpy(memory, packet.get(), 16);
//            const int32_t size = id == 2 ? -1 : id == 3 ? 1024 : 4;
//            std::memcpy(memory + 8, &size, 4);
//        }
//    }
//    if (mode == "copy")
//    {
//        std::shared_ptr<CopyTask> copy(new CopyTask);
//        copy->setParam(&area, &memories, &assigned, &notification, &incoming, &delivered);
//        task = copy;
//    }
//    else if (mode == "receive")
//    {
//        std::shared_ptr<ReceiveTask> receive(new ReceiveTask);
//        receive->setParam(&callbacks, &incoming, &notification);
//        task = receive;
//    }
//    else if (mode == "send" || mode == "name")
//    {
//        ProcessWork::instance().addProcessReceiveCallback(&probe);
//        if (!ProcessWork::instance().initReceive(64, 4))
//{
//    return 4;
//}

//        if (mode == "send")
//        {
//            std::shared_ptr<SendTask> send(new SendTask);
//            send->setParam(&draining, &outgoing, &notification);
//            task = send;
//        }
//        else
//        {
//            std::shared_ptr<SendNameTask> send(new SendNameTask);
//            send->setParam(&draining, &outgoing, &notification);
//            task = send;
//        }
//    }
//    else { return 2; }
//    // 模拟上一轮StopTask残留的计数，队列明确为空。
//    for (uint32_t index = 0; index < 8; ++index) { notification.signal(); }
//    const uint32_t worker = CTaskThreadManager::Instance().Init();
//    if (worker == 0) { return 4; }
//    CTaskThreadManager::Instance().GetThreadInterface(worker)->PostTask(task);
//    CSystem::Sleep(50);
//    bool passed = true;
//    if (mode == "copy")
//    {
//        const int32_t values[] = { 0, 999, 2, 3, 4, 1 };
//        for (int32_t value : values) { assigned.push(value); notification.signal(); }
//        passed = delivered.wait(2000);
//        char* packet = nullptr;
//        passed = incoming.pop(&packet) && packet != nullptr && passed;
//        if (packet != nullptr)
//        {
//            int32_t size = 0;
//            std::memcpy(&size, packet + 8, 4);
//            passed = passed && size == 4 && std::memcmp(packet + 12, "ABCD", 4) == 0;
//            delete[] packet;
//        }
//        passed = passed && !delivered.wait(30) && incoming.empty();
//        passed = passed && slots[4] == 0 && slots[6] == 0 && slots[8] == 0;
//    }
//    else
//    {
//        LockFreeQueue<char*>* queue = mode == "receive" ? &incoming : &outgoing;
//        queue->push(nullptr);
//        notification.signal();
//        for (uint32_t index = 0; index < 2; ++index)
//        {
//            const MessageType type = index == 0 ? INIT : MESSAGE;
//            char* packet = nullptr;
//            if (mode == "name")
//            {
//                const std::string name = CSystem::GetCurrentExeName();
//                packet = new char[name.size() + 13];
//                std::memcpy(packet, name.c_str(), name.size() + 1);
//                const int32_t fields[2] = { static_cast<int32_t>(type), 4 };
//                std::memcpy(packet + name.size() + 1, fields, 8);
//                std::memcpy(packet + name.size() + 9, "ABCD", 4);
//            }
//            else { packet = WakePacket(static_cast<int32_t>(CSystem::currentProcessPid()), type); }
//            queue->push(packet);
//            notification.signal();
//        }
//        passed = probe.m_received.wait(2000) && probe.m_received.wait(2000);
//    }
//    task->StopTask();
//    CTaskThreadManager::Instance().Uninit(worker);
//    task.reset();
//    if (mode == "send" || mode == "name") { ProcessWork::instance().uninitReceive(); }
//    passed = passed && !probe.m_failed.load() && (mode == "copy" || probe.m_count.load() == 2);
//    std::printf("PROCESS_WORK_WAKE mode=%s %s\n", mode.c_str(), passed ? "PASS" : "FAIL");
//    return passed ? 0 : 5;
//}
// PROCESS_WORK_WAKE_UNIT_TEST_END

// PROCESS_WORK_RELIABILITY_UNIT_TEST_BEGIN
//#define PROCESS_WORK_RELIABILITY_UNIT_TEST
//#include "NetInterfaceIpcTestCases.h"
//#include "SendTask.h"
//#include "SendNameTask.h"
//#include "CopyTask.h"
//#include "CSystem/CSystemAPI.h"
//#include "SharedMemory/SharedMemoryAPI.h"
//#include "AtomicMath/AtomicMathAPI.h"
//#include <Windows.h>
//#include <new>
//#include <cstdlib>
//#include <cstdio>
//#include <cstring>
//#include <memory>
//#include <vector>
//
//static volatile LONG s_faultThread = 0;
//static volatile LONG s_faultKind = 0;
//static volatile LONG s_faultEnabled = 0;
//static volatile LONG s_faultCount = 0;
//static volatile LONG s_initFailure = 0;
//static volatile LONG s_threadAllocations = 0;
//static DWORD s_mainThread = 0;
//__declspec(thread) static bool s_arrayAllocation = false;
//static volatile LONG s_trackMappings = 0;
//static volatile LONG s_mappingCount = 0;
//static volatile LONG s_holdFirstMapping = 0;
//static void* s_views[128] = {};
//
//void* operator new(size_t size)
//{
//    const LONG kind = InterlockedCompareExchange(&s_faultKind, 0, 0);
//    if (s_faultEnabled && static_cast<LONG>(GetCurrentThreadId()) == s_faultThread &&
//        (kind == 1 || (kind == 2 && s_arrayAllocation) || (kind == 3 && !s_arrayAllocation)))
//    {
//        if (InterlockedIncrement(&s_faultCount) == 1)
//        {
//            const char text[] = "FAULT_OBSERVED\n";
//            DWORD written = 0;
//            WriteFile(GetStdHandle(STD_ERROR_HANDLE), text, sizeof(text) - 1, &written, nullptr);
//        }
//        throw std::bad_alloc(); // Isolated, deliberate allocator fault injection.
//    }
//    if (s_initFailure && GetCurrentThreadId() == s_mainThread && size == sizeof(CTaskThread) &&
//        InterlockedIncrement(&s_threadAllocations) == 2)
//    {
//        InterlockedExchange(&s_initFailure, 0);
//        throw std::bad_alloc();
//    }
//    void* memory = std::malloc(size == 0 ? 1 : size);
//    if (memory == nullptr)
//    {
//        throw std::bad_alloc();
//    }
//    return memory;
//}
//
//void operator delete(void* memory) throw()
//{
//    std::free(memory);
//}
//
//void* operator new[](size_t size)
//{
//    const bool previous = s_arrayAllocation;
//    s_arrayAllocation = true;
//    try
//    {
//        void* memory = ::operator new(size);
//        s_arrayAllocation = previous;
//        return memory;
//    }
//    catch (...)
//    {
//        s_arrayAllocation = previous;
//        throw;
//    }
//}
//
//void operator delete[](void* memory) throw()
//{
//    ::operator delete(memory);
//}
//
//void* operator new[](size_t size, const std::nothrow_t&) throw()
//{
//    try
//    {
//        return ::operator new[](size);
//    }
//    catch (...)
//    {
//        return nullptr;
//    }
//}
//
//void operator delete[](void* memory, const std::nothrow_t&) throw()
//{
//    ::operator delete[](memory);
//}
//
//LPVOID ReliabilityMapViewOfFile(HANDLE mapping, DWORD access, DWORD high, DWORD low, SIZE_T bytes)
//{
//    LPVOID view = ::MapViewOfFile(mapping, access, high, low, bytes);
//    if (s_trackMappings)
//    {
//        const LONG index = InterlockedIncrement(&s_mappingCount) - 1;
//        if (index < 128)
//        {
//            s_views[index] = view;
//        }
//        if (index == 0)
//        {
//            while (InterlockedCompareExchange(&s_holdFirstMapping, 0, 0))
//            {
//                Sleep(0);
//            }
//        }
//    }
//    return view;
//}
//
//ReliabilityReceiver::ReliabilityReceiver() :
//m_count(0),
//m_failed(false),
//m_seen(0),
//m_checkOrder(true)
//{
//    std::memset(m_next, 0, sizeof(m_next));
//}
//
//void ReliabilityReceiver::receive(int32_t, const char* buffer, int32_t length, MessageType type)
//{
//    int32_t value = -1;
//    bool valid = buffer != nullptr && length == 4 && type == MessageType::MESSAGE;
//    if (valid)
//    {
//        std::memcpy(&value, buffer, 4);
//        valid = value >= 0 && value < 8;
//    }
//    if (valid)
//    {
//        const uint32_t bit = 1U << value;
//        valid = (m_seen.fetch_or(bit) & bit) == 0;
//        if (m_checkOrder)
//        {
//            valid = valid && value % 2 == m_next[value / 2]++;
//        }
//    }
//    if (!valid)
//    {
//        m_failed.store(true);
//    }
//    m_count.fetch_add(1);
//}
//
//ReliabilityArmedTask::ReliabilityArmedTask(const std::shared_ptr<CTask>& inner, int32_t kind) :
//m_inner(inner),
//m_kind(kind)
//{
//}
//
//void ReliabilityArmedTask::DoTask()
//{
//    InterlockedExchange(&s_faultThread, static_cast<LONG>(GetCurrentThreadId()));
//    InterlockedExchange(&s_faultKind, m_kind);
//    InterlockedExchange(&s_faultEnabled, 1);
//    m_inner->DoTask();
//    InterlockedExchange(&s_faultEnabled, 0);
//}
//
//void ReliabilityArmedTask::StopTask()
//{
//    m_inner->StopTask();
//}
//
//bool ReliabilityCacheSender::mappingsReady()
//{
//    for (auto it = m_sendProcessAssignMap.begin(); it != m_sendProcessAssignMap.end(); ++it)
//    {
//        if (it->second->memory() == nullptr) { return false; }
//    }
//    for (auto it = m_sendProcessReadMap.begin(); it != m_sendProcessReadMap.end(); ++it)
//    {
//        if (it->second->memory() == nullptr) { return false; }
//    }
//    for (auto it = m_sendMemoryMap.begin(); it != m_sendMemoryMap.end(); ++it)
//    {
//        if (it->second->memory() == nullptr) { return false; }
//    }
//    return !m_sendProcessAssignMap.empty() && !m_sendProcessReadMap.empty() && !m_sendMemoryMap.empty();
//}
//
//ReliabilityCacheTask::ReliabilityCacheTask(ReliabilityCacheSender* sender, int32_t pid, int32_t producer) :
//m_sender(sender),
//m_pid(pid),
//m_producer(producer)
//{
//}
//
//void ReliabilityCacheTask::DoTask()
//{
//    for (int32_t sequence = 0; sequence < 2; ++sequence)
//    {
//        const int32_t value = m_producer * 2 + sequence;
//        m_sender->send(m_pid, reinterpret_cast<const char*>(&value), 4);
//    }
//}
//
///** Wait for an atomic count with a finite diagnostic deadline.
// */
//static bool WaitCount(const std::atomic<int32_t>& count, int32_t expected, DWORD milliseconds = 1500)
//{
//    const DWORD start = GetTickCount();
//    while (count.load() < expected && GetTickCount() - start < milliseconds)
//    {
//        CSystem::sleepPrecise(1);
//    }
//    return count.load() == expected;
//}
//
///** Observe a real allocation failure without releasing the test's fault.
// */
//static bool WaitFault()
//{
//    const DWORD start = GetTickCount();
//    while (!InterlockedCompareExchange(&s_faultCount, 0, 0) && GetTickCount() - start < 1000)
//    {
//        CSystem::sleepPrecise(1);
//    }
//    return s_faultCount != 0;
//}
//
///** Execute the real sender task against a live receiver, then recover or explicitly stop.
// */
//static bool CheckSender(bool byName, bool stop)
//{
//    ReliabilityReceiver receiver;
//    ProcessWork& work = ProcessWork::instance();
//    work.addProcessReceiveCallback(&receiver);
//    if (!work.initReceive(128, 4)) { return false; }
//    LockFreeQueue<char*> queue;
//    Semaphore available;
//    std::atomic<bool> waitEnd(true);
//    const std::string name = CSystem::GetCurrentExeName();
//    for (int32_t value = 0; value < 2; ++value)
//    {
//        const size_t prefix = byName ? name.size() + 1 : 4;
//        char* bytes = new char[prefix + 8 + 4];
//        if (byName) { std::memcpy(bytes, name.c_str(), prefix); }
//        else { const int32_t pid = GetCurrentProcessId(); std::memcpy(bytes, &pid, 4); }
//        const int32_t header[] = { static_cast<int32_t>(MessageType::MESSAGE), 4 };
//        std::memcpy(bytes + prefix, header, 8);
//        std::memcpy(bytes + prefix + 8, &value, 4);
//        queue.push(bytes);
//        available.signal();
//    }
//    available.signal();
//    std::shared_ptr<CTask> actual;
//    if (byName)
//    {
//        std::shared_ptr<SendNameTask> task(new SendNameTask());
//        task->setParam(&waitEnd, &queue, &available);
//        actual = task;
//    }
//    else
//    {
//        std::shared_ptr<SendTask> task(new SendTask());
//        task->setParam(&waitEnd, &queue, &available);
//        actual = task;
//    }
//    std::shared_ptr<ReliabilityArmedTask> armed(new ReliabilityArmedTask(actual, 1));
//    const uint32_t thread = CTaskThreadManager::Instance().Init();
//    CTaskThreadManager::Instance().GetThreadInterface(thread)->PostTask(armed);
//    bool passed = WaitFault();
//    CSystem::sleepPrecise(20);
//    passed = passed && receiver.m_count.load() == 0 && queue.size() == 1;
//    if (stop)
//    {
//        const DWORD start = GetTickCount();
//        CTaskThreadManager::Instance().Uninit(thread);
//        passed = passed && GetTickCount() - start < 500 && receiver.m_count.load() == 0;
//        InterlockedExchange(&s_faultEnabled, 0);
//    }
//    else
//    {
//        InterlockedExchange(&s_faultEnabled, 0);
//        passed = WaitCount(receiver.m_count, 2) && passed;
//        CTaskThreadManager::Instance().WaitForEnd(thread);
//        passed = passed && !receiver.m_failed && receiver.m_seen.load() == 3 && queue.empty();
//    }
//    char* remaining = nullptr;
//    while (queue.pop(&remaining)) { delete[] remaining; }
//    work.uninitReceive();
//    return passed;
//}
//
///** Keep both legitimate shared blocks owned until copying and queue publication succeed.
// */
//static bool CheckCopy(int32_t kind, bool stop)
//{
//    const int32_t pid = GetCurrentProcessId();
//    const std::string name = "ReliabilityCopy_" + std::to_string(pid);
//    SharedMemory assignment(name + "_assign", 20, false);
//    int32_t* words = static_cast<int32_t*>(assignment.writeWithoutLock());
//    if (words == nullptr) { return false; }
//    const int32_t initial[] = { 2, 1, pid, 2, pid };
//    std::memcpy(words, initial, sizeof(initial));
//    std::map<int32_t, std::shared_ptr<SharedMemory>> memories;
//    LockFreeQueue<int32_t> assigned;
//    LockFreeQueue<char*> copied;
//    Semaphore available, delivered;
//    for (int32_t index = 1; index <= 2; ++index)
//    {
//        std::shared_ptr<SharedMemory> memory(new SharedMemory(name + "_" + std::to_string(index), 64, false));
//        int32_t* frame = static_cast<int32_t*>(memory->writeWithoutLock());
//        if (frame == nullptr) { return false; }
//        const int32_t value[] = { pid, static_cast<int32_t>(MessageType::MESSAGE), 4, index - 1 };
//        std::memcpy(frame, value, sizeof(value));
//        memories[index] = memory;
//        assigned.push(index);
//        available.signal();
//    }
//    std::shared_ptr<CopyTask> actual(new CopyTask());
//    actual->setParam(&assignment, &memories, &assigned, &available, &copied, &delivered);
//    std::shared_ptr<ReliabilityArmedTask> armed(new ReliabilityArmedTask(actual, kind));
//    const uint32_t thread = CTaskThreadManager::Instance().Init();
//    CTaskThreadManager::Instance().GetThreadInterface(thread)->PostTask(armed);
//    bool passed = WaitFault();
//    CSystem::sleepPrecise(20);
//    passed = passed && words[2] == pid && words[4] == pid && copied.empty() && !delivered.wait(0);
//    if (!stop)
//    {
//        InterlockedExchange(&s_faultEnabled, 0);
//        for (int32_t value = 0; value < 2; ++value)
//        {
//            char* bytes = nullptr;
//            bool received = delivered.wait(1000) && copied.pop(&bytes) && bytes != nullptr;
//            std::unique_ptr<char[]> owned(bytes);
//            const int32_t expected[] = { pid, static_cast<int32_t>(MessageType::MESSAGE), 4, value };
//            passed = received && passed;
//            if (received) { passed = passed && std::memcmp(bytes, expected, sizeof(expected)) == 0; }
//        }
//        passed = passed && words[2] == 0 && words[4] == 0 && !delivered.wait(20) && copied.empty();
//    }
//    const DWORD start = GetTickCount();
//    CTaskThreadManager::Instance().Uninit(thread);
//    InterlockedExchange(&s_faultEnabled, 0);
//    passed = passed && GetTickCount() - start < 500;
//    char* remaining = nullptr;
//    while (copied.pop(&remaining)) { delete[] remaining; }
//    return passed;
//}
//
///** Fail creation of the second post thread, preserving prequeued work for a full retry.
// */
//static bool CheckInitialization()
//{
//    ReliabilityReceiver receiver;
//    receiver.m_checkOrder = false;
//    ProcessWork& work = ProcessWork::instance();
//    work.addProcessReceiveCallback(&receiver);
//    if (!work.initReceive(128, 4)) { return false; }
//    const int32_t baseline = CTaskThreadManager::Instance().Count();
//    const int32_t first = 0, second = 1;
//    work.post(static_cast<int32_t>(GetCurrentProcessId()), reinterpret_cast<const char*>(&first), 4);
//    work.post(CSystem::GetCurrentExeName(), reinterpret_cast<const char*>(&second), 4);
//    InterlockedExchange(&s_initFailure, 1);
//    try { work.initPostThread(); }
//    catch (const std::bad_alloc&) { }
//    InterlockedExchange(&s_initFailure, 0);
//    const int32_t failed = CTaskThreadManager::Instance().Count();
//    const bool preserved = failed == baseline && receiver.m_count.load() == 0;
//    work.initPostThread();
//    const int32_t retried = CTaskThreadManager::Instance().Count();
//    bool passed = preserved && retried == baseline + 2;
//    passed = WaitCount(receiver.m_count, 2) && passed;
//    passed = passed && !receiver.m_failed && receiver.m_seen.load() == 3;
//    work.uninitReceive();
//    std::printf("INIT baseline=%d failed=%d retried=%d\n", baseline, failed, retried);
//    return passed && CTaskThreadManager::Instance().Count() == 0;
//}
//
///** Force late callers to arrive while the first real cold mapping has not returned.
// */
//static bool CheckCache()
//{
//    ReliabilityReceiver receiver;
//    ProcessWork& work = ProcessWork::instance();
//    work.addProcessReceiveCallback(&receiver);
//    if (!work.initReceive(128, 4)) { return false; }
//    ReliabilityCacheSender sender;
//    std::vector<uint32_t> threads;
//    InterlockedExchange(&s_holdFirstMapping, 1);
//    InterlockedExchange(&s_trackMappings, 1);
//    for (int32_t index = 0; index < 4; ++index)
//    {
//        std::shared_ptr<ReliabilityCacheTask> task(new ReliabilityCacheTask(&sender, GetCurrentProcessId(), index));
//        const uint32_t thread = CTaskThreadManager::Instance().Init();
//        threads.push_back(thread);
//        CTaskThreadManager::Instance().GetThreadInterface(thread)->PostTask(task);
//        if (index == 0)
//        {
//            const DWORD start = GetTickCount();
//            while (!s_mappingCount && GetTickCount() - start < 1000) { CSystem::sleepPrecise(1); }
//        }
//    }
//    // The new cache holds its original write lock until mapping completes; old readers can map concurrently.
//    CSystem::sleepPrecise(50);
//    InterlockedExchange(&s_holdFirstMapping, 0);
//    for (size_t index = 0; index < threads.size(); ++index)
//    {
//        CTaskThreadManager::Instance().WaitForEnd(threads[index]);
//    }
//    InterlockedExchange(&s_trackMappings, 0);
//    bool passed = WaitCount(receiver.m_count, 8) && !receiver.m_failed && receiver.m_seen.load() == 255 && sender.mappingsReady();
//    sender.clear();
//    int32_t leaked = 0;
//    for (LONG index = 0; index < s_mappingCount && index < 128; ++index)
//    {
//        MEMORY_BASIC_INFORMATION info = {};
//        if (s_views[index] && VirtualQuery(s_views[index], &info, sizeof(info)) == sizeof(info) &&
//            info.State == MEM_COMMIT && info.Type == MEM_MAPPED && info.AllocationBase == s_views[index])
//        {
//            ++leaked;
//            UnmapViewOfFile(s_views[index]); // Only this test's recorded leftover view.
//        }
//    }
//    work.uninitReceive();
//    std::printf("CACHE views=%ld leaked=%d received=%d\n", s_mappingCount, leaked, receiver.m_count.load());
//    return passed && leaked == 0;
//}
//
//void ReliabilityLiveReceiver::armCopyFailure(int32_t kind)
//{
//    std::shared_ptr<CopyTask> task(new CopyTask());
//    task->setParam(m_areaAssign, &m_memoryMap, &m_assignQueue, &m_assignSemaphore, &m_receiveQueue, &m_receiveSemaphore);
//    std::shared_ptr<ReliabilityArmedTask> armed(new ReliabilityArmedTask(task, kind));
//    CTaskThreadManager::Instance().GetThreadInterface(m_copyThreadId)->PostTask(armed, 2);
//}
//
//bool ReliabilityLiveReceiver::sharedBlocksHeld()
//{
//    int32_t* words = static_cast<int32_t*>(m_areaAssign->memory());
//    return words && AtomicMath::selfAdd(words + 2, 0) != 0 && AtomicMath::selfAdd(words + 4, 0) != 0;
//}
//
///** Public send must commit both frames before the receiver's allocation fault is released.
// */
//static bool CheckLiveReceive(int32_t kind)
//{
//    ReliabilityReceiver callback;
//    ReliabilityLiveReceiver receiver;
//    receiver.addProcessReceiveCallback(&callback);
//    if (!receiver.initReceive(128, 2)) { return false; }
//    receiver.armCopyFailure(kind);
//    const DWORD start = GetTickCount();
//    while (!s_faultThread && GetTickCount() - start < 1000) { CSystem::sleepPrecise(1); }
//    ProcessWork& sender = ProcessWork::instance();
//    for (int32_t value = 0; value < 2; ++value)
//    {
//        sender.send(static_cast<int32_t>(GetCurrentProcessId()), reinterpret_cast<const char*>(&value), 4);
//    }
//    bool passed = WaitFault();
//    CSystem::sleepPrecise(20);
//    const bool held = callback.m_count.load() == 0 && receiver.sharedBlocksHeld();
//    InterlockedExchange(&s_faultEnabled, 0);
//    passed = WaitCount(callback.m_count, 2) && held && passed;
//    CSystem::sleepPrecise(20);
//    passed = passed && callback.m_count.load() == 2 && !callback.m_failed && callback.m_seen.load() == 3;
//    receiver.uninitReceive();
//    sender.clear();
//    std::printf("LIVE_RECEIVE committed=2 held=%d received=%d\n", held, callback.m_count.load());
//    return passed;
//}
//
//int main(int argc, char** argv)
//{
//    s_mainThread = GetCurrentThreadId();
//    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
//    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
//    if (argc != 2) { return 2; }
//    const std::string mode(argv[1]);
//    bool passed = false;
//    if (mode == "sender-pid" || mode == "sender-name" || mode == "sender-pid-stop" || mode == "sender-name-stop")
//    {
//        passed = CheckSender(mode.find("name") != std::string::npos, mode.find("stop") != std::string::npos);
//    }
//    else if (mode == "copy-alloc" || mode == "copy-queue" || mode == "copy-alloc-stop" || mode == "copy-queue-stop")
//    {
//        passed = CheckCopy(mode.find("queue") != std::string::npos ? 3 : 2, mode.find("stop") != std::string::npos);
//    }
//    else if (mode == "receive-live-alloc" || mode == "receive-live-queue")
//    {
//        passed = CheckLiveReceive(mode == "receive-live-alloc" ? 2 : 3);
//    }
//    else if (mode == "init") { passed = CheckInitialization(); }
//    else if (mode == "cache") { passed = CheckCache(); }
//    std::printf("%s %s\n", passed ? "PASS" : "FAIL", mode.c_str());
//    return passed ? 0 : 1;
//}
// PROCESS_WORK_RELIABILITY_UNIT_TEST_END

void NetInterfaceIpcTestCases::inboxIsolation()
{
    CheckIpc(NetSenderLocalHubTest::run() == 0, "bounded inbox isolation and full-window FIFO");
}

void NetInterfaceIpcTestCases::hubLifecycle()
{
    ProcessWork& work = ProcessWork::instance();
    NetSenderLocalHub& hub = NetSenderLocalHub::instance();
    NetSenderLocalInbox first;
    NetSenderLocalInbox second;
    uint32_t leases = 0;
    bool external = false;
    CheckIpc(work.receiveBufferCount() == 0, "hub test requires an idle test receiver");
    try
    {
        // 只attach曾经会偷偷登记Hub回调，detach又忘记登记，后续重试造成重复回调。
        CheckIpc(hub.attach(&first) == NI_INVALID_STATE && hub.attach(nullptr) == NI_INVALID_ARGUMENT,
            "unowned attach registered a ProcessWork callback");
        CheckIpc(hub.prepareExternal() == NI_OK, "hub external preparation");
        external = true;
        CheckIpc(hub.prepareExternal() == NI_OK && hub.acquire() == NI_INVALID_STATE && hub.attach(&first) == NI_INVALID_STATE,
            "hub lease accepted before receiver readiness");
        CheckIpc(work.initReceive(kNetSenderLocalReceiveBytes, 4), "hub external receiver startup");
        CheckIpc(hub.acquire() == NI_OK, "hub first external lease");
        ++leases;
        CheckIpc(hub.acquire() == NI_OK, "hub second external lease");
        ++leases;
        CheckIpc(hub.attach(&first) == NI_OK && hub.attach(&first) == NI_OK && hub.attach(&second) == NI_OK,
            "hub repeated attach or shared targets");
        hub.release();
        --leases;
        hub.detach(&first);
        CheckIpc(work.receiveBufferCount() == 4 && hub.externalStopped() == NI_INVALID_STATE,
            "hub released another external client");
        hub.release();
        --leases;
        hub.detach(&second);
        CheckIpc(work.receiveBufferCount() == 4 && hub.externalStopped() == NI_INVALID_STATE,
            "hub stopped a host-owned receiver");
        work.uninitReceive();
        CheckIpc(hub.externalStopped() == NI_OK, "hub external registration did not return to idle");
        external = false;
        CheckIpc(hub.attach(&first) == NI_INVALID_STATE, "hub retained a stale registration");
        CheckIpc(hub.acquire() == NI_OK, "hub managed startup after external shutdown");
        ++leases;
        CheckIpc(hub.attach(&first) == NI_OK && work.receivePayloadCapacity() == 16440, "hub managed receiver capacity");
        hub.release();
        --leases;
        CheckIpc(work.receiveBufferCount() == 64, "hub stopped while a borrowed inbox was still attached");
        hub.detach(&first);
        CheckIpc(work.receiveBufferCount() == 0, "last detach did not stop managed receiver");
        hub.release();
        CheckIpc(work.receiveBufferCount() == 0, "extra release restarted receiver");
        for (uint32_t index = 0; index < 64; ++index)
        {
            CheckIpc(hub.acquire() == NI_OK, "hub lease capacity below 64");
            ++leases;
        }
        CheckIpc(hub.acquire() == NI_LIMIT_EXCEEDED, "hub unbounded lease count");
        while (leases != 0)
        {
            hub.release();
            --leases;
        }
        CheckIpc(work.receiveBufferCount() == 0, "hub final lease leaked receiver tasks");
    }
    catch (...)
    {
        hub.detach(&first);
        hub.detach(&second);
        while (leases != 0)
        {
            hub.release();
            --leases;
        }
        work.uninitReceive();
        if (external)
        {
            hub.externalStopped();
        }
        throw;
    }
}
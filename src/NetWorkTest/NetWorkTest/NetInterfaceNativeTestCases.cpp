#include "NetInterfaceNativeTestCases.h"
#include "NetWorkTestCases.h"
#include "NetInterfaceTcpProbe.h"
#include "uv.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CSystem/CSystemAPI.h"
#include <stdexcept>
#include <sstream>
#include <chrono>
#include <thread>
#include "NetWork/NetWorkAPI.h"
#include <cstring>
#include "CStringManager/CStringManagerAPI.h"
#ifdef _WIN32
#include <Windows.h>
#endif

/** 检查真实传输预期，所有循环都有统一取消及期限
@param [in] condition 必须成立的条件
@param [in] reason 失败信息
*/
static void CheckTcp(bool condition, const char* reason)
{
    netTestCheckpoint();
    if (!condition)
    {
        throw std::runtime_error(reason);
    }
}

/** 同一测试单调时钟
@return 单调毫秒
*/
static uint64_t FileTransferTestNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

void NetInterfaceNativeTestCases::boundedTcp()
{
    NetInterfaceTcpProbe serverEvents;
    NetInterfaceTcpProbe clientEvents;
    LibuvChannel server;
    LibuvChannel client;
    CheckTcp(server.listen("127.0.0.1", 0, &serverEvents) == 0 && server.localPort() != 0, "listen TCP");
    uint64_t clientPeer = 0;
    CheckTcp(client.connect("127.0.0.1", server.localPort(), &clientEvents, clientPeer) == 0, "connect TCP");
    while (!serverEvents.m_peer || !clientEvents.m_peer)
    {
        netTestCheckpoint();
        server.poll();
        client.poll();
    }
    CheckTcp(server.pauseRead(serverEvents.m_peer, true) == 0, "pause TCP reads");
    std::vector<uint8_t> block(32768);
    for (size_t index = 0; index < block.size(); ++index)
    {
        block[index] = static_cast<uint8_t>(index * 31 + 5);
    }
    CheckTcp(client.write(clientPeer, &block[0], 32768) == 0 &&
        client.write(clientPeer, &block[0], 32768) == 0, "accept 64KiB");
    CheckTcp(client.write(clientPeer, &block[0], 1) == UV_ENOBUFS, "byte budget missing");
    for (int index = 0; index < 10; ++index)
    {
        server.poll();
        client.poll();
    }
    CheckTcp(serverEvents.m_received.empty(), "paused stream received data");
    CheckTcp(server.pauseRead(serverEvents.m_peer, false) == 0, "resume TCP reads");
    while (serverEvents.m_received.size() < 65536 || clientEvents.m_writable < 2)
    {
        netTestCheckpoint();
        server.poll();
        client.poll();
    }
    std::vector<uint8_t> expected(block);
    expected.insert(expected.end(), block.begin(), block.end());
    CheckTcp(serverEvents.m_received == expected, "raw binary altered or implicit length prefix");
    for (int index = 0; index < 64; ++index)
    {
        CheckTcp(client.write(clientPeer, &block[0], 1) == 0, "small writes admission");
    }
    CheckTcp(client.write(clientPeer, &block[0], 1) == UV_ENOBUFS, "request count budget missing");
    client.close(clientPeer, 123);
    client.close(clientPeer, 321);
    CheckTcp(client.write(clientPeer, &block[0], 1) == UV_ENOTCONN, "write after close");
    client.shutdown();
    server.shutdown();
    CheckTcp(clientEvents.m_closed[clientPeer] == 1 && clientEvents.m_reasons[clientPeer] == 123,
        "close result repeated or replaced");
    CheckTcp(client.peerCount() == 0 && server.peerCount() == 0 && !client.poll() && !server.poll(), "shutdown left live handles");
}

void NetInterfaceNativeTestCases::tcpLifecycle()
{
    NetInterfaceTcpProbe serverEvents;
    NetInterfaceTcpProbe firstEvents;
    NetInterfaceTcpProbe secondEvents;
    NetInterfaceTcpProbe thirdEvents;
    LibuvChannel server;
    LibuvChannel first;
    LibuvChannel second;
    LibuvChannel third;
    CheckTcp(server.listen("127.0.0.1", 0, &serverEvents, 1) == 0, "listen one-peer server");
    uint64_t firstId = 0;
    CheckTcp(first.connect("not-a-numeric-address", server.localPort(), &firstEvents, firstId) == UV_EINVAL && firstId == 0, "numeric address validation");
    CheckTcp(first.connect("127.0.0.1", server.localPort(), &firstEvents, firstId) == 0, "connect first");
    while (!serverEvents.m_peer || !firstEvents.m_peer)
    {
        netTestCheckpoint();
        server.poll();
        first.poll();
    }
    const uint64_t oldServerId = serverEvents.m_peer;
    uint64_t secondId = 0;
    CheckTcp(second.connect("127.0.0.1", server.localPort(), &secondEvents, secondId) == 0, "connect over capacity");
    while (secondEvents.m_closed.empty())
    {
        netTestCheckpoint();
        server.poll();
        first.poll();
        second.poll();
    }
    CheckTcp(serverEvents.m_connections == 1 && server.peerCount() == 1, "over-capacity client admitted");
    serverEvents.m_throwOnData = true;
    const uint8_t byte = 42;
    CheckTcp(first.write(firstId, &byte, 1) == 0, "trigger listener exception");
    while (serverEvents.m_closed.empty())
    {
        netTestCheckpoint();
        server.poll();
        first.poll();
    }
    CheckTcp(serverEvents.m_reasons[oldServerId] == UV_EPROTO, "listener exception escaped or no terminal result");
    serverEvents.m_throwOnData = false;
    uint64_t thirdId = 0;
    CheckTcp(third.connect("127.0.0.1", server.localPort(), &thirdEvents, thirdId) == 0, "connect replacement");
    while (serverEvents.m_connections < 2 || !thirdEvents.m_peer)
    {
        netTestCheckpoint();
        server.poll();
        third.poll();
    }
    CheckTcp(serverEvents.m_peer != oldServerId && server.write(oldServerId, &byte, 1) == UV_ENOTCONN, "stale peer ID reused");
    third.shutdown();
    second.shutdown();
    first.shutdown();
    server.shutdown();
    CheckTcp(server.peerCount() == 0, "replacement leaked connection");
}

#ifdef _WIN32
/** 统计真实进程中MEM_MAPPED区域，不用产品私有计数器证明释放
@return 映射区域数
*/
static uint64_t MappedRegionCount()
{
    SYSTEM_INFO limits;
    GetSystemInfo(&limits);
    uintptr_t cursor = reinterpret_cast<uintptr_t>(limits.lpMinimumApplicationAddress);
    const uintptr_t maximum = reinterpret_cast<uintptr_t>(limits.lpMaximumApplicationAddress);
    uint64_t count = 0;
    while (cursor < maximum)
    {
        netTestCheckpoint();
        MEMORY_BASIC_INFORMATION region = {};
        if (VirtualQuery(reinterpret_cast<void*>(cursor), &region, sizeof(region)) == 0)
        {
            break;
        }
        if (region.Type == MEM_MAPPED)
        {
            ++count;
        }
        const uintptr_t next = reinterpret_cast<uintptr_t>(region.BaseAddress) + region.RegionSize;
        if (next <= cursor)
        {
            throw std::runtime_error("VirtualQuery address did not advance");
        }
        cursor = next;
    }
    return count;
}

#endif

void NetInterfaceNativeTestCases::sharedMemoryCapacity()
{
    std::ostringstream name;
#ifdef _WIN32
    name << "NetWorkTest_capacity_" << GetCurrentProcessId();
    const uint64_t original = MappedRegionCount();
    {
        SharedMemory owner(name.str(), 8192, false);
        CheckTcp(!owner.isFailed() && owner.writeWithoutLock() != nullptr, "create capacity mapping");
        SharedMemory reader(name.str());
        const uint64_t before = MappedRegionCount();
        uint32_t wrongSizes = 0;
        for (int index = 0; index < 128; ++index)
        {
            netTestCheckpoint();
            if (owner.realSize() != 8192 || reader.realSize() != 8192)
            {
                ++wrongSizes;
            }
        }
        const uint64_t after = MappedRegionCount();
        if (wrongSizes != 0 || after > before + 1)
        {
            std::ostringstream detail;
            detail << "capacity wrong=" << wrongSizes << ", mapped before=" << before << ", after=" << after;
            throw std::runtime_error(detail.str());
        }
    }
    CheckTcp(MappedRegionCount() == original, "capacity query retained mapping after destruction");
#else
    name << "NetWorkTest_capacity_" << CSystem::currentProcessPid();
    SharedMemory owner(name.str(), 8192, false);
    CheckTcp(!owner.isFailed() && owner.realSize() >= 8192, "shared memory capacity");
#endif
}

void NetInterfaceNativeTestCases::durableFile()
{
    uint8_t random[8] = {};
    CheckTcp(NetTransport::randomBytes(random, sizeof(random)) == NI_OK, "file fixture random");
    std::ostringstream name;
    name << "NifFile65_" << CSystem::currentProcessPid() << '_';
    for (uint32_t index = 0; index < sizeof(random); ++index) name << std::hex << static_cast<uint32_t>(random[index]);
    std::string parent = CSystem::GetCurrentExePath();
#ifdef _WIN32
    parent = CStringManager::AnsiToUtf8(parent);
#endif
    const std::string directory = CSystem::joinPath(parent, name.str());
    CheckTcp(CSystem::createDirectoriesUtf8(directory), "file fixture directory");
    const std::string part = CSystem::joinPath(directory, "owned.part");
    const std::string final = CSystem::joinPath(directory, "owned.bin");
    const std::string another = CSystem::joinPath(directory, "another.part");
    NetNativeFileTestOwner writer;
    NetNativeFileTestOwner reader;
    const uint8_t expected[3] = {'a', 0, 'b'};
    CheckTcp(((writer.m_handle = CSystem::openFileHandle(part.c_str(), 2)) != -1) && CSystem::writeFileHandle(writer.m_handle, 0, expected, sizeof(expected)) &&
        CSystem::writeFileHandle(writer.m_handle, 4096, expected, sizeof(expected)) && CSystem::flushFileHandle(writer.m_handle), "actual file range write and flush");
    uint64_t length = 0;
    CheckTcp(CSystem::fileHandleSize(writer.m_handle, &length) && length == 4099, "actual file size");
    uint8_t received[3] = {};
    uint32_t count = 0;
    CheckTcp(CSystem::readFileHandle(writer.m_handle, 4096, received, sizeof(received), &count) && count == sizeof(received) &&
        std::memcmp(received, expected, sizeof(expected)) == 0, "actual offset read");
    CheckTcp(CSystem::resizeFileHandle(writer.m_handle, 3) && CSystem::flushFileHandle(writer.m_handle) && CSystem::closeFileHandle(&writer.m_handle), "actual prefix truncate");
    CheckTcp(CSystem::publishFileUtf8(part.c_str(), final.c_str()) && ((reader.m_handle = CSystem::openFileHandle(final.c_str(), 1)) != -1), "atomic file publish");
    CheckTcp(CSystem::fileHandleSize(reader.m_handle, &length) && length == 3 && CSystem::readFileHandle(reader.m_handle, 0, received, sizeof(received), &count) &&
        count == 3 && std::memcmp(received, expected, 3) == 0, "published bytes differ");
    CheckTcp(!CSystem::writeFileHandle(reader.m_handle, 0, expected, sizeof(expected)) && CSystem::closeFileHandle(&reader.m_handle) && CSystem::closeFileHandle(&reader.m_handle), "read-only or repeated close");
    CheckTcp(((writer.m_handle = CSystem::openFileHandle(another.c_str(), 2)) != -1) && CSystem::writeFileHandle(writer.m_handle, 0, reinterpret_cast<const uint8_t*>("new"), 3) &&
        CSystem::flushFileHandle(writer.m_handle) && CSystem::closeFileHandle(&writer.m_handle), "second staged file");
    CheckTcp(!CSystem::publishFileUtf8(another.c_str(), final.c_str()) && ((reader.m_handle = CSystem::openFileHandle(final.c_str(), 1)) != -1) &&
        CSystem::readFileHandle(reader.m_handle, 0, received, sizeof(received), &count) && std::memcmp(received, expected, 3) == 0,
        "default publish overwrote an existing file");
    CSystem::closeFileHandle(&reader.m_handle);
    // The isolated files remain beside the test executable as inspection artifacts.
}

void NetInterfaceNativeTestCases::asyncFileTransfer()
{
    runAsyncFileTransfer(NET_FILE_IO_OPEN_NEW);
}

void NetInterfaceNativeTestCases::ownedTemporaryTransfer()
{
    runAsyncFileTransfer(NET_FILE_IO_OPEN_TEMPORARY);
}

void NetInterfaceNativeTestCases::resumedFileTransfer()
{
    runAsyncFileTransfer(NET_FILE_IO_OPEN_WRITE, 1024);
}

void NetInterfaceNativeTestCases::fileTransferIdentity()
{
    for (uint32_t mode = 1; mode <= 3; ++mode)
    {
        runAsyncFileTransfer(NET_FILE_IO_OPEN_TEMPORARY, 0, mode);
    }
}

void NetInterfaceNativeTestCases::runAsyncFileTransfer(uint32_t createOperation, uint32_t resumeOffset, uint32_t failureMode)
{
    std::string parent = CSystem::GetCurrentExePath();
#ifdef _WIN32
    parent = CStringManager::AnsiToUtf8(parent);
#endif
    uint8_t random[8] = {};
    CheckTcp(NetTransport::randomBytes(random, sizeof(random)) == NI_OK, "transfer random");
    std::ostringstream name;
    name << "NifAsync66_" << CSystem::currentProcessPid() << '_';
    for (uint32_t index = 0; index < sizeof(random); ++index) name << std::hex << static_cast<uint32_t>(random[index]);
    const std::string directory = CSystem::joinPath(parent, name.str());
    CheckTcp(CSystem::createDirectoriesUtf8(directory), "transfer directory");
    const std::string source = CSystem::joinPath(directory, "source.bin");
    const std::string part = CSystem::joinPath(directory, "receive.part");
    const std::string final = CSystem::joinPath(directory, "receive.bin");
    std::vector<uint8_t> expected(2357);
    for (uint32_t index = 0; index < expected.size(); ++index) expected[index] = static_cast<uint8_t>(index * 17);
    NetNativeFileTestOwner fixture;
    CheckTcp(((fixture.m_handle = CSystem::openFileHandle(source.c_str(), 2)) != -1) && CSystem::writeFileHandle(fixture.m_handle, 0, expected.data(), static_cast<uint32_t>(expected.size())) &&
        CSystem::flushFileHandle(fixture.m_handle) && CSystem::closeFileHandle(&fixture.m_handle), "transfer source fixture");
    if (resumeOffset != 0)
    {
        CheckTcp(resumeOffset < expected.size() &&
            (fixture.m_handle = CSystem::openFileHandle(part.c_str(), 2)) != -1 &&
            CSystem::writeFileHandle(fixture.m_handle, 0, expected.data(), resumeOffset) &&
            CSystem::flushFileHandle(fixture.m_handle) && CSystem::closeFileHandle(&fixture.m_handle), "transfer resume prefix");
    }
    NetFileIo io;
    CheckTcp(io.init() == NI_OK, "transfer io init");
    uint64_t sourceFile = 0;
    uint64_t destinationFile = 0;
    for (uint32_t index = 0; index < 2; ++index)
    {
        NetFileIoRequest request = {};
        CheckTcp(io.nextRequestId(request.id) == NI_OK, "transfer open id");
        request.operation = index == 0 ? NET_FILE_IO_OPEN_READ : createOperation;
        request.path = index == 0 ? source.c_str() : part.c_str();
        CheckTcp(io.submit(request) == NI_OK, "transfer open submit");
        NetFileIoResult result = {};
        const uint64_t deadline = FileTransferTestNow() + 5000;
        while (io.peek(result) == NI_NEED_MORE)
        {
            CheckTcp(FileTransferTestNow() < deadline, "transfer open timeout");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        CheckTcp(result.id == request.id && result.result == NI_OK && result.file != 0, "transfer actual open");
        if (index == 0) sourceFile = result.file;
        else destinationFile = result.file;
        CheckTcp(io.consume(result.id) == NI_OK, "transfer consume open");
    }
    NetInterfaceFileInfo info = {};
    info.direction = 1;
    info.state = NI_FILE_TRANSFERRING;
    info.chunk = 1024;
    info.window = 2;
    info.total = expected.size();
    info.offset = resumeOffset;
    info.stream = UINT64_C(0x1234567887654321);
    info.expiresUtc = 2000000000;
    info.transfer[0] = 1;
    info.token[0] = 2;
    CheckTcp(NetInterface::sha256(expected.data(), static_cast<uint32_t>(expected.size()), info.hash, 32) == NI_OK, "transfer fixture hash");
    NetFileTransfer sender;
    NetFileTransfer receiver;
    CheckTcp(sender.start(io, sourceFile, info, 7, true, nullptr, nullptr, FileTransferTestNow()) == NI_OK &&
        receiver.start(io, destinationFile, info, 7, false, part.c_str(), final.c_str(), FileTransferTestNow()) == NI_OK,
        "transfer start actual handles");
    bool injected = false;
    const uint64_t deadline = FileTransferTestNow() + 5000;
    while (!sender.done() || !receiver.done())
    {
        CheckTcp(FileTransferTestNow() < deadline, "transfer completion timeout");
        NetFileIoResult result = {};
        while (io.peek(result) == NI_READY)
        {
            bool handled = false;
            if (failureMode != 0 && !injected && result.operation == NET_FILE_IO_READ && result.file == sourceFile)
            {
                NetFileIoResult invalid = result;
                if (failureMode == 1) { ++invalid.file; }
                if (failureMode == 2) { invalid.operation = NET_FILE_IO_WRITE; }
                if (failureMode == 3) { ++invalid.offset; }
                CheckTcp(sender.onIoResult(invalid), "matching job must be consumed as a failed completion");
                receiver.cancel();
                injected = true;
                handled = true;
            }
            CheckTcp(handled || sender.onIoResult(result) || receiver.onIoResult(result), "transfer foreign io result");
            CheckTcp(io.consume(result.id) == NI_OK, "transfer consume io");
        }
        const int32_t sent = sender.poll(FileTransferTestNow());
        const int32_t received = receiver.poll(FileTransferTestNow());
        CheckTcp(injected || (sent >= 0 && received >= 0), "transfer io or protocol failure");
        if (injected)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        NetInterfaceFrame frame = {};
        if (sender.peekOutput(frame) == NI_READY)
        {
            const int32_t accepted = receiver.accept(frame);
            CheckTcp(accepted == NI_OK || accepted == NI_LIMIT_EXCEEDED, "transfer receiver accept");
            if (accepted == NI_OK) CheckTcp(sender.consumeOutput() == NI_OK, "transfer data consume");
        }
        if (receiver.peekOutput(frame) == NI_READY)
        {
            CheckTcp(sender.accept(frame) == NI_OK && receiver.consumeOutput() == NI_OK, "transfer ack consume");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    NetInterfaceFileInfo snapshot = {};
    int32_t status = 0;
    sender.snapshot(snapshot, status);
    if (failureMode == 0)
    {
        CheckTcp(status == NI_READY && snapshot.state == NI_FILE_COMPLETED && snapshot.offset == expected.size(), "transfer committed result");
    }
    else
    {
        CheckTcp(injected && status == NI_INVALID_STATE && snapshot.state == NI_FILE_FAILED, "invalid identity accepted as data");
        // 在共享后端仍活动时核对两个文件号已释放，不能靠最终stop兜底冒充流自身清理。
        const uint64_t files[2] = {sourceFile, destinationFile};
        for (uint32_t index = 0; index < 2; ++index)
        {
            NetFileIoRequest request = {};
            CheckTcp(io.nextRequestId(request.id) == NI_OK, "closed file probe id");
            request.operation = NET_FILE_IO_SIZE;
            request.file = files[index];
            CheckTcp(io.submit(request) == NI_OK, "closed file probe submit");
            NetFileIoResult result = {};
            const uint64_t until = FileTransferTestNow() + 5000;
            int32_t ready = NI_NEED_MORE;
            while ((ready = io.peek(result)) == NI_NEED_MORE && FileTransferTestNow() < until)
            {
                netTestCheckpoint();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            CheckTcp(ready == NI_READY && result.id == request.id && result.result < 0, "flow retained file after done");
            CheckTcp(io.consume(result.id) == NI_OK, "closed file probe consume");
        }
    }
    io.stop();
    while (!io.done())
    {
        netTestCheckpoint();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    io.uninit();
    CheckTcp(io.status() == NI_OK, "file backend final release");
    if (failureMode != 0)
    {
        fixture.m_handle = CSystem::openFileHandle(part.c_str(), 1);
        CheckTcp(fixture.m_handle == -1, "cancelled owned temporary survived flow cleanup");
        return;
    }
    uint64_t size = 0;
    std::vector<uint8_t> received(expected.size());
    uint32_t count = 0;
    CheckTcp(((fixture.m_handle = CSystem::openFileHandle(final.c_str(), 1)) != -1) && CSystem::fileHandleSize(fixture.m_handle, &size) && size == expected.size() &&
        CSystem::readFileHandle(fixture.m_handle, 0, received.data(), static_cast<uint32_t>(received.size()), &count) &&
        count == expected.size() && received == expected && CSystem::closeFileHandle(&fixture.m_handle), "transfer actual published bytes");
}

NetNativeFileTestOwner::NetNativeFileTestOwner() : m_handle(-1)
{

}

NetNativeFileTestOwner::~NetNativeFileTestOwner()
{
    CSystem::closeFileHandle(&m_handle);
}
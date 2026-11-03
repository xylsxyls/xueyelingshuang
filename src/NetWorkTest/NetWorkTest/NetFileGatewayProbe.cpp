#include "NetSenderTestClient.h"
#include <stdexcept>
#include "NetFileGatewayProbe.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetInterfaceNativeTestCases.h"
#ifdef _WIN32
#include "NetInterfaceTlsTestCases.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <windows.h>

NetFileGatewayProbe::NetFileGatewayProbe(bool server) :
m_finished(false),
m_server(server),
m_ready(false),
m_bad(false),
m_closed(false),
m_receiving(false),
m_replyPending(false),
m_completed(0),
m_file(0),
m_openJob(0),
m_request(0),
m_finishAt(0),
m_sender(nullptr)
{
    std::memset(&m_control, 0, sizeof(m_control));
    std::memset(&m_info, 0, sizeof(m_info));
    std::memset(&m_outer, 0, sizeof(m_outer));
    std::memset(&m_origin, 0, sizeof(m_origin));
    m_expected.resize(2357);
    for (uint32_t index = 0; index < m_expected.size(); ++index)
    {
        m_expected[index] = static_cast<uint8_t>(index * 17);
    }
    std::string parent = CStringManager::AnsiToUtf8(CSystem::GetCurrentExePath());
    uint8_t random[8] = {};
    m_bad = NetInterface::entropyBytes(CSystem::secureRandomBytesCallback, nullptr, random, sizeof(random)) != NI_OK;
    std::ostringstream directory;
    directory << "FileGateway68_" << CSystem::currentProcessPid() << '_';
    for (uint32_t index = 0; index < 8; ++index)
    {
        directory << std::hex << static_cast<uint32_t>(random[index]);
    }
    m_directory = CSystem::joinPath(parent, directory.str());
    m_bad = m_bad || !CSystem::createDirectoriesUtf8(m_directory);
    m_part = CSystem::joinPath(m_directory, "owned.part");
    m_final = CSystem::joinPath(m_directory, "owned.bin");
    m_source = m_server ? m_final : CSystem::joinPath(m_directory, "source.bin");
    if (!m_server)
    {
        NetNativeFileTestOwner fixture;
        m_bad = m_bad || !((fixture.m_handle = CSystem::openFileHandle(m_source.c_str(), 2)) != -1) ||
                !CSystem::writeFileHandle(fixture.m_handle, 0, m_expected.data(), static_cast<uint32_t>(m_expected.size())) || !CSystem::flushFileHandle(fixture.m_handle) || !CSystem::closeFileHandle(&fixture.m_handle);
    }
    m_bad = m_bad || m_io.init() != NI_OK;
}

NetFileGatewayProbe::~NetFileGatewayProbe()
{
    finish();
}

bool NetFileGatewayProbe::finish()
{
    if (m_finished)
    {
        return !m_bad;
    }
    m_finished = true;
    if (m_flow && !m_flow->done())
    {
        m_flow->cancel();
    }
    m_io.stop();
    const uint64_t fileDeadline = now() + 5000;
    while (!m_io.done() && now() < fileDeadline)
    {
        CSystem::Sleep(1);
    }
    if (!m_io.done())
    {
        // 正常返回失败给父测试；尚未完成的已停止后端按其安全保留规则处理。
        std::fprintf(stderr, "FILE_BACKEND_STOP_TIMEOUT\n");
        m_bad = true;
    }
    m_io.uninit();
    if (m_flow)
    {
        m_flow->poll(now());
        m_flow.reset();
    }
    if (m_service)
    {
        m_service->shutdown();
    }
    if (m_sender)
    {
        m_sender->uninitGateway();
        const uint64_t gatewayDeadline = now() + 5000;
        while (m_sender->pollGateway() && now() < gatewayDeadline)
        {
            CSystem::sleepPrecise(1);
        }
        m_bad = m_bad || m_sender->pollGateway();
    }
    if (m_receiving)
    {
        ProcessWork::instance().uninitReceive();
    }
    if (m_sender)
    {
        m_sender->releaseGateway();
    }
    return !m_bad;
}

uint64_t NetFileGatewayProbe::now()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

bool NetFileGatewayProbe::openFile()
{
    NetFileIoRequest request = {};
    if (m_io.nextRequestId(request.id) != NI_OK)
    {
        return false;
    }
    const bool sender = m_server ? m_completed == 1 : m_completed == 0;
    request.operation = sender ? NET_FILE_IO_OPEN_READ : NET_FILE_IO_OPEN_NEW;
    request.path = sender ? m_source.c_str() : m_part.c_str();
    if (m_io.submit(request) != NI_OK)
    {
        return false;
    }
    m_openJob = request.id;
    return true;
}

bool NetFileGatewayProbe::sendOpen()
{
    std::memset(&m_control, 0, sizeof(m_control));
    m_control.operation = m_completed == 0 ? NI_FILE_OPEN_UPLOAD : NI_FILE_OPEN_DOWNLOAD;
    m_control.chunk = 1024;
    m_control.window = 2;
    if (NetInterface::entropyBytes(CSystem::secureRandomBytesCallback, nullptr, m_control.transfer, 16) != NI_OK)
    {
        return false;
    }
    m_control.key = reinterpret_cast<const uint8_t*>("fixture");
    m_control.keySize = 7;
    if (m_completed == 0)
    {
        m_control.total = m_expected.size();
        if (NetInterface::sha256(m_expected.data(), static_cast<uint32_t>(m_expected.size()), m_control.hash, 32) != NI_OK)
        {
            return false;
        }
    }
    uint8_t body[2680] = {};
    uint32_t required = 0;
    return NetInterfaceFile::encodeControl(m_control, body, sizeof(body), required) == NI_OK &&
           m_sender->beginRequest(4, m_control.operation, 8000, m_request) == NI_OK &&
           m_sender->postRequest(m_request, body, required) == NI_OK;
}

void NetFileGatewayProbe::onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
{
    (void)generation;
    (void)maxRequests;
    (void)maxChunk;
    m_ready = true;
    m_bad = !openFile();
}

void NetFileGatewayProbe::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status,
                                          const uint8_t* data, uint32_t size)
{
    (void)generation;
    m_bad = m_bad || requestId != m_request || product != 4 || type != m_control.operation || status != NI_RPC_SUCCESS ||
            NetInterfaceFile::parseInfo(data, size, m_info) != NI_OK || m_info.total != m_expected.size() ||
            std::memcmp(m_info.transfer, m_control.transfer, 16) != 0;
    if (m_bad)
    {
        return;
    }
    m_flow.reset(new NetFileTransfer());
    m_bad = m_flow->start(m_io, m_file, m_info, 4, m_completed == 0, m_part.c_str(), m_final.c_str(), now()) != NI_OK;
    if (!m_bad)
    {
        m_file = 0;
    }
}

bool NetFileGatewayProbe::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    (void)generation;
    if (!m_flow)
    {
        m_bad = true;
        return true;
    }
    const int32_t result = m_flow->accept(frame);
    if (result == NI_LIMIT_EXCEEDED)
    {
        return false;
    }
    m_bad = m_bad || result < 0;
    return true;
}

void NetFileGatewayProbe::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    (void)generation;
    m_bad = m_bad || reason != 0 || detail != 0 || m_completed != 2;
    m_closed = true;
}

void NetFileGatewayProbe::onServiceReady(uint64_t endpoint)
{
    m_ready = endpoint != 0;
    std::printf("FILE_SERVICE_READY\n");
    std::fflush(stdout);
}

bool NetFileGatewayProbe::onServiceFrame(const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin)
{
    if (frame.m_kind == NI_DATA || frame.m_kind == NI_DATA_ACK || frame.m_kind == NI_EVENT)
    {
        if (!m_flow || origin.m_endpoint != m_origin.m_endpoint || std::memcmp(origin.m_device, m_origin.m_device, 16) != 0)
        {
            m_bad = true;
            return true;
        }
        const int32_t result = m_flow->accept(frame);
        if (result == NI_LIMIT_EXCEEDED)
        {
            return false;
        }
        m_bad = m_bad || result < 0;
        return true;
    }
    if (m_flow && !m_flow->done())
    {
        return false;
    }
    NetInterfaceRpcView rpc = {};
    NetInterfaceFileControl control = {};
    if (m_completed >= 2 || m_openJob != 0 || frame.m_kind != NI_REQUEST || frame.m_productId != 4 ||
        frame.m_messageType != static_cast<uint32_t>(m_completed == 0 ? NI_FILE_OPEN_UPLOAD : NI_FILE_OPEN_DOWNLOAD) ||
        NetInterfaceRpc::parse(frame, rpc) != NI_OK || rpc.m_parent != 0 ||
        NetInterfaceFile::parseControl(frame.m_messageType, rpc.m_data, rpc.m_size, control) != NI_OK || control.keySize != 7 ||
        std::memcmp(control.key, "fixture", 7) != 0 || origin.m_platform != 1 || origin.m_device[0] != 0x42)
    {
        m_bad = true;
        return true;
    }
    m_control = control;
    m_control.key = nullptr;
    m_control.authorization = nullptr;
    m_outer = frame;
    m_outer.m_payload = nullptr;
    m_origin = origin;
    m_flow.reset();
    m_bad = !openFile();
    return true;
}

void NetFileGatewayProbe::onServiceClosed(int32_t reason)
{
    (void)reason;
    m_closed = true;
    m_bad = m_bad || m_completed != 2;
}

void NetFileGatewayProbe::pollFiles()
{
    NetFileIoResult result = {};
    while (m_io.peek(result) == NI_READY)
    {
        if (result.id == m_openJob)
        {
            m_openJob = 0;
            m_file = result.file;
            m_bad = m_bad || result.result != NI_OK || result.file == 0;
            if (!m_bad && m_server)
            {
                std::memset(&m_info, 0, sizeof(m_info));
                m_info.direction = m_completed == 0 ? 1 : 2;
                m_info.state = NI_FILE_TRANSFERRING;
                m_info.chunk = 1024;
                m_info.window = 2;
                m_info.total = m_expected.size();
                m_info.stream = m_completed + 1;
                m_info.expiresUtc = static_cast<uint64_t>(std::time(nullptr)) + 60;
                std::memcpy(m_info.transfer, m_control.transfer, 16);
                m_bad = NetInterface::entropyBytes(CSystem::secureRandomBytesCallback, nullptr, m_info.token, 32) != NI_OK ||
                        NetInterface::sha256(m_expected.data(), static_cast<uint32_t>(m_expected.size()), m_info.hash, 32) != NI_OK ||
                        (m_completed == 0 && (m_control.total != m_info.total || std::memcmp(m_control.hash, m_info.hash, 32) != 0)) ||
                        (m_completed == 1 && result.length != m_info.total);
                if (!m_bad)
                {
                    uint8_t metadata[160];
                    uint32_t required = 0;
                    m_reply.resize(176);
                    m_bad = NetInterfaceFile::encodeInfo(m_info, metadata, sizeof(metadata)) != NI_OK ||
                            NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, metadata, sizeof(metadata), m_reply.data(), 176,
                                                    required) != NI_OK;
                    m_flow.reset(new NetFileTransfer());
                    m_bad =
                        m_bad || m_flow->start(m_io, m_file, m_info, 4, m_completed == 1, m_part.c_str(), m_final.c_str(), now()) != NI_OK;
                    if (!m_bad)
                    {
                        m_file = 0;
                        m_replyPending = true;
                    }
                }
            }
            else if (!m_bad)
            {
                m_bad = !sendOpen();
            }
        }
        else
        {
            m_bad = m_bad || !m_flow || !m_flow->onIoResult(result);
        }
        m_bad = m_bad || m_io.consume(result.id) != NI_OK;
    }
    if (m_bad)
    {
        return;
    }
    if (m_replyPending)
    {
        NetInterfaceFrame reply = m_outer;
        reply.m_kind = NI_RESPONSE;
        reply.m_payload = m_reply.data();
        reply.m_payloadSize = static_cast<uint32_t>(m_reply.size());
        const int32_t sent = m_service->send(reply);
        if (sent == NI_LIMIT_EXCEEDED)
        {
            return;
        }
        m_bad = sent != NI_OK;
        if (m_bad)
        {
            return;
        }
        m_replyPending = false;
        NetInterface::wipe(m_reply.data(), static_cast<uint32_t>(m_reply.size()));
        m_reply.clear();
    }
    if (!m_flow || m_completed >= 2)
    {
        return;
    }
    m_bad = m_flow->poll(now()) < 0;
    if (m_bad)
    {
        return;
    }
    NetInterfaceFrame output = {};
    if (m_flow->peekOutput(output) == NI_READY)
    {
        const int32_t sent = m_server ? m_service->send(output) :
            (m_completed == 0 ? m_sender->sendData(output) : m_sender->postData(output));
        if (sent != NI_LIMIT_EXCEEDED)
        {
            m_bad = sent != NI_OK || m_flow->consumeOutput() != NI_OK;
        }
    }
    if (!m_bad && m_flow->done())
    {
        NetInterfaceFileInfo snapshot = {};
        int32_t state = 0;
        m_flow->snapshot(snapshot, state);
        m_bad = state != NI_READY || snapshot.offset != m_expected.size();
        if (!m_bad)
        {
            ++m_completed;
            std::printf("FILE_DIRECTION_COMPLETE count=%u\n", m_completed);
            std::fflush(stdout);
            if (!m_server && m_completed == 1)
            {
                m_flow.reset();
                m_bad = !openFile();
            }
            if (m_server && m_completed == 1)
            {
                m_flow.reset();
            }
            if (m_completed == 2)
            {
                m_finishAt = now() + 1000;
            }
        }
    }
}

int NetFileGatewayProbe::run(bool server, int32_t pid)
{
    NetFileGatewayProbe probe(server);
    if (probe.m_bad)
    {
        return 80;
    }
    ProcessWork& work = ProcessWork::instance();
    if (server)
    {
        work.addProcessReceiveCallback(&probe.m_inbox);
        probe.m_service.reset(new NetSenderService(probe.m_inbox));
    }
    else
    {
        probe.m_sender = &NetSenderTestClient<NetSender, NetSenderClient>::instance();
        if (probe.m_sender->prepareGateway() != NI_OK)
        {
            return 81;
        }
    }
    if (server)
    {
        probe.m_receiving = work.initReceive(16452, 64);
        if (!probe.m_receiving)
        {
            work.uninitReceive();
            return 82;
        }
    }
    int32_t connected = NI_INVALID_STATE;
    if (server)
    {
        uint8_t secret[32];
        for (uint32_t index = 0; index < 32; ++index)
        {
            secret[index] = static_cast<uint8_t>(index * 7 + 4 * 31 + 1);
        }
        auto tls = NetInterfaceTlsTestPair::options(false, "localhost");
        connected = probe.m_service->connect(pid, tls, 4, secret, &probe);
    }
    else
    {
        NetSenderGatewayOptions options = {};
        options.m_localGatewayPid = 0;
        if (NetSenderDesktopBackend::configureGateway(*probe.m_sender, pid) != NI_OK)
        {
            throw std::runtime_error("desktop gateway backend configuration");
        }
        options.m_network.structSize = sizeof(NetWorkClientOptions);
        options.m_network.version = 1;
        options.m_network.platform = 1;
        options.m_network.capabilities = 7;
        options.m_network.maxRequests = 4;
        options.m_network.maxChunk = 4096;
        options.m_network.handshakeTimeoutMs = 5000;
        std::memset(options.m_network.device, 0x42, 16);
        options.m_network.tls = NetInterfaceTlsTestPair::options(false, "localhost");
        options.m_network.tls.entropy = nullptr;
        options.m_network.tls.entropyContext = nullptr;
        connected = probe.m_sender->connectGateway(options, &probe);
    }
    const uint64_t deadline = now() + 20000;
    while (connected == NI_OK && !probe.m_bad && !probe.m_closed && now() < deadline)
    {
        if (server)
        {
            probe.m_service->poll();
        }
        else
        {
            probe.m_sender->pollGateway();
        }
        if (probe.m_ready)
        {
            probe.pollFiles();
        }
        if (probe.m_completed == 2 && now() >= probe.m_finishAt)
        {
            if (server)
            {
                break;
            }
            probe.m_sender->closeGateway(0);
        }
        Sleep(1);
    }
    if (!server && probe.m_completed == 2 && !probe.m_bad)
    {
        NetNativeFileTestOwner file;
        std::vector<uint8_t> bytes(probe.m_expected.size());
        uint32_t read = 0;
        probe.m_bad = !((file.m_handle = CSystem::openFileHandle(probe.m_final.c_str(), 1)) != -1) || !CSystem::readFileHandle(file.m_handle, 0, bytes.data(), static_cast<uint32_t>(bytes.size()), &read) ||
                      read != bytes.size() || bytes != probe.m_expected || !CSystem::closeFileHandle(&file.m_handle);
    }
    std::printf("FILE_GATEWAY_RESULT completed=%u bad=%u\n", probe.m_completed, probe.m_bad ? 1 : 0);
    const bool finished = probe.finish();
    return connected == NI_OK && probe.m_completed == 2 && finished ? 0 : 83;
}

#endif

#if 0 // DIRECTORY_CACHE_UNIT_TEST_BEGIN
#include "NetWork/NetWorkAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <chrono>
#include <cstdio>
#include <cstring>

/** 用例生命周期执行者推进停止，不在产品poll中等待
@param [in,out] store 本用例独占缓存
@return 5秒内是否完成并可释放
*/
static bool DirectoryCacheTestStop(NetGatewayDirectoryStore& store)
{
    store.shutdown();
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!store.stopped() && std::chrono::steady_clock::now() < deadline)
    {
        store.poll();
        CSystem::Sleep(1);
    }
    return store.stopped();
}

int main()
{
    uint32_t assertions = 0;
#define CACHE_CHECK(condition) { ++assertions; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
    const std::string root = CSystem::joinPath(CStringManager::AnsiToUtf8(CSystem::GetCurrentExePath()), "cache-unit/new-parent");
    const std::string path = CSystem::joinPath(root, "nodes.cache");
    const std::wstring wide = CStringManager::Utf8ToUnicode(path);
    NetGatewayDirectoryStore store;
    CACHE_CHECK(store.stopped());
    CACHE_CHECK(store.open(path, CSystem::secureRandomBytesCallback) == NI_NOT_SUPPORTED);
    CACHE_CHECK(NetWork::configureNativeFileIo() == NI_OK);
    CACHE_CHECK(store.open("", CSystem::secureRandomBytesCallback) == NI_INVALID_ARGUMENT);
    CACHE_CHECK(store.open(std::string(4097, 'a'), CSystem::secureRandomBytesCallback) == NI_INVALID_ARGUMENT);
    CACHE_CHECK(store.open(path, CSystem::secureRandomBytesCallback) == NI_OK);
    CACHE_CHECK(!store.stopped() && store.open(path) == NI_INVALID_STATE);
    NetInterfaceDirectoryPoint point;
    point.m_name = "node." + std::string(32, 'a');
    point.m_address = "127.0.0.1";
    point.m_port = 12345;
    point.m_capacity = 32;
    point.m_boot = 7;
    std::vector<NetInterfaceDirectoryPoint> points(1, point);
    CACHE_CHECK(store.queue(points) == NI_OK);
    points[0].m_port = 23456;
    points[0].m_boot = 9;
    CACHE_CHECK(store.queue(points) == NI_OK);
    points[0].m_port = 0;
    CACHE_CHECK(store.queue(points) == NI_INVALID_ARGUMENT);
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!CSystem::fileExists(wide) && std::chrono::steady_clock::now() < deadline)
    {
        store.poll();
        CSystem::Sleep(1);
    }
    CACHE_CHECK(CSystem::fileExists(wide));
    CACHE_CHECK(DirectoryCacheTestStop(store));
    std::vector<uint8_t> bytes;
    CACHE_CHECK(CSystem::readBinaryFile(wide, NET_INTERFACE_DIRECTORY_BYTES + 48, bytes));
    CACHE_CHECK(bytes.size() > 48 && std::memcmp(bytes.data(), "NGC3", 4) == 0);
    points.clear();
    CACHE_CHECK(NetInterfaceDirectory::parseCache(bytes.data(), static_cast<uint32_t>(bytes.size()), NetGatewayDirectoryStore::utc(), points) == NI_OK);
    CACHE_CHECK(points.size() == 1 && points[0].m_name == point.m_name && points[0].m_address == "127.0.0.1");
    CACHE_CHECK(points[0].m_port == 23456 && points[0].m_capacity == 32 && points[0].m_boot == 9);
    CACHE_CHECK(NetGatewayDirectoryStore::load(path, points) == NI_NOT_SUPPORTED && points.size() == 1);
    CACHE_CHECK(store.open(path, CSystem::secureRandomBytesCallback) == NI_OK);
    const std::chrono::steady_clock::time_point reloadDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    NetInterfaceDirectoryNodeData loadedPoints[64] = {};
    for (auto& record : loadedPoints)
    {
        record.m_structSize = sizeof(record);
        record.m_version = 1;
    }
    uint32_t loadedCount = 0;
    int32_t loaded = NI_NEED_MORE;
    while (loaded == NI_NEED_MORE && std::chrono::steady_clock::now() < reloadDeadline)
    {
        store.poll();
        loaded = store.takeLoaded(loadedPoints, 64, loadedCount);
        CSystem::Sleep(1);
    }
    CACHE_CHECK(loaded == NI_READY && loadedCount == 1 && loadedPoints[0].m_port == 23456 && loadedPoints[0].m_boot == 9);
    CACHE_CHECK(store.takeLoaded(loadedPoints, 64, loadedCount) == NI_NEED_MORE && loadedPoints[0].m_port == 23456);
    CACHE_CHECK(DirectoryCacheTestStop(store));
    // 损坏缓存不能产生候选，更不能恢复网络信任
    intptr_t file = CSystem::openFileHandle(path.c_str(), 3);
    CACHE_CHECK(file != -1);
    const uint8_t invalid = 0x78;
    CACHE_CHECK(CSystem::writeFileHandle(file, 0, &invalid, 1) && CSystem::flushFileHandle(file) && CSystem::closeFileHandle(&file));
    CACHE_CHECK(store.open(path, CSystem::secureRandomBytesCallback) == NI_OK);
    const std::chrono::steady_clock::time_point badDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    int32_t result = NI_OK;
    while (result != NI_PROTOCOL_ERROR && std::chrono::steady_clock::now() < badDeadline)
    {
        result = store.poll();
        CSystem::Sleep(1);
    }
    CACHE_CHECK(result == NI_PROTOCOL_ERROR && store.takeLoaded(loadedPoints, 64, loadedCount) == NI_NEED_MORE);
    CACHE_CHECK(DirectoryCacheTestStop(store));
    CACHE_CHECK(CSystem::deleteFile(wide));
    std::printf("Directory asynchronous cache PASS %u assertions\n", assertions);
    return 0;
#undef CACHE_CHECK
}

#endif // DIRECTORY_CACHE_UNIT_TEST_END
#include "NetGatewayDirectoryStore.h"
#include "NetPeerMacro.h"
#include "NetGatewayDirectoryStoreState.h"
#include "NetHostSocket.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <chrono>
#include <cstring>

/** 合并间隔使用单调时钟，UTC只用于缓存格式中的期限
@return 单调毫秒
*/
static uint64_t DirectoryStoreNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

NetGatewayDirectoryStore::NetGatewayDirectoryStore() :
m_state(new NetGatewayDirectoryStoreState())
{

}

NetGatewayDirectoryStore::~NetGatewayDirectoryStore()
{
    shutdown();
    poll();
    delete m_state;
}

uint64_t NetGatewayDirectoryStore::utc()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

int32_t NetGatewayDirectoryStore::load(const std::string& path, std::vector<NetInterfaceDirectoryPoint>& points)
{
    (void)path;
    (void)points;
    return NI_NOT_SUPPORTED;
}

int32_t NetGatewayDirectoryStore::open(const std::string& path, NetInterfaceEntropyFunction entropy, void* context)
{
    if (!stopped())
    {
        return NI_INVALID_STATE;
    }
    if (path.empty() || path.size() > 4096 || path.find('\0') != std::string::npos)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        m_state->m_path = path;
        const int32_t initialized = m_state->m_io.init();
        if (initialized != NI_OK)
        {
            return initialized;
        }
        m_state->m_entropy = entropy;
        m_state->m_context = context;
        m_state->m_phase = NET_CACHE_LOAD_OPEN;
        m_state->m_job = 0;
        m_state->m_file = 0;
        m_state->m_length = 0;
        m_state->m_lastWrite = 0;
        m_state->m_result = NI_OK;
        m_state->m_loadedReady = false;
        return NI_OK;
    }
    catch (...)
    {
        shutdown();
        return NI_NO_MEMORY;
    }
}

int32_t NetGatewayDirectoryStore::takeLoaded(NetInterfaceDirectoryNodeData* points, uint32_t capacity, uint32_t& count)
{
    if (!m_state->m_loadedReady)
    {
        count = 0;
        return NI_NEED_MORE;
    }
    const int32_t copied = m_state->m_loaded.copy(points, capacity, count);
    if (copied != NI_OK)
    {
        return copied;
    }
    m_state->m_loaded.reset();
    m_state->m_loadedReady = false;
    return NI_READY;
}

int32_t NetGatewayDirectoryStore::queue(const std::vector<NetInterfaceDirectoryPoint>& points)
{
    if (points.empty() || points.size() > NET_INTERFACE_DIRECTORY_NODES)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceDirectoryNodeData records[NET_INTERFACE_DIRECTORY_NODES] = {};
    for (uint32_t index = 0; index < points.size(); ++index)
    {
        const NetInterfaceDirectoryPoint& point = points[index];
        if (point.m_name.size() >= sizeof(records[index].m_name) || point.m_address.size() >= sizeof(records[index].m_address) ||
            point.m_name.find('\0') != std::string::npos || point.m_address.find('\0') != std::string::npos)
        {
            return NI_INVALID_ARGUMENT;
        }
        records[index].m_structSize = sizeof(records[index]);
        records[index].m_version = 1;
        std::memcpy(records[index].m_name, point.m_name.c_str(), point.m_name.size() + 1);
        std::memcpy(records[index].m_address, point.m_address.c_str(), point.m_address.size() + 1);
        records[index].m_port = point.m_port;
        records[index].m_capacity = point.m_capacity;
        records[index].m_boot = point.m_boot;
    }
    return queue(records, static_cast<uint32_t>(points.size()));
}

int32_t NetGatewayDirectoryStore::queue(const NetInterfaceDirectoryNodeData* points, uint32_t count)
{
    if (m_state->m_phase == NET_CACHE_CLOSED || m_state->m_phase == NET_CACHE_STOPPING)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::string encoded;
        NetDirectorySnapshot snapshot;
        const int32_t created = snapshot.create(points, count);
        if (created != NI_OK)
        {
            return created;
        }
        const int32_t result = snapshot.cache(utc(), encoded);
        if (result == NI_OK)
        {
            m_state->m_pending.swap(encoded);
        }
        return result;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetGatewayDirectoryStore::submit()
{
    NetFileIoRequest request = {};
    switch (m_state->m_phase)
    {
    case NET_CACHE_LOAD_OPEN:
        request.operation = NET_FILE_IO_OPEN_READ;
        request.path = m_state->m_path.c_str();
        break;
    case NET_CACHE_LOAD_READ:
        request.operation = NET_FILE_IO_READ;
        request.file = m_state->m_file;
        request.size = static_cast<uint32_t>(m_state->m_length);
        break;
    case NET_CACHE_LOAD_CLOSE:
    case NET_CACHE_SAVE_CLOSE:
        request.operation = NET_FILE_IO_CLOSE;
        request.file = m_state->m_file;
        break;
    case NET_CACHE_SAVE_OPEN:
        request.operation = NET_FILE_IO_OPEN_TEMPORARY;
        request.path = m_state->m_temporary.c_str();
        break;
    case NET_CACHE_SAVE_WRITE:
        request.operation = NET_FILE_IO_WRITE;
        request.file = m_state->m_file;
        request.flags = 1;
        request.bytes = reinterpret_cast<const uint8_t*>(m_state->m_active.data());
        request.size = static_cast<uint32_t>(m_state->m_active.size());
        break;
    case NET_CACHE_SAVE_PUBLISH:
        request.operation = NET_FILE_IO_PUBLISH;
        request.file = m_state->m_file;
        request.flags = 1;
        request.path = m_state->m_temporary.c_str();
        request.destination = m_state->m_path.c_str();
        break;
    default:
        return NI_NEED_MORE;
    }
    const int32_t allocated = m_state->m_io.nextRequestId(request.id);
    if (allocated != NI_OK)
    {
        return allocated;
    }
    const int32_t accepted = m_state->m_io.submit(request);
    if (accepted == NI_OK)
    {
        m_state->m_job = request.id;
        m_state->m_operation = request.operation;
    }
    return accepted;
}

void NetGatewayDirectoryStore::complete(const NetFileIoResult& result)
{
    if (m_state->m_phase == NET_CACHE_LOAD_OPEN)
    {
        if (result.result != NI_OK || result.file == 0)
        {
            m_state->m_result = result.result == NI_OK ? NI_PROTOCOL_ERROR : result.result;
            m_state->m_phase = NET_CACHE_IDLE;
        }
        else
        {
            m_state->m_file = result.file;
            m_state->m_length = result.length;
            const bool bounded = result.length >= 48 && result.length <= NET_INTERFACE_DIRECTORY_BYTES + 48;
            m_state->m_phase = bounded ? NET_CACHE_LOAD_READ : NET_CACHE_LOAD_CLOSE;
            if (!bounded)
            {
                m_state->m_result = NI_PROTOCOL_ERROR;
            }
        }
    }
    else if (m_state->m_phase == NET_CACHE_LOAD_READ)
    {
        m_state->m_result = result.result;
        if (result.result == NI_OK)
        {
            m_state->m_result = result.size == m_state->m_length ?
                m_state->m_loaded.parse(result.bytes, result.size, utc()) : NI_PROTOCOL_ERROR;
            m_state->m_loadedReady = m_state->m_result == NI_OK;
        }
        m_state->m_phase = NET_CACHE_LOAD_CLOSE;
    }
    else if (m_state->m_phase == NET_CACHE_LOAD_CLOSE || m_state->m_phase == NET_CACHE_SAVE_CLOSE)
    {
        if (result.result != NI_OK)
        {
            m_state->m_result = result.result;
        }
        m_state->m_file = 0;
        m_state->m_active.clear();
        m_state->m_temporary.clear();
        m_state->m_phase = NET_CACHE_IDLE;
    }
    else if (m_state->m_phase == NET_CACHE_SAVE_OPEN)
    {
        if (result.result == NI_OK && result.file != 0)
        {
            m_state->m_file = result.file;
            m_state->m_phase = NET_CACHE_SAVE_WRITE;
        }
        else
        {
            m_state->m_result = result.result == NI_OK ? NI_PROTOCOL_ERROR : result.result;
            m_state->m_active.clear();
            m_state->m_temporary.clear();
            m_state->m_phase = NET_CACHE_IDLE;
        }
    }
    else if (m_state->m_phase == NET_CACHE_SAVE_WRITE)
    {
        m_state->m_result = result.result;
        if (result.result == NI_OK && result.length != m_state->m_active.size())
        {
            m_state->m_result = NI_PROTOCOL_ERROR;
        }
        m_state->m_phase = m_state->m_result == NI_OK ? NET_CACHE_SAVE_PUBLISH : NET_CACHE_SAVE_CLOSE;
    }
    else if (m_state->m_phase == NET_CACHE_SAVE_PUBLISH)
    {
        m_state->m_result = result.result;
        m_state->m_file = 0;
        m_state->m_active.clear();
        m_state->m_temporary.clear();
        m_state->m_phase = NET_CACHE_IDLE;
    }
}

int32_t NetGatewayDirectoryStore::poll()
{
    if (m_state->m_phase == NET_CACHE_STOPPING || m_state->m_phase == NET_CACHE_CLOSED)
    {
        if (m_state->m_io.done())
        {
            m_state->m_io.uninit();
            m_state->m_phase = NET_CACHE_CLOSED;
        }
        return m_state->m_result;
    }
    try
    {
        if (m_state->m_job != 0)
        {
            NetFileIoResult result = {};
            const int32_t available = m_state->m_io.peek(result);
            if (available == NI_NEED_MORE)
            {
                return m_state->m_result;
            }
            if (available != NI_READY || result.id != m_state->m_job || result.operation != m_state->m_operation)
            {
                m_state->m_result = available < 0 ? available : NI_PROTOCOL_ERROR;
                shutdown();
                return m_state->m_result;
            }
            complete(result);
            const int32_t consumed = m_state->m_io.consume(m_state->m_job);
            m_state->m_job = 0;
            if (consumed != NI_OK)
            {
                m_state->m_result = consumed;
                shutdown();
                return consumed;
            }
        }
        const uint64_t now = DirectoryStoreNow();
        if (m_state->m_phase == NET_CACHE_IDLE && !m_state->m_pending.empty() &&
            (m_state->m_lastWrite == 0 || now - m_state->m_lastWrite >= 2000))
        {
            m_state->m_lastWrite = now;
            uint8_t random[16] = {};
            const int32_t entropy = m_state->m_entropy == nullptr ? NetHostSocket::randomBytes(random, sizeof(random)) :
                NetInterface::entropyBytes(m_state->m_entropy, m_state->m_context, random, sizeof(random));
            if (entropy != NI_OK)
            {
                m_state->m_result = entropy;
                return entropy;
            }
            const char* hex = "0123456789abcdef";
            m_state->m_temporary = m_state->m_path + ".tmp.";
            for (uint32_t index = 0; index < sizeof(random); ++index)
            {
                m_state->m_temporary.push_back(hex[random[index] >> 4]);
                m_state->m_temporary.push_back(hex[random[index] & 15]);
            }
            m_state->m_active.swap(m_state->m_pending);
            m_state->m_phase = NET_CACHE_SAVE_OPEN;
        }
        const int32_t submitted = submit();
        if (submitted < 0 && submitted != NI_LIMIT_EXCEEDED)
        {
            m_state->m_result = submitted;
            shutdown();
        }
    }
    catch (...)
    {
        m_state->m_result = NI_NO_MEMORY;
        shutdown();
    }
    return m_state->m_result;
}

void NetGatewayDirectoryStore::shutdown()
{
    m_state->m_pending.clear();
    m_state->m_active.clear();
    m_state->m_loaded.reset();
    m_state->m_loadedReady = false;
    m_state->m_entropy = nullptr;
    m_state->m_context = nullptr;
    m_state->m_io.stop();
    m_state->m_phase = NET_CACHE_STOPPING;
}

bool NetGatewayDirectoryStore::stopped() const
{
    return m_state->m_phase == NET_CACHE_CLOSED && m_state->m_io.done();
}

#if 0 // DIRECTORY_CACHE_CONTROLLED_UNIT_TEST_BEGIN
#include "NetPeer/NetGatewayDirectoryStore.h"
#include "NetPeer/NetGatewayDirectoryStoreTest.h"
#include <cstring>
#include <cstdio>

NetDirectoryStoreTestBackend::NetDirectoryStoreTestBackend() :
m_busy(0),
m_accepted(0),
m_consumed(0),
m_ready(false),
m_job(),
m_reply()
{

}

int32_t NetDirectoryStoreTestBackend::submit(const NetFileIoRequest& request)
{
    if (m_busy != 0)
    {
        --m_busy;
        return NI_LIMIT_EXCEEDED;
    }
    if (m_job.id != 0 || m_ready)
    {
        return NI_INVALID_STATE;
    }
    m_job = request;
    m_job.bytes = nullptr;
    m_job.path = nullptr;
    m_job.destination = nullptr;
    if (request.operation == NET_FILE_IO_WRITE && request.size != 0)
    {
        m_copy.assign(request.bytes, request.bytes + request.size);
    }
    ++m_accepted;
    return NI_OK;
}

int32_t NetDirectoryStoreTestBackend::peek(NetFileIoResult& result)
{
    result = m_ready ? m_reply : NetFileIoResult();
    return m_ready ? NI_READY : NI_NEED_MORE;
}

int32_t NetDirectoryStoreTestBackend::consume(uint64_t id)
{
    if (!m_ready || id != m_reply.id)
    {
        return NI_INVALID_STATE;
    }
    m_ready = false;
    m_job = NetFileIoRequest();
    m_reply = NetFileIoResult();
    ++m_consumed;
    return NI_OK;
}

void NetDirectoryStoreTestBackend::finish(int32_t result, uint64_t file, uint64_t length, const uint8_t* bytes, uint32_t size)
{
    m_reply = NetFileIoResult();
    m_reply.id = m_job.id;
    m_reply.operation = m_job.operation;
    m_reply.result = result;
    m_reply.file = file;
    m_reply.length = length;
    m_reply.bytes = bytes;
    m_reply.size = size;
    m_ready = true;
}

NetFileIoBackend* NetDirectoryStoreTestBackend::create()
{
    return new NetDirectoryStoreTestBackend();
}

/** 测试专用确定性熵，不连接网络或签发身份。
@param [in] context 未使用
@param [out] output 临时名输入
@param [in] size 字节数
@return 0
*/
static int32_t DirectoryControlledEntropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    std::memset(output, 0x55, size);
    return 0;
}

/** 忙不得丢工作，畸形元数据不得产生可信缓存，停止必须等done。
@return 0通过，1失败
*/
int main()
{
    uint32_t assertions = 0;
#define CONTROLLED_CHECK(condition) { ++assertions; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
    CONTROLLED_CHECK(NetFileIo::configureBackend(NetDirectoryStoreTestBackend::create) == NI_OK);
    for (uint32_t test = 0; test < 13; ++test)
    {
        NetGatewayDirectoryStore store;
        CONTROLLED_CHECK(store.open("controlled.nodes.cache", DirectoryControlledEntropy) == NI_OK);
        NetDirectoryStoreTestBackend* backend = static_cast<NetDirectoryStoreTestBackend*>(NetFileIoTestBackend::m_current);
        backend->m_busy = 2;
        CONTROLLED_CHECK(store.poll() == NI_OK && store.poll() == NI_OK && backend->m_accepted == 0);
        CONTROLLED_CHECK(store.poll() == NI_OK && backend->m_accepted == 1 && backend->m_job.operation == NET_FILE_IO_OPEN_READ);
        NetInterfaceDirectoryNodeData loaded[64] = {};
        for (auto& record : loaded)
        {
            record.m_structSize = sizeof(record);
            record.m_version = 1;
        }
        uint32_t loadedCount = 0;
        const uint8_t invalid[48] = {};
        if (test < 2)
        {
            backend->finish(NI_OK, 41, 48);
            if (test == 0)
            {
                ++backend->m_reply.id;
            }
            else
            {
                backend->m_reply.operation = NET_FILE_IO_WRITE;
            }
            CONTROLLED_CHECK(store.poll() == NI_PROTOCOL_ERROR && backend->m_stopped && !store.stopped());
            CONTROLLED_CHECK(backend->m_consumed == 0 && store.takeLoaded(loaded, 64, loadedCount) == NI_NEED_MORE);
        }
        else if (test <= 5 || test == 9)
        {
            backend->finish(NI_OK, test == 2 ? 0 : 41, test == 3 ? NET_INTERFACE_DIRECTORY_BYTES + 49 : 48);
            store.poll();
            if (test == 4 || test == 5 || test == 9)
            {
                CONTROLLED_CHECK(backend->m_job.operation == NET_FILE_IO_READ);
                backend->finish(NI_OK, 41, 48, test == 5 ? nullptr : invalid, test == 4 ? 47 : 48);
                CONTROLLED_CHECK(store.poll() < 0);
            }
            CONTROLLED_CHECK(store.takeLoaded(loaded, 64, loadedCount) == NI_NEED_MORE && loadedCount == 0 && loaded[0].m_name[0] == 0);
            if (test != 2)
            {
                CONTROLLED_CHECK(backend->m_job.operation == NET_FILE_IO_CLOSE && backend->m_job.file == 41);
                backend->finish(NI_OK, 0, 0);
                CONTROLLED_CHECK(store.poll() < 0);
            }
        }
        else if (test == 12)
        {
            // 目录由NI持有，测试侧只提供字节和固定值，兼容独立DLL堆
            NetInterfaceDirectoryNodeData node = {};
            node.m_structSize = sizeof(node);
            node.m_version = 1;
            std::memcpy(node.m_name, "node.0123456789abcdef0123456789abcdef", sizeof(node.m_name));
            std::memcpy(node.m_address, "127.0.0.1", 10);
            node.m_port = 23456;
            node.m_capacity = 32;
            node.m_boot = UINT64_C(0xfedcba9876543210);
            NetInterfaceDirectoryHandle directory = nullptr;
            CONTROLLED_CHECK(NetInterface::directoryCreateNodes(&node, 1, &directory) == NI_OK);
            uint32_t bytesSize = 0;
            const uint64_t now = NetGatewayDirectoryStore::utc();
            CONTROLLED_CHECK(NetInterface::directoryCache(directory, static_cast<uint32_t>(now),
                static_cast<uint32_t>(now >> 32), nullptr, 0, &bytesSize) == NI_BUFFER_TOO_SMALL);
            std::vector<uint8_t> bytes(bytesSize);
            CONTROLLED_CHECK(NetInterface::directoryCache(directory, static_cast<uint32_t>(now),
                static_cast<uint32_t>(now >> 32), bytes.data(), bytesSize, &bytesSize) == NI_OK);
            NetInterface::directoryDestroy(directory);
            backend->finish(NI_OK, 41, bytesSize);
            CONTROLLED_CHECK(store.poll() == NI_OK && backend->m_job.operation == NET_FILE_IO_READ);
            backend->finish(NI_OK, 41, bytesSize, bytes.data(), bytesSize);
            CONTROLLED_CHECK(store.poll() == NI_OK && backend->m_job.operation == NET_FILE_IO_CLOSE);
            CONTROLLED_CHECK(store.takeLoaded(nullptr, 0, loadedCount) == NI_BUFFER_TOO_SMALL && loadedCount == 1);
            const NetInterfaceDirectoryNodeData before = loaded[0];
            CONTROLLED_CHECK(store.takeLoaded(loaded, 0, loadedCount) == NI_BUFFER_TOO_SMALL && loadedCount == 1 &&
                std::memcmp(loaded, &before, sizeof(before)) == 0);
            loaded[0].m_version = 2;
            CONTROLLED_CHECK(store.takeLoaded(loaded, 64, loadedCount) == NI_INVALID_ARGUMENT && loadedCount == 0 &&
                loaded[0].m_version == 2 && loaded[0].m_name[0] == 0);
            loaded[0].m_version = 1;
            CONTROLLED_CHECK(store.takeLoaded(loaded, 64, loadedCount) == NI_READY && loadedCount == 1 &&
                loaded[0].m_port == node.m_port && loaded[0].m_boot == node.m_boot &&
                std::strcmp(loaded[0].m_name, node.m_name) == 0 && std::strcmp(loaded[0].m_address, node.m_address) == 0);
            const NetInterfaceDirectoryNodeData consumed = loaded[0];
            CONTROLLED_CHECK(store.takeLoaded(loaded, 64, loadedCount) == NI_NEED_MORE && loadedCount == 0 &&
                std::memcmp(loaded, &consumed, sizeof(consumed)) == 0);
            backend->finish(NI_OK, 0, 0);
            CONTROLLED_CHECK(store.poll() == NI_OK);
        }
        else if (test == 10)
        {
            store.shutdown();
            CONTROLLED_CHECK(backend->m_stopped && !store.stopped());
            store.poll();
            CONTROLLED_CHECK(!store.stopped());
        }
        else
        {
            backend->finish(NI_NOT_FOUND, 0, 0);
            store.poll();
            NetInterfaceDirectoryPoint point;
            point.m_name = "node.0123456789abcdef0123456789abcdef";
            point.m_address = "127.0.0.1";
            point.m_port = 23456;
            point.m_capacity = 32;
            point.m_boot = 9;
            std::vector<NetInterfaceDirectoryPoint> points(1, point);
            CONTROLLED_CHECK(store.queue(points) == NI_OK);
            store.poll();
            CONTROLLED_CHECK(backend->m_job.operation == NET_FILE_IO_OPEN_TEMPORARY);
            backend->finish(NI_OK, test == 11 ? 0 : 41, 0);
            backend->m_busy = test == 6 ? 2 : 0;
            store.poll();
            if (test == 6)
            {
                CONTROLLED_CHECK(backend->m_job.id == 0 && backend->m_copy.empty());
                store.poll();
                CONTROLLED_CHECK(backend->m_job.id == 0);
                store.poll();
            }
            if (test == 11)
            {
                CONTROLLED_CHECK(backend->m_job.id == 0 && store.poll() == NI_PROTOCOL_ERROR);
            }
            else
            {
                CONTROLLED_CHECK(backend->m_job.operation == NET_FILE_IO_WRITE && backend->m_job.flags == 1 &&
                    backend->m_copy.size() > 48 && std::memcmp(backend->m_copy.data(), "NGC3", 4) == 0);
                backend->finish(NI_OK, 41, backend->m_copy.size() - (test == 7 ? 1 : 0));
                store.poll();
                CONTROLLED_CHECK(backend->m_job.operation == static_cast<uint32_t>(test == 7 ? NET_FILE_IO_CLOSE : NET_FILE_IO_PUBLISH));
                backend->finish(test == 8 ? NI_INVALID_STATE : NI_OK, 0, 0);
                const int32_t result = store.poll();
                CONTROLLED_CHECK(test == 6 ? result == NI_OK : result < 0);
            }
        }
        store.shutdown();
        CONTROLLED_CHECK(!store.stopped());
        const uint32_t destroyed = NetFileIoTestBackend::m_destroyed;
        backend->m_done = true;
        store.poll();
        CONTROLLED_CHECK(store.stopped() && NetFileIoTestBackend::m_destroyed == destroyed + 1);
    }
    std::printf("Directory cache controlled assertions=%u PASS\n", assertions);
    return 0;
#undef CONTROLLED_CHECK
}

#endif // DIRECTORY_CACHE_CONTROLLED_UNIT_TEST_END
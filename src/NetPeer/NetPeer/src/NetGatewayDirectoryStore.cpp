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
    if (points.empty() || points.size() > UINT32_MAX)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::vector<NetInterfaceDirectoryNodeData> records(points.size());
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
        return queue(records.data(), static_cast<uint32_t>(points.size()));
    }
    catch (...) { return NI_NO_MEMORY; }
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
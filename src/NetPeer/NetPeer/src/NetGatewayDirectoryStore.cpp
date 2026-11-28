#include "NetGatewayDirectoryStore.h"
#include "NetPeerMacro.h"
#include "NetGatewayDirectoryStoreState.h"
#include "NetHostSocket.h"
#include "CStringManager/CStringManagerAPI.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <chrono>
#include <cstring>

// 同目录临时名称在目标路径后追加标记和十六进制随机数。
static const char DirectoryTemporaryMarker[] = ".tmp.";
static const uint32_t DirectoryRandomBytes = 16;
// 文件后端没有执行循环通知接口，等待完成或容量时采用有限补查。
static const uint32_t kDirectoryFilePollMs = 100;

/** 合并间隔使用单调时钟，UTC只用于缓存格式中的期限
@return 单调毫秒
*/
static uint64_t DirectoryStoreNow()
{
    return NetInterface::monotonicMilliseconds();
}

NetGatewayDirectoryStore::NetGatewayDirectoryStore() : NetGatewayDirectoryStore(2000)
{
}

NetGatewayDirectoryStore::NetGatewayDirectoryStore(uint32_t writeIntervalMs) :
m_state(new NetGatewayDirectoryStoreState(writeIntervalMs))
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
    if (path.empty() || path.size() > 4096 || CStringManager::boundedLength(path.c_str(), path.size()) != path.size())
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        m_state->m_path = path;
        const int32_t initialized = m_state->m_io.init();
        if (initialized != NI_OK)
        {
            m_state->m_result = initialized;
            if (!m_state->m_io.released())
            {
                m_state->m_phase = NET_CACHE_STOPPING;
                NetExecution::progress();
            }
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
        NetExecution::progress();
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
    if (points.empty())
    {
        return NI_INVALID_ARGUMENT;
    }
    if (points.size() > NET_INTERFACE_DIRECTORY_BYTES / NET_INTERFACE_DIRECTORY_NAME_BYTES)
    {
        return NI_LIMIT_EXCEEDED;
    }
    try
    {
        std::vector<NetInterfaceDirectoryNodeData> records(points.size());
        for (uint32_t index = 0; index < points.size(); ++index)
        {
            const NetInterfaceDirectoryPoint& point = points[index];
            if (point.m_name.size() >= sizeof(records[index].m_name) || point.m_address.size() >= sizeof(records[index].m_address) ||
                CStringManager::boundedLength(point.m_name.c_str(), point.m_name.size()) != point.m_name.size() ||
                CStringManager::boundedLength(point.m_address.c_str(), point.m_address.size()) != point.m_address.size())
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
        if (m_state->m_path.size() > 4096 - (sizeof(DirectoryTemporaryMarker) - 1) - 2 * DirectoryRandomBytes)
        {
            return NI_INVALID_ARGUMENT;
        }
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
            if (m_state->m_phase == NET_CACHE_IDLE)
            {
                // 同轮空闲 poll 之后仍可接纳新快照；覆盖只更换正文，不延长原写入期限。
                const uint64_t elapsed = m_state->m_lastWrite == 0 ? m_state->m_writeIntervalMs :
                    DirectoryStoreNow() - m_state->m_lastWrite;
                NetExecution::deadline(m_state->m_writeIntervalMs, elapsed);
            }
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
    if (m_state->m_phase == NET_CACHE_CLOSED) { return m_state->m_result; }
    if (m_state->m_phase == NET_CACHE_STOPPING)
    {
        if (m_state->m_io.done())
        {
            m_state->m_io.uninit();
            if (m_state->m_io.released()) { m_state->m_phase = NET_CACHE_CLOSED; }
            else if (m_state->m_result == NI_OK) { m_state->m_result = m_state->m_io.status(); }
        }
        if (m_state->m_phase != NET_CACHE_CLOSED) { NetExecution::deadline(kDirectoryFilePollMs, 0); }
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
                NetExecution::deadline(kDirectoryFilePollMs, 0);
                return m_state->m_result;
            }
            // 打开成功时宿主分配文件号。后续读、写、关闭和发布都必须回报该文件号。
            if (available != NI_READY || result.id != m_state->m_job || result.operation != m_state->m_operation ||
                result.offset != 0 || (m_state->m_file != 0 && result.file != m_state->m_file))
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
        if (m_state->m_phase == NET_CACHE_IDLE && !m_state->m_pending.empty())
        {
            if (m_state->m_lastWrite != 0 && now - m_state->m_lastWrite < m_state->m_writeIntervalMs)
            {
                NetExecution::deadline(m_state->m_writeIntervalMs, now - m_state->m_lastWrite);
                return m_state->m_result;
            }
            m_state->m_lastWrite = now;
            uint8_t random[DirectoryRandomBytes] = {};
            const int32_t entropy = m_state->m_entropy == nullptr ? NetHostSocket::randomBytes(random, sizeof(random)) :
                NetInterface::entropyBytes(m_state->m_entropy, m_state->m_context, random, sizeof(random));
            if (entropy != NI_OK)
            {
                m_state->m_result = entropy;
                NetExecution::deadline(m_state->m_writeIntervalMs != 0 ? m_state->m_writeIntervalMs : kDirectoryFilePollMs, 0);
                return entropy;
            }
            const char* hex = "0123456789abcdef";
            m_state->m_temporary = m_state->m_path + DirectoryTemporaryMarker;
            for (uint32_t index = 0; index < sizeof(random); ++index)
            {
                m_state->m_temporary.push_back(hex[random[index] >> 4]);
                m_state->m_temporary.push_back(hex[random[index] & 15]);
            }
            m_state->m_active.swap(m_state->m_pending);
            m_state->m_phase = NET_CACHE_SAVE_OPEN;
        }
        const int32_t submitted = submit();
        if (submitted == NI_OK)
        {
            // 只在新工作被接纳时补一次推进；后端尚未完成时由上面的有限补查等待。
            NetExecution::progress();
        }
        else if (submitted == NI_LIMIT_EXCEEDED)
        {
            NetExecution::deadline(kDirectoryFilePollMs, 0);
        }
        else if (submitted < 0)
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
    if (stopped())
    {
        return;
    }
    m_state->m_pending.clear();
    m_state->m_active.clear();
    m_state->m_loaded.reset();
    m_state->m_loadedReady = false;
    m_state->m_entropy = nullptr;
    m_state->m_context = nullptr;
    m_state->m_io.stop();
    if (m_state->m_phase != NET_CACHE_STOPPING)
    {
        m_state->m_phase = NET_CACHE_STOPPING;
        NetExecution::progress();
    }
}

bool NetGatewayDirectoryStore::stopped() const
{
    return m_state->m_phase == NET_CACHE_CLOSED && m_state->m_io.released();
}
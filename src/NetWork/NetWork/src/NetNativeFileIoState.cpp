#ifndef __EMSCRIPTEN__
#include "NetNativeFileIoState.h"
#include "CStringManager/CStringManagerAPI.h"
#include <cstring>
#include <algorithm>

#ifdef _WIN32
// Windows拒绝写入只读文件的错误码
static const uint32_t kNetFileIoReadOnlyError = 5;
#else
// POSIX拒绝写入只读文件的错误码
static const uint32_t kNetFileIoReadOnlyError = 13;
#endif

void NetFileIoState::hash(NetFileIoEntry& entry, NetFileIoHandle& file)
{
    uint64_t length = 0;
    if (!file.size(&length))
    {
        entry.m_result.result = NI_INVALID_STATE;
        entry.m_result.detail = file.lastError();
        return;
    }
    if (entry.m_request.offset > length)
    {
        entry.m_result.result = NI_INVALID_ARGUMENT;
        return;
    }
    if (entry.m_request.offset != 0)
    {
        length = entry.m_request.offset;
    }
    NetInterfaceHashHandle hash = nullptr;
    entry.m_result.result = NetInterface::hashCreate(&hash);
    if (entry.m_result.result != NI_OK)
    {
        return;
    }
    std::vector<uint8_t> bytes;
    try
    {
        bytes.resize(65536);
        uint64_t offset = 0;
        while (offset < length && entry.m_result.result == NI_OK)
        {
            if (m_stop.load() || entry.m_cancelled.load())
            {
                entry.m_result.result = NI_CANCELLED;
                break;
            }
            const uint32_t size = static_cast<uint32_t>((std::min)(UINT64_C(65536), length - offset));
            uint32_t read = 0;
            if (!file.read(offset, bytes.data(), size, &read) || read != size)
            {
                entry.m_result.result = NI_INVALID_STATE;
                entry.m_result.detail = file.lastError();
                break;
            }
            entry.m_result.result = NetInterface::hashUpdate(hash, bytes.data(), read);
            offset += read;
        }
        if (entry.m_result.result == NI_OK)
        {
            entry.m_output.resize(32);
            entry.m_result.result = NetInterface::hashFinish(hash, entry.m_output.data(), 32);
            entry.m_result.length = length;
        }
        NetInterface::wipe(bytes.data(), static_cast<uint32_t>(bytes.size()));
    }
    catch (...)
    {
        NetInterface::wipe(bytes.empty() ? nullptr : bytes.data(), static_cast<uint32_t>(bytes.size()));
        NetInterface::hashDestroy(hash);
        throw;
    }
    NetInterface::hashDestroy(hash);
}

void NetFileIoState::execute(NetFileIoEntry& entry)
{
    const auto& request = entry.m_request;
    if (request.operation <= NET_FILE_IO_DUPLICATE_READ || request.operation == NET_FILE_IO_OPEN_TEMPORARY)
    {
        if (m_files.size() >= 32 || m_lastFile == UINT64_MAX)
        {
            entry.m_result.result = NI_LIMIT_EXCEEDED;
            return;
        }
        std::unique_ptr<NetFileIoHandle> file(new NetFileIoHandle());
        const bool opened = request.operation == NET_FILE_IO_OPEN_TEMPORARY ? file->openTemporary(entry.m_path.c_str()) :
            (request.operation == NET_FILE_IO_DUPLICATE_READ ? file->duplicate(request.descriptor, false) :
            file->open(entry.m_path.c_str(), request.operation));
        if (!opened)
        {
            entry.m_result.result = NI_INVALID_STATE;
            entry.m_result.detail = file->lastError();
            return;
        }
        uint64_t size = 0;
        if (!file->size(&size))
        {
            entry.m_result.result = NI_INVALID_STATE;
            entry.m_result.detail = file->lastError();
            return;
        }
        const uint64_t id = ++m_lastFile;
        m_files.insert(std::make_pair(id, std::move(file)));
        entry.m_result.file = id;
        entry.m_result.length = size;
        return;
    }
    if (request.operation == NET_FILE_IO_PUBLISH)
    {
        if (m_stop.load() || entry.m_cancelled.load())
        {
            entry.m_result.result = NI_CANCELLED;
            return;
        }
        if (request.file != 0)
        {
            const auto temporary = m_files.find(request.file);
            if (temporary == m_files.end())
            {
                entry.m_result.result = NI_NOT_FOUND;
                return;
            }
            const bool published = temporary->second->publish(entry.m_path.c_str(), entry.m_destination.c_str(), request.flags != 0);
            entry.m_result.result = published ? NI_OK : NI_INVALID_STATE;
            m_files.erase(temporary);
        }
        else
        {
            entry.m_result.result =
                CSystem::publishFileUtf8(entry.m_path.c_str(), entry.m_destination.c_str(), request.flags != 0) ? NI_OK : NI_INVALID_STATE;
        }
        return;
    }
    const auto found = m_files.find(request.file);
    if (found == m_files.end())
    {
        entry.m_result.result = NI_NOT_FOUND;
        return;
    }
    NetFileIoHandle& file = *found->second;
    bool result = false;
    if (request.operation == NET_FILE_IO_READ)
    {
        entry.m_output.resize(request.size);
        uint32_t received = 0;
        result = file.read(request.offset, request.size ? entry.m_output.data() : nullptr, request.size, &received);
        entry.m_output.resize(result ? received : 0);
        entry.m_result.length = received;
    }
    else if (request.operation == NET_FILE_IO_WRITE)
    {
        result = file.write(request.offset, entry.m_input.empty() ? nullptr : entry.m_input.data(), request.size);
        if (result && request.flags != 0)
        {
            result = file.flush();
        }
        entry.m_result.length = result ? request.size : 0;
    }
    else if (request.operation == NET_FILE_IO_SIZE)
    {
        result = file.size(&entry.m_result.length);
    }
    else if (request.operation == NET_FILE_IO_RESIZE)
    {
        result = file.resize(request.offset);
    }
    else if (request.operation == NET_FILE_IO_FLUSH)
    {
        result = file.flush();
    }
    else if (request.operation == NET_FILE_IO_HASH)
    {
        hash(entry, file);
        return;
    }
    else if (request.operation == NET_FILE_IO_CLOSE)
    {
        result = file.close();
        entry.m_result.detail = file.lastError();
        m_files.erase(found);
        entry.m_result.result = result ? NI_OK : NI_INVALID_STATE;
        return;
    }
    entry.m_result.result = result ? NI_OK : NI_INVALID_STATE;
    entry.m_result.detail = result ? 0 : file.lastError();
}

NetFileIoHandle::NetFileIoHandle() :
m_handle(-1),
m_error(0),
m_writable(false),
m_ownsTemporary(false)
{

}

NetFileIoHandle::~NetFileIoHandle()
{
    close();
}

bool NetFileIoHandle::open(const char* path, uint32_t mode)
{
    if (m_handle != -1)
    {
        return false;
    }
    // 新建或恢复的可写文件记录精确源路径，以便PUBLISH携带file核对来源；普通CLOSE仍不删除它。
    // 自动删除权仍只由openTemporary在独占创建成功后授予。
    if (mode == NET_FILE_IO_OPEN_NEW || mode == NET_FILE_IO_OPEN_WRITE)
    {
        m_temporary = CSystem::canonicalPathUtf8(path, false);
        if (m_temporary.empty()) { return false; }
    }
    m_handle = CSystem::openFileHandle((mode == NET_FILE_IO_OPEN_NEW || mode == NET_FILE_IO_OPEN_WRITE) ? m_temporary.c_str() : path, mode, &m_error);
    m_writable = m_handle != -1 && mode != 1;
    return m_handle != -1;
}

bool NetFileIoHandle::openTemporary(const char* path)
{
    if (m_handle != -1 || path == nullptr)
    {
        return false;
    }
    m_temporary = CSystem::canonicalPathUtf8(path, false);
    const std::string parent = CSystem::GetName(m_temporary, 4);
    if (m_temporary.empty() || parent.empty() || !CSystem::createDirectoriesUtf8(parent))
    {
        return false;
    }
#ifdef _WIN32
    // 必须在创建文件前完成可失败的转换；取得删除责任后，关闭路径不再分配。
    m_cleanupPath = CStringManager::Utf8ToUnicode(m_temporary);
    if (m_cleanupPath.empty())
    {
        return false;
    }
#endif
    if (!open(m_temporary.c_str(), 2))
    {
        return false;
    }
    m_ownsTemporary = true;
    return true;
}

bool NetFileIoHandle::publish(const char* source, const char* destination, bool replace)
{
    if (!m_writable || m_handle == -1 || m_temporary.empty() || source == nullptr || destination == nullptr ||
        CSystem::canonicalPathUtf8(source, false) != m_temporary)
    {
        return false;
    }
    if (!CSystem::closeFileHandle(&m_handle, &m_error) ||
        !CSystem::publishFileUtf8(m_temporary.c_str(), destination, replace))
    {
        return false;
    }
    m_ownsTemporary = false;
    m_temporary.clear();
#ifdef _WIN32
    m_cleanupPath.clear();
#endif
    return true;
}

bool NetFileIoHandle::duplicate(intptr_t native, bool writable)
{
    if (m_handle != -1)
    {
        return false;
    }
    m_handle = CSystem::duplicateFileHandle(native, &m_error);
    m_writable = m_handle != -1 && writable;
    return m_handle != -1;
}

bool NetFileIoHandle::read(uint64_t offset, uint8_t* bytes, uint32_t size, uint32_t* received)
{
    return CSystem::readFileHandle(m_handle, offset, bytes, size, received, &m_error);
}

bool NetFileIoHandle::write(uint64_t offset, const uint8_t* bytes, uint32_t size)
{
    if (!m_writable)
    {
        m_error = kNetFileIoReadOnlyError;
        return false;
    }
    return CSystem::writeFileHandle(m_handle, offset, bytes, size, &m_error);
}

bool NetFileIoHandle::flush()
{
    if (!m_writable)
    {
        m_error = kNetFileIoReadOnlyError;
        return false;
    }
    return CSystem::flushFileHandle(m_handle, &m_error);
}

bool NetFileIoHandle::size(uint64_t* bytes)
{
    return CSystem::fileHandleSize(m_handle, bytes, &m_error);
}

bool NetFileIoHandle::resize(uint64_t bytes)
{
    if (!m_writable)
    {
        m_error = kNetFileIoReadOnlyError;
        return false;
    }
    return CSystem::resizeFileHandle(m_handle, bytes, &m_error);
}

bool NetFileIoHandle::close()
{
    m_writable = false;
    const bool closed = CSystem::closeFileHandle(&m_handle, &m_error);
    bool removed = true;
    if (m_ownsTemporary)
    {
#ifdef _WIN32
        removed = CSystem::deleteFile(m_cleanupPath);
#else
        // POSIX路径原本就是UTF8字节，避免经过宽字符再次分配并转换。
        removed = CSystem::deleteFile(m_temporary.c_str());
#endif
        if (removed)
        {
            m_ownsTemporary = false;
            m_temporary.clear();
#ifdef _WIN32
            m_cleanupPath.clear();
#endif
        }
    }
    return closed && removed;
}

uint32_t NetFileIoHandle::lastError() const
{
    return m_error;
}

NetFileIoEntry::NetFileIoEntry(const NetFileIoRequest& request) :
m_request(request),
m_cancelled(false),
m_completed(false)
{
    std::memset(&m_result, 0, sizeof(m_result));
    m_result.id = request.id;
    m_result.operation = request.operation;
    m_result.file = request.file;
    m_result.offset = request.offset;
    m_result.result = NI_OK;
    if (request.path != nullptr)
    {
        m_path = request.path;
    }
    if (request.destination != nullptr)
    {
        m_destination = request.destination;
    }
    if (request.operation == NET_FILE_IO_WRITE && request.size != 0)
    {
        m_input.assign(request.bytes, request.bytes + request.size);
    }
    m_request.path = nullptr;
    m_request.destination = nullptr;
    m_request.bytes = nullptr;
}

NetFileIoEntry::~NetFileIoEntry()
{
    NetInterface::wipe(m_input.empty() ? nullptr : m_input.data(), static_cast<uint32_t>(m_input.size()));
    NetInterface::wipe(m_output.empty() ? nullptr : m_output.data(), static_cast<uint32_t>(m_output.size()));
}

NetFileIoState::NetFileIoState() :
m_stop(false),
m_done(false),
m_waitFailed(false),
m_lastRequest(0),
m_allocatedRequest(0),
m_lastFile(0),
m_inputBytes(0)
{

}

void NetFileIoState::stop()
{
    if (!m_gate.valid())
    {
        m_stop.store(true);
        return;
    }
    {
        // 与DoTask的谓词检查和原子解锁／等待配对，避免停止通知丢失后永久阻塞回收。
        std::lock_guard<NetWorkGate> lock(m_gate);
        m_stop.store(true);
    }
    m_gate.notify();
}

NetFileIoTask::NetFileIoTask(const std::shared_ptr<NetFileIoState>& state) :
m_state(state)
{

}

void NetFileIoTask::StopTask()
{
    m_state->stop();
}

void NetFileIoTask::DoTask()
{
    for (;;)
    {
        std::shared_ptr<NetFileIoEntry> entry;
        {
            std::lock_guard<NetWorkGate> lock(m_state->m_gate);
            while (!m_state->m_stop.load() && m_state->m_queued.empty())
            {
                if (!m_state->m_gate.wait())
                {
                    m_state->m_waitFailed = true;
                    m_state->m_stop.store(true);
                    break;
                }
            }
            if (m_state->m_queued.empty())
            {
                break;
            }
            entry = m_state->m_queued.front();
            m_state->m_queued.pop_front();
        }
        try
        {
            if (m_state->m_stop.load() || entry->m_cancelled.load())
            {
                entry->m_result.result = m_state->m_waitFailed ? NI_INVALID_STATE : NI_CANCELLED;
            }
            else
            {
                m_state->execute(*entry);
            }
        }
        catch (...)
        {
            entry->m_result.result = NI_NO_MEMORY;
        }
        // CLOSE／PUBLISH的完成结果消耗file所有权，即使排队时已取消或执行异常也必须回收。
        // 文件表仅本执行者访问，释放句柄及临时文件时不持协调锁。
        if (entry->m_request.operation == NET_FILE_IO_CLOSE || entry->m_request.operation == NET_FILE_IO_PUBLISH)
        {
            m_state->m_files.erase(entry->m_request.file);
        }
        {
            std::lock_guard<NetWorkGate> lock(m_state->m_gate);
            // 完成标志原地更新，内存不足时也不会丢失已接纳工作或阻塞done。
            entry->m_completed = true;
        }
    }
    m_state->m_files.clear();
    m_state->m_done.store(true);
}
#endif
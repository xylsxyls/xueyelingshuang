#if !defined(_WIN32) && !defined(_FILE_OFFSET_BITS)
#define _FILE_OFFSET_BITS 64
#endif
#include "LogManagerFile.h"
#include "LogManagerHelper.h"
#include "CSystem/CSystemAPI.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

LogManagerFile::LogManagerFile() :
    m_linkIndex(-1), m_currentIndex(0), m_rollRequested(false), m_linkFileLock(nullptr),
#ifdef _WIN32
    m_file(INVALID_HANDLE_VALUE), m_granularity(0),
#else
    m_fd(-1),
#endif
    m_tail(0), m_written(0)
{
}

LogManagerFile::~LogManagerFile()
{
    closeEntity();
    LogManagerHelper::closeFileLock(m_linkFileLock);
}

bool LogManagerFile::openEntity(const std::string& path)
{
    const intptr_t handle = CSystem::createDurableFile(path);
    if (handle == -1)
    {
        return false;
    }
    closeEntity();
#ifdef _WIN32
    m_file = reinterpret_cast<void*>(handle);
    SYSTEM_INFO system;
    GetSystemInfo(&system);
    m_granularity = system.dwAllocationGranularity;
#else
    m_fd = static_cast<int>(handle);
#endif
    m_tail.store(0);
    m_written.store(0);
    m_rollRequested.store(false);
    return true;
}

void LogManagerFile::closeEntity()
{
#ifdef _WIN32
    if (m_file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_file);
        m_file = INVALID_HANDLE_VALUE;
    }
#else
    if (m_fd >= 0)
    {
        close(m_fd);
        m_fd = -1;
    }
#endif
}

#ifndef _WIN32
bool LogManagerFile::writeAt(int64_t offset, const char* data, size_t size)
{
    while (size != 0)
    {
        const size_t chunk = (std::min)(size, static_cast<size_t>(0x40000000));
        if (offset > (std::numeric_limits<off_t>::max)())
        {
            return false;
        }
        ssize_t written = pwrite(m_fd, data, chunk, static_cast<off_t>(offset));
        if (written < 0 && errno == EINTR)
        {
            continue;
        }
        if (written <= 0)
        {
            return false;
        }
        offset += written;
        data += written;
        size -= written;
    }
    return true;
}

#endif

#ifdef _WIN32
/** 写入映射区并提交标志，映射设备错误作为写入失败处理
@param [out] target 本记录独占的映射区
@param [in] data 含临时标志的记录
@param [in] size 记录字节数
@param [in] file 映射对应的文件句柄
@return 正文与标志都已刷新时返回 true
*/
static bool CopyAndCommit(char* target, const char* data, size_t size, HANDLE file)
{
#ifdef _MSC_VER
    __try
    {
#endif
        std::memcpy(target, data, 4); // Publish the record's own newline and provisional marker first.
        MemoryBarrier();
        std::memcpy(target + 4, data + 4, size - 4);
        if (!FlushViewOfFile(target, size) || !CSystem::syncDurableFile(reinterpret_cast<intptr_t>(file)))
        {
            return false;
        }
        InterlockedExchange8(reinterpret_cast<volatile CHAR*>(target + 2), '1');
        return FlushViewOfFile(target + 2, 1) && CSystem::syncDurableFile(reinterpret_cast<intptr_t>(file));
#ifdef _MSC_VER
    }
    __except (GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return false;
    }
#endif
}
#endif

bool LogManagerFile::commit(int64_t start, const std::string& record)
{
#ifdef _WIN32
    const uint64_t end = static_cast<uint64_t>(start) + record.size();
    const size_t prefix = static_cast<size_t>(start % m_granularity);
    if (prefix > SIZE_MAX - record.size())
    {
        return false;
    }
    const uint64_t base = static_cast<uint64_t>(start) - prefix;
    HANDLE mapping = CreateFileMappingA(m_file, nullptr, PAGE_READWRITE, static_cast<DWORD>(end >> 32),
        static_cast<DWORD>(end), nullptr);
    if (!mapping)
    {
        return false;
    }
    char* view = static_cast<char*>(MapViewOfFile(mapping, FILE_MAP_WRITE, static_cast<DWORD>(base >> 32),
        static_cast<DWORD>(base), prefix + record.size()));
    bool ok = false;
    if (view)
    {
        ok = CopyAndCommit(view + prefix, record.data(), record.size(), m_file);
        UnmapViewOfFile(view);
    }
    CloseHandle(mapping);
    return ok;
#else
    return writeAt(start, record.data(), 4)
        && writeAt(start + 4, record.data() + 4, record.size() - 4) && CSystem::syncDurableFile(m_fd)
        && writeAt(start + 2, "1", 1) && CSystem::syncDurableFile(m_fd);
#endif
}

bool LogManagerFile::append(const std::string& line)
{
    std::string record("\n[0]");
    record.reserve(record.size() + line.size());
    for (size_t i = 0; i < line.size(); ++i)
    {
        if (line[i] == '\n')
        {
            record += "\\n";
        }
        else if (line[i] == '\r')
        {
            record += "\\r";
        }
        else
        {
            record += line[i];
        }
    }
    int64_t start = m_tail.load();
    do
    {
        if (record.size() > static_cast<uint64_t>(INT64_MAX - start))
        {
            return false;
        }
    } while (!m_tail.compare_exchange_weak(start, start + static_cast<int64_t>(record.size())));

    // Independent offsets allow later records to finish across an incomplete one.
    // Persist the body before changing its single ASCII commit byte.
    const bool ok = commit(start, record);
    if (m_config.m_maxFileBytes != -1 && (m_written.fetch_add(1) + 1) % m_config.m_checkFileSizeInterval == 0
        && m_tail.load() > m_config.m_maxFileBytes)
    {
        m_rollRequested.store(true);
    }
    return ok;
}

LogManagerWriterGuard::LogManagerWriterGuard(std::atomic<uint32_t>& state) :
m_state(state),
m_acquired(false)
{
    uint32_t count = m_state.load();
    while (count < kClosing - 1)
    {
        if (m_state.compare_exchange_weak(count, count + 1))
        {
            m_acquired = true;
            break;
        }
    }
}

LogManagerWriterGuard::~LogManagerWriterGuard()
{
    if (m_acquired)
    {
        m_state.fetch_sub(1);
    }
}

bool LogManagerWriterGuard::acquired() const
{
    return m_acquired;
}

LogManagerRegistryGuard::LogManagerRegistryGuard(std::mutex& mutex, std::atomic<uint32_t>& state) :
m_lock(mutex),
m_state(state)
{
    m_state.fetch_or(LogManagerWriterGuard::kClosing);
    while (m_state.load() != LogManagerWriterGuard::kClosing)
    {
        std::this_thread::yield();
    }
}

LogManagerRegistryGuard::~LogManagerRegistryGuard()
{
    m_state.store(0);
}
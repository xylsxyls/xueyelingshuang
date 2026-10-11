#ifndef _WIN32
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#endif
#include "KaraokeStorage.h"
#include <algorithm>
#include <limits>
#include <cstdlib>
#include <string>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

KaraokeStorage::KaraokeStorage() :
m_file(nullptr),
m_size(0),
m_error("")
{
#ifdef _WIN32
    wchar_t directory[MAX_PATH + 1] = {}, path[MAX_PATH + 1] = {};
    const DWORD length = GetTempPathW(MAX_PATH, directory);
    if (length == 0 || length >= MAX_PATH || GetTempFileNameW(directory, L"LKR", 0, path) == 0)
    {
        fail("Cannot create karaoke temporary storage");
        return;
    }
    HANDLE handle = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
    {
        DeleteFileW(path);
        fail("Cannot open karaoke temporary storage");
        return;
    }
    const int descriptor = _open_osfhandle(reinterpret_cast<intptr_t>(handle), _O_BINARY | _O_RDWR);
    if (descriptor < 0)
    {
        CloseHandle(handle);
    }
    else
    {
        m_file = _fdopen(descriptor, "w+b");
        if (m_file == nullptr)
        {
            _close(descriptor);
        }
    }
#else
    // Mobile hosts supply their writable sandbox cache directory through TMPDIR.
    const char* directory = std::getenv("TMPDIR");
    if (directory && *directory)
    {
        std::string path(directory);
        if (path.back() != '/')
        {
            path += '/';
        }
        path += "LumaKaraoke-XXXXXX";
        std::vector<char> name(path.begin(), path.end());
        name.push_back(0);
        const int descriptor = mkstemp(name.data());
        if (descriptor >= 0)
        {
            if (unlink(name.data()) == 0)
            {
                fcntl(descriptor, F_SETFD, FD_CLOEXEC);
                m_file = fdopen(descriptor, "w+b");
            }
            if (!m_file)
            {
                close(descriptor);
            }
        }
    }
    else
    {
        m_file = std::tmpfile();
    }
#endif
    if (m_file == nullptr)
    {
        fail("Cannot initialize karaoke temporary storage");
    }
}

KaraokeStorage::~KaraokeStorage()
{
    if (m_file != nullptr)
    {
        std::fclose(m_file);
    }
}

bool KaraokeStorage::valid() const
{
    return m_file != nullptr;
}

const char* KaraokeStorage::error()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_error;
}

bool KaraokeStorage::fail(const char* message)
{
    m_error = message;
    return false;
}

bool KaraokeStorage::seek(uint64_t offset)
{
    if (m_file == nullptr)
    {
        return false;
    }
    if (offset > static_cast<uint64_t>((std::numeric_limits<int64_t>::max)()))
    {
        return fail("Karaoke storage offset overflow");
    }
#ifdef _WIN32
    const int result = _fseeki64(m_file, static_cast<int64_t>(offset), SEEK_SET);
#else
    if (offset > static_cast<uint64_t>((std::numeric_limits<off_t>::max)()))
    {
        return fail("Karaoke storage platform offset overflow");
    }
    const int result = fseeko(m_file, static_cast<off_t>(offset), SEEK_SET);
#endif
    if (result != 0)
    {
        return fail("Karaoke storage seek failed");
    }
    return true;
}

uint64_t KaraokeStorage::append(const void* data, size_t bytes)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file == nullptr)
    {
        return UINT64_MAX;
    }
    if (bytes > static_cast<uint64_t>((std::numeric_limits<int64_t>::max)()) - m_size)
    {
        fail("Karaoke storage size overflow");
        return UINT64_MAX;
    }
    std::clearerr(m_file);
    if (!seek(m_size))
    {
        return UINT64_MAX;
    }
    const unsigned char silence[4096] = {};
    const unsigned char* input = static_cast<const unsigned char*>(data);
    size_t remaining = bytes;
    while (remaining != 0)
    {
        const size_t count = input ? remaining : (std::min)(remaining, sizeof(silence));
        if (std::fwrite(input ? input : silence, 1, count, m_file) != count)
        {
            fail("Karaoke temporary disk is full or not writable; recording retained");
            return UINT64_MAX;
        }
        remaining -= count;
        if (input != nullptr)
        {
            input += count;
        }
    }
    if (std::fflush(m_file) != 0)
    {
        fail("Karaoke temporary storage flush failed; recording retained");
        return UINT64_MAX;
    }
    const uint64_t start = m_size;
    m_size += bytes;
    return start;
}

bool KaraokeStorage::read(uint64_t offset, void* data, size_t bytes)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file == nullptr)
    {
        return false;
    }
    if ((data == nullptr && bytes != 0) || offset > m_size || bytes > m_size - offset)
    {
        return fail("Invalid karaoke storage read");
    }
    if (bytes == 0)
    {
        return true;
    }
    std::clearerr(m_file);
    if (!seek(offset))
    {
        return false;
    }
    if (std::fread(data, 1, bytes, m_file) != bytes)
    {
        return fail("Karaoke temporary storage read failed");
    }
    return true;
}

uint64_t KaraokeStorage::size()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_size;
}

bool KaraokeStorage::write(uint64_t offset, const void* data, size_t bytes)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file == nullptr)
    {
        return false;
    }
    if ((data == nullptr && bytes != 0) || offset > m_size || bytes > m_size - offset)
    {
        return fail("Invalid karaoke scratch write");
    }
    if (bytes == 0)
    {
        return true;
    }
    std::clearerr(m_file);
    if (!seek(offset))
    {
        return false;
    }
    if (std::fwrite(data, 1, bytes, m_file) != bytes || std::fflush(m_file) != 0)
    {
        return fail("Karaoke scratch write failed");
    }
    return true;
}
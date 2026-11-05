#include "Semaphore.h"
#include "SemaphoreProcessMemory.h"
#include <system_error>
#include <chrono>
#include <algorithm>
#include <limits.h>
#include <stdexcept>
#ifdef _WIN32
#include <Windows.h>
#else
#include <semaphore.h>
#include <fcntl.h>
#include <cerrno>
#include <time.h>
#ifdef SEMAPHORE_SHARED_COUNTER
#include <atomic>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#endif

Semaphore::Semaphore() :
m_count(0),
m_eventPool(nullptr),
m_processSemaphore(nullptr),
m_processCreated(false)
{
    if (!m_count.is_lock_free() || !m_eventPool.is_lock_free())
    {
        throw std::system_error(std::make_error_code(std::errc::not_supported));
    }
}

Semaphore::~Semaphore()
{
    delete m_eventPool.load();
    if ((m_processSemaphore != nullptr || m_processMemory || m_processCreated) &&
        closeProcessResult().status != SEMAPHORE_OK)
    {
        SemaphoreWake::recordCleanupFailure();
    }
}

void Semaphore::signal()
{
    checkLocalResult(trySignal());
}

SemaphoreResult Semaphore::trySignal()
{
    uint64_t count = m_count.load();
    do
    {
        if (count == UINT64_MAX)
        {
            return SemaphoreResult(SEMAPHORE_OVERFLOW);
        }
    } while (!m_count.compare_exchange_weak(count, count + 1));
    SemaphoreResult result = m_wake.notify();
    result.committed = true;
    return result;
}

void Semaphore::wait()
{
    if (consumeCount())
    {
        return;
    }
    checkLocalResult(waitResult());
}

bool Semaphore::wait(uint32_t timeout)
{
    if (consumeCount())
    {
        return true;
    }
    const SemaphoreResult result = waitResult(timeout);
    checkLocalResult(result);
    return result.status == SEMAPHORE_OK;
}

SemaphoreResult Semaphore::waitResult()
{
    return waitLocal(false, true, 0);
}

SemaphoreResult Semaphore::waitResult(uint32_t timeout)
{
    return waitLocal(false, false, timeout);
}

void Semaphore::reset()
{
    // 外部静止；系统中即使有旧提示，也不再代表许可或事件。
    m_count.store(0);
}

void Semaphore::event()
{
    checkLocalResult(tryEvent(false));
}

void Semaphore::eventAll()
{
    checkLocalResult(tryEvent(true));
}

SemaphoreResult Semaphore::tryEvent(bool all)
{
    SemaphoreEventPool* pool = m_eventPool.load();
    if (pool == nullptr)
    {
        return SemaphoreResult();
    }
    const bool direct = m_wake.canWaitAddress();
    SemaphoreResult result = pool->notify(all, direct ? &m_wake : nullptr);
    // 没有合格事件登记者时，不触发无业务作用的系统广播
    if (direct || result.status != SEMAPHORE_OK || !result.committed)
    {
        return result;
    }
    const SemaphoreResult wake = m_wake.notify();
    result.status = wake.status;
    result.nativeError = wake.nativeError;
    return result;
}

void Semaphore::eventWait()
{
    checkLocalResult(eventWaitResult());
}

bool Semaphore::eventWait(int32_t timeout)
{
    const SemaphoreResult result = eventWaitResult(timeout);
    checkLocalResult(result);
    return result.status == SEMAPHORE_OK;
}

SemaphoreResult Semaphore::eventWaitResult()
{
    return waitLocal(true, true, 0);
}

SemaphoreResult Semaphore::eventWaitResult(int32_t timeout)
{
    if (timeout <= 0)
    {
        return SemaphoreResult(SEMAPHORE_TIMEOUT);
    }
    return waitLocal(true, false, static_cast<uint32_t>(timeout));
}

SemaphoreResult Semaphore::configureEventPool(const SemaphorePoolConfig& config)
{
    try
    {
        std::unique_ptr<SemaphoreEventPool> candidate(new SemaphoreEventPool(config));
        delete m_eventPool.exchange(candidate.release());
        return SemaphoreResult();
    }
    catch (const std::invalid_argument&)
    {
        return SemaphoreResult(SEMAPHORE_INVALID);
    }
    catch (const std::bad_alloc&)
    {
        return SemaphoreResult(SEMAPHORE_NO_MEMORY);
    }
    catch (const std::system_error& error)
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, error.code().value());
    }
    catch (...)
    {
        return SemaphoreResult(SEMAPHORE_POLICY);
    }
}

SemaphoreResult Semaphore::maintainEventPool(size_t& released)
{
    SemaphoreEventPool* pool = m_eventPool.load();
    released = 0;
    return pool != nullptr ? pool->maintain(released) : SemaphoreResult();
}

SemaphorePoolStats Semaphore::eventPoolStatistics() const
{
    SemaphoreEventPool* pool = m_eventPool.load();
    if (pool != nullptr)
    {
        return pool->statistics();
    }
    const SemaphorePoolStats empty = {};
    return empty;
}

bool Semaphore::setProcessDirectory(const std::string& directory)
{
#if defined(SEMAPHORE_SHARED_COUNTER) && !defined(_WIN32)
    if (SemaphoreProcessMemory::directorySealed().load())
    {
        return directory == SemaphoreProcessMemory::directory();
    }
    const int32_t descriptor = SemaphoreProcessMemory::openDirectory(directory);
    if (descriptor < 0)
    {
        return false;
    }
    if (::close(descriptor) != 0)
    {
        SemaphoreWake::recordCleanupFailure();
        return false;
    }
    try
    {
        SemaphoreProcessMemory::directory() = directory;
    }
    catch (const std::bad_alloc&)
    {
        return false;
    }
    return true;
#else
    return directory.empty();
#endif
}

void Semaphore::createProcessSemaphore(const std::string& name, int32_t signalCount)
{
    // 保留旧void接口的探测式用途；新接入用逐调用结果，不据void返回判断成功。
    createProcessResult(name, signalCount);
}

SemaphoreResult Semaphore::createProcessResult(const std::string& name, int32_t signalCount)
{
    if (m_processSemaphore != nullptr || m_processMemory || m_processCreated || !validProcessName(name) ||
        signalCount <= 0)
    {
        return SemaphoreResult(SEMAPHORE_INVALID);
    }
    try
    {
#ifdef SEMAPHORE_SHARED_COUNTER
        std::unique_ptr<SemaphoreProcessMemory> candidate(new SemaphoreProcessMemory);
        const SemaphoreResult result = candidate->open(name, signalCount);
        if (result.status != SEMAPHORE_OK)
        {
            return result;
        }
        m_processSemaphore = candidate->m_words;
        m_processMemory.swap(candidate);
#elif defined(_WIN32)
        m_processSemaphore = ::CreateSemaphoreA(nullptr, 0, signalCount, name.c_str());
        if (m_processSemaphore == nullptr)
        {
            return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, static_cast<int32_t>(::GetLastError()));
        }
#elif defined(__EMSCRIPTEN__)
        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
#else
        // 所有可能抛异常的字符串工作先于取得原生引用。
        m_processName = "/" + name;
        sem_t* value = ::sem_open(m_processName.c_str(), O_CREAT | O_EXCL, 0600, 0);
        m_processCreated = value != SEM_FAILED;
        if (value == SEM_FAILED && errno == EEXIST)
        {
            value = ::sem_open(m_processName.c_str(), 0);
        }
        if (value == SEM_FAILED)
        {
            const int32_t error = errno;
            m_processName.clear();
            return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, error);
        }
        m_processSemaphore = value;
#endif
    }
    catch (const std::bad_alloc&)
    {
        return SemaphoreResult(SEMAPHORE_NO_MEMORY);
    }
    catch (const std::length_error&)
    {
        return SemaphoreResult(SEMAPHORE_INVALID);
    }
    return SemaphoreResult();
}

void Semaphore::openProcessSemaphore(const std::string& name)
{
    openProcessResult(name);
}

bool Semaphore::tryOpenProcessSemaphore(const std::string& name)
{
    return openProcessResult(name).status == SEMAPHORE_OK;
}

SemaphoreResult Semaphore::openProcessResult(const std::string& name)
{
    if (m_processSemaphore != nullptr)
    {
        return SemaphoreResult();
    }
    if (m_processMemory || m_processCreated)
    {
        // 上一次部分关闭仍有句柄/撤名责任，不能覆盖其所有权。
        return SemaphoreResult(SEMAPHORE_INVALID);
    }
    if (!validProcessName(name))
    {
        return SemaphoreResult(SEMAPHORE_INVALID);
    }
    try
    {
#ifdef SEMAPHORE_SHARED_COUNTER
        std::unique_ptr<SemaphoreProcessMemory> candidate(new SemaphoreProcessMemory);
        const SemaphoreResult result = candidate->open(name, 0);
        if (result.status != SEMAPHORE_OK)
        {
            return result;
        }
        m_processSemaphore = candidate->m_words;
        m_processMemory.swap(candidate);
#elif defined(_WIN32)
        m_processSemaphore = ::OpenSemaphoreA(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, name.c_str());
        if (m_processSemaphore == nullptr)
        {
            return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, static_cast<int32_t>(::GetLastError()));
        }
#elif defined(__EMSCRIPTEN__)
        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
#else
        m_processName = "/" + name;
        sem_t* value = ::sem_open(m_processName.c_str(), 0);
        if (value == SEM_FAILED)
        {
            const int32_t error = errno;
            m_processName.clear();
            return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, error);
        }
        m_processSemaphore = value;
#endif
    }
    catch (const std::bad_alloc&)
    {
        return SemaphoreResult(SEMAPHORE_NO_MEMORY);
    }
    catch (const std::length_error&)
    {
        return SemaphoreResult(SEMAPHORE_INVALID);
    }
    return SemaphoreResult();
}

void Semaphore::closeProcessSemaphore()
{
    if (closeProcessResult().status != SEMAPHORE_OK)
    {
        SemaphoreWake::recordCleanupFailure();
    }
}

SemaphoreResult Semaphore::closeProcessResult()
{
    SemaphoreResult closed;
#ifdef SEMAPHORE_SHARED_COUNTER
    if (m_processMemory)
    {
        const SemaphoreResult result = m_processMemory->close();
        m_processSemaphore = m_processMemory->m_words;
        if (result.status != SEMAPHORE_OK)
        {
            return result;
        }
        m_processMemory.reset();
    }
#elif defined(_WIN32)
    if (m_processSemaphore != nullptr && !::CloseHandle(m_processSemaphore))
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, static_cast<int32_t>(::GetLastError()));
    }
#elif !defined(__EMSCRIPTEN__)
    if (m_processCreated && !m_processName.empty())
    {
        if (::sem_unlink(m_processName.c_str()) != 0 && errno != ENOENT)
        {
            closed = SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
        }
        else
        {
            m_processCreated = false;
            m_processName.clear();
        }
    }
    if (m_processSemaphore != nullptr && ::sem_close(static_cast<sem_t*>(m_processSemaphore)) != 0)
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
    }
#endif
    m_processSemaphore = nullptr;
    if (closed.status != SEMAPHORE_OK)
    {
        // 本地引用已释放，仍保留撤名责任供显式重试，不能漏掉sem_close。
        return closed;
    }
    m_processCreated = false;
    m_processName.clear();
    return SemaphoreResult();
}

bool Semaphore::unlinkProcessName()
{
#ifdef SEMAPHORE_SHARED_COUNTER
    return m_processMemory && m_processMemory->unlinkName().status == SEMAPHORE_OK;
#elif defined(_WIN32) || defined(__EMSCRIPTEN__)
    return false;
#else
    if (!m_processCreated || m_processName.empty())
    {
        return false;
    }
    // 只有独占创建者可撤名；打开者和同名加入者不能删除本代或后来重建的名称。
    if (::sem_unlink(m_processName.c_str()) != 0 && errno != ENOENT)
    {
        return false;
    }
    // 一次撤名权已消耗；旧引用不能随后再删除同名新代。
    m_processCreated = false;
    m_processName.clear();
    return true;
#endif
}

void Semaphore::processSignal()
{
    processSignalResult();
}

int32_t Semaphore::tryProcessSignal()
{
    const SemaphoreResult result = processSignalResult();
    return result.status == SEMAPHORE_OK ? 0 : (result.status == SEMAPHORE_OVERFLOW ? 1 : -1);
}

SemaphoreResult Semaphore::processSignalResult()
{
    if (m_processSemaphore == nullptr)
    {
        return SemaphoreResult(SEMAPHORE_NOT_OPEN);
    }
#ifdef SEMAPHORE_SHARED_COUNTER
    int32_t* words = static_cast<int32_t*>(m_processSemaphore);
    const int32_t limit = SemaphoreProcessMemory::load(words + 1);
    int32_t count = SemaphoreProcessMemory::load(words);
    while (limit > 0 && count >= 0 && count < limit)
    {
        if (SemaphoreProcessMemory::compareExchange(words, count, count + 1))
        {
            return SemaphoreResult(SEMAPHORE_OK, true);
        }
        count = SemaphoreProcessMemory::load(words);
    }
    return SemaphoreResult(limit > 0 && count == limit ? SEMAPHORE_OVERFLOW : SEMAPHORE_INVALID);
#elif defined(_WIN32)
    if (::ReleaseSemaphore(m_processSemaphore, 1, nullptr))
    {
        return SemaphoreResult(SEMAPHORE_OK, true);
    }
    const DWORD error = ::GetLastError();
    return SemaphoreResult(error == ERROR_TOO_MANY_POSTS ? SEMAPHORE_OVERFLOW : SEMAPHORE_SYSTEM_ERROR, false,
        error == ERROR_TOO_MANY_POSTS ? 0 : static_cast<int32_t>(error));
#elif defined(__EMSCRIPTEN__)
    return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
#else
    if (::sem_post(static_cast<sem_t*>(m_processSemaphore)) == 0)
    {
        return SemaphoreResult(SEMAPHORE_OK, true);
    }
    const int32_t error = errno;
    return SemaphoreResult(
        error == EOVERFLOW ? SEMAPHORE_OVERFLOW : SEMAPHORE_SYSTEM_ERROR, false, error == EOVERFLOW ? 0 : error);
#endif
}

void Semaphore::processWait()
{
    processWaitResult(-1);
}

bool Semaphore::processWait(int32_t timeout)
{
    return processWaitResult(timeout).status == SEMAPHORE_OK;
}

SemaphoreResult Semaphore::processWaitResult(int32_t timeout)
{
    if (timeout < -1)
    {
        return SemaphoreResult(SEMAPHORE_INVALID);
    }
#ifdef __EMSCRIPTEN__
    return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
#else
    if (m_processSemaphore == nullptr)
    {
        return SemaphoreResult(SEMAPHORE_NOT_OPEN);
    }
#if defined(_WIN32) && !defined(SEMAPHORE_SHARED_COUNTER)
    const DWORD waited =
        ::WaitForSingleObject(m_processSemaphore, timeout == -1 ? INFINITE : static_cast<DWORD>(timeout));
    if (waited == WAIT_OBJECT_0)
    {
        return SemaphoreResult(SEMAPHORE_OK, true);
    }
    return SemaphoreResult(waited == WAIT_TIMEOUT ? SEMAPHORE_TIMEOUT : SEMAPHORE_SYSTEM_ERROR, false,
        waited == WAIT_TIMEOUT ? 0 : static_cast<int32_t>(::GetLastError()));
#else
    const uint64_t started = monotonicMilliseconds();
    for (;;)
    {
#ifdef SEMAPHORE_SHARED_COUNTER
        int32_t* words = static_cast<int32_t*>(m_processSemaphore);
        const int32_t maximum = SemaphoreProcessMemory::load(words + 1);
        int32_t count = SemaphoreProcessMemory::load(words);
        if (maximum <= 0 || count < 0 || count > maximum)
        {
            return SemaphoreResult(SEMAPHORE_INVALID);
        }
        while (count > 0)
        {
            if (SemaphoreProcessMemory::compareExchange(words, count, count - 1))
            {
                return SemaphoreResult(SEMAPHORE_OK, true);
            }
            count = SemaphoreProcessMemory::load(words);
        }
#else
        const int32_t result = timeout < 0 ? ::sem_wait(static_cast<sem_t*>(m_processSemaphore))
                                           : ::sem_trywait(static_cast<sem_t*>(m_processSemaphore));
        if (result == 0)
        {
            return SemaphoreResult(SEMAPHORE_OK, true);
        }
        const int32_t error = errno;
        if (error != EINTR && error != EAGAIN)
        {
            return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, error);
        }
        if (timeout < 0)
        {
            continue;
        }
#endif
        const uint64_t elapsed = monotonicMilliseconds() - started;
        if (timeout >= 0 && elapsed >= static_cast<uint32_t>(timeout))
        {
            return SemaphoreResult(SEMAPHORE_TIMEOUT);
        }
        // 沿用移动端1ms/桌面POSIX有限等待2ms补查，EINTR也不重置总预算。
#ifdef SEMAPHORE_SHARED_COUNTER
        const uint32_t maximumSlice = 1;
#else
        const uint32_t maximumSlice = 2;
#endif
        const uint32_t slice = timeout < 0
            ? maximumSlice
            : static_cast<uint32_t>((std::min)(uint64_t(maximumSlice), static_cast<uint32_t>(timeout) - elapsed));
#ifdef _WIN32
        ::Sleep(slice);
#else
        struct timespec pause = {};
        pause.tv_nsec = static_cast<long>(slice) * 1000000;
        if (::nanosleep(&pause, nullptr) != 0 && errno != EINTR)
        {
            return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
        }
#endif
    }
#endif
#endif
}

uint32_t Semaphore::cleanupFailures()
{
    return SemaphoreWake::cleanupFailures();
}

SemaphoreResult Semaphore::prepareSignalAndWait()
{
    return m_wake.prepareSignalAndWait();
}

SemaphoreSignalWaitResult Semaphore::signalAndWaitResult(Semaphore& waitOn)
{
    return signalAndWaitLocal(waitOn, true, 0);
}

SemaphoreSignalWaitResult Semaphore::signalAndWaitResult(Semaphore& waitOn, uint32_t timeout)
{
    return signalAndWaitLocal(waitOn, false, timeout);
}

SemaphoreSignalWaitResult Semaphore::processSignalAndWaitResult(Semaphore& waitOn, int32_t timeout)
{
    if (timeout < -1)
    {
        return SemaphoreSignalWaitResult(SEMAPHORE_INVALID);
    }
#ifdef __EMSCRIPTEN__
    return SemaphoreSignalWaitResult(SEMAPHORE_UNSUPPORTED);
#else
    // 两端同时预检，目标未打开时不能先把请求发出去。
    if (m_processSemaphore == nullptr || waitOn.m_processSemaphore == nullptr)
    {
        return SemaphoreSignalWaitResult(SEMAPHORE_NOT_OPEN);
    }
#if defined(_WIN32) && !defined(SEMAPHORE_SHARED_COUNTER)
    DWORD waited = ::SignalObjectAndWait(
        m_processSemaphore, waitOn.m_processSemaphore, timeout < 0 ? INFINITE : static_cast<DWORD>(timeout), FALSE);
    DWORD error = waited == WAIT_FAILED ? ::GetLastError() : 0;
    if (waited == WAIT_OBJECT_0)
    {
        return SemaphoreSignalWaitResult(SEMAPHORE_OK, SEMAPHORE_COMMIT_DONE, SEMAPHORE_COMMIT_DONE);
    }
    if (waited == WAIT_TIMEOUT)
    {
        return SemaphoreSignalWaitResult(SEMAPHORE_TIMEOUT, SEMAPHORE_COMMIT_DONE, SEMAPHORE_COMMIT_NONE);
    }
    if (waited == WAIT_FAILED && error == ERROR_TOO_MANY_POSTS)
    {
        return SemaphoreSignalWaitResult(SEMAPHORE_OVERFLOW);
    }
    // 文档没有给其他失败的阶段承诺；不得伪造“未发出，可重试”，也不自动重复调用。
    return SemaphoreSignalWaitResult(SEMAPHORE_SYSTEM_ERROR, SEMAPHORE_COMMIT_UNKNOWN, SEMAPHORE_COMMIT_UNKNOWN,
        static_cast<int32_t>(error != 0 ? error : ERROR_GEN_FAILURE));
#else
    const uint64_t started = timeout < 0 ? 0 : monotonicMilliseconds();
    const SemaphoreResult sent = processSignalResult();
    if (sent.status != SEMAPHORE_OK)
    {
        return SemaphoreSignalWaitResult(sent.status, sent.committed ? SEMAPHORE_COMMIT_DONE : SEMAPHORE_COMMIT_NONE,
            SEMAPHORE_COMMIT_NONE, sent.nativeError);
    }
    int32_t remaining = timeout;
    if (timeout >= 0)
    {
        const uint64_t elapsed = monotonicMilliseconds() - started;
        remaining = elapsed >= static_cast<uint32_t>(timeout) ? 0 : timeout - static_cast<int32_t>(elapsed);
    }
    const SemaphoreResult received = waitOn.processWaitResult(remaining);
    return SemaphoreSignalWaitResult(received.status, SEMAPHORE_COMMIT_DONE,
        received.committed ? SEMAPHORE_COMMIT_DONE : SEMAPHORE_COMMIT_NONE, received.nativeError);
#endif
#endif
}

SemaphoreResult Semaphore::prepareEventPool(SemaphoreEventPool*& pool)
{
    pool = m_eventPool.load();
    if (pool != nullptr)
    {
        return SemaphoreResult();
    }
    try
    {
        std::unique_ptr<SemaphoreEventPool> candidate(new SemaphoreEventPool(SemaphorePoolConfig()));
        if (m_eventPool.compare_exchange_strong(pool, candidate.get()))
        {
            pool = candidate.release();
        }
        return SemaphoreResult();
    }
    catch (const std::bad_alloc&)
    {
        return SemaphoreResult(SEMAPHORE_NO_MEMORY);
    }
    catch (const std::system_error& error)
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, error.code().value());
    }
}

SemaphoreResult Semaphore::waitLocal(bool isEvent, bool infinite, uint32_t timeout)
{
    if (!isEvent)
    {
        if (consumeCount())
        {
            return SemaphoreResult(SEMAPHORE_OK, true);
        }
        if (!infinite && timeout == 0)
        {
            return consumeCount() ? SemaphoreResult(SEMAPHORE_OK, true) : SemaphoreResult(SEMAPHORE_TIMEOUT);
        }
    }
    const uint64_t started = infinite ? 0 : monotonicMilliseconds();
    SemaphoreEventPool* pool = nullptr;
    SemaphoreEventRecord* record = nullptr;
    if (isEvent)
    {
        if (!SemaphoreWake::canBlock())
        {
            return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
        }
        const SemaphoreResult prepared = prepareEventPool(pool);
        if (prepared.status != SEMAPHORE_OK)
        {
            return prepared;
        }
        const SemaphoreResult registered = pool->begin(record);
        if (registered.status != SEMAPHORE_OK)
        {
            return registered;
        }
    }
    // 独立记录已有登记和引用，无需再争用公共睡眠计数或唤醒无关计数等待者
    const bool direct = isEvent && m_wake.canWaitAddress();
    SemaphoreResult result = direct ? SemaphoreResult() : m_wake.enterWait();
    if (result.status != SEMAPHORE_OK)
    {
        if (record != nullptr)
        {
            result.committed = pool->finish(record);
        }
        return result;
    }
    for (;;)
    {
        const uint32_t expected = direct ? 0 : m_wake.snapshot();
        if (isEvent ? record->m_state.load() == 1 : consumeCount())
        {
            result = SemaphoreResult(SEMAPHORE_OK, true);
            break;
        }
        const uint64_t elapsed = infinite ? 0 : monotonicMilliseconds() - started;
        if (!infinite && elapsed >= timeout)
        {
            // 事件结束CAS与通知CAS竞争；计数最终探测不抹掉迟到许可。
            const bool received = !isEvent && consumeCount();
            result = received ? SemaphoreResult(SEMAPHORE_OK, true) : SemaphoreResult(SEMAPHORE_TIMEOUT);
            break;
        }
        const uint32_t slice = infinite ? 32 : static_cast<uint32_t>((std::min)(uint64_t(32), timeout - elapsed));
        SemaphoreResult slept = m_wake.sleep(expected, slice, direct ? &record->m_state : nullptr);
        if (slept.status != SEMAPHORE_OK && slept.status != SEMAPHORE_TIMEOUT)
        {
            result = slept;
            break;
        }
    }
    if (record != nullptr)
    {
        result.committed = pool->finish(record);
        if (result.committed && result.status == SEMAPHORE_TIMEOUT)
        {
            result.status = SEMAPHORE_OK;
        }
    }
    if (!direct)
    {
        m_wake.leaveWait();
    }
    return result;
}

SemaphoreSignalWaitResult Semaphore::signalAndWaitLocal(Semaphore& waitOn, bool infinite, uint32_t timeout)
{
    const uint64_t started = infinite ? 0 : monotonicMilliseconds();
    if (!m_wake.canSignalAndWait(waitOn.m_wake))
    {
        const SemaphoreResult sent = trySignal();
        if (sent.status != SEMAPHORE_OK)
        {
            return SemaphoreSignalWaitResult(sent.status,
                sent.committed ? SEMAPHORE_COMMIT_DONE : SEMAPHORE_COMMIT_NONE, SEMAPHORE_COMMIT_NONE,
                sent.nativeError);
        }
    }
    else
    {
        // 目标先登记、快照，发布后再查真实许可；双方发布/重查均保持seq_cst。
        const SemaphoreResult entered = waitOn.m_wake.enterWait();
        if (entered.status != SEMAPHORE_OK)
        {
            return SemaphoreSignalWaitResult(
                entered.status, SEMAPHORE_COMMIT_NONE, SEMAPHORE_COMMIT_NONE, entered.nativeError);
        }
        const uint32_t expected = waitOn.m_wake.snapshot();
        uint64_t count = m_count.load();
        do
        {
            if (count == UINT64_MAX)
            {
                waitOn.m_wake.leaveWait();
                return SemaphoreSignalWaitResult(SEMAPHORE_OVERFLOW);
            }
        } while (!m_count.compare_exchange_weak(count, count + 1));
        bool received = waitOn.consumeCount();
        const uint64_t elapsed = infinite ? 0 : monotonicMilliseconds() - started;
        if (received || (!infinite && elapsed >= timeout))
        {
            const SemaphoreResult woke = m_wake.notify();
            if (!received)
            {
                received = waitOn.consumeCount();
            }
            waitOn.m_wake.leaveWait();
            return SemaphoreSignalWaitResult(
                woke.status != SEMAPHORE_OK ? woke.status : (received ? SEMAPHORE_OK : SEMAPHORE_TIMEOUT),
                SEMAPHORE_COMMIT_DONE, received ? SEMAPHORE_COMMIT_DONE : SEMAPHORE_COMMIT_NONE, woke.nativeError);
        }
        const uint32_t slice = infinite ? 32 : static_cast<uint32_t>((std::min)(uint64_t(32), timeout - elapsed));
        const SemaphoreResult slept = m_wake.signalAndSleep(waitOn.m_wake, expected, slice);
        waitOn.m_wake.leaveWait();
        if (slept.status != SEMAPHORE_OK && slept.status != SEMAPHORE_TIMEOUT)
        {
            return SemaphoreSignalWaitResult(
                slept.status, SEMAPHORE_COMMIT_DONE, SEMAPHORE_COMMIT_NONE, slept.nativeError);
        }
    }
    // 原生返回只是提示，仍由目标64位计数CAS决定是否取得许可；后续等待不重复发送。
    uint32_t remaining = timeout;
    if (!infinite)
    {
        const uint64_t elapsed = monotonicMilliseconds() - started;
        remaining = elapsed >= timeout ? 0 : timeout - static_cast<uint32_t>(elapsed);
    }
    const SemaphoreResult received = infinite ? waitOn.waitResult() : waitOn.waitResult(remaining);
    return SemaphoreSignalWaitResult(received.status, SEMAPHORE_COMMIT_DONE,
        received.committed ? SEMAPHORE_COMMIT_DONE : SEMAPHORE_COMMIT_NONE, received.nativeError);
}

bool Semaphore::consumeCount()
{
    uint64_t count = m_count.load();
    while (count != 0)
    {
        if (m_count.compare_exchange_weak(count, count - 1))
        {
            return true;
        }
    }
    return false;
}

uint64_t Semaphore::monotonicMilliseconds()
{
#ifdef _WIN32
    LARGE_INTEGER ticks;
    LARGE_INTEGER frequency;
    if (::QueryPerformanceCounter(&ticks) && ::QueryPerformanceFrequency(&frequency) && frequency.QuadPart > 0)
    {
        return static_cast<uint64_t>(static_cast<double>(ticks.QuadPart) * 1000.0 / frequency.QuadPart);
    }
    return ::GetTickCount64();
#else
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
#endif
}

void Semaphore::checkLocalResult(const SemaphoreResult& result)
{
    if (result.status == SEMAPHORE_OVERFLOW)
    {
        throw std::overflow_error("Semaphore notification counter exhausted; not committed");
    }
    if (result.status != SEMAPHORE_OK && result.status != SEMAPHORE_TIMEOUT)
    {
        if (result.status == SEMAPHORE_UNSUPPORTED)
        {
            throw std::system_error(std::make_error_code(std::errc::not_supported));
        }
        if (result.status == SEMAPHORE_NO_MEMORY)
        {
            throw std::bad_alloc();
        }
        if (result.nativeError == 0)
        {
            throw std::system_error(
                std::make_error_code(result.status == SEMAPHORE_CAPACITY ? std::errc::resource_unavailable_try_again
                                                                         : std::errc::invalid_argument),
                "Semaphore operation rejected");
        }
        throw std::system_error(result.nativeError, std::system_category(),
            result.committed ? "Semaphore notification committed; wake failed; do not publish again"
                             : "Semaphore operation failed");
    }
}

bool Semaphore::validProcessName(const std::string& name)
{
    if (name.empty() || name.find('\0') != std::string::npos || name.find('/') != std::string::npos)
    {
        return false;
    }
#ifdef SEMAPHORE_SHARED_COUNTER
    // 保留原移动协议的30字节约束；桌面原生名称交由系统核对真实长度/命名空间。
    if (name.size() > 30)
    {
        return false;
    }
#endif
    return true;
}

// 调用示例：已在Windows x64/VS2013运行通过，以下全部注释，不参与正式库编译
// 学习时可复制此main和所需include到独立调用工程；正式测试仍位于SemaphoreTest工程
// #include "Semaphore.h"
// #include <cstdio>
// #include <cstdlib>
// #include <stdexcept>
// #include <thread>
// #ifdef _WIN32
// #include <Windows.h>
// #else
// #include <unistd.h>
// #include <time.h>
// #include <cerrno>
// #endif
//
// /** Semaphore公开接口的调用演示，不采样、不计时、不测吞吐
//     同线程按步骤展示生产/消费两端；真实业务可把相应步骤放到不同线程或进程
//     事件部分由独立线程周期发布状态，结束前先停止并join，再销毁Semaphore
//     Android/iOS须先由宿主设置SEMAPHORE_DEMO_DIRECTORY为已存在的私有共享目录
//     Wasm主线程不能运行以下阻塞/命名IPC流程；其能力示例见SemaphoreTest工程
// @return 0演示完成，1操作失败，77当前宿主不适用
// */
// int32_t main()
// {
// #ifdef __EMSCRIPTEN__
//     std::printf("This native blocking demo is not applicable to a Wasm main thread.\n");
//     return 77;
// #else
//     const uint32_t cleanupBefore = Semaphore::cleanupFailures();
//     try
//     {
//         /** 记录预期未满足的步骤，由main统一报告并正常退出
//         @param [in] condition 当前步骤是否成立
//         @param [in] step 步骤名称
//         */
//         const auto expect = [](bool condition, const char* step)
//         {
//             if (!condition)
//             {
//                 throw std::runtime_error(step);
//             }
//         };
//         /** 展示普通Result；失败只退出示例，不重发可能已提交的通知
//         @param [in] step 步骤名称
//         @param [in] result 当前操作的状态和提交信息
//         */
//         const auto showResult = [](const char* step, const SemaphoreResult& result)
//         {
//             std::printf("%s: status=%d committed=%d native=%d\n", step,
//                 static_cast<int32_t>(result.status), result.committed ? 1 : 0, result.nativeError);
//             if (result.status != SEMAPHORE_OK)
//             {
//                 throw std::runtime_error(step);
//             }
//         };
//         /** 分别展示组合操作的发送和消费阶段，不能因等待失败重发整个操作
//         @param [in] step 步骤名称
//         @param [in] result 本次组合结果；UNKNOWN须由业务代次恢复
//         */
//         const auto showCombined = [](const char* step, const SemaphoreSignalWaitResult& result)
//         {
//             std::printf("%s: status=%d signal=%d wait=%d native=%d\n", step,
//                 static_cast<int32_t>(result.status), static_cast<int32_t>(result.signalState),
//                 static_cast<int32_t>(result.waitState), result.nativeError);
//             if (result.status != SEMAPHORE_OK || result.signalState != SEMAPHORE_COMMIT_DONE ||
//                 result.waitState != SEMAPHORE_COMMIT_DONE)
//             {
//                 throw std::runtime_error(step);
//             }
//         };
//
//         // 一、本地任务计数：通知可提前到达，每次成功等待恰好消费一个许可
//         {
//             Semaphore tasks;
//             tasks.signal();
//             tasks.wait();
//             tasks.signal();
//             expect(tasks.wait(1000), "count timed wait");
//             showResult("count publish", tasks.trySignal());
//             showResult("count infinite wait", tasks.waitResult());
//             showResult("count publish again", tasks.trySignal());
//             showResult("count timed result", tasks.waitResult(1000));
//
//             // reset只用于生产者、消费者均已停止的静止阶段，不作为并发取消接口
//             tasks.signal();
//             tasks.reset();
//             expect(tasks.waitResult(0).status == SEMAPHORE_TIMEOUT, "reset clears pending count");
//         }
//
//         // 二、周期状态事件：事件不积存，唤醒后读取业务版本，不能把提示当成任务数量
//         {
//             Semaphore changed;
//             SemaphorePoolConfig config;
//             config.initialCapacity = 2;
//             config.maxCapacity = 8;
//             /** 提供可选缩容策略；维护仍遵守初始容量下限与安全回收条件
//             @param [in] context 本次维护的容量快照
//             @return 最多回收4条、连续空闲至少1000毫秒的记录
//             */
//             config.shrink = [](const SemaphoreShrinkContext& context)
//             {
//                 SemaphoreShrinkDecision decision = {};
//                 decision.shrinkCount = context.currentCapacity > context.initialCapacity ? 4 : 0;
//                 decision.idleMilliseconds = 1000;
//                 return decision;
//             };
//             showResult("configure optional event pool", changed.configureEventPool(config));
//             std::atomic<bool> stop(false);
//             std::atomic<bool> publishFailed(false);
//             std::atomic<uint32_t> version(0);
//             /** 周期发布新的业务版本；轮流展示单个提示、广播及Result版本
//                 周期更新允许错过中间版本；休眠让出CPU，不用持续轮询等待登记
//             */
//             std::thread publisher([&]()
//             {
//                 while (!stop.load())
//                 {
//                     const uint32_t current = version.fetch_add(1) + 1;
//                     try
//                     {
//                         if (current % 3 == 1)
//                         {
//                             changed.event();
//                         }
//                         else if (current % 3 == 2)
//                         {
//                             // 有多个订阅者时，eventAll提示本次已登记的全部等待者
//                             changed.eventAll();
//                         }
//                         else
//                         {
//                             const SemaphoreResult result = changed.tryEvent();
//                             if (result.status != SEMAPHORE_OK)
//                             {
//                                 publishFailed.store(true);
//                             }
//                             // committed=false表示此刻没有合格等待者，不是积存了一次事件
//                         }
//                     }
//                     catch (...)
//                     {
//                         publishFailed.store(true);
//                     }
// #ifdef _WIN32
//                     ::Sleep(20);
// #else
//                     struct timespec delay = {0, 20000000};
//                     if (::nanosleep(&delay, nullptr) != 0 && errno != EINTR)
//                     {
//                         publishFailed.store(true);
//                     }
// #endif
//                 }
//             });
//             try
//             {
//                 // 无参数版本无限等待，要求发布者仍存活；有限版本便于检查退出/业务条件
//                 changed.eventWait();
//                 std::printf("event observed version=%u\n", static_cast<unsigned>(version.load()));
//                 showResult("event infinite result", changed.eventWaitResult());
//                 expect(changed.eventWait(1000), "event timed wait");
//                 showResult("event timed result", changed.eventWaitResult(1000));
//             }
//             catch (...)
//             {
//                 stop.store(true);
//                 publisher.join();
//                 throw;
//             }
//             stop.store(true);
//             publisher.join();
//             expect(!publishFailed.load(), "event publisher");
//             const SemaphorePoolStats stats = changed.eventPoolStatistics();
//             std::printf("event pool: capacity=%u free=%u\n",
//                 static_cast<unsigned>(stats.capacity), static_cast<unsigned>(stats.freeCapacity));
//             size_t released = 0;
//             showResult("optional event pool maintenance", changed.maintainEventPool(released));
//             // 本例没有超过初始容量，released为0也正常；普通事件使用不必配置或维护池
//             std::printf("event pool released=%u\n", static_cast<unsigned>(released));
//         }
//
//         // 三、本地生产/消费组合：发布已完成工作，然后取得下一空槽
//         // 假设当前生产者已持有一个槽位，另有一个空槽；这里只演示通知，不承载真实队列数据
//         {
//             Semaphore jobs;
//             Semaphore freeSlots;
//             showResult("prepare jobs", jobs.prepareSignalAndWait());
//             showResult("prepare free slots", freeSlots.prepareSignalAndWait());
//             freeSlots.signal();
//             showCombined("publish job and acquire next slot", jobs.signalAndWaitResult(freeSlots));
//             jobs.wait();
//             freeSlots.signal(); // 消费者处理上一工作后归还槽位
//             showCombined("publish job and acquire slot with budget", jobs.signalAndWaitResult(freeSlots, 1000));
//             jobs.wait();
//             // 真实冷通知后立即等待可优先使用组合接口；持续热往返不宜机械切换准备模式
//         }
//
//         // 四、命名通知：同一程序按顺序模拟两个进程端，真实两端必须约定相同目录和名称
// #if defined(SEMAPHORE_SHARED_COUNTER) && !defined(_WIN32)
//         const char* directory = std::getenv("SEMAPHORE_DEMO_DIRECTORY");
//         expect(directory != nullptr, "set an existing owned SEMAPHORE_DEMO_DIRECTORY");
//         expect(Semaphore::setProcessDirectory(directory), "configure mobile shared directory");
// #else
//         expect(Semaphore::setProcessDirectory(""), "desktop native notification directory");
// #endif
//         {
//             char name[32];
// #ifdef _WIN32
//             std::sprintf(name, "SemDemo%lu", static_cast<unsigned long>(::GetCurrentProcessId()));
// #else
//             std::sprintf(name, "SemDemo%lu", static_cast<unsigned long>(::getpid()));
// #endif
//             const std::string jobsName = std::string(name) + "j";
//             const std::string slotsName = std::string(name) + "s";
//             Semaphore jobsOwner, slotsOwner, jobsReader, slotsReader, probe;
//
//             // 旧void包装便于兼容；新调用方优先Result接口，以便观察失败原因
//             jobsOwner.createProcessSemaphore(jobsName, 4);
//             showResult("create named slots", slotsOwner.createProcessResult(slotsName, 4));
//             expect(probe.tryOpenProcessSemaphore(jobsName), "probe existing named jobs");
//             jobsReader.openProcessSemaphore(jobsName);
//             showResult("open named slots", slotsReader.openProcessResult(slotsName));
//
//             jobsOwner.processSignal();
//             jobsReader.processWait();
//             expect(jobsOwner.tryProcessSignal() == 0, "publish named job");
//             expect(jobsReader.processWait(1000), "consume named job with budget");
//             showResult("publish named job result", jobsOwner.processSignalResult());
//             showResult("consume named job result", jobsReader.processWaitResult(1000));
//
//             // 空槽通知可以先到达；此组合只合并发布工作和等待空槽，不是两阶段原子事务
//             showResult("return named free slot", slotsOwner.processSignalResult());
//             showCombined("named publish and acquire slot", jobsOwner.processSignalAndWaitResult(slotsReader, 1000));
//             showResult("consume combined named job", jobsReader.processWaitResult(1000));
//
//             const bool unlinked = jobsOwner.unlinkProcessName();
// #ifdef _WIN32
//             expect(!unlinked, "Windows names are released with kernel references");
// #else
//             expect(unlinked, "owner unlinks its POSIX name");
// #endif
//             probe.closeProcessSemaphore();
//             jobsReader.closeProcessSemaphore();
//             showResult("close named reader", slotsReader.closeProcessResult());
//             showResult("close named owner", jobsOwner.closeProcessResult());
//             slotsOwner.closeProcessSemaphore();
//             // 所有调用结束后离开作用域，由析构兜底释放；不手动调用析构函数
//         }
//
//         // 可选诊断适用于两组接口；读取计数不执行资源清理，也不是其他接口的调用前提
//         const uint32_t cleanupAfter = Semaphore::cleanupFailures();
//         std::printf("cleanup failures: before=%u after=%u\n",
//             static_cast<unsigned>(cleanupBefore), static_cast<unsigned>(cleanupAfter));
//         expect(cleanupAfter == cleanupBefore, "resource cleanup");
//         std::printf("Semaphore public API demo completed.\n");
//         return 0;
//     }
//     catch (const std::exception& error)
//     {
//         std::fprintf(stderr, "Semaphore demo failed: %s; cleanup failures=%u\n", error.what(),
//             static_cast<unsigned>(Semaphore::cleanupFailures()));
//     }
//     catch (...)
//     {
//         std::fprintf(stderr, "Semaphore demo failed with an unknown exception; cleanup failures=%u\n",
//             static_cast<unsigned>(Semaphore::cleanupFailures()));
//     }
//     return 1;
// #endif
// }
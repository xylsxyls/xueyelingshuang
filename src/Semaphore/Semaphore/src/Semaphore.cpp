#include "Semaphore.h"
#include "SemaphoreProcessMemory.h"
#include "SemaphoreDeadline.h"
#include <system_error>
#include <new>
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
    const SemaphoreResult result = trySignal();
    if (result.status != SEMAPHORE_OK)
    {
        checkLocalResult(result);
    }
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
    return waitCount(true, 0);
}

SemaphoreResult Semaphore::waitResult(uint32_t timeout)
{
    return waitCount(false, timeout);
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
    return waitEvent(true, 0);
}

SemaphoreResult Semaphore::eventWaitResult(int32_t timeout)
{
    if (timeout <= 0)
    {
        return SemaphoreResult(SEMAPHORE_TIMEOUT);
    }
    return waitEvent(false, static_cast<uint32_t>(timeout));
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
    return signalCount > 0 ? connectProcess(name, signalCount) : SemaphoreResult(SEMAPHORE_INVALID);
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
    // 保持旧流程：已打开的对象不因再次open改变其连接和所有权。
    return m_processSemaphore != nullptr ? SemaphoreResult() : connectProcess(name, 0);
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
    const SemaphoreDeadline deadline(timeout < 0, timeout < 0 ? 0 : static_cast<uint32_t>(timeout));
    return waitProcess(deadline, timeout < 0);
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
    const SemaphoreDeadline deadline(timeout < 0, timeout < 0 ? 0 : static_cast<uint32_t>(timeout));
    const SemaphoreResult sent = processSignalResult();
    if (sent.status != SEMAPHORE_OK)
    {
        return SemaphoreSignalWaitResult(sent.status, sent.committed ? SEMAPHORE_COMMIT_DONE : SEMAPHORE_COMMIT_NONE,
            SEMAPHORE_COMMIT_NONE, sent.nativeError);
    }
    const SemaphoreResult received = waitOn.waitProcess(deadline, timeout < 0);
    return SemaphoreSignalWaitResult(received.status, SEMAPHORE_COMMIT_DONE,
        received.committed ? SEMAPHORE_COMMIT_DONE : SEMAPHORE_COMMIT_NONE, received.nativeError);
#endif
#endif
}

SemaphoreResult Semaphore::connectProcess(const std::string& name, int32_t signalCount)
{
    if (m_processSemaphore != nullptr || m_processMemory || m_processCreated || !validProcessName(name))
    {
        return SemaphoreResult(SEMAPHORE_INVALID);
    }
    try
    {
#ifdef SEMAPHORE_SHARED_COUNTER
        std::unique_ptr<SemaphoreProcessMemory> candidate(new (std::nothrow) SemaphoreProcessMemory);
        if (!candidate)
        {
            return SemaphoreResult(SEMAPHORE_NO_MEMORY);
        }
        const SemaphoreResult result = candidate->open(name, signalCount);
        if (result.status != SEMAPHORE_OK)
        {
            return result;
        }
        m_processSemaphore = candidate->m_words;
        m_processMemory.swap(candidate);
#elif defined(_WIN32)
        m_processSemaphore = signalCount > 0
            ? ::CreateSemaphoreA(nullptr, 0, signalCount, name.c_str())
            : ::OpenSemaphoreA(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, name.c_str());
        if (m_processSemaphore == nullptr)
        {
            return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, static_cast<int32_t>(::GetLastError()));
        }
#elif defined(__EMSCRIPTEN__)
        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
#else
        // 所有可能抛异常的字符串工作先于取得原生引用。
        m_processName = "/" + name;
        sem_t* value = signalCount > 0 ? ::sem_open(m_processName.c_str(), O_CREAT | O_EXCL, 0600, 0)
                                      : ::sem_open(m_processName.c_str(), 0);
        m_processCreated = signalCount > 0 && value != SEM_FAILED;
        if (signalCount > 0 && value == SEM_FAILED && errno == EEXIST)
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

SemaphoreResult Semaphore::waitProcess(const SemaphoreDeadline& deadline, bool infinite)
{
#if defined(__EMSCRIPTEN__) || (defined(_WIN32) && !defined(SEMAPHORE_SHARED_COUNTER))
    (void)deadline;
    (void)infinite;
    return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
#else
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
        const int32_t result = infinite ? ::sem_wait(static_cast<sem_t*>(m_processSemaphore))
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
        if (infinite)
        {
            continue;
        }
#endif
        // 沿用移动端1ms/桌面POSIX有限等待2ms补查，EINTR也不重置总预算。
#ifdef SEMAPHORE_SHARED_COUNTER
        const uint32_t maximumSlice = 1;
#else
        const uint32_t maximumSlice = 2;
#endif
        const uint32_t slice = deadline.remaining(maximumSlice);
        if (slice == 0)
        {
            return SemaphoreResult(SEMAPHORE_TIMEOUT);
        }
        struct timespec pause = {};
        pause.tv_nsec = static_cast<long>(slice) * 1000000;
        if (::nanosleep(&pause, nullptr) != 0 && errno != EINTR)
        {
            return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
        }
    }
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

SemaphoreResult Semaphore::waitCount(bool infinite, uint32_t timeout)
{
    if (consumeCount())
    {
        return SemaphoreResult(SEMAPHORE_OK, true);
    }
    if (!infinite && timeout == 0)
    {
        // 最终探测与发布竞争，既不清空计数，也不把迟到的许可丢弃。
        return consumeCount() ? SemaphoreResult(SEMAPHORE_OK, true) : SemaphoreResult(SEMAPHORE_TIMEOUT);
    }
    const SemaphoreDeadline deadline(infinite, timeout);
    return waitCountRegistered(deadline);
}

SemaphoreResult Semaphore::waitCountRegistered(const SemaphoreDeadline& deadline)
{
    SemaphoreResult result = m_wake.enterWait();
    if (result.status != SEMAPHORE_OK)
    {
        return result;
    }
    for (;;)
    {
        const uint32_t expected = m_wake.snapshot();
        if (consumeCount())
        {
            result = SemaphoreResult(SEMAPHORE_OK, true);
            break;
        }
        const uint32_t slice = deadline.remaining(32);
        if (slice == 0)
        {
            result = consumeCount() ? SemaphoreResult(SEMAPHORE_OK, true) : SemaphoreResult(SEMAPHORE_TIMEOUT);
            break;
        }
        result = m_wake.sleep(expected, slice);
        if (result.status != SEMAPHORE_OK && result.status != SEMAPHORE_TIMEOUT)
        {
            break;
        }
    }
    m_wake.leaveWait();
    return result;
}

SemaphoreResult Semaphore::waitEvent(bool infinite, uint32_t timeout)
{
    const SemaphoreDeadline deadline(infinite, timeout);
    if (!SemaphoreWake::canBlock())
    {
        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
    }
    SemaphoreEventPool* pool = nullptr;
    SemaphoreResult result = prepareEventPool(pool);
    if (result.status != SEMAPHORE_OK)
    {
        return result;
    }
    SemaphoreEventRecord* record = nullptr;
    result = pool->begin(record);
    if (result.status != SEMAPHORE_OK)
    {
        return result;
    }
    // 地址等待直接使用已登记记录；其他后端还需登记公共系统提示。
    const bool direct = m_wake.canWaitAddress();
    if (!direct)
    {
        result = m_wake.enterWait();
        if (result.status != SEMAPHORE_OK)
        {
            result.committed = pool->finish(record);
            return result;
        }
    }
    for (;;)
    {
        const uint32_t expected = direct ? 0 : m_wake.snapshot();
        if (record->m_state.load() == 1)
        {
            result = SemaphoreResult(SEMAPHORE_OK, true);
            break;
        }
        const uint32_t slice = deadline.remaining(32);
        if (slice == 0)
        {
            result = SemaphoreResult(SEMAPHORE_TIMEOUT);
            break;
        }
        result = m_wake.sleep(expected, slice, direct ? &record->m_state : nullptr);
        if (result.status != SEMAPHORE_OK && result.status != SEMAPHORE_TIMEOUT)
        {
            break;
        }
    }
    // finish的CAS决定通知与超时谁先完成；发生系统错误仍如实保留已提交状态。
    result.committed = pool->finish(record);
    if (result.committed && result.status == SEMAPHORE_TIMEOUT)
    {
        result.status = SEMAPHORE_OK;
    }
    if (!direct)
    {
        m_wake.leaveWait();
    }
    return result;
}

SemaphoreSignalWaitResult Semaphore::signalAndWaitLocal(Semaphore& waitOn, bool infinite, uint32_t timeout)
{
    const SemaphoreDeadline deadline(infinite, timeout);
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
        const uint32_t slice = deadline.remaining(32);
        if (received || slice == 0)
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
        const SemaphoreResult slept = m_wake.signalAndSleep(waitOn.m_wake, expected, slice);
        waitOn.m_wake.leaveWait();
        if (slept.status != SEMAPHORE_OK && slept.status != SEMAPHORE_TIMEOUT)
        {
            return SemaphoreSignalWaitResult(
                slept.status, SEMAPHORE_COMMIT_DONE, SEMAPHORE_COMMIT_NONE, slept.nativeError);
        }
    }
    // 原生返回只是提示，仍由目标64位计数CAS决定是否取得许可；后续等待不重复发送。
    const SemaphoreResult received = waitOn.consumeCount()
        ? SemaphoreResult(SEMAPHORE_OK, true)
        : (!infinite && deadline.remaining() == 0 ? SemaphoreResult(SEMAPHORE_TIMEOUT)
                                                : waitOn.waitCountRegistered(deadline));
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
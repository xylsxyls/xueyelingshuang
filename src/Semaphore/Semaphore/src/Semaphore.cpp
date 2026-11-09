#include "Semaphore.h"
#include "SemaphoreProcessMemory.h"
#include "SemaphoreDeadline.h"
#include "SemaphoreCount.h"
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

// 编译期核实当前工具链的位保持整数转换；不生成运行期断言或终止路径。
static_assert(static_cast<int64_t>(UINT64_MAX) == -1 &&
    static_cast<int64_t>(uint64_t(1) << 63) == (-INT64_MAX - 1), "Semaphore requires bit-preserving integer conversion");

Semaphore::Semaphore() :
m_count(0),
m_eventPool(nullptr),
m_countPool(nullptr),
m_processSemaphore(nullptr),
m_processCreated(false)
{
    if (!m_count.is_lock_free() || !m_eventPool.is_lock_free() || !m_countPool.is_lock_free())
    {
        throw std::system_error(std::make_error_code(std::errc::not_supported));
    }
}

Semaphore::~Semaphore()
{
    delete m_eventPool.load();
    SemaphoreCount* countPool = m_countPool.load();
    if (countPool != nullptr)
    {
        countPool->clear(m_count);
        delete countPool;
    }
    if ((m_processSemaphore != nullptr || m_processMemory || m_processCreated) &&
        closeProcessResult().status != SEMAPHORE_OK)
    {
        SemaphoreWake::recordCleanupFailure();
    }
}

void Semaphore::signal()
{
    if (!incrementCount())
    {
        checkLocalResult(publishCount(true));
        return;
    }
    SemaphoreResult result = m_wake.notify();
    if (result.status != SEMAPHORE_OK)
    {
        // 只在错误分支补上已提交状态，兼容入口的异常不能误导调用方重复发布。
        result.committed = true;
        checkLocalResult(result);
    }
}

SemaphoreResult Semaphore::trySignal()
{
    if (incrementCount())
    {
        SemaphoreResult result = m_wake.notify();
        result.committed = true;
        return result;
    }
    return publishCount(true);
}

SemaphoreResult Semaphore::publishCount(bool notify)
{
    if (incrementCount())
    {
        SemaphoreResult result = notify ? m_wake.notify() : SemaphoreResult();
        result.committed = true;
        return result;
    }
    SemaphoreCount* pool = nullptr;
    const SemaphoreResult prepared = prepareCountPool(pool);
    return prepared.status == SEMAPHORE_OK ? pool->signal(m_count, m_wake, notify) : prepared;
}

bool Semaphore::incrementCount()
{
    uint64_t count = m_count.load();
    for (;;)
    {
        const uint64_t next = count + 1;
        // 记录地址非空且至少4字节对齐，不会编码为UINT64_MAX；这里只修改普通积存数。
        if (static_cast<int64_t>(next) < 0)
        {
            return false;
        }
        if (m_count.compare_exchange_weak(count, next))
        {
            return true;
        }
    }
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
    SemaphoreCount* pool = m_countPool.load();
    if (pool != nullptr)
    {
        pool->clear(m_count);
    }
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

SemaphoreResult Semaphore::configureCountPool(const SemaphorePoolConfig& config)
{
    if (m_count.load() != 0)
    {
        return SemaphoreResult(SEMAPHORE_INVALID);
    }
    try
    {
        std::unique_ptr<SemaphoreCount> candidate(new SemaphoreCount(config));
        delete m_countPool.exchange(candidate.release());
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

SemaphoreResult Semaphore::maintainCountPool(size_t& released)
{
    SemaphoreCount* pool = m_countPool.load();
    released = 0;
    return pool != nullptr ? pool->maintain(released) : SemaphoreResult();
}

SemaphorePoolStats Semaphore::countPoolStatistics() const
{
    SemaphoreCount* pool = m_countPool.load();
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

SemaphoreResult Semaphore::prepareCountPool(SemaphoreCount*& pool)
{
    pool = m_countPool.load();
    if (pool != nullptr)
    {
        return SemaphoreResult();
    }
    try
    {
        std::unique_ptr<SemaphoreCount> candidate(new SemaphoreCount);
        if (m_countPool.compare_exchange_strong(pool, candidate.get()))
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
        const uint64_t state = m_count.load();
        if (state <= (UINT64_MAX >> 1) || (state & 1) == 0)
        {
            return consumeCount() ? SemaphoreResult(SEMAPHORE_OK, true) : SemaphoreResult(SEMAPHORE_TIMEOUT);
        }
        SemaphoreEventRecord* unused = nullptr;
        return m_countPool.load()->begin(m_count, unused, true);
    }
    const SemaphoreDeadline deadline(infinite, timeout);
    return waitCountRegistered(deadline);
}

SemaphoreResult Semaphore::waitCountRegistered(const SemaphoreDeadline& deadline)
{
    if (consumeCount())
    {
        return SemaphoreResult(SEMAPHORE_OK, true);
    }
    const uint64_t observed = m_count.load();
    if (observed > (UINT64_MAX >> 1) && (observed & 1) != 0)
    {
        SemaphoreEventRecord* unused = nullptr;
        const SemaphoreResult result = m_countPool.load()->begin(m_count, unused, true);
        if (result.status != SEMAPHORE_TIMEOUT)
        {
            return result;
        }
    }
    if (deadline.remaining() == 0)
    {
        return consumeCount() ? SemaphoreResult(SEMAPHORE_OK, true) : SemaphoreResult(SEMAPHORE_TIMEOUT);
    }
    if (!SemaphoreWake::canBlock())
    {
        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
    }
    bool priority = false;
    const SemaphoreResult competitive = waitCountCompetitive(deadline, priority);
    if (!priority)
    {
        return competitive;
    }
    if (deadline.remaining() == 0)
    {
        return consumeCount() ? SemaphoreResult(SEMAPHORE_OK, true) : SemaphoreResult(SEMAPHORE_TIMEOUT);
    }
    SemaphoreCount* pool = nullptr;
    SemaphoreResult result = prepareCountPool(pool);
    if (result.status != SEMAPHORE_OK)
    {
        return result;
    }
    SemaphoreEventRecord* record = nullptr;
    result = pool->begin(m_count, record, deadline.remaining() == 0);
    if (result.status != SEMAPHORE_OK || record == nullptr)
    {
        return result;
    }
    return waitCountRecord(record, deadline);
}

SemaphoreResult Semaphore::waitCountCompetitive(const SemaphoreDeadline& deadline, bool& priority)
{
    priority = false;
    SemaphoreResult result = m_wake.enterWait();
    if (result.status != SEMAPHORE_OK)
    {
        return result;
    }
    // 普通等待先复用共享系统提示；单次等待持续32ms未取得许可后再进入优先交付链。
    // 这是防止长期抢不到许可的升级时间，不是每个线程必须等待的时间或调度上限。
    const SemaphoreDeadline promotion(false, 32);
    for (;;)
    {
        const uint32_t expected = m_wake.snapshot();
        if (consumeCount())
        {
            result = SemaphoreResult(SEMAPHORE_OK, true);
            break;
        }
        const uint64_t state = m_count.load();
        const uint32_t allowance = promotion.remaining(32);
        if (allowance == 0 || (state > (UINT64_MAX >> 1) && (state & 1) != 0))
        {
            // 大计数也交给受读者保护的路径处理，不能在这里解引用记录地址。
            priority = true;
            result = SemaphoreResult();
            break;
        }
        const uint32_t slice = deadline.remaining(allowance);
        if (slice == 0)
        {
            result = SemaphoreResult(SEMAPHORE_TIMEOUT);
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

SemaphoreResult Semaphore::waitCountRecord(
    SemaphoreEventRecord* record, const SemaphoreDeadline& deadline, bool entered)
{
    SemaphoreCount* pool = m_countPool.load();
    SemaphoreResult result;
    const bool direct = m_wake.canWaitAddress();
    if (!direct && !entered)
    {
        result = m_wake.enterWait();
        if (result.status != SEMAPHORE_OK)
        {
            result.committed = pool->finish(m_count, record);
            return result;
        }
        entered = true;
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
    result.committed = pool->finish(m_count, record);
    if (result.committed && result.status == SEMAPHORE_TIMEOUT)
    {
        result.status = SEMAPHORE_OK;
    }
    if (entered)
    {
        m_wake.leaveWait();
    }
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
        const SemaphoreResult received = waitOn.waitCountRegistered(deadline);
        return SemaphoreSignalWaitResult(received.status, SEMAPHORE_COMMIT_DONE,
            received.committed ? SEMAPHORE_COMMIT_DONE : SEMAPHORE_COMMIT_NONE, received.nativeError);
    }
    // 系统等待先登记；业务许可仍在源发布之后才从目标消费。
    const SemaphoreResult entered = waitOn.m_wake.enterWait();
    if (entered.status != SEMAPHORE_OK)
    {
        return SemaphoreSignalWaitResult(
            entered.status, SEMAPHORE_COMMIT_NONE, SEMAPHORE_COMMIT_NONE, entered.nativeError);
    }
    const uint32_t expected = waitOn.m_wake.snapshot();
    const SemaphoreResult sent = publishCount(false);
    if (sent.status != SEMAPHORE_OK)
    {
        waitOn.m_wake.leaveWait();
        return SemaphoreSignalWaitResult(sent.status,
            sent.committed ? SEMAPHORE_COMMIT_DONE : SEMAPHORE_COMMIT_NONE, SEMAPHORE_COMMIT_NONE, sent.nativeError);
    }
    SemaphoreCount* pool = nullptr;
    SemaphoreEventRecord* record = nullptr;
    SemaphoreResult received = waitOn.consumeCount() ? SemaphoreResult(SEMAPHORE_OK, true) : waitOn.prepareCountPool(pool);
    if (received.status == SEMAPHORE_OK && !received.committed)
    {
        received = pool->begin(waitOn.m_count, record, deadline.remaining() == 0);
    }
    if (record == nullptr)
    {
        // 已经发布的源许可必须提示，即使目标登记/分配失败也不能漏掉这一阶段。
        const SemaphoreResult woke = m_wake.notify();
        waitOn.m_wake.leaveWait();
        return SemaphoreSignalWaitResult(woke.status != SEMAPHORE_OK ? woke.status : received.status,
            SEMAPHORE_COMMIT_DONE, received.committed ? SEMAPHORE_COMMIT_DONE : SEMAPHORE_COMMIT_NONE,
            woke.status != SEMAPHORE_OK ? woke.nativeError : received.nativeError);
    }
    const uint32_t slice = deadline.remaining(32);
    const SemaphoreResult slept = record->m_state.load() == 1 || slice == 0
        ? m_wake.notify() : m_wake.signalAndSleep(waitOn.m_wake, expected, slice);
    if (slept.status != SEMAPHORE_OK && slept.status != SEMAPHORE_TIMEOUT)
    {
        const bool committed = pool->finish(waitOn.m_count, record);
        waitOn.m_wake.leaveWait();
        return SemaphoreSignalWaitResult(slept.status, SEMAPHORE_COMMIT_DONE,
            committed ? SEMAPHORE_COMMIT_DONE : SEMAPHORE_COMMIT_NONE, slept.nativeError);
    }
    received = waitOn.waitCountRecord(record, deadline, true);
    return SemaphoreSignalWaitResult(received.status, SEMAPHORE_COMMIT_DONE,
        received.committed ? SEMAPHORE_COMMIT_DONE : SEMAPHORE_COMMIT_NONE, received.nativeError);
}

bool Semaphore::consumeCount()
{
    uint64_t count = m_count.load();
    for (;;)
    {
        const uint64_t next = count - 1;
        // 0减一的最高位为1；非空记录编码严格大于2^63，减一后仍为指针状态。
        if (static_cast<int64_t>(next) < 0)
        {
            return false;
        }
        if (m_count.compare_exchange_weak(count, next))
        {
            return true;
        }
    }
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
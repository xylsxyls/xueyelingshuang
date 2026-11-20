#include "Semaphore.h"
#include "SemaphoreProcessMemory.h"
#include "SemaphoreDeadline.h"
#include "SemaphoreCount.h"
#include "SemaphoreRetry.h"
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
        m_wake.failInitialization(SEMAPHORE_UNSUPPORTED);
    }
}

Semaphore::Semaphore(const SemaphoreConfig& config) : Semaphore()
{
    m_policy = config.waitPolicy();
    SemaphoreResult result = initializationResult();
    if (result.status == SEMAPHORE_OK && config.eventPool() != nullptr)
    {
        result = configureEventPool(*config.eventPool());
    }
    if (result.status == SEMAPHORE_OK && config.countPool() != nullptr)
    {
        result = configureCountPool(*config.countPool());
    }
    if (result.status != SEMAPHORE_OK)
    {
        delete m_eventPool.exchange(nullptr);
        m_wake.failInitialization(result.status, result.nativeError);
    }
}

SemaphoreWaitPolicy Semaphore::waitPolicy() const
{
    return m_policy;
}

SemaphoreResult Semaphore::initializationResult() const
{
    return m_wake.initializationResult();
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

bool Semaphore::signal()
{
    if (m_wake.initializationStatus() != SEMAPHORE_OK)
    {
        return false;
    }
    uint64_t count = m_count.load();
    const uint64_t next = count + 1;
    // 只尝试普通积存；竞争、大计数和等待链交给通用重试路径。
    if (static_cast<int64_t>(next) < 0 || !m_count.compare_exchange_strong(count, next))
    {
        return signalSlow(false);
    }
    // 发布后再读登记数，与等待方先登记再检查许可组成顺序一致的握手。
    if (m_wake.hasWaiters())
    {
        return signalSlow(true);
    }
    return true;
}

SemaphoreResult Semaphore::trySignal(bool retryContention, bool retryCapacity)
{
    uint32_t contention = 0;
    uint32_t capacity = 0;
    for (;;)
    {
        // 每轮先离开计数池读者保护，再重试或休眠，允许退役记录回收。
        const SemaphoreResult result = publishCount(true);
        if (result.committed)
        {
            return result;
        }
        if (result.status == SEMAPHORE_BUSY && retryContention)
        {
            SemaphoreRetry::contention(contention, m_policy);
            continue;
        }
        if (retryCapacity && (result.status == SEMAPHORE_OVERFLOW ||
            result.status == SEMAPHORE_CAPACITY || result.status == SEMAPHORE_RETRY))
        {
            const SemaphoreResult waited = SemaphoreRetry::capacity(capacity, m_policy.m_capacityAttempts, m_policy.m_capacitySleep);
            if (waited.status != SEMAPHORE_OK)
            {
                return waited;
            }
            continue;
        }
        return result;
    }
}

SemaphoreResult Semaphore::publishCount(bool notify)
{
    if (m_wake.initializationStatus() != SEMAPHORE_OK)
    {
        return m_wake.initializationResult();
    }
    uint64_t count = m_count.load();
    const uint64_t next = count + 1;
    // 地址编码和超出普通范围的计数都由计数池处理。
    if (static_cast<int64_t>(next) >= 0)
    {
        if (m_count.compare_exchange_strong(count, next))
        {
            SemaphoreResult result = notify ? m_wake.notify() : SemaphoreResult();
            result.committed = true;
            return result;
        }
        if (count < SemaphoreCount::directLimit())
        {
            return SemaphoreResult(SEMAPHORE_BUSY);
        }
    }
    SemaphoreCount* pool = nullptr;
    const SemaphoreResult prepared = prepareCountPool(pool);
    return prepared.status == SEMAPHORE_OK ? pool->signal(m_count, m_wake, notify, false) : prepared;
}

SemaphoreResult Semaphore::publishForWait(const SemaphoreDeadline& deadline, bool process, bool notify)
{
    uint32_t contention = 0;
    do
    {
        const SemaphoreResult result = process ? publishProcess() : publishCount(notify);
        if (result.status != SEMAPHORE_BUSY || result.committed)
        {
            return result;
        }
        if (deadline.remaining() == 0)
        {
            break;
        }
        SemaphoreRetry::contention(contention, m_policy);
    } while (deadline.remaining() != 0);
    return SemaphoreResult(SEMAPHORE_TIMEOUT);
}

// 将结果构造和重试循环留在慢路径，避免普通发布为了冷分支建立较大的栈帧。
#ifdef _MSC_VER
__declspec(noinline)
#elif defined(__GNUC__)
__attribute__((noinline))
#endif
bool Semaphore::signalSlow(bool committed)
{
    if (committed)
    {
        m_wake.notify();
        return true; // 提示失败不能撤销已发布许可，也不能使调用方误判为可重发。
    }
    return trySignal(true, true).committed;
}

// 普通bool等待只需要提交位，完整Result及慢等待调用所需空间不占用热路径栈帧。
#ifdef _MSC_VER
__declspec(noinline)
#elif defined(__GNUC__)
__attribute__((noinline))
#endif
bool Semaphore::waitCountCommitted(bool infinite, uint32_t timeout)
{
    return waitCount(infinite, timeout).committed;
}

bool Semaphore::wait()
{
    return waitCountQuick(true, 0);
}

bool Semaphore::wait(uint32_t timeout)
{
    return waitCountQuick(false, timeout);
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
    else
    {
        m_count.store(0);
    }
}

bool Semaphore::event()
{
    const SemaphoreResult result = tryEvent(false, true, true);
    return result.committed || result.status == SEMAPHORE_OK;
}

bool Semaphore::eventAll()
{
    const SemaphoreResult result = tryEvent(true, true, true);
    return result.committed || result.status == SEMAPHORE_OK;
}

SemaphoreResult Semaphore::tryEvent(bool all, bool retryContention, bool retryCapacity)
{
    if (m_wake.initializationStatus() != SEMAPHORE_OK)
    {
        return m_wake.initializationResult();
    }
    uint32_t contention = 0;
    uint32_t capacity = 0;
    for (;;)
    {
        SemaphoreEventPool* pool = m_eventPool.load();
        if (pool == nullptr)
        {
            return SemaphoreResult();
        }
        const bool direct = m_wake.canWaitAddress();
        SemaphoreResult result = pool->notify(all, direct ? &m_wake : nullptr, false);
        if (result.committed)
        {
            if (!direct && result.status == SEMAPHORE_OK)
            {
                const SemaphoreResult woke = m_wake.notify();
                result.status = woke.status;
                result.nativeError = woke.nativeError;
            }
            return result;
        }
        if (result.status == SEMAPHORE_BUSY && retryContention)
        {
            SemaphoreRetry::contention(contention, m_policy);
            continue;
        }
        if (result.status == SEMAPHORE_CAPACITY && retryCapacity)
        {
            const SemaphoreResult waited = SemaphoreRetry::capacity(capacity, m_policy.m_capacityAttempts, m_policy.m_capacitySleep);
            if (waited.status != SEMAPHORE_OK)
            {
                return waited;
            }
            continue;
        }
        return result;
    }
}

bool Semaphore::eventWait()
{
    return eventWaitResult().committed;
}

bool Semaphore::eventWait(int32_t timeout)
{
    const SemaphoreResult result = eventWaitResult(timeout);
    return result.committed;
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

/** 静止期先完整构造候选池，成功后才替换实例拥有的指针
@param [in,out] slot 当前实例的池指针
@param [in] config 候选池的容量和策略
@return 成功或具体的配置、分配、系统或回调错误
*/
template <typename Pool>
static SemaphoreResult ReplacePool(std::atomic<Pool*>& slot, const SemaphorePoolConfig& config)
{
    try
    {
        std::unique_ptr<Pool> candidate(new Pool(config));
        const SemaphoreResult prepared = candidate->initializationResult();
        if (prepared.status != SEMAPHORE_OK)
        {
            return prepared;
        }
        delete slot.exchange(candidate.release());
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

SemaphoreResult Semaphore::configureEventPool(const SemaphorePoolConfig& config)
{
    return ReplacePool(m_eventPool, config);
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
    return ReplacePool(m_countPool, config);
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
        m_processSemaphore = nullptr;
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

bool Semaphore::processSignal()
{
    return tryProcessSignalResult(true, true).committed;
}

int32_t Semaphore::tryProcessSignal(bool retryContention, bool retryCapacity)
{
    const SemaphoreResult result = tryProcessSignalResult(retryContention, retryCapacity);
    return result.committed ? 0 : (result.status == SEMAPHORE_OVERFLOW ? 1 :
        (result.status == SEMAPHORE_BUSY ? 2 : -1));
}

SemaphoreResult Semaphore::processSignalResult()
{
    return tryProcessSignalResult(true, true);
}

SemaphoreResult Semaphore::tryProcessSignalResult(bool retryContention, bool retryCapacity)
{
    uint32_t contention = 0;
    uint32_t capacity = 0;
    for (;;)
    {
        const SemaphoreResult result = publishProcess();
        if (result.committed)
        {
            return result;
        }
        if (result.status == SEMAPHORE_BUSY && retryContention)
        {
            SemaphoreRetry::contention(contention, m_policy);
            continue;
        }
        if (result.status == SEMAPHORE_OVERFLOW && retryCapacity)
        {
            const SemaphoreResult waited = SemaphoreRetry::capacity(capacity, m_policy.m_capacityAttempts, m_policy.m_capacitySleep);
            if (waited.status != SEMAPHORE_OK)
            {
                return waited;
            }
            continue;
        }
        return result;
    }
}

SemaphoreResult Semaphore::publishProcess()
{
#ifdef __EMSCRIPTEN__
    return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
#else
    if (m_processSemaphore == nullptr)
    {
        return SemaphoreResult(SEMAPHORE_NOT_OPEN);
    }
#ifdef SEMAPHORE_SHARED_COUNTER
    int32_t* words = static_cast<int32_t*>(m_processSemaphore);
    const int32_t limit = SemaphoreProcessMemory::load(words + 1);
    int32_t count = SemaphoreProcessMemory::load(words);
    if (limit > 0 && count >= 0 && count < limit)
    {
        if (SemaphoreProcessMemory::compareExchange(words, count, count + 1))
        {
            return SemaphoreResult(SEMAPHORE_OK, true);
        }
        count = SemaphoreProcessMemory::load(words);
        if (count >= 0 && count < limit)
        {
            return SemaphoreResult(SEMAPHORE_BUSY);
        }
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
#else
    if (::sem_post(static_cast<sem_t*>(m_processSemaphore)) == 0)
    {
        return SemaphoreResult(SEMAPHORE_OK, true);
    }
    const int32_t error = errno;
    return SemaphoreResult(
        error == EOVERFLOW ? SEMAPHORE_OVERFLOW : SEMAPHORE_SYSTEM_ERROR, false, error == EOVERFLOW ? 0 : error);
#endif
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
    const SemaphoreResult sent = publishForWait(deadline, true, true);
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
            if (!infinite && deadline.remaining() == 0)
            {
                return SemaphoreResult(SEMAPHORE_TIMEOUT);
            }
            count = SemaphoreProcessMemory::load(words);
            if (count < 0 || count > maximum)
            {
                return SemaphoreResult(SEMAPHORE_INVALID);
            }
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
        // 按实例配置的移动端或桌面POSIX间隔补查，EINTR也不重置总预算。
#ifdef SEMAPHORE_SHARED_COUNTER
        const uint32_t maximumSlice = m_policy.m_mobileSlice;
#else
        const uint32_t maximumSlice = m_policy.m_posixSlice;
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
        const SemaphoreResult prepared = candidate->initializationResult();
        if (prepared.status != SEMAPHORE_OK)
        {
            return prepared;
        }
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
        const SemaphoreResult prepared = candidate->initializationResult();
        if (prepared.status != SEMAPHORE_OK)
        {
            return prepared;
        }
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
    if (m_wake.initializationStatus() != SEMAPHORE_OK)
    {
        return m_wake.initializationResult();
    }
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
        const SemaphoreDeadline probeDeadline(false, 0);
        return m_countPool.load()->begin(m_count, unused, true, &probeDeadline);
    }
    const SemaphoreDeadline deadline(infinite, timeout);
    return waitCountRegistered(deadline);
}

SemaphoreResult Semaphore::waitCountRegistered(const SemaphoreDeadline& deadline)
{
    if (consumeCount(&deadline))
    {
        return SemaphoreResult(SEMAPHORE_OK, true);
    }
    const uint64_t observed = m_count.load();
    if (observed > (UINT64_MAX >> 1) && (observed & 1) != 0)
    {
        SemaphoreEventRecord* unused = nullptr;
        const SemaphoreResult result = m_countPool.load()->begin(m_count, unused, true, &deadline);
        if (result.status != SEMAPHORE_TIMEOUT)
        {
            return result;
        }
    }
    if (deadline.remaining() == 0)
    {
        return consumeCount(&deadline) ? SemaphoreResult(SEMAPHORE_OK, true) : SemaphoreResult(SEMAPHORE_TIMEOUT);
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
        return consumeCount(&deadline) ? SemaphoreResult(SEMAPHORE_OK, true) : SemaphoreResult(SEMAPHORE_TIMEOUT);
    }
    SemaphoreCount* pool = nullptr;
    SemaphoreResult result = prepareCountPool(pool);
    if (result.status != SEMAPHORE_OK)
    {
        return result;
    }
    SemaphoreEventRecord* record = nullptr;
    result = pool->begin(m_count, record, deadline.remaining() == 0, &deadline);
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
    // 普通等待先复用共享系统提示；单次等待达到配置的升级时间仍未取得许可后再进入优先交付链。
    // 这是防止长期抢不到许可的升级时间，不是每个线程必须等待的时间或调度上限。
    const SemaphoreDeadline promotion(false, m_policy.m_promotionMilliseconds);
    for (;;)
    {
        const uint32_t expected = m_wake.snapshot();
        if (consumeCount(&deadline))
        {
            result = SemaphoreResult(SEMAPHORE_OK, true);
            break;
        }
        const uint64_t state = m_count.load();
        const uint32_t allowance = promotion.remaining(m_policy.m_localSlice);
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

SemaphoreResult Semaphore::waitForDelivery(SemaphoreEventRecord& record, const SemaphoreDeadline& deadline, bool direct)
{
    for (;;)
    {
        // 公共后端先读提示代次，再查真实交付状态；地址后端直接等待该记录离开状态0。
        const uint32_t expected = direct ? 0 : m_wake.snapshot();
        if (record.m_state.load() == 1)
        {
            return SemaphoreResult(SEMAPHORE_OK, true);
        }
        const uint32_t slice = deadline.remaining(m_policy.m_localSlice);
        if (slice == 0)
        {
            return SemaphoreResult(SEMAPHORE_TIMEOUT);
        }
        const SemaphoreResult slept = m_wake.sleep(expected, slice, direct ? &record.m_state : nullptr);
        if (slept.status != SEMAPHORE_OK && slept.status != SEMAPHORE_TIMEOUT)
        {
            return slept;
        }
    }
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
    result = waitForDelivery(*record, deadline, direct);
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
    if (m_wake.initializationStatus() != SEMAPHORE_OK)
    {
        return m_wake.initializationResult();
    }
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
    result = waitForDelivery(*record, deadline, direct);
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
    if (m_wake.initializationStatus() != SEMAPHORE_OK || waitOn.m_wake.initializationStatus() != SEMAPHORE_OK)
    {
        const SemaphoreResult failed = m_wake.initializationStatus() != SEMAPHORE_OK
            ? m_wake.initializationResult() : waitOn.m_wake.initializationResult();
        return SemaphoreSignalWaitResult(failed.status, SEMAPHORE_COMMIT_NONE, SEMAPHORE_COMMIT_NONE, failed.nativeError);
    }
    const SemaphoreDeadline deadline(infinite, timeout);
    if (!m_wake.canSignalAndWait(waitOn.m_wake))
    {
        // 普通组合操作重试竞争；容量错误立即报告，不能因发布端满载无限延长有限等待。
        const SemaphoreResult sent = publishForWait(deadline, false, true);
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
    const SemaphoreResult sent = publishForWait(deadline, false, false);
    if (sent.status != SEMAPHORE_OK)
    {
        waitOn.m_wake.leaveWait();
        return SemaphoreSignalWaitResult(sent.status,
            sent.committed ? SEMAPHORE_COMMIT_DONE : SEMAPHORE_COMMIT_NONE, SEMAPHORE_COMMIT_NONE, sent.nativeError);
    }
    SemaphoreCount* pool = nullptr;
    SemaphoreEventRecord* record = nullptr;
    SemaphoreResult received;
    if (waitOn.consumeCount(&deadline))
    {
        received.committed = true;
    }
    else if (deadline.remaining() == 0)
    {
        // 探测已有大计数或普通许可，不为零预算的空目标创建登记池。
        received = waitOn.waitResult(0);
    }
    else
    {
        received = waitOn.prepareCountPool(pool);
        if (received.status == SEMAPHORE_OK)
        {
            received = pool->begin(waitOn.m_count, record, deadline.remaining() == 0, &deadline);
        }
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
    const uint32_t slice = deadline.remaining(waitOn.m_policy.m_localSlice);
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

bool Semaphore::waitCountQuick(bool infinite, uint32_t timeout)
{
    if (m_wake.initializationStatus() == SEMAPHORE_OK)
    {
        uint64_t count = m_count.load();
        const uint64_t next = count - 1;
        if (static_cast<int64_t>(next) >= 0)
        {
            if (m_count.compare_exchange_strong(count, next))
            {
                return true;
            }
        }
    }
    return waitCountCommitted(infinite, timeout);
}

bool Semaphore::consumeCount(const SemaphoreDeadline* deadline)
{
    if (m_wake.initializationStatus() != SEMAPHORE_OK)
    {
        return false;
    }
    uint64_t count = m_count.load();
    const uint64_t next = count - 1;
    // 0减一和记录地址仍是负数；这两种状态必须进入原有系统等待/记录路径。
    if (static_cast<int64_t>(next) < 0)
    {
        return false;
    }
    return m_count.compare_exchange_strong(count, next) || (deadline != nullptr && consumeCountContended(*deadline));
}

bool Semaphore::consumeCountContended(const SemaphoreDeadline& deadline)
{
    uint32_t contention = 0;
    while (deadline.remaining() != 0)
    {
        SemaphoreRetry::contention(contention, m_policy);
        // 退避后读取最新状态，避免使用失败CAS带回的过时比较值。
        uint64_t count = m_count.load();
        const uint64_t next = count - 1;
        if (static_cast<int64_t>(next) < 0)
        {
            return false;
        }
        if (m_count.compare_exchange_strong(count, next))
        {
            return true;
        }
    }
    return false;
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

// 使用示例：复制下面的头文件和 main 到独立调用程序，去掉行首注释即可运行。
// #include "Semaphore.h"
// #include <atomic>
// #include <cstdio>
// #include <cstdlib>
// #include <stdexcept>
// #include <thread>
// #ifdef __APPLE__
// #include <TargetConditionals.h>
// #endif
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
//             SemaphoreConfig startup;
//             startup.configureEventPool().initialCapacity = 4;
//             SemaphoreWaitPolicy policy;
//             policy.m_promotionMilliseconds = 16;
//             showResult("configure wait policy", startup.setWaitPolicy(policy));
//             Semaphore tasks(startup);
//             expect(tasks.waitPolicy().m_promotionMilliseconds == 16, "instance wait policy");
//             showResult("initialize count", tasks.initializationResult());
//             // 可选：并发启动前、计数为空时预分配计数等待记录；不调用也会按需建立默认池
//             SemaphorePoolConfig countConfig;
//             countConfig.initialCapacity = 16;
//             countConfig.maxCapacity = 64;
//             showResult("configure optional count pool", tasks.configureCountPool(countConfig));
//             const SemaphorePoolStats countStats = tasks.countPoolStatistics();
//             expect(countStats.capacity == 16, "count pool prepared");
//             expect(tasks.signal(), "tasks signal");
//             expect(tasks.wait(), "tasks wait");
//             expect(tasks.signal(), "tasks signal");
//             expect(tasks.wait(1000), "count timed wait");
//             showResult("count publish", tasks.trySignal(true, false));
//             showResult("count infinite wait", tasks.waitResult());
//             showResult("count publish again", tasks.trySignal());
//             showResult("count timed result", tasks.waitResult(1000));
//             // 持续阻塞会按需进入优先交付链，避免后来者夺走已交付许可；无需改变调用方式
//             // wait(0)只探测，不适合循环抢占；停止时先提交退出状态，再发退出许可并join
//
//             // reset只用于生产者、消费者均已停止的静止阶段，不作为并发取消接口
//             expect(tasks.signal(), "tasks signal");
//             tasks.reset();
//             expect(tasks.waitResult(0).status == SEMAPHORE_TIMEOUT, "reset clears pending count");
//             // 维护不是每次等待的前置步骤；没有缩容策略时只尝试安全回收退役记录
//             size_t countReleased = 0;
//             showResult("maintain optional count pool", tasks.maintainCountPool(countReleased));
//         }
//
//         // 二、周期状态事件：事件不积存，唤醒后读取业务版本，不能把提示当成任务数量
//         {
//             Semaphore changed;
//             showResult("initialize event", changed.initializationResult());
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
//                             if (!changed.event()) publishFailed.store(true);
//                         }
//                         else if (current % 3 == 2)
//                         {
//                             // 有多个订阅者时，eventAll提示本次已登记的全部等待者
//                             if (!changed.eventAll()) publishFailed.store(true);
//                         }
//                         else
//                         {
//                             const SemaphoreResult result = changed.tryEvent(false, true, false);
//                             if (result.status != SEMAPHORE_OK)
//                             {
//                                 publishFailed.store(true);
//                             }
//                             // committed=false表示此刻没有合格等待者，不是积存了一次事件
//                             // committed=true但status失败时也不能盲目重发，本例只记录失败
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
//                 expect(changed.eventWait(), "event infinite wait");
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
//             showResult("initialize jobs", jobs.initializationResult());
//             showResult("initialize free slots", freeSlots.initializationResult());
//             showResult("prepare jobs", jobs.prepareSignalAndWait());
//             showResult("prepare free slots", freeSlots.prepareSignalAndWait());
//             expect(freeSlots.signal(), "freeSlots signal");
//             showCombined("publish job and acquire next slot", jobs.signalAndWaitResult(freeSlots));
//             expect(jobs.wait(), "jobs wait");
//             expect(freeSlots.signal(), "freeSlots signal"); // 消费者处理上一工作后归还槽位
//             showCombined("publish job and acquire slot with budget", jobs.signalAndWaitResult(freeSlots, 1000));
//             expect(jobs.wait(), "jobs wait");
//             // 真实冷通知后立即等待可优先使用组合接口；持续热往返不宜机械切换准备模式
//         }
//
//         // 四、命名通知：同一程序按顺序模拟两个进程端，真实两端必须约定相同目录和名称
// #if defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IPHONE)
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
//             showResult("initialize jobs owner", jobsOwner.initializationResult());
//             showResult("initialize slots owner", slotsOwner.initializationResult());
//             showResult("initialize jobs reader", jobsReader.initializationResult());
//             showResult("initialize slots reader", slotsReader.initializationResult());
//             showResult("initialize probe", probe.initializationResult());
//
//             // 旧void包装便于兼容；新调用方优先Result接口，以便观察失败原因
//             jobsOwner.createProcessSemaphore(jobsName, 4);
//             showResult("create named slots", slotsOwner.createProcessResult(slotsName, 4));
//             expect(probe.tryOpenProcessSemaphore(jobsName), "probe existing named jobs");
//             jobsReader.openProcessSemaphore(jobsName);
//             showResult("open named slots", slotsReader.openProcessResult(slotsName));
//
//             // 普通发送以bool报告是否提交；竞争及满载会内部重试，需要期限时优先try接口。
//             expect(jobsOwner.processSignal(), "publish named job bool");
//             expect(jobsReader.processWait(1000), "consume legacy named job with budget");
//             // 此处没有其他消费者，确认发布成功后才演示旧的无限等待
//             expect(jobsOwner.tryProcessSignal() == 0, "publish named job");
//             expect(jobsReader.processWait(1000), "consume named try job");
//             // 允许重试竞争，但满载立即交还调用方，避免无限等自己应释放的容量。
//             showResult("named try result", jobsOwner.tryProcessSignalResult(true, false));
//             jobsReader.processWait();
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
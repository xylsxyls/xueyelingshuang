#include "ProcessWork.h"
#include "ProcessWorkAtomic.h"
#include "ProcessWorkTime.h"
#include "ProcessReceiveCallback.h"
#include "CSystem/CSystemAPI.h"
#include "AtomicMath/AtomicMathAPI.h"
#include "ReadTask.h"
#include "ReceiveTask.h"
#include "SendTask.h"
#include <cstring>
#include <cstdio>
#include <limits.h>
#include <thread>
#include <limits>
#include <exception>
#include <new>

ProcessWork::ProcessWork() :
m_nameOwned(false), m_thisProcessPid(CSystem::currentProcessPid()),
m_readThreadId(0), m_receiveThreadId(0), m_postThreadId(0), m_postStarted(false), m_fullCount(0), m_waitEndPost(false)
{

}

ProcessWork& ProcessWork::instance()
{
    static ProcessWork s_processWork;
    return s_processWork;
}

bool ProcessWork::configure(const ProcessWorkConfig& config)
{
    if (m_readThreadId != 0 || m_receiveThreadId != 0 || m_postThreadId != 0 ||
        m_channel || !m_sendChannels.empty() || !m_sendNameMemoryMap.empty() || m_postQueue.hasPending() || m_receiveQueue.hasPending())
    {
        return false;
    }
    try
    {
        if (!ProcessWorkMemory::valid(config.m_receiveMemory) || !ProcessWorkMemory::valid(config.m_postMemory) ||
            m_sendChannels.lockFreeMapConfigure(config.m_sendChannels) != LOCK_FREE_MAP_OK ||
            m_sendNameMemoryMap.lockFreeMapConfigure(config.m_sendNames) != LOCK_FREE_MAP_OK ||
            !m_postQueue.configure(config.m_postMemory))
        {
            return false;
        }
        if (config.m_configureManager && CTaskThreadManager::instance().configure(config.m_manager).m_status != CTaskThreadOk)
        {
            return false;
        }
        m_components = config;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ProcessWork::addProcessReceiveCallback(ProcessReceiveCallback* callback)
{
    if (m_channel || m_readThreadId != 0 || m_receiveThreadId != 0)
    {
        return false;
    }
    try
    {
        m_callback.push_back(callback);
        return true;
    }
    catch (...)
    {
        // vector插入失败保持原列表，生命周期调用方可处理资源错误后重试。
        return false;
    }
}

std::string ProcessWork::nameKey(const std::string& name)
{
    uint64_t value = 14695981039346656037ULL;
    for (size_t i = 0; i < name.size(); ++i)
    {
        value ^= static_cast<unsigned char>(name[i]);
        value *= 1099511628211ULL;
    }
    // 这里只缩短查找键，打开后必须逐字节核对完整名称。
    return "P3N" + std::to_string(value);
}

bool ProcessWork::setReceiveName(const std::string& name)
{
    if (m_channel || m_readThreadId != 0 || m_receiveThreadId != 0 || name.size() >= 1024 || name.find('\0') != std::string::npos)
    {
        return false;
    }
    try
    {
        std::string replacement(name);
        m_receiveName.swap(replacement);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ProcessWork::claimReceiveName()
{
#ifdef _WIN32
    const std::string name = m_receiveName.empty() ? CSystem::GetCurrentExeName() : m_receiveName;
#else
    const std::string name = m_receiveName.empty() ? CSystem::GetCurrentExeFullName() : m_receiveName;
#endif
    if (name.empty() || name.size() >= 1024)
    {
        return false;
    }
    m_nameMemory.reset(new SharedMemory(nameKey(name), sizeof(ProcessWorkNameDirectory), true));
    // 原生创建者未必是CAS登记获胜者；撤名由逻辑所有者明确执行。
    m_nameMemory->preserveName();
    ProcessWorkNameDirectory* directory = static_cast<ProcessWorkNameDirectory*>(m_nameMemory->writeWithoutLock());
    if (directory == nullptr || m_nameMemory->realSize() < sizeof(ProcessWorkNameDirectory))
    {
        return false;
    }
    const int32_t previous = ProcessWorkAtomic::load(&directory->m_pid);
    uint64_t selfIdentity = 0;
    uint64_t previousIdentity = 0;
    if (!CSystem::processIdentity(m_thisProcessPid, selfIdentity))
    {
        // 身份查询失败不能发布名称记录；保留回调和已有Post任务，允许调用方重试。
        return false;
    }
    const bool previousKnown = previous != 0 && previous != INT32_MIN && CSystem::processIdentity(previous < 0 ? -previous : previous, previousIdentity);
    const bool previousExited = previous != 0 && previous != INT32_MIN && CSystem::processDefinitelyExited(previous < 0 ? -previous : previous);
    const bool previousAlive = previous != 0 && !previousExited &&
        (previous < 0 || !previousKnown || previousIdentity == static_cast<uint64_t>(ProcessWorkAtomic::load(reinterpret_cast<int64_t*>(&directory->m_identity))));
    if ((!(previousAlive && previous < 0) && ProcessWorkAtomic::load(&directory->m_length) != 0 && (ProcessWorkAtomic::load(&directory->m_length) != name.size() ||
            std::memcmp(directory->m_name, name.data(), name.size()) != 0)) ||
        (!previousAlive && !AtomicMath::compareAndSwap(&directory->m_pid, previous, -m_thisProcessPid)))
    {
        return false;
    }
    // 同名进程仍可通过各自PID收发；名称只属于最先登记且存活的接收器。
    m_nameOwned = !previousAlive;
    if (m_nameOwned)
    {
        int64_t* identity = reinterpret_cast<int64_t*>(&directory->m_identity);
        AtomicMath::compareAndSwap(identity, ProcessWorkAtomic::load(identity), static_cast<int64_t>(selfIdentity));
        if (ProcessWorkAtomic::load(&directory->m_length) == 0)
        {
            std::memcpy(directory->m_name, name.c_str(), name.size() + 1);
            AtomicMath::compareAndSwap(&directory->m_length, 0, static_cast<int32_t>(name.size()));
        }
    }
    return true;
}

bool ProcessWork::initReceive(int32_t receiveSize, int32_t areaCount, int32_t expandThreshold,
    int32_t expandCount, int32_t idleSeconds, int32_t maxExpandTimes)
{
    return initReceive(receiveSize, areaCount, expandThreshold, expandCount, idleSeconds, maxExpandTimes, m_components.m_receiveMemory);
}

bool ProcessWork::initReceive(int32_t receiveSize, int32_t areaCount, int32_t expandThreshold,
    int32_t expandCount, int32_t idleSeconds, int32_t maxExpandTimes, const ProcessWorkMemoryConfig& memory)
{
    if (m_channel || m_readThreadId != 0 || m_receiveThreadId != 0)
    {
        return false;
    }
    // 唯一回滚出口：任何准备失败或异常均先加入借用任务，再销毁映射。
    // 参数为本次尚未交付的资源；按对应所有者规则释放，不传播异常。
    auto rollback = [](ProcessWork* owner)
    {
        owner->releaseReceive(false);
    };
    std::unique_ptr<ProcessWork, decltype(rollback)> pending(this, rollback);
    try
    {
        if (!m_receiveQueue.configure(memory))
        {
            return false;
        }
        m_channel.reset(new ProcessWorkChannel);
        if (!m_channel->configureProducers(m_components.m_producers) ||
            !m_channel->openReceiver(receiveSize, areaCount, expandThreshold, expandCount, idleSeconds, maxExpandTimes, memory))
        {
            return false;
        }
        if (!claimReceiveName())
        {
            return false;
        }
        m_fullCount = 0;
        std::shared_ptr<ReadTask> readTask(new ReadTask);
        std::shared_ptr<ReceiveTask> receiveTask(new ReceiveTask);
        readTask->setParam(m_channel.get(), &m_receiveQueue, &m_fullCount);
        receiveTask->setParam(&m_callback, &m_receiveQueue, &m_fullCount);
        m_readThreadId = CTaskThreadManager::instance().init(m_components.m_readThread);
        m_receiveThreadId = CTaskThreadManager::instance().init(m_components.m_receiveThread);
        std::shared_ptr<CTaskThread> reader = CTaskThreadManager::instance().getThreadInterface(m_readThreadId);
        std::shared_ptr<CTaskThread> receiver = CTaskThreadManager::instance().getThreadInterface(m_receiveThreadId);
        if (!reader || !receiver)
        {
            return false;
        }
        // 初始化线程尚无可释放额度的任务；有限配置不足时必须失败回滚，不能等待自身启动。
        if (!receiver->tryPostTask(receiveTask, 1, true, false) || !reader->tryPostTask(readTask, 1, true, false))
        {
            return false;
        }
        m_channel->publish();
        if (m_nameOwned)
        {
            ProcessWorkNameDirectory* directory = static_cast<ProcessWorkNameDirectory*>(m_nameMemory->memory());
            AtomicMath::compareAndSwap(&directory->m_pid, -m_thisProcessPid, m_thisProcessPid);
        }
        pending.release();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void ProcessWork::uninitReceive()
{
    releaseReceive(true);
}

void ProcessWork::releaseReceive(bool clearProcessState)
{
    if (m_channel)
    {
        m_channel->stop();
    }
    if (m_nameOwned && m_nameMemory && m_nameMemory->memory())
    {
        ProcessWorkNameDirectory* directory = static_cast<ProcessWorkNameDirectory*>(m_nameMemory->memory());
        // 先保留关闭中标记，再撤POSIX名称，防止新同名接收器发布后被旧析构撤名。
        AtomicMath::compareAndSwap(&directory->m_pid, m_thisProcessPid, -m_thisProcessPid);
        // 已撤名的旧映射保持关闭标志，不能让持有旧引用的初始化者重新认领。
        if (!m_nameMemory->unlinkName())
        {
            AtomicMath::compareAndSwap(&directory->m_pid, -m_thisProcessPid, 0);
        }
    }
    if (clearProcessState)
    {
        uninitPostThread();
        if (m_postThreadId != 0)
        {
            return;
        }
    }
    if (m_readThreadId != 0)
    {
        const CTaskThreadResult result = CTaskThreadManager::instance().uninit(m_readThreadId);
        if (result.m_status != CTaskThreadOk && result.m_status != CTaskThreadNotFound)
        {
            return;
        }
        m_readThreadId = 0;
    }
    if (m_receiveThreadId != 0)
    {
        const CTaskThreadResult result = CTaskThreadManager::instance().uninit(m_receiveThreadId);
        if (result.m_status != CTaskThreadOk && result.m_status != CTaskThreadNotFound)
        {
            return;
        }
        m_receiveThreadId = 0;
    }
    m_channel.reset();
    m_nameMemory.reset();
    m_nameOwned = false;
    m_fullCount = 0;
    m_receiveQueue.clear();
    if (clearProcessState)
    {
        m_callback.clear();
        clear();
    }
}

std::shared_ptr<ProcessWorkChannel> ProcessWork::sender(int32_t pid)
{
    if (pid <= 0)
    {
        return std::shared_ptr<ProcessWorkChannel>();
    }
    std::shared_ptr<ProcessWorkChannel> cached;
    LockFreeMapToken token;
    const LockFreeMapStatus found = m_sendChannels.lockFreeMapGet(pid, cached, token);
    if (found == LOCK_FREE_MAP_OK && cached->ready())
    {
        return cached;
    }
    std::shared_ptr<ProcessWorkChannel> channel(new ProcessWorkChannel);
    if (!channel->configureProducers(m_components.m_producers) || !channel->openSender(pid))
    {
        return std::shared_ptr<ProcessWorkChannel>();
    }
    // 映射与身份检查在发布之前完成；缓存竞争或预算耗尽不使有效发送失败。
    if (found == LOCK_FREE_MAP_OK)
    {
        m_sendChannels.tryUpdate(pid, token, channel, false, false);
    }
    else
    {
        const LockFreeMapStatus inserted = m_sendChannels.tryGetOrInsert(pid, channel, cached, token, false, false);
        if (inserted == LOCK_FREE_MAP_EXISTS && cached->ready())
        {
            return cached;
        }
    }
    return channel;
}

bool ProcessWork::send(int32_t destPid, const char* buffer, int32_t length, MessageType type)
{
    return sendResult(destPid, buffer, length, type) == PROCESS_WORK_SENT;
}

int32_t ProcessWork::sendResult(int32_t destPid, const char* buffer, int32_t length, MessageType type)
{
    return trySend(destPid, buffer, length, type, true, true);
}

int32_t ProcessWork::sendChannel(const std::shared_ptr<ProcessWorkChannel>& channel,
    const char* buffer, int32_t length, MessageType type, bool retryContention, bool retryCapacity)
{
    return channel ? channel->trySend(buffer, length, static_cast<int32_t>(type),
        retryContention, retryCapacity, SendTask::currentStopFlag()) : PROCESS_WORK_UNAVAILABLE;
}

int32_t ProcessWork::trySend(int32_t destPid, const char* buffer, int32_t length, MessageType type,
    bool retryContention, bool retryCapacity)
{
    if (destPid <= 0 || length < 0 || (length > 0 && buffer == nullptr) || static_cast<int32_t>(type) < 0)
    {
        return PROCESS_WORK_INVALID;
    }
    try
    {
        ProcessWorkSendCacheLease cached;
        ProcessWorkChannel* direct = m_sendCache.find(destPid, cached);
        if (direct != nullptr)
        {
            const int32_t result = direct->trySend(buffer, length, static_cast<int32_t>(type),
                retryContention, retryCapacity, SendTask::currentStopFlag());
            // transmit自身检查会话和正文发布前的关闭状态，成功发送不再重复预查一次。
            // 失效通道仍经权威Map重新打开；关闭标记等其他不可用结果不重发消息。
            if (result != PROCESS_WORK_UNAVAILABLE || direct->ready())
            {
                return result;
            }
            m_sendCache.clear();
        }
        return sendUncached(destPid, buffer, length, type, retryContention, retryCapacity);
    }
    catch (const std::bad_alloc&)
    {
        return PROCESS_WORK_NO_MEMORY;
    }
    catch (...)
    {
        return PROCESS_WORK_UNAVAILABLE;
    }
}

int32_t ProcessWork::sendUncached(int32_t destPid, const char* buffer, int32_t length, MessageType type,
    bool retryContention, bool retryCapacity)
{
    try
    {
        const uint64_t cacheEpoch = m_sendCache.epoch();
        bool handled = false;
        int32_t result = PROCESS_WORK_UNAVAILABLE;
        std::shared_ptr<ProcessWorkChannel> retained;
        // 热路径只借用通道；需要等待时复制一次引用，并先退出Map保护，允许缓存回收。
        m_sendChannels.lockFreeMapVisit(destPid,
            [this, destPid, cacheEpoch, buffer, length, type, retryContention, retryCapacity, &handled, &result, &retained]
            (const std::shared_ptr<ProcessWorkChannel>& channel)
        {
            if (channel && channel->ready())
            {
                m_sendCache.remember(destPid, cacheEpoch, channel);
                handled = true;
                result = sendChannel(channel, buffer, length, type, false, false);
                if ((result == PROCESS_WORK_BUSY && retryContention) ||
                    ((result == PROCESS_WORK_CAPACITY || result == PROCESS_WORK_RESOURCE) && retryCapacity))
                {
                    retained = channel;
                }
            }
        });
        if (!handled)
        {
            retained = sender(destPid);
            m_sendCache.remember(destPid, cacheEpoch, retained);
        }
        return retained ? sendChannel(retained, buffer, length, type, retryContention, retryCapacity) : result;
    }
    catch (const std::bad_alloc&)
    {
        return PROCESS_WORK_NO_MEMORY;
    }
    catch (...)
    {
        return PROCESS_WORK_UNAVAILABLE;
    }
}

std::shared_ptr<ProcessWorkChannel> ProcessWork::resolveName(const std::string& name)
{
    if (name.empty() || name.size() >= 1024 || name.find('\0') != std::string::npos)
    {
        return std::shared_ptr<ProcessWorkChannel>();
    }
    std::shared_ptr<SharedMemory> memory;
    LockFreeMapToken token;
    m_sendNameMemoryMap.lockFreeMapGet(name, memory, token);
    for (int32_t attempt = 0; attempt < 2; ++attempt)
    {
        if (!memory)
        {
            std::shared_ptr<SharedMemory> candidate(new SharedMemory(nameKey(name)));
            if (candidate->writeWithoutLock() == nullptr || candidate->realSize() < sizeof(ProcessWorkNameDirectory))
            {
                return std::shared_ptr<ProcessWorkChannel>();
            }
            // SharedMemory的首次映射会修改本地成员，只能在候选独占时完成。
            const LockFreeMapStatus inserted = m_sendNameMemoryMap.tryGetOrInsert(name, candidate, memory, token, false, false);
            if (inserted != LOCK_FREE_MAP_EXISTS && inserted != LOCK_FREE_MAP_COMMITTED)
            {
                memory = candidate;
            }
        }
        ProcessWorkNameDirectory* directory = static_cast<ProcessWorkNameDirectory*>(memory->memory());
        const int32_t pid = ProcessWorkAtomic::load(&directory->m_pid);
        if (pid > 0 && ProcessWorkAtomic::load(&directory->m_length) == name.size() && std::memcmp(directory->m_name, name.data(), name.size()) == 0)
        {
            const uint64_t identity = static_cast<uint64_t>(ProcessWorkAtomic::load(reinterpret_cast<int64_t*>(&directory->m_identity)));
            std::shared_ptr<ProcessWorkChannel> channel = sender(pid);
            if (channel && channel->matchesIdentity(identity) && ProcessWorkAtomic::load(&directory->m_pid) == pid)
            {
                return channel;
            }
            return std::shared_ptr<ProcessWorkChannel>();
        }
        std::shared_ptr<SharedMemory> current;
        if (m_sendNameMemoryMap.lockFreeMapGet(name, current, token) == LOCK_FREE_MAP_OK && current == memory)
        {
            m_sendNameMemoryMap.tryEraseIf(name, token, false, false);
        }
        memory.reset();
    }
    return std::shared_ptr<ProcessWorkChannel>();
}

bool ProcessWork::send(const std::string& processName, const char* buffer, int32_t length, MessageType type)
{
    return sendResult(processName, buffer, length, type) == PROCESS_WORK_SENT;
}

int32_t ProcessWork::sendResult(const std::string& processName, const char* buffer, int32_t length, MessageType type)
{
    return trySend(processName, buffer, length, type, true, true);
}

int32_t ProcessWork::trySend(const std::string& processName, const char* buffer, int32_t length,
    MessageType type, bool retryContention, bool retryCapacity)
{
    if (processName.empty() || processName.size() >= 1024 || processName.find('\0') != std::string::npos ||
        length < 0 || (length > 0 && buffer == nullptr) || static_cast<int32_t>(type) < 0)
    {
        return PROCESS_WORK_INVALID;
    }
    // 保留名称核验时的通道；再次按PID查找会跨过关闭/重建或PID复用边界。
    try
    {
        return sendChannel(resolveName(processName), buffer, length, type, retryContention, retryCapacity);
    }
    catch (const std::bad_alloc&)
    {
        return PROCESS_WORK_NO_MEMORY;
    }
    catch (...)
    {
        return PROCESS_WORK_UNAVAILABLE;
    }
}

bool ProcessWork::send(int32_t destPid, const std::string& message, MessageType type)
{
    if (message.size() <= INT32_MAX)
    {
        return send(destPid, message.data(), static_cast<int32_t>(message.size()), type);
    }
    return false;
}

bool ProcessWork::send(const std::string& name, const std::string& message, MessageType type)
{
    if (message.size() <= INT32_MAX)
    {
        return send(name, message.data(), static_cast<int32_t>(message.size()), type);
    }
    return false;
}

int32_t ProcessWork::enqueue(int32_t pid, const std::string& name, const char* buffer, int32_t length,
    MessageType type, bool retryTransient, bool retryCapacity)
{
    if ((pid <= 0 && name.empty()) || name.size() >= 1024 || name.find('\0') != std::string::npos || length < 0 ||
        length > INT32_MAX - 1040 || (length > 0 && buffer == nullptr) || static_cast<int32_t>(type) < 0)
    {
        return PROCESS_WORK_POST_INVALID;
    }
    LockFreeQueueResult result;
    // 参数为本次尚未交付的资源；按对应所有者规则释放，不传播异常。
    auto release = [this](char* packet)
    {
        m_postQueue.release(packet);
    };
    uint32_t capacity = 0;
    std::unique_ptr<char, decltype(release)> packet(nullptr, release);
    for (;;)
    {
        packet.reset(m_postQueue.allocate(16 + name.size() + static_cast<size_t>(length), result, retryTransient));
        if (packet || !retryCapacity || result != LockFreeQueueCapacityLimit)
        {
            break;
        }
        // 未启动消费者、正在结束或发送线程重入都不能等待自己释放预算。
        if (!m_postStarted || m_waitEndPost.load() || SendTask::currentStopFlag() != nullptr ||
            !ProcessWorkTime::waitCapacity(capacity))
        {
            return PROCESS_WORK_POST_UNAVAILABLE;
        }
    }
    if (!packet)
    {
        return postQueueResult(result);
    }
    const int32_t header[4] = { pid, static_cast<int32_t>(type), length, static_cast<int32_t>(name.size()) };
    std::memcpy(packet.get(), header, sizeof(header));
    if (!name.empty())
    {
        std::memcpy(packet.get() + 16, name.data(), name.size());
    }
    if (length != 0)
    {
        std::memcpy(packet.get() + 16 + name.size(), buffer, length);
    }
    uint32_t contention = 0;
    for (;;)
    {
        result = m_postQueue.submit(packet.get());
        if (result == LockFreeQueueSuccess)
        {
            break;
        }
        if (retryTransient && result == LockFreeQueueTemporaryUnavailable)
        {
            if (++contention == 64)
            {
                CSystem::Sleep(1);
                contention = 0;
            }
            else
            {
                std::this_thread::yield();
            }
            continue;
        }
        if (retryCapacity && (result == LockFreeQueueCapacityLimit ||
            result == LockFreeQueueResourceUnavailable || result == LockFreeQueueGrowthDeclined))
        {
            if (!m_postStarted || m_waitEndPost.load() || SendTask::currentStopFlag() != nullptr ||
                !ProcessWorkTime::waitCapacity(capacity))
            {
                return PROCESS_WORK_POST_UNAVAILABLE;
            }
            continue;
        }
        return postQueueResult(result);
    }
    packet.release();
    return PROCESS_WORK_POSTED;
}

int32_t ProcessWork::postQueueResult(LockFreeQueueResult result)
{
    if (result == LockFreeQueueInvalidConfiguration)
    {
        return PROCESS_WORK_POST_INVALID;
    }
    if (result == LockFreeQueueSuccess)
    {
        return PROCESS_WORK_POSTED;
    }
    if (result == LockFreeQueueAllocationFailure)
    {
        return PROCESS_WORK_POST_NO_MEMORY;
    }
    if (result == LockFreeQueueBusy)
    {
        return PROCESS_WORK_POST_BUSY;
    }
    if (result == LockFreeQueueCapacityLimit)
    {
        return PROCESS_WORK_POST_CAPACITY;
    }
    if (result == LockFreeQueueResourceUnavailable)
    {
        return PROCESS_WORK_POST_RESOURCE;
    }
    return PROCESS_WORK_POST_UNAVAILABLE;
}

int32_t ProcessWork::postResult(int32_t destPid, const char* buffer, int32_t length, MessageType type)
{
    return destPid <= 0 ? PROCESS_WORK_POST_INVALID : enqueue(destPid, std::string(), buffer, length, type, true, true);
}

int32_t ProcessWork::postResult(const std::string& name, const char* buffer, int32_t length, MessageType type)
{
    return enqueue(0, name, buffer, length, type, true, true);
}

int32_t ProcessWork::tryPost(int32_t destPid, const char* buffer, int32_t length, MessageType type,
    bool retryContention, bool retryCapacity)
{
    return destPid <= 0 ? PROCESS_WORK_POST_INVALID : enqueue(destPid, std::string(), buffer, length, type, retryContention, retryCapacity);
}

int32_t ProcessWork::tryPost(const std::string& name, const char* buffer, int32_t length, MessageType type,
    bool retryContention, bool retryCapacity)
{
    return enqueue(0, name, buffer, length, type, retryContention, retryCapacity);
}

bool ProcessWork::configurePostMemory(const ProcessWorkMemoryConfig& memory)
{
    return m_postThreadId == 0 && m_postQueue.configure(memory);
}

uint64_t ProcessWork::postPendingBytes() const
{
    return m_postQueue.used();
}

uint64_t ProcessWork::receivePendingBytes() const
{
    return m_receiveQueue.used();
}

bool ProcessWork::post(int32_t destPid, const char* buffer, int32_t length, MessageType type)
{
    return postResult(destPid, buffer, length, type) == PROCESS_WORK_POSTED;
}

bool ProcessWork::post(const std::string& name, const char* buffer, int32_t length, MessageType type)
{
    return postResult(name, buffer, length, type) == PROCESS_WORK_POSTED;
}

bool ProcessWork::post(int32_t destPid, const std::string& message, MessageType type)
{
    if (message.size() <= INT32_MAX)
    {
        return post(destPid, message.data(), static_cast<int32_t>(message.size()), type);
    }
    return false;
}

bool ProcessWork::post(const std::string& name, const std::string& message, MessageType type)
{
    if (message.size() <= INT32_MAX)
    {
        return post(name, message.data(), static_cast<int32_t>(message.size()), type);
    }
    return false;
}

bool ProcessWork::initPostThread()
{
    if (m_postThreadId != 0)
    {
        return m_postStarted;
    }
    try
    {
        std::shared_ptr<SendTask> task(new SendTask);
        m_postThreadId = CTaskThreadManager::instance().init(m_components.m_postThread);
        std::shared_ptr<CTaskThread> thread = CTaskThreadManager::instance().getThreadInterface(m_postThreadId);
        if (thread)
        {
            m_waitEndPost = false;
            task->setParam(&m_waitEndPost, &m_postQueue, false, this, &m_sendCache);
            if (thread->tryPostTask(task, 1, true, false))
            {
                task->start();
                m_postStarted = true;
                return true;
            }
        }
    }
    catch (...)
    {
        // 构造和任务配置失败尚未接纳发送任务，统一回收已创建的空闲线程。
    }
    if (m_postThreadId != 0)
    {
        const CTaskThreadResult result = CTaskThreadManager::instance().uninit(m_postThreadId);
        if (result.m_status == CTaskThreadOk || result.m_status == CTaskThreadNotFound)
        {
            m_postThreadId = 0;
        }
    }
    return false;
}

void ProcessWork::uninitPostThread()
{
    if (m_postThreadId != 0)
    {
        const CTaskThreadResult result = CTaskThreadManager::instance().uninit(m_postThreadId);
        if (result.m_status != CTaskThreadOk && result.m_status != CTaskThreadNotFound)
        {
            return;
        }
        m_postThreadId = 0;
    }
    m_postStarted = false;
    m_waitEndPost = false;
    m_postQueue.clear();
}

void ProcessWork::waitForPostEnd()
{
    if (m_postThreadId == 0)
    {
        return;
    }
    m_waitEndPost = true;
    m_postQueue.wake();
    const CTaskThreadResult result = CTaskThreadManager::instance().waitForEnd(m_postThreadId);
    if (result.m_status != CTaskThreadOk && result.m_status != CTaskThreadNotFound)
    {
        return;
    }
    m_postThreadId = 0;
    m_postStarted = false;
    initPostThread();
}

void ProcessWork::clearDestPid(int32_t destPid)
{
    m_sendChannels.erase(destPid);
    m_sendCache.clear();
}

void ProcessWork::clearUseless()
{
    m_sendCache.clear();
    m_sendChannels.forEach([this](int32_t pid, const std::shared_ptr<ProcessWorkChannel>& channel)
    {
        if (!channel->ready() || CSystem::processDefinitelyExited(pid))
        {
            std::shared_ptr<ProcessWorkChannel> current;
            LockFreeMapToken token;
            if (m_sendChannels.lockFreeMapGet(pid, current, token) == LOCK_FREE_MAP_OK && current == channel)
            {
                m_sendChannels.tryEraseIf(pid, token, true, false);
            }
        }
    });
    m_sendNameMemoryMap.forEach([this](const std::string& name, const std::shared_ptr<SharedMemory>& memory)
    {
        ProcessWorkNameDirectory* directory = static_cast<ProcessWorkNameDirectory*>(memory->memory());
        const int32_t pid = directory == nullptr ? 0 : ProcessWorkAtomic::load(&directory->m_pid);
        if (pid <= 0 || CSystem::processDefinitelyExited(pid))
        {
            std::shared_ptr<SharedMemory> current;
            LockFreeMapToken token;
            if (m_sendNameMemoryMap.lockFreeMapGet(name, current, token) == LOCK_FREE_MAP_OK && current == memory)
            {
                m_sendNameMemoryMap.tryEraseIf(name, token, true, false);
            }
        }
    });
    // 遍历时可能有发送者重新缓存旧项，清理结束后再使这批借用缓存失效。
    m_sendCache.clear();
}

void ProcessWork::clear()
{
    m_sendChannels.clear();
    m_sendCache.clear();
    m_sendNameMemoryMap.clear();
    size_t released = 0;
    m_sendChannels.lockFreeMapMaintain((std::numeric_limits<size_t>::max)(), released);
    m_sendNameMemoryMap.lockFreeMapMaintain((std::numeric_limits<size_t>::max)(), released);
}

int32_t ProcessWork::receiveBufferCount() const
{
    return m_channel ? m_channel->bufferCount() : 0;
}

int32_t ProcessWork::receivePayloadCapacity() const
{
    return m_channel ? m_channel->payloadCapacity() : 0;
}

/* PROCESS_WORK_USAGE_DEMO_BEGIN
// 将本块复制到应用的独立main源文件编译；库本身保持本块注释。
// 自发自收便于直接运行。跨进程时接收端保留初始化/回调，发送端使用对方PID或逻辑名称。
#include "ProcessWork/ProcessWorkAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <exception>
#include <string>
#include <thread>

// 示例回调对象由main拥有；析构先关闭工作线程，再销毁回调中的状态。
class ProcessWorkDemoCallback : public ProcessReceiveCallback
{
public:
    ProcessWorkDemoCallback() : m_received(0), m_bad(false)
    {

    }

    ~ProcessWorkDemoCallback()
    {
        ProcessWork::instance().uninitReceive();
    }

    void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
    {
        // buffer只借用到本次回调返回；异步使用时必须复制。
        if (sendPid != CSystem::currentProcessPid() || length != 6 || buffer == nullptr ||
            std::memcmp(buffer, "123456", 6) != 0 || type != MessageType::MESSAGE)
        {
            m_bad.store(true);
        }
        m_received.fetch_add(1);
    }

    void bufferFull(int32_t count)
    {
        // 合并的满载通知，仅供监控；不要在此等待本接收线程排空。
        std::printf("shared buffers busy, count=%d\n", count);
    }

public:
    // 演示跨线程观察完成数量；业务应使用自己的请求/应答协议确认远端处理。
    std::atomic<int32_t> m_received;
    // 演示对正文与消息类型的检查。
    std::atomic<bool> m_bad;
};

int main()
{
    try
    {
        // 在程序串行启动阶段主动创建单例，管理器应比IPC先初始化。
        CTaskThreadManager::instance();
        ProcessWork& ipc = ProcessWork::instance();
        ProcessWorkDemoCallback callback;
        ProcessWorkConfig config;
        if (!config.m_receiveMemory.setLocalQueueLimitMB(128) || !config.m_postMemory.setLocalQueueLimitMB(-1))
        {
            return 1;
        }
        // 队列节点、正文备用槽和积压额度是三个不同概念。
        config.m_receiveMemory.m_receiveBatchReserve = 4;
        config.m_receiveMemory.m_queuePool.initialCapacity = 64;
        config.m_receiveMemory.m_freeQueuePool.initialCapacity = 64;
        config.m_postMemory.m_postBufferReserve = 64;
        config.m_postMemory.m_queuePool.initialCapacity = 64;
        // 其他池、Map和三条任务线程的参数可通过config相应字段逐项设置。
        // 不设置m_configureManager，避免覆盖同进程其他库的线程管理器配置。
        if (!ipc.configure(config))
        {
            return 2;
        }
        // 单独调整Post参数的入口；与完整configure择一即可，这里仅演示调用顺序。
        if (!ipc.configurePostMemory(config.m_postMemory))
        {
            return 3;
        }
        const int32_t pid = CSystem::currentProcessPid();
        const std::string name = "process_work_demo_" + std::to_string(pid);
        if (!ipc.setReceiveName(name) || !ipc.addProcessReceiveCallback(&callback))
        {
            return 4;
        }
        // 65548含12字节头，正文最大65536；初始128块，遇满后扩64块，空闲60秒缩容。
        // 最后-1是共享扩容份数不设业务上限，不是预先申请无限内存。
        if (!ipc.initReceive(65548, 128, 0, 64, 60, -1, config.m_receiveMemory) || !ipc.initPostThread())
        {
            return 5;
        }
        std::printf("blocks=%d, max payload=%d\n", ipc.receiveBufferCount(), ipc.receivePayloadCapacity());
        const std::string message("123456");
        // send成功：完整正文已经共享提交，但不表示对方业务处理完成。
        if (!ipc.send(pid, message.data(), 6) || !ipc.send(pid, message) ||
            !ipc.send(name, message.data(), 6) || !ipc.send(name, message))
        {
            return 6;
        }
        // post成功：本地副本被接纳，原message可立即修改/释放；后台才执行真实发送。
        if (!ipc.post(pid, message.data(), 6) || !ipc.post(pid, message) ||
            !ipc.post(name, message.data(), 6) || !ipc.post(name, message))
        {
            return 7;
        }
        // 需要具体错误原因时使用Result接口；成功枚举为0，不能当bool判断。
        if (ipc.sendResult(pid, message.data(), 6) != PROCESS_WORK_SENT ||
            ipc.sendResult(name, message.data(), 6) != PROCESS_WORK_SENT ||
            ipc.postResult(pid, message.data(), 6) != PROCESS_WORK_POSTED ||
            ipc.postResult(name, message.data(), 6) != PROCESS_WORK_POSTED)
        {
            return 8;
        }
        // 两个开关分别表示内部重试竞争和容量；默认false,false。
        // 示例选择true,true。业务若选择默认值，失败未提交，可保留该消息稍后重试。
        if (ipc.trySend(pid, message.data(), 6, MessageType::MESSAGE, true, true) != PROCESS_WORK_SENT ||
            ipc.tryPost(name, message.data(), 6, MessageType::MESSAGE, true, true) != PROCESS_WORK_POSTED)
        {
            return 9;
        }
        // 先停止新的post，再排空。这里只完成发送工作，并不等待远端业务回调。
        ipc.waitForPostEnd();
        const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (callback.m_received.load() < 14 && std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        std::printf("post charged bytes=%llu, receive charged bytes=%llu\n",
            static_cast<unsigned long long>(ipc.postPendingBytes()), static_cast<unsigned long long>(ipc.receivePendingBytes()));
        // -1时postPendingBytes返回0表示未计费，不能用它判断排空。
        // 缓存清理可与发送并发，但不是逐包必做的操作。
        ipc.clearDestPid(pid);
        ipc.clearUseless();
        ipc.clear();
        ipc.uninitPostThread();
        ipc.uninitReceive();
        // uninitReceive也清除回调登记；重新启动需要重新登记回调。
        const int32_t received = callback.m_received.load();
        std::printf("received=%d\n", received);
        return received == 14 && !callback.m_bad.load() ? 0 : 10;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "demo setup failed: %s\n", error.what());
        return 11;
    }
    catch (...)
    {
        std::fprintf(stderr, "demo setup failed\n");
        return 12;
    }
}
PROCESS_WORK_USAGE_DEMO_END */
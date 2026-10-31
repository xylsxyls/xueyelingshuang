#include "ProcessWork.h"
#include "ProcessReceiveCallback.h"
#include "CSystem/CSystemAPI.h"
#include "AtomicMath/AtomicMathAPI.h"
#include "ReadTask.h"
#include "ReceiveTask.h"
#include "SendTask.h"
#include <cstring>
#include <cstdio>
#include <limits.h>

ProcessWork::ProcessWork(): m_nameOwned(false), m_thisProcessPid(CSystem::currentProcessPid()),
    m_readThreadId(0), m_receiveThreadId(0), m_postThreadId(0), m_fullCount(0), m_waitEndPost(false)
{
}

ProcessWork& ProcessWork::instance()
{
    static ProcessWork s_processWork;
    return s_processWork;
}

void ProcessWork::addProcessReceiveCallback(ProcessReceiveCallback* callback)
{
    m_callback.push_back(callback);
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
    return "P2N" + std::to_string(value);
}

bool ProcessWork::setReceiveName(const std::string& name)
{
    if (m_channel || m_readThreadId != 0 || m_receiveThreadId != 0 || name.size() >= 1024 || name.find('\0') != std::string::npos)
    {
        return false;
    }
    m_receiveName = name;
    return true;
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
    const int32_t previous = AtomicMath::selfAdd(&directory->m_pid, 0);
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
        (previous < 0 || !previousKnown || previousIdentity == static_cast<uint64_t>(AtomicMath::selfAdd(reinterpret_cast<int64_t*>(&directory->m_identity), static_cast<int64_t>(0))));
    if ((!(previousAlive && previous < 0) && AtomicMath::selfAdd(&directory->m_length, 0) != 0 && (AtomicMath::selfAdd(&directory->m_length, 0) != name.size() ||
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
        AtomicMath::compareAndSwap(identity, AtomicMath::selfAdd(identity, static_cast<int64_t>(0)), static_cast<int64_t>(selfIdentity));
        if (AtomicMath::selfAdd(&directory->m_length, 0) == 0)
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
    if (m_channel || m_readThreadId != 0 || m_receiveThreadId != 0)
    {
        return false;
    }
    // 唯一回滚出口：任何准备失败或异常均先加入借用任务，再销毁映射。
    auto rollback = [](ProcessWork* owner) { owner->releaseReceive(false); };
    std::unique_ptr<ProcessWork, decltype(rollback)> pending(this, rollback);
    try
    {
        m_channel.reset(new ProcessWorkChannel);
        if (!m_channel->openReceiver(receiveSize, areaCount, expandThreshold, expandCount, idleSeconds, maxExpandTimes))
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
        readTask->setParam(m_channel.get(), &m_receiveQueue, &m_receiveSemaphore, &m_fullCount);
        receiveTask->setParam(&m_callback, &m_receiveQueue, &m_receiveSemaphore, &m_fullCount);
        m_readThreadId = CTaskThreadManager::Instance().Init();
        m_receiveThreadId = CTaskThreadManager::Instance().Init();
        std::shared_ptr<CTaskThread> reader = CTaskThreadManager::Instance().GetThreadInterface(m_readThreadId);
        std::shared_ptr<CTaskThread> receiver = CTaskThreadManager::Instance().GetThreadInterface(m_receiveThreadId);
        if (!reader || !receiver)
        {
            return false;
        }
        receiver->PostTask(receiveTask);
        reader->PostTask(readTask);
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
    }
    if (m_readThreadId != 0)
    {
        CTaskThreadManager::Instance().Uninit(m_readThreadId);
        m_readThreadId = 0;
    }
    if (m_receiveThreadId != 0)
    {
        CTaskThreadManager::Instance().Uninit(m_receiveThreadId);
        m_receiveThreadId = 0;
    }
    m_channel.reset();
    m_nameMemory.reset();
    m_nameOwned = false;
    m_fullCount = 0;
    char* packet = nullptr;
    while (m_receiveQueue.pop(&packet))
    {
        delete[] packet;
        packet = nullptr;
    }
    m_receiveSemaphore.reset();
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
    {
        ReadLock lock(m_sendProcessAssignMapMutex);
        auto found = m_sendChannels.find(pid);
        if (found != m_sendChannels.end() && found->second->ready())
        {
            return found->second;
        }
    }
    WriteLock lock(m_sendProcessAssignMapMutex);
    auto found = m_sendChannels.find(pid);
    if (found != m_sendChannels.end())
    {
        if (found->second->ready())
        {
            return found->second;
        }
        m_sendChannels.erase(found);
    }
    std::shared_ptr<ProcessWorkChannel> channel(new ProcessWorkChannel);
    if (!channel->openSender(pid))
    {
        return std::shared_ptr<ProcessWorkChannel>();
    }
    m_sendChannels[pid] = channel;
    return channel;
}

void ProcessWork::send(int32_t destPid, const char* buffer, int32_t length, MessageType type)
{
    if (destPid <= 0 || length < 0 || (length > 0 && buffer == nullptr) || static_cast<int32_t>(type) < 0)
    {
        return;
    }
    sendChannel(sender(destPid), buffer, length, type);
}

void ProcessWork::sendChannel(const std::shared_ptr<ProcessWorkChannel>& channel, const char* buffer, int32_t length, MessageType type)
{
    if (!channel)
    {
        return;
    }
    const std::atomic<bool>* stop = SendTask::currentStopFlag();
    while (stop == nullptr || !stop->load())
    {
        const int32_t result = channel->trySend(buffer, length, static_cast<int32_t>(type));
        if (result != PROCESS_WORK_BUSY)
        {
            return;
        }
        if (!channel->waitForSpace())
        {
            return;
        }
    }
}

int32_t ProcessWork::trySend(int32_t destPid, const char* buffer, int32_t length, MessageType type)
{
    if (destPid <= 0 || length < 0 || (length > 0 && buffer == nullptr) || static_cast<int32_t>(type) < 0)
    {
        return PROCESS_WORK_INVALID;
    }
    try
    {
        std::shared_ptr<ProcessWorkChannel> channel = sender(destPid);
        return channel ? channel->trySend(buffer, length, static_cast<int32_t>(type)) : PROCESS_WORK_UNAVAILABLE;
    }
    catch (...)
    {
        return PROCESS_WORK_BUSY;
    }
}

std::shared_ptr<ProcessWorkChannel> ProcessWork::resolveName(const std::string& name)
{
    if (name.empty() || name.size() >= 1024 || name.find('\0') != std::string::npos)
    {
        return std::shared_ptr<ProcessWorkChannel>();
    }
    std::shared_ptr<SharedMemory> memory;
    {
        ReadLock lock(m_sendNameMemoryMapMutex);
        auto found = m_sendNameMemoryMap.find(name);
        if (found != m_sendNameMemoryMap.end())
        {
            memory = found->second;
        }
    }
    for (int32_t attempt = 0; attempt < 2; ++attempt)
    {
        if (!memory)
        {
            WriteLock lock(m_sendNameMemoryMapMutex);
            auto found = m_sendNameMemoryMap.find(name);
            if (found != m_sendNameMemoryMap.end())
            {
                memory = found->second;
            }
            else
            {
                memory.reset(new SharedMemory(nameKey(name)));
                if (memory->writeWithoutLock() == nullptr || memory->realSize() < sizeof(ProcessWorkNameDirectory))
                {
                    return std::shared_ptr<ProcessWorkChannel>();
                }
                m_sendNameMemoryMap[name] = memory;
            }
        }
        ProcessWorkNameDirectory* directory = static_cast<ProcessWorkNameDirectory*>(memory->memory());
        const int32_t pid = AtomicMath::selfAdd(&directory->m_pid, 0);
        if (pid > 0 && AtomicMath::selfAdd(&directory->m_length, 0) == name.size() && std::memcmp(directory->m_name, name.data(), name.size()) == 0)
        {
            const uint64_t identity = static_cast<uint64_t>(AtomicMath::selfAdd(reinterpret_cast<int64_t*>(&directory->m_identity), static_cast<int64_t>(0)));
            std::shared_ptr<ProcessWorkChannel> channel = sender(pid);
            if (channel && channel->matchesIdentity(identity) && AtomicMath::selfAdd(&directory->m_pid, 0) == pid)
            {
                return channel;
            }
            return std::shared_ptr<ProcessWorkChannel>();
        }
        WriteLock lock(m_sendNameMemoryMapMutex);
        auto found = m_sendNameMemoryMap.find(name);
        if (found != m_sendNameMemoryMap.end() && found->second == memory)
        {
            m_sendNameMemoryMap.erase(found);
        }
        memory.reset();
    }
    return std::shared_ptr<ProcessWorkChannel>();
}

void ProcessWork::send(const std::string& processName, const char* buffer, int32_t length, MessageType type)
{
    if (length < 0 || (length > 0 && buffer == nullptr) || static_cast<int32_t>(type) < 0)
    {
        return;
    }
    // 保留名称核验时的通道；再次按PID查找会跨过关闭/重建或PID复用边界。
    sendChannel(resolveName(processName), buffer, length, type);
}

void ProcessWork::send(int32_t destPid, const std::string& message, MessageType type)
{
    if (message.size() <= INT32_MAX)
    {
        send(destPid, message.data(), static_cast<int32_t>(message.size()), type);
    }
}

void ProcessWork::send(const std::string& name, const std::string& message, MessageType type)
{
    if (message.size() <= INT32_MAX)
    {
        send(name, message.data(), static_cast<int32_t>(message.size()), type);
    }
}

void ProcessWork::enqueue(int32_t pid, const std::string& name, const char* buffer, int32_t length, MessageType type)
{
    if ((pid <= 0 && name.empty()) || name.size() >= 1024 || name.find('\0') != std::string::npos || length < 0 ||
        length > INT32_MAX - 1040 || (length > 0 && buffer == nullptr) || static_cast<int32_t>(type) < 0)
        {
            return;
        }
    std::unique_ptr<char[]> packet(new char[16 + name.size() + static_cast<size_t>(length)]);
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
    m_postQueue.push(packet.get());
    packet.release();
    m_postSemaphore.signal();
}

void ProcessWork::post(int32_t destPid, const char* buffer, int32_t length, MessageType type)
{
    if (destPid > 0)
    {
        enqueue(destPid, std::string(), buffer, length, type);
    }
}

void ProcessWork::post(const std::string& name, const char* buffer, int32_t length, MessageType type)
{
    if (!name.empty())
    {
        enqueue(0, name, buffer, length, type);
    }
}

void ProcessWork::post(int32_t destPid, const std::string& message, MessageType type)
{
    if (message.size() <= INT32_MAX)
    {
        post(destPid, message.data(), static_cast<int32_t>(message.size()), type);
    }
}

void ProcessWork::post(const std::string& name, const std::string& message, MessageType type)
{
    if (message.size() <= INT32_MAX)
    {
        post(name, message.data(), static_cast<int32_t>(message.size()), type);
    }
}

void ProcessWork::initPostThread()
{
    if (m_postThreadId != 0)
    {
        return;
    }
    std::shared_ptr<SendTask> task(new SendTask);
    m_postThreadId = CTaskThreadManager::Instance().Init();
    std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(m_postThreadId);
    if (!thread)
    {
        m_postThreadId = 0;
        return;
    }
    m_waitEndPost = false;
    task->setParam(&m_waitEndPost, &m_postQueue, &m_postSemaphore, false, this);
    try
    {
        thread->PostTask(task);
        task->start();
    }
    catch (...)
    {
        CTaskThreadManager::Instance().Uninit(m_postThreadId);
        m_postThreadId = 0;
        throw;
    }
}

void ProcessWork::uninitPostThread()
{
    if (m_postThreadId != 0)
    {
        CTaskThreadManager::Instance().Uninit(m_postThreadId);
        m_postThreadId = 0;
    }
    m_waitEndPost = false;
    char* packet = nullptr;
    while (m_postQueue.pop(&packet))
    {
        delete[] packet;
        packet = nullptr;
    }
    m_postSemaphore.reset();
}

void ProcessWork::waitForPostEnd()
{
    if (m_postThreadId == 0)
    {
        return;
    }
    m_waitEndPost = true;
    m_postSemaphore.signal();
    CTaskThreadManager::Instance().WaitForEnd(m_postThreadId);
    m_postThreadId = 0;
    initPostThread();
}

void ProcessWork::clearDestPid(int32_t destPid)
{
    WriteLock lock(m_sendProcessAssignMapMutex);
    m_sendChannels.erase(destPid);
}

void ProcessWork::clearUseless()
{
    {
        WriteLock lock(m_sendProcessAssignMapMutex);
        for (auto it = m_sendChannels.begin(); it != m_sendChannels.end();)
        {
            if (!it->second->ready() || CSystem::processDefinitelyExited(it->first))
            {
                it = m_sendChannels.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    WriteLock lock(m_sendNameMemoryMapMutex);
    for (auto it = m_sendNameMemoryMap.begin(); it != m_sendNameMemoryMap.end();)
    {
        ProcessWorkNameDirectory* directory = static_cast<ProcessWorkNameDirectory*>(it->second->memory());
        const int32_t pid = directory == nullptr ? 0 : AtomicMath::selfAdd(&directory->m_pid, 0);
        if (pid <= 0 || CSystem::processDefinitelyExited(pid))
        {
            it = m_sendNameMemoryMap.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ProcessWork::clear()
{
    { WriteLock lock(m_sendProcessAssignMapMutex); m_sendChannels.clear(); }
    { WriteLock lock(m_sendNameMemoryMapMutex); m_sendNameMemoryMap.clear(); }
}

int32_t ProcessWork::receiveBufferCount() const
{
    return m_channel ? m_channel->bufferCount() : 0;
}

int32_t ProcessWork::receivePayloadCapacity() const
{
    return m_channel ? m_channel->payloadCapacity() : 0;
}

// PROCESS_WORK_V2_TESTS_BEGIN
//#include <set>
//#include <chrono>
//#include <cstdlib>
//#include <iostream>
//#include <algorithm>
//#include <functional>
//#include <vector>
//#include <ctime>
//#include <stdexcept>
//#include <new>
//#include <type_traits>
//#include <cerrno>
//#include <string>
//#include <fstream>
//#include <thread>
//#if defined(__linux__)
//#include <pthread.h>
//#include <fcntl.h>
//#include <sys/wait.h>
//#include <unistd.h>
//#include <signal.h>
//#endif
//
//// Faults replace only OS results, leaving the production state machine unchanged.
//static std::atomic<uint32_t> s_testUnknownPid(0);
//static std::atomic<int32_t> s_testUnknownIdentityHits(0), s_testUnknownExitedHits(0);
//static std::atomic<int32_t> s_testSignalFault(0), s_testSignalHits(0);
//
///** Inject an unknown identity only for the PID selected by main.
//@param [in] pid Queried process
//@param [out] identity Actual identity, or zero for an unknown result
//@return Whether the identity was obtained
//*/
//bool ProcessWorkTestChannelIdentity(uint32_t pid, uint64_t& identity)
//{
//#ifdef PROCESS_WORK_TEST_ALLOCATION
//    if (pid != 0 && pid == s_testUnknownPid.load())
//    {
//        ++s_testUnknownIdentityHits;
//        identity = 0;
//        return false;
//    }
//#endif
//    return CSystem::processIdentity(pid, identity);
//}
//
///** Preserve the distinction between an unknown process and a confirmed exit.
//@param [in] pid Queried process
//@return Actual exit result, or false while main selects an unknown result
//*/
//bool ProcessWorkTestChannelExited(uint32_t pid)
//{
//#ifdef PROCESS_WORK_TEST_ALLOCATION
//    if (pid != 0 && pid == s_testUnknownPid.load())
//    {
//        ++s_testUnknownExitedHits;
//        return false;
//    }
//#endif
//    return CSystem::processDefinitelyExited(pid);
//}
//
///** Suppress the OS notification only while main selects a fault mode.
//@param [in] notification Real notification object
//@return The real result, or the selected error/saturation/lost-hint result
//*/
//int32_t ProcessWorkTestChannelSignal(Semaphore& notification)
//{
//#ifdef PROCESS_WORK_TEST_ALLOCATION
//    const int32_t fault = s_testSignalFault.load();
//    if (fault != 0)
//    {
//        ++s_testSignalHits;
//        // Model an OS error, saturation, or successful but lost hint.
//        return fault == 1 ? -1 : (fault == 2 ? 1 : 0);
//    }
//#endif
//    return notification.tryProcessSignal();
//}
//
//// Exact production Linux stat parser extracted by the test builder.
//bool ProcessWorkTestLinuxStatExited(const std::string& line);
//bool ProcessWorkTestLinuxStatBaseline(const std::string& line);
//bool ProcessWorkTestLinuxIdentity(const std::string& record, uint64_t& identity, bool readError);
//bool ProcessWorkTestLinuxIdentityBaseline(const std::string& record, uint64_t& identity, bool readError);
//std::string ProcessWorkTestWindowsGetCurrentExeName(const std::string& path, bool failed);
//std::string ProcessWorkTestWindowsGetCurrentExeFullName(const std::string& path, bool failed);
//std::string ProcessWorkTestLinuxGetCurrentExeName(const std::string& path, bool failed);
//std::string ProcessWorkTestLinuxGetCurrentExeFullName(const std::string& path, bool failed);
//std::string ProcessWorkTestWindowsGetCurrentExeNameBaseline(const std::string& path, bool failed);
//std::string ProcessWorkTestWindowsGetCurrentExeFullNameBaseline(const std::string& path, bool failed);
//std::string ProcessWorkTestLinuxGetCurrentExeNameBaseline(const std::string& path, bool failed);
//std::string ProcessWorkTestLinuxGetCurrentExeFullNameBaseline(const std::string& path, bool failed);
//#if defined(__APPLE__) && !TARGET_OS_IPHONE
//#include <sys/sysctl.h>
//#include <sys/proc.h>
//#include <sys/wait.h>
//#include <unistd.h>
//#endif
//#if defined(__linux__) && !defined(__ANDROID__)
//#include <sys/statvfs.h>
//#endif
//#include "NetSender/NetSenderAPI.h"
//#ifdef _WIN32
//#include <Windows.h>
//#include <Psapi.h>
//#ifdef PROCESS_WORK_PORTABLE_TEST
//#include <mmsystem.h>
//#pragma comment(lib, "winmm.lib")
//#endif
//#endif
//
//// 仅关闭的测试入口使用此分配适配；main选定目标CTask和失败模式，生产构建无替换分配器。
//#ifdef PROCESS_WORK_TEST_ALLOCATION
//static std::atomic<uint32_t> s_testCommitThread(0);
//static std::atomic<int32_t> s_testCommitPhase(0), s_testCommitReached(0);
//static std::atomic<bool> s_testCommitRelease(false), s_testCommitTrace(false), s_testCommitExpired(false);
//
///** 只暂停指定发送线程的实际提交边界；不修改消息、游标或缓冲区状态。
//@param [in] phase 1正文复制完成尚未提交，2提交完成尚未帮助推进尾游标
//*/
//void ProcessWorkTestCommitBoundary(int32_t phase)
//{
//#ifdef _WIN32
//    if (::GetCurrentThreadId() == s_testCommitThread.load() && phase == s_testCommitPhase.load())
//    {
//        s_testCommitPhase = 0;
//        s_testCommitReached = phase;
//        if (s_testCommitTrace) { std::printf("BOUNDARY %d\n", phase); }
//        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(45);
//        while (!s_testCommitRelease && std::chrono::steady_clock::now() < deadline) { ::Sleep(1); }
//        if (!s_testCommitRelease) { s_testCommitExpired = true; }
//    }
//#endif
//}
//
//static std::atomic<uint32_t> s_testAllocationThread(0);
//static std::atomic<int32_t> s_testAllocationMode(0);
//static std::atomic<int32_t> s_testAllocationHits(0);
//static std::atomic<bool> s_testAllocationPaused(false);
//static std::atomic<bool> s_testAllocationRelease(false);
//static size_t s_testAllocationSize = 0;
//static std::atomic<int32_t> s_testAllocationRemaining(0);
//static std::atomic<int32_t> s_testAllocationAttempts(0);
//
///** 对指定测试执行者注入可恢复分配失败，其他线程照常分配。
//@param [in] size 申请字节
//@param [in] array 是否来自new[]
//@return 分配地址；注入时抛bad_alloc供真实代码恢复
//*/
//static void* TestAllocate(size_t size, bool array)
//{
//#ifdef _WIN32
//    const int32_t mode = s_testAllocationMode.load();
//    if (mode == 6 && ::GetCurrentThreadId() == s_testAllocationThread.load())
//    {
//        ++s_testAllocationAttempts;
//        if (--s_testAllocationRemaining == 0)
//        {
//            s_testAllocationMode = 0;
//            ++s_testAllocationHits;
//            throw std::bad_alloc();
//        }
//    }
//    if (mode == 4 && array && ::GetCurrentThreadId() == s_testAllocationThread.load())
//    {
//        s_testAllocationMode = 0;
//        ++s_testAllocationHits;
//        ::Sleep(1200);
//    }
//    if (mode == 5 && !array && size == s_testAllocationSize && ::GetCurrentThreadId() == s_testAllocationThread.load())
//    {
//        s_testAllocationMode = 0;
//        ++s_testAllocationHits;
//        s_testAllocationPaused = true;
//        while (!s_testAllocationRelease) { ::Sleep(1); }
//    }
//    if (mode == 3 && ::GetCurrentThreadId() == s_testAllocationThread.load())
//    {
//        ++s_testAllocationHits;
//        ::Sleep(2);
//    }
//    if (mode != 0 && ::GetCurrentThreadId() == s_testAllocationThread.load() &&
//        ((mode == 1 && array) || (mode == 2 && !array)))
//    {
//        ++s_testAllocationHits;
//        throw std::bad_alloc();
//    }
//#endif
//    void* result = std::malloc(size == 0 ? 1 : size);
//    if (result == nullptr) { throw std::bad_alloc(); }
//    return result;
//}
//
///** 测试标量分配入口。
//@param [in] size 字节数
//@return 由malloc取得的地址
//*/
//void* operator new(size_t size) { return TestAllocate(size, false); }
///** 测试数组分配入口。
//@param [in] size 字节数
//@return 由malloc取得的地址
//*/
//void* operator new[](size_t size) { return TestAllocate(size, true); }
///** 释放与测试分配器配对的标量内存。
//@param [in] memory 地址，可空
//*/
//void operator delete(void* memory) throw() { std::free(memory); }
///** 释放与测试分配器配对的数组内存。
//@param [in] memory 地址，可空
//*/
//void operator delete[](void* memory) throw() { std::free(memory); }
//
///** Pause at the semantic openSender boundary, independent of container representation. */
//void ProcessWorkTestSenderBoundary()
//{
//#ifdef _WIN32
//    if (s_testAllocationMode == 7 && ::GetCurrentThreadId() == s_testAllocationThread.load())
//    {
//        s_testAllocationMode = 0;
//        ++s_testAllocationHits;
//        s_testAllocationPaused = true;
//        while (!s_testAllocationRelease) { ::Sleep(1); }
//    }
//#endif
//}
//#endif
//
//#ifdef PROCESS_WORK_TEST_ALLOCATION
//static std::atomic<bool> s_testNamePause(false), s_testNameReached(false), s_testNameRelease(false);
//static std::atomic<bool> s_testSelfIdentityFail(false);
//static std::atomic<int32_t> s_testSelfIdentityHits(0);
//
///** 测试构建在已核对名称/身份、返回解析结果前暂停；状态调度和判定均由main执行。 */
//void ProcessWorkTestNameBoundary()
//{
//    if (s_testNamePause)
//    {
//        s_testNameReached = true;
//        while (!s_testNameRelease) { CSystem::Sleep(1); }
//    }
//}
//#endif
//
///** Replace only the second self-identity query during receiver initialization in test builds. */
//bool ProcessWorkTestSelfIdentity(uint32_t pid, uint64_t& identity)
//{
//#ifdef PROCESS_WORK_TEST_ALLOCATION
//    if (s_testSelfIdentityFail.exchange(false))
//    {
//        ++s_testSelfIdentityHits;
//        identity = 0;
//        return false;
//    }
//#endif
//    return CSystem::processIdentity(pid, identity);
//}
//
//#ifdef PROCESS_WORK_TEST_ALLOCATION
//void ProcessWorkTestSemaphoreAccess::set(Semaphore& semaphore, uint64_t count)
//{
//    std::unique_lock<std::mutex> lock(semaphore.m_mtx);
//    semaphore.m_count = static_cast<decltype(semaphore.m_count)>(count);
//}
//
//uint64_t ProcessWorkTestSemaphoreAccess::count(Semaphore& semaphore)
//{
//    std::unique_lock<std::mutex> lock(semaphore.m_mtx);
//    return static_cast<uint64_t>(semaphore.m_count);
//}
//
//static std::atomic<uint32_t> s_testQueueThread(0);
//static std::atomic<bool> s_testQueueReached(false);
//static std::atomic<bool> s_testQueueRelease(false);
//static std::atomic<uintptr_t> s_testQueueAddress(0);
//
///** 暂停指定生产者在真实tail.exchange之后的间隙；不改队列内容。
//@param [in] queue 正在入队的实际队列地址
//*/
//void ProcessWorkTestQueueBoundary(const void* queue)
//{
//#ifdef _WIN32
//    if (reinterpret_cast<uintptr_t>(queue) == s_testQueueAddress.load() && ::GetCurrentThreadId() == s_testQueueThread.load())
//    {
//        s_testQueueThread = 0;
//        s_testQueueReached = true;
//        while (!s_testQueueRelease) { ::Sleep(1); }
//    }
//#endif
//}
//#endif
//
//#ifdef PROCESS_WORK_TEST_ALLOCATION
//void ProcessWorkTestQueueCountAccess::set(LockFreeQueue<int32_t>& queue, uint64_t count)
//{
//    queue.m_count.store(static_cast<decltype(queue.m_count.load())>(count));
//}
//
//uint64_t ProcessWorkTestQueueCountAccess::count(const LockFreeQueue<int32_t>& queue)
//{
//    return static_cast<uint64_t>(queue.m_count.load());
//}
//#endif
//
//#ifdef PROCESS_WORK_TEST_ALLOCATION
//void ProcessWorkTestTaskIdAccess::set(CTaskThreadManager& manager, uint32_t count)
//{
//    manager.m_threadId.store(count);
//}
//
//uint32_t ProcessWorkTestTaskIdAccess::count(const CTaskThreadManager& manager)
//{
//    return manager.m_threadId.load();
//}
//#endif
//
//#include "NetSender/NetSenderAPI.h"
//#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
//#include "SharedMemory/SharedMemoryAPI.h"
//#include "AtomicMath/AtomicMathAPI.h"
//#include "CSystem/CSystemAPI.h"
//#include "NetInterface/NetInterfaceTypes.h"
//#include <chrono>
//#include <cstring>
//#include <sstream>
//#include <stdexcept>
//#include <vector>
//#include <memory>
//#ifdef _WIN32
//#include <windows.h>
//#endif
//
///** 带统一取消/时限的独立断言
//@param [in] expected 预期条件
//@param [in] detail 失败说明
//*/
//static void CheckIpc(bool expected, const char* detail)
//{
//    netTestCheckpoint();
//    if (!expected)
//    {
//        throw std::runtime_error(detail);
//    }
//}
//
//void NetInterfaceIpcTestCases::localPackets()
//{
//    const uint8_t body[] = { 0, 0xff, 0x80 };
//    NetSenderLocalPacketView packet = {};
//    packet.m_kind = NET_LOCAL_OPEN;
//    packet.m_packet = UINT64_C(0xfedcba9876543210);
//    packet.m_source[0] = 1;
//    packet.m_data = body;
//    packet.m_size = sizeof(body);
//    uint8_t encoded[60] = {};
//    uint32_t required = 0;
//    CheckIpc(NetSenderLocalPacket::encode(packet, encoded + 1, 59, required) == NI_OK && required == 59,
//        "local TLS packet encode");
//    const uint8_t prefix[] = { 'N', 'I', 'P', '2', 0, 0, 0, 1,
//        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10, 1 };
//    CheckIpc(std::memcmp(encoded + 1, prefix, sizeof(prefix)) == 0 && encoded[52] == 3 &&
//        encoded[57] == 0 && encoded[58] == 0xff && encoded[59] == 0x80,
//        "independent local header/body vector");
//    NetSenderLocalPacketView view = {};
//    CheckIpc(NetSenderLocalPacket::parse(encoded + 1, 59, view) == NI_OK &&
//        view.m_packet == packet.m_packet && view.m_size == sizeof(body) &&
//        std::memcmp(view.m_data, body, sizeof(body)) == 0 &&
//        NetSenderLocalPacket::sameGeneration(view.m_source, packet.m_source), "unaligned local packet parse");
//    for (uint32_t length = 0; length < 59; ++length)
//    {
//        CheckIpc(NetSenderLocalPacket::parse(encoded + 1, length, view) == NI_PROTOCOL_ERROR &&
//            view.m_packet == 0 && view.m_data == nullptr, "truncated local packet accepted");
//    }
//    encoded[56] = 1;
//    CheckIpc(NetSenderLocalPacket::parse(encoded + 1, 59, view) == NI_PROTOCOL_ERROR, "local reserved bit accepted");
//    encoded[56] = 0;
//    encoded[33] = 1;
//    CheckIpc(NetSenderLocalPacket::parse(encoded + 1, 59, view) == NI_PROTOCOL_ERROR, "OPEN named an old target");
//    encoded[33] = 0;
//    packet.m_kind = NET_LOCAL_DATA;
//    CheckIpc(NetSenderLocalPacket::encode(packet, encoded, sizeof(encoded), required) == NI_INVALID_ARGUMENT,
//        "DATA missing destination generation");
//    packet.m_target[0] = 2;
//    packet.m_size = 16385;
//    CheckIpc(NetSenderLocalPacket::encode(packet, encoded, sizeof(encoded), required) == NI_INVALID_ARGUMENT &&
//        required == 0, "oversized IPC ciphertext");
//    packet.m_size = 3;
//    encoded[0] = 0x55;
//    CheckIpc(NetSenderLocalPacket::encode(packet, encoded, 58, required) == NI_BUFFER_TOO_SMALL &&
//        required == 59 && encoded[0] == 0x55, "short local output mutated");
//    packet.m_kind = NET_LOCAL_ACK;
//    packet.m_size = 0;
//    packet.m_data = nullptr;
//    CheckIpc(NetSenderLocalPacket::encode(packet, encoded, sizeof(encoded), required) == NI_OK &&
//        required == 56 && NetSenderLocalPacket::parse(encoded, required, view) == NI_OK,
//        "local ACK frame");
//    packet.m_kind = NET_LOCAL_END;
//    CheckIpc(NetSenderLocalPacket::encode(packet, encoded, sizeof(encoded), required) == NI_INVALID_ARGUMENT,
//        "END with packet number");
//    packet.m_packet = 0;
//    CheckIpc(NetSenderLocalPacket::encode(packet, encoded, sizeof(encoded), required) == NI_OK,
//        "local transport EOF frame");
//    NetSenderLocalWindow window;
//    for (uint64_t number = 1; number <= 4; ++number)
//    {
//        CheckIpc(window.next() == number && window.published(number) == NI_OK, "local send window admission");
//    }
//    CheckIpc(window.outstanding() == 4 && window.next() == 0 && window.published(5) == NI_INVALID_STATE &&
//        window.acknowledge(5) == NI_PROTOCOL_ERROR && window.outstanding() == 4, "local window overflow");
//    CheckIpc(window.acknowledge(2) == NI_OK && window.published(5) == NI_OK && window.published(6) == NI_OK &&
//        window.acknowledge(2) == NI_OK && window.outstanding() == 4 && window.acknowledge(6) == NI_OK &&
//        window.outstanding() == 0 && window.next() == 7, "cumulative/duplicate ACK released wrong quota");
//    CheckIpc(window.received(0) == NI_PROTOCOL_ERROR && window.received(1) == NI_OK &&
//        window.received(1) == NI_NOT_FOUND && window.received(3) == NI_PROTOCOL_ERROR &&
//        window.received(2) == NI_OK && window.receipt() == 2, "local receive sequence replay/gap");
//}
//
///** 构造当前测试拥有的对象名
//@param [in] prefix 固定前缀
//@param [in] pid 对应进程号
//@param [in] block 可选缓存号
//@return 对象名
//*/
//static std::string TestIpcName(const char* prefix, int32_t pid, int32_t block = 0)
//{
//    std::ostringstream result;
//    result << prefix << pid;
//    if (block != 0)
//    {
//        result << '_' << block;
//    }
//    return result.str();
//}
//
//void NetInterfaceIpcTestCases::notifications()
//{
//    const std::string name = TestIpcName("NetWorkTest_notify_", CSystem::currentProcessPid());
//    Semaphore owner;
//    Semaphore peer;
//    CheckIpc(!peer.tryOpenProcessSemaphore(name) && peer.tryProcessSignal() < 0,
//        "missing semaphore must not be created");
//    owner.createProcessSemaphore(name, 1);
//    CheckIpc(peer.tryOpenProcessSemaphore(name), "open existing semaphore");
//    CheckIpc(peer.tryProcessSignal() == 0, "publish notification");
//#ifdef _WIN32
//    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
//    for (int index = 0; index < 256; ++index)
//    {
//        CheckIpc(peer.tryProcessSignal() == 1, "full count must return busy without duplicate");
//    }
//    CheckIpc(std::chrono::steady_clock::now() - start < std::chrono::seconds(1), "full semaphore blocked");
//#endif
//    CheckIpc(owner.processWait(0) && !owner.processWait(0), "exactly one notification");
//    peer.processSignal();
//    CheckIpc(owner.processWait(0), "legacy signal compatibility");
//    CheckIpc(peer.tryProcessSignal() == 0 && owner.processWait(0), "notification after draining");
//    peer.closeProcessSemaphore();
//    CheckIpc(peer.tryProcessSignal() < 0, "signal on closed object");
//}
//
//void NetInterfaceIpcTestCases::stalledReceiver()
//{
//    const int32_t pid = CSystem::currentProcessPid();
//    ProcessWorkChannel receiver;
//    CheckIpc(receiver.openReceiver(4096, 2, 0, 0, 0, 0), "create bounded v2 receiver");
//    receiver.publish();
//    LockFreeQueue<char*> packets;
//    Semaphore delivered;
//    std::atomic<int32_t> full(0);
//    NetSenderIpc sender;
//    CheckIpc(sender.open(pid, pid, 8192) == NET_SENDER_IPC_TOO_LARGE, "open refuses unsupported declared payload");
//    CheckIpc(sender.open(pid, pid, 4084) == NET_SENDER_IPC_ACCEPTED, "open stalled v2 receiver");
//    receiver.pump(packets, delivered, full);
//    CheckIpc(sender.trySend(nullptr, 1, 0) == NET_SENDER_IPC_INVALID &&
//        sender.trySend(nullptr, 0, -1) == NET_SENDER_IPC_INVALID, "IPC input validation");
//    std::vector<uint8_t> tooLarge(8192, 17);
//    CheckIpc(sender.trySend(tooLarge.data(), 8192, 0) == NET_SENDER_IPC_TOO_LARGE,
//        "declared capacity rejection does not reserve a block");
//    CheckIpc(sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_ACCEPTED &&
//        sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_ACCEPTED, "both available blocks committed");
//    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
//    for (int32_t index = 0; index < 512; ++index)
//    {
//        CheckIpc(sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_BUSY, "full receiver remains bounded");
//    }
//    CheckIpc(std::chrono::steady_clock::now() - start < std::chrono::seconds(1), "stalled receiver blocked sender");
//    CheckIpc(receiver.pump(packets, delivered, full) == 2, "drain two committed packets");
//    for (int32_t index = 0; index < 2; ++index)
//    {
//        char* packet = nullptr;
//        CheckIpc(packets.pop(&packet) && packet != nullptr, "copied empty packet exists");
//        std::unique_ptr<char[]> owned(packet);
//        int32_t header[3] = {};
//        std::memcpy(header, packet, sizeof(header));
//        CheckIpc(header[0] == pid && header[1] == 0 && header[2] == 0, "empty packet header exact");
//    }
//    CheckIpc(packets.empty(), "no duplicate from busy attempts");
//    sender.close();
//    receiver.close();
//    // 只有旧协议对象存在时不能假装已打开v2接收器。
//    SharedMemory legacy(TestIpcName("ProcessAreaAssign_", pid), 20, false);
//    CheckIpc(sender.open(pid, pid, 100) == NET_SENDER_IPC_UNAVAILABLE, "legacy protocol rejected explicitly");
//}
//
//void NetInterfaceIpcTestCases::publication()
//{
//    const int32_t pid = CSystem::currentProcessPid();
//    ProcessWorkChannel receiver;
//    CheckIpc(receiver.openReceiver(4096, 1, 0, 0, 0, 0), "create publication receiver");
//    receiver.publish();
//    LockFreeQueue<char*> packets;
//    Semaphore delivered;
//    std::atomic<int32_t> full(0);
//    NetSenderIpc sender;
//    CheckIpc(sender.open(pid, pid, 4084) == NET_SENDER_IPC_ACCEPTED, "open v2 publication receiver");
//    receiver.pump(packets, delivered, full);
//    const uint8_t payload[] = { 0, 255, 128, 7, 0 };
//    CheckIpc(sender.trySend(payload, sizeof(payload), 42) == NET_SENDER_IPC_ACCEPTED, "publish binary payload");
//    CheckIpc(sender.pendingNotifications() == 0 && sender.flushNotifications() == NET_SENDER_IPC_ACCEPTED,
//        "v2 notification hint has no duplicate payload retry");
//    CheckIpc(sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_BUSY, "published block cannot be overwritten");
//    sender.close();
//    CheckIpc(receiver.pump(packets, delivered, full) == 1, "sender close preserves committed message");
//    char* packet = nullptr;
//    CheckIpc(packets.pop(&packet) && packet != nullptr, "published packet delivered");
//    std::unique_ptr<char[]> owned(packet);
//    const int32_t expected[] = { pid, 42, sizeof(payload) };
//    CheckIpc(std::memcmp(packet, expected, 12) == 0 && std::memcmp(packet + 12, payload, sizeof(payload)) == 0,
//        "v2 local packet header and binary body exact");
//    CheckIpc(packets.empty(), "one publication delivered exactly once");
//    CheckIpc(sender.open(pid, pid, 4084) == NET_SENDER_IPC_ACCEPTED &&
//        sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_ACCEPTED, "sender resumes after receiver releases block");
//    CheckIpc(receiver.pump(packets, delivered, full) == 1, "empty body delivered");
//    packet = nullptr;
//    CheckIpc(packets.pop(&packet) && packet != nullptr, "empty body copied");
//    owned.reset(packet);
//    int32_t header[3] = {};
//    std::memcpy(header, packet, sizeof(header));
//    CheckIpc(header[0] == pid && header[1] == 0 && header[2] == 0 && packets.empty(), "empty body exact and no duplicates");
//}
//
///** 可独立重建的测试长度，覆盖空消息和128KiB跨页消息
//@param [in] index 消息序号0至39
//@return 消息字节数
//*/
//static int32_t IpcPayloadSize(int32_t index)
//{
//    return index == 0 ? 0 : (index == 39 ? 131072 : index * 97 + 1);
//}
//
//NetInterfaceIpcReceiver::NetInterfaceIpcReceiver(int32_t parentPid) :
//m_count(0), m_failed(false), m_parentPid(parentPid)
//{
//
//}
//
//void NetInterfaceIpcReceiver::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
//{
//    const int32_t index = m_count.load();
//    if (sendPid != m_parentPid || index >= 40 || length != IpcPayloadSize(index) ||
//        static_cast<int32_t>(type) != 42 || (length != 0 && buffer == nullptr))
//    {
//        m_failed.store(true);
//    }
//    else
//    {
//        for (int32_t offset = 0; offset < length; ++offset)
//        {
//            if (static_cast<uint8_t>(buffer[offset]) != static_cast<uint8_t>(index + offset * 13))
//            {
//                m_failed.store(true);
//                break;
//            }
//        }
//    }
//    if (m_count.fetch_add(1) + 1 >= 40 || m_failed.load())
//    {
//        m_done.signal();
//    }
//}
//
//#ifdef __APPLE__
//void NetInterfaceIpcTestCases::appleResources()
//{
//    const int32_t pid = CSystem::currentProcessPid();
//    const std::string name = TestIpcName("PTA", pid);
//    CheckIpc(!CSystem::processName(pid).empty(), "macOS live PID lookup");
//    {
//        SharedMemory owner(name, 8192, false);
//        CheckIpc(!owner.isFailed() && owner.realSize() == 8192, "Apple create exact capacity");
//        SharedMemory exclusive(name, 8192, false);
//        SharedMemory oversized(name, 16384, true);
//        CheckIpc(exclusive.isFailed() && oversized.isFailed(), "active mapping replaced or resized");
//        uint8_t* writer = static_cast<uint8_t*>(owner.writeWithoutLock());
//        CheckIpc(writer != nullptr, "map Apple writer");
//        writer[8191] = 193;
//        SharedMemory peer(name);
//        uint8_t* reader = static_cast<uint8_t*>(peer.readWithoutLock());
//        CheckIpc(reader && reader[8191] == 193 && peer.realSize() == 8192, "shared mapping contents");
//        peer.close();
//        reader = static_cast<uint8_t*>(peer.writeWithoutLock());
//        CheckIpc(reader && reader[8191] == 193, "unmap/remap retained native shared object");
//        reader[0] = 91;
//        CheckIpc(writer[0] == 91, "bidirectional shared visibility");
//    }
//    SharedMemory absent(name);
//    CheckIpc(absent.writeWithoutLock() == nullptr, "last mapping left an attachable stale object");
//    SharedMemory replacement(name, 4096, false);
//    CheckIpc(!replacement.isFailed() && replacement.realSize() == 4096, "new mapping generation");
//    SharedMemory bad("../outside", 4096, false);
//    CheckIpc(bad.isFailed(), "mapping escaped scope");
//    SharedMemory anonymous("", 4096, false);
//    CheckIpc(!anonymous.isFailed() && anonymous.writeWithoutLock(), "anonymous mapping");
//    int32_t value = 3;
//    int64_t wide = INT64_C(0x100000001);
//    CheckIpc(AtomicMath::selfAddOne(&value) == 4 && AtomicMath::selfSubOne(&value) == 3 &&
//        AtomicMath::selfAdd(&value, 7) == 10 && AtomicMath::selfSub(&value, 2) == 8 &&
//        !AtomicMath::compareAndSwap(&value, 7, 20) && AtomicMath::compareAndSwap(&value, 8, 20), "Apple int32 atomics");
//    CheckIpc(AtomicMath::selfAddOne(&wide) == INT64_C(0x100000002) &&
//        AtomicMath::selfSubOne(&wide) == INT64_C(0x100000001) &&
//        AtomicMath::selfAdd(&wide, INT64_C(9)) == INT64_C(0x10000000A) &&
//        AtomicMath::selfSub(&wide, INT64_C(2)) == INT64_C(0x100000008) &&
//        AtomicMath::compareAndSwap(&wide, INT64_C(0x100000008), INT64_C(0x200000000)), "Apple int64 atomics");
//}
//#endif
//
//#ifdef _WIN32
//int NetInterfaceIpcTestCases::childMain(int32_t parentPid)
//{
//    NetInterfaceIpcReceiver receiver(parentPid);
//    ProcessWork& work = ProcessWork::instance();
//    work.addProcessReceiveCallback(&receiver);
//    if (!work.initReceive(131072 + 12, 4))
//    {
//        work.uninitReceive();
//        return 10;
//    }
//    Semaphore ready;
//    const bool opened = ready.tryOpenProcessSemaphore(TestIpcName("NetWorkTest_child_ready_", parentPid));
//    const bool announced = opened && ready.tryProcessSignal() == 0;
//    const bool done = announced && receiver.m_done.wait(10000);
//    work.uninitReceive();
//    return done && receiver.m_count.load() == 40 && !receiver.m_failed.load() ? 0 : 11;
//}
//
//void NetInterfaceIpcTestCases::childTransfer()
//{
//    const int32_t pid = CSystem::currentProcessPid();
//    Semaphore ready;
//    ready.createProcessSemaphore(TestIpcName("NetWorkTest_child_ready_", pid), 1);
//    wchar_t path[32768] = {};
//    const DWORD length = GetModuleFileNameW(nullptr, path, 32768);
//    CheckIpc(length > 0 && length < 32768, "test executable path");
//    std::wostringstream arguments;
//    arguments << L'"' << path << L"\" --ipc-child=" << pid;
//    std::wstring command = arguments.str();
//    STARTUPINFOW startup = {};
//    startup.cb = sizeof(startup);
//    startup.dwFlags = STARTF_USESHOWWINDOW;
//    startup.wShowWindow = SW_HIDE;
//    PROCESS_INFORMATION child = {};
//    netTestCheckpoint();
//    if (!CreateProcessW(path, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
//        nullptr, nullptr, &startup, &child))
//    {
//        throw std::runtime_error("start owned IPC child");
//    }
//    CloseHandle(child.hThread);
//    try
//    {
//        while (!ready.processWait(10))
//        {
//            CheckIpc(WaitForSingleObject(child.hProcess, 0) == WAIT_TIMEOUT, "IPC child exited during startup");
//        }
//        NetSenderIpc sender;
//        CheckIpc(sender.open(static_cast<int32_t>(child.dwProcessId), pid, 131072) == NET_SENDER_IPC_ACCEPTED,
//            "open real child ProcessWork");
//        for (int32_t index = 0; index < 40; ++index)
//        {
//            std::vector<uint8_t> payload(IpcPayloadSize(index));
//            for (size_t offset = 0; offset < payload.size(); ++offset)
//            {
//                payload[offset] = static_cast<uint8_t>(index + offset * 13);
//            }
//            int32_t result;
//            do
//            {
//                netTestCheckpoint();
//                result = sender.trySend(payload.empty() ? nullptr : &payload[0],
//                    static_cast<uint32_t>(payload.size()), 42);
//                if (result == NET_SENDER_IPC_BUSY)
//                {
//                    Sleep(1);
//                }
//            }
//            while (result == NET_SENDER_IPC_BUSY);
//            CheckIpc(result == NET_SENDER_IPC_ACCEPTED, "real child send failed");
//        }
//        while (WaitForSingleObject(child.hProcess, 10) == WAIT_TIMEOUT)
//        {
//            netTestCheckpoint();
//            // 接收者收到40包后可以先撤销共享入口再退出；不把正常收尾当作通知失败。
//            sender.flushNotifications();
//        }
//        DWORD exitCode = 0;
//        CheckIpc(GetExitCodeProcess(child.hProcess, &exitCode) && exitCode == 0, "real ProcessWork child rejected transfer");
//    }
//    catch (...)
//    {
//        // 只终止本用例创建的子进程；绝不按名字查找/结束外部程序。
//        TerminateProcess(child.hProcess, 12);
//        WaitForSingleObject(child.hProcess, 2000);
//        CloseHandle(child.hProcess);
//        throw;
//    }
//    CloseHandle(child.hProcess);
//}
//#endif
//
//NetInterfaceIpcAuditReceiver::NetInterfaceIpcAuditReceiver(bool fail) :
//m_throw(fail), m_count(0), m_bad(false)
//{
//}
//
//void NetInterfaceIpcAuditReceiver::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
//{
//    if (sendPid != static_cast<int32_t>(CSystem::currentProcessPid()) || length != 4 || buffer == nullptr ||
//        std::memcmp(buffer, "TEST", 4) != 0 || static_cast<int32_t>(type) != 42)
//    {
//        m_bad = true;
//    }
//    ++m_count;
//    if (m_throw)
//    {
//        throw std::runtime_error("controlled IPC callback failure");
//    }
//}
//
//#ifdef _WIN32
//void NetInterfaceIpcTestCases::runAuditChild(const std::wstring& mode)
//{
//    wchar_t path[32768] = {};
//    const DWORD size = GetModuleFileNameW(nullptr, path, 32768);
//    CheckIpc(size > 0 && size < 32768, "audit executable path");
//    std::wstring command = L"\"" + std::wstring(path) + L"\" --ipc-audit=" + mode;
//    STARTUPINFOW startup = {};
//    startup.cb = sizeof(startup);
//    startup.dwFlags = STARTF_USESHOWWINDOW;
//    startup.wShowWindow = SW_HIDE;
//    PROCESS_INFORMATION child = {};
//    CheckIpc(CreateProcessW(path, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
//        nullptr, nullptr, &startup, &child) != FALSE, "start owned IPC audit child");
//    CloseHandle(child.hThread);
//    const DWORD waited = WaitForSingleObject(child.hProcess, mode == L"read" ? 10000 : 5000);
//    DWORD code = 0;
//    const bool exited = waited == WAIT_OBJECT_0 && GetExitCodeProcess(child.hProcess, &code) != FALSE;
//    if (!exited)
//    {
//        TerminateProcess(child.hProcess, 98);
//        WaitForSingleObject(child.hProcess, 2000);
//    }
//    CloseHandle(child.hProcess);
//    std::printf("IPC_AUDIT mode=%ls exited=%u code=%u\n", mode.c_str(), exited ? 1 : 0, static_cast<uint32_t>(code));
//    CheckIpc(exited && code == 0, "owned ProcessWork audit child failed or timed out");
//}
//
//void NetInterfaceIpcTestCases::callbackIsolation()
//{
//    runAuditChild(L"callback");
//}
//
//void NetInterfaceIpcTestCases::readSlotIsolation()
//{
//    runAuditChild(L"read");
//    runAuditChild(L"read-crash");
//}
//
//void NetInterfaceIpcTestCases::initializationBounds()
//{
//    runAuditChild(L"init");
//}
//
//void NetInterfaceIpcTestCases::notificationCreationFailure()
//{
//    runAuditChild(L"semaphore");
//}
//
//void NetInterfaceIpcTestCases::invalidSendBounds()
//{
//    runAuditChild(L"send-bounds");
//}
//
//void NetInterfaceIpcTestCases::ipcCapacity()
//{
//    runAuditChild(L"gateway-capacity-open");
//}
//
//void NetInterfaceIpcTestCases::receiverOwnership()
//{
//    runAuditChild(L"gateway-receiver-owner");
//}
//
//void NetInterfaceIpcTestCases::externalReceiverCapacity()
//{
//    runAuditChild(L"gateway-external-capacity");
//}
//
//void NetInterfaceIpcTestCases::localReceiverCapacity()
//{
//    runAuditChild(L"gateway-local-capacity");
//}
//
//void NetInterfaceIpcTestCases::managedReceiverRollback()
//{
//    runAuditChild(L"gateway-managed-rollback");
//}
//
//int NetInterfaceIpcTestCases::auditChild(const std::string& mode)
//{
//    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
//    const int32_t pid = static_cast<int32_t>(CSystem::currentProcessPid());
//    if (mode == "read" || mode == "read-crash")
//    {
//        PROCESS_INFORMATION child = {};
//        uint64_t childIdentity = 0;
//        if (mode == "read-crash")
//        {
//            wchar_t command[] = L"cmd.exe /d /c exit 17";
//            STARTUPINFOW startup = {};
//            startup.cb = sizeof(startup);
//            if (!CreateProcessW(nullptr, command, nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
//                nullptr, nullptr, &startup, &child)) { return 32; }
//            CloseHandle(child.hThread);
//            const DWORD waited = WaitForSingleObject(child.hProcess, 2000);
//            FILETIME created = {}, exited = {}, kernel = {}, user = {};
//            if (waited != WAIT_OBJECT_0 || !GetProcessTimes(child.hProcess, &created, &exited, &kernel, &user))
//            {
//                if (waited != WAIT_OBJECT_0) { TerminateProcess(child.hProcess, 98); WaitForSingleObject(child.hProcess, 2000); }
//                CloseHandle(child.hProcess);
//                return 33;
//            }
//            childIdentity = (static_cast<uint64_t>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
//        }
//        // 保留退出进程句柄，验证“创建标识仍可查但已经退出”的回收分支。
//        auto closeChild = [](void* handle) { if (handle != nullptr) { CloseHandle(handle); } };
//        std::unique_ptr<void, decltype(closeChild)> ownedChild(child.hProcess, closeChild);
//        const int32_t phases = mode == "read" ? 1 : 4;
//        for (int32_t phase = 0; phase < phases; ++phase)
//        {
//            ProcessWorkChannel receiver;
//            if (!receiver.openReceiver(64, 3, 0, 0, 0, 0)) { return 34; }
//            receiver.publish();
//            SharedMemory directoryMemory("P2D" + std::to_string(pid));
//            ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//            if (directory == nullptr) { return 34; }
//            SharedMemory controlMemory(ProcessWorkChannel::objectName('C', pid, directory->m_identity, static_cast<uint32_t>(directory->m_session)));
//            ProcessWorkControl* control = static_cast<ProcessWorkControl*>(controlMemory.writeWithoutLock());
//            if (control == nullptr) { return 34; }
//            LockFreeQueue<char*> packets;
//            Semaphore delivered;
//            std::atomic<int32_t> full(0);
//            if (mode == "read")
//            {
//                NetSenderIpc sender;
//                if (sender.open(pid, pid, 52) != NET_SENDER_IPC_ACCEPTED) { return 35; }
//                receiver.pump(packets, delivered, full);
//                const int64_t token = control->m_peers[0].m_token;
//                if (token <= 0 || !AtomicMath::compareAndSwap(&control->m_blocks[0].m_owner, static_cast<int64_t>(0), token)) { return 35; }
//                // 未提交写者没有占读取环；其他线程已提交正文可以交付，自己的占用仍须保留。
//                if (sender.trySend(reinterpret_cast<const uint8_t*>("TEST"), 4, 11) != NET_SENDER_IPC_ACCEPTED ||
//                    receiver.pump(packets, delivered, full) != 1 || control->m_blocks[0].m_owner != token) { return 36; }
//                char* packet = nullptr;
//                if (!packets.pop(&packet) || packet == nullptr) { return 36; }
//                std::unique_ptr<char[]> owned(packet);
//                int32_t header[3] = {};
//                std::memcpy(header, packet, sizeof(header));
//                if (header[1] != 11 || header[2] != 4 || std::memcmp(packet + 12, "TEST", 4) != 0) { return 36; }
//                AtomicMath::compareAndSwap(&control->m_blocks[0].m_owner, token, static_cast<int64_t>(0));
//                const int32_t expected[] = { 17, 19 };
//                for (int32_t i = 0; i < 2; ++i)
//                {
//                    if (sender.trySend(reinterpret_cast<const uint8_t*>("TEST"), 4, expected[i]) != NET_SENDER_IPC_ACCEPTED) { return 36; }
//                }
//                if (receiver.pump(packets, delivered, full) != 2) { return 36; }
//                for (int32_t i = 0; i < 2; ++i)
//                {
//                    packet = nullptr;
//                    if (!packets.pop(&packet) || packet == nullptr) { return 36; }
//                    owned.reset(packet);
//                    std::memcpy(header, packet, sizeof(header));
//                    if (header[1] != expected[i] || header[2] != 4 || std::memcmp(packet + 12, "TEST", 4) != 0) { return 36; }
//                }
//                if (!packets.empty()) { return 36; }
//            }
//            else
//            {
//                const int32_t dead = static_cast<int32_t>(child.dwProcessId);
//                control->m_peers[0].m_pid = dead;
//                control->m_peers[0].m_identity = childIdentity;
//                control->m_peers[0].m_token = control->m_nextToken = 7;
//                control->m_blocks[0].m_owner = 7;
//                SharedMemory memory(ProcessWorkChannel::objectName('B', pid, directory->m_identity,
//                    static_cast<uint32_t>(directory->m_session), 0, 1));
//                char* data = static_cast<char*>(memory.writeWithoutLock());
//                if (data == nullptr) { return 34; }
//                const int32_t header[] = { dead, 42, 4 };
//                if (phase >= 1) { std::memcpy(data, header, 12); std::memcpy(data + 12, "TEST", 4); }
//                if (phase >= 2) { control->m_ring[0] = 1; }
//                if (phase == 3)
//                {
//                    uint64_t identity = 0;
//                    if (!CSystem::processIdentity(pid, identity)) { return 35; }
//                    control->m_peers[1].m_pid = pid;
//                    control->m_peers[1].m_identity = identity;
//                    control->m_peers[1].m_token = control->m_nextToken = 8;
//                    control->m_blocks[1].m_owner = 8;
//                }
//                const int32_t copied = receiver.pump(packets, delivered, full);
//                if (copied != (phase >= 2 ? 1 : 0) || control->m_blocks[0].m_owner != 0 ||
//                    control->m_head != control->m_tail || (phase == 3 && control->m_blocks[1].m_owner != 8)) { return 36; }
//                if (phase >= 2)
//                {
//                    char* packet = nullptr;
//                    if (!packets.pop(&packet) || packet == nullptr) { return 36; }
//                    std::unique_ptr<char[]> owned(packet);
//                    if (std::memcmp(packet, header, 12) != 0 || std::memcmp(packet + 12, "TEST", 4) != 0) { return 36; }
//                }
//                if (!packets.empty()) { return 36; }
//            }
//        }
//        return 0;
//    }
//    if (mode == "send-bounds")
//    {
//        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
//        ProcessWork& work = ProcessWork::instance();
//        const MessageType type = static_cast<MessageType>(42);
//        const std::string name = CSystem::GetCurrentExeName();
//        // 原实现在第一个负长度post中发生实际越界复制；父runner有总期限并核对退出码。
//        work.post(pid, "TEST", -1, type);
//        work.post(name, "TEST", -1, type);
//        work.post(pid, "TEST", INT32_MAX, type);
//        work.post(name, "TEST", INT32_MAX, type);
//        NetInterfaceIpcAuditReceiver receiver(false);
//        work.addProcessReceiveCallback(&receiver);
//        if (!work.initReceive(64, 4)) { work.uninitReceive(); return 37; }
//        work.initPostThread();
//        work.send(pid, "TEST", -1, type);
//        work.send(name, "TEST", -1, type);
//        // v2按声明块容量校验，不把Windows页尾余量当成有效正文容量。
//        const std::vector<char> oversized(53, 'x');
//        if (work.trySend(pid, oversized.data(), static_cast<int32_t>(oversized.size()), type) != PROCESS_WORK_TOO_LARGE)
//        {
//            work.uninitReceive();
//            return 38;
//        }
//        for (uint32_t index = 0; index < 8; ++index)
//        {
//            work.send(pid, oversized.data(), static_cast<int32_t>(oversized.size()), type);
//        }
//        work.post(pid, "TEST", 4, type);
//        work.post(name, "TEST", 4, type);
//        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
//        while (receiver.m_count.load() < 2 && std::chrono::steady_clock::now() < deadline) { CSystem::sleepPrecise(1); }
//        const bool passed = receiver.m_count.load() == 2 && !receiver.m_bad.load();
//        work.uninitReceive();
//        return passed ? 0 : 38;
//    }
//    if (mode == "semaphore")
//    {
//        ProcessWork& work = ProcessWork::instance();
//        NetInterfaceIpcAuditReceiver receiver(false);
//        work.initPostThread();
//        work.addProcessReceiveCallback(&receiver);
//        uint64_t identity = 0;
//        if (!CSystem::processIdentity(pid, identity)) { work.uninitReceive(); return 28; }
//        SharedMemory directoryMemory("P2D" + std::to_string(pid), sizeof(ProcessWorkDirectory), true);
//        ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//        if (directory == nullptr) { work.uninitReceive(); return 28; }
//        const uint32_t next = static_cast<uint32_t>(AtomicMath::selfAdd(&directory->m_nextSession, static_cast<int64_t>(0)) + 1);
//        const std::string name = ProcessWorkChannel::objectName('R', pid, identity, next);
//        HANDLE incompatible = CreateEventA(nullptr, TRUE, FALSE, name.c_str());
//        if (incompatible == nullptr)
//        {
//            work.uninitReceive();
//            return 28;
//        }
//        const bool initialized = work.initReceive(64, 4);
//        CloseHandle(incompatible);
//        if (initialized)
//        {
//            work.uninitReceive();
//            return 29;
//        }
//        // 不重建发送线程、不重登回调、不显式uninit；失败初始化必须只撤销自己的资源。
//        if (!work.initReceive(64, 4))
//        {
//            work.uninitReceive();
//            return 30;
//        }
//        work.post(pid, "TEST", 4, static_cast<MessageType>(42));
//        work.post(CSystem::GetCurrentExeName(), "TEST", 4, static_cast<MessageType>(42));
//        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
//        while (receiver.m_count.load() < 2 && std::chrono::steady_clock::now() < deadline)
//        {
//            CSystem::sleepPrecise(1);
//        }
//        const bool passed = receiver.m_count.load() == 2 && !receiver.m_bad.load();
//        work.uninitReceive();
//        return passed ? 0 : 31;
//    }
//    if (mode != "callback" && mode != "init" && mode != "semaphore")
//    {
//        return 23;
//    }
//    ProcessWork& work = ProcessWork::instance();
//    NetInterfaceIpcAuditReceiver throwing(true);
//    NetInterfaceIpcAuditReceiver good(false);
//    if (mode == "callback")
//    {
//        work.addProcessReceiveCallback(&throwing);
//        work.addProcessReceiveCallback(nullptr);
//    }
//    work.addProcessReceiveCallback(&good);
//    if (mode == "init" && (work.initReceive(0, 1) || work.initReceive(16, 0) || work.initReceive(16, -1)))
//    {
//        work.uninitReceive();
//        return 24;
//    }
//    if (!work.initReceive(64, 4))
//    {
//        work.uninitReceive();
//        return 25;
//    }
//    if (mode == "init" && work.initReceive(64, 4))
//    {
//        work.uninitReceive();
//        return 26;
//    }
//    NetSenderIpc sender;
//    bool passed = sender.open(pid, pid, 52) == NET_SENDER_IPC_ACCEPTED;
//    for (uint32_t i = 0; passed && i < 2; ++i)
//    {
//        const auto sendDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
//        int32_t result = NET_SENDER_IPC_BUSY;
//        while (result == NET_SENDER_IPC_BUSY && std::chrono::steady_clock::now() < sendDeadline)
//        {
//            result = sender.trySend(reinterpret_cast<const uint8_t*>("TEST"), 4, 42);
//            if (result == NET_SENDER_IPC_BUSY) { CSystem::sleepPrecise(1); }
//        }
//        passed = result == NET_SENDER_IPC_ACCEPTED;
//    }
//    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
//    while (passed && good.m_count.load() < 2 && std::chrono::steady_clock::now() < deadline)
//    {
//        passed = sender.flushNotifications() >= 0;
//        CSystem::sleepPrecise(1);
//    }
//    passed = passed && good.m_count.load() == 2 && !good.m_bad.load() &&
//        (mode != "callback" || (throwing.m_count.load() == 2 && !throwing.m_bad.load()));
//    sender.close();
//    work.uninitReceive();
//    return passed ? 0 : 27;
//}
//#endif
//
//void NetInterfaceIpcTestCases::queueLifecycle()
//{
//    LockFreeQueue<int32_t> queue;
//    int32_t value = 99;
//    CheckIpc(!queue.pop(&value) && value == 99, "empty queue changed output");
//    queue.push(7);
//    CheckIpc(queue.pop(&value) && value == 7 && queue.empty(), "normal queue lifecycle");
//    queue.destroy();
//    queue.push(9);
//    CheckIpc(!queue.pop(&value) && queue.empty() && queue.size() == 0 && value == 7,
//        "destroyed queue must reject publication without abort or leaked nodes");
//    queue.init();
//    queue.push(11);
//    CheckIpc(queue.pop(&value) && value == 11 && queue.empty(), "queue failed to recover after init");
//}
//
//
//
//int main(int argc, char* argv[])
//{
//    std::setvbuf(stdout, nullptr, _IONBF, 0);
//#ifdef _WIN32
//    ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
//#endif
//#if defined(_WIN32) && defined(PROCESS_WORK_PORTABLE_TEST)
//    // Windows替代验证避免Sleep(1)被默认约16ms时钟粒度放大；不进入生产库或原生性能构建。
//    if (::timeBeginPeriod(1) != TIMERR_NOERROR) { return 3; }
//    auto endPeriod = [](int*) { ::timeEndPeriod(1); };
//    int period = 1;
//    std::unique_ptr<int, decltype(endPeriod)> periodScope(&period, endPeriod);
//#endif
//    if (argc < 2) { std::fprintf(stderr, "Specify unit or mesh\n"); return 2; }
//    int32_t errors = 0;
//    int32_t checks = 0;
//    // 每项独立条件失败只记录，最终返回非零，不调用assert/abort。
//    auto check = [&](bool value, const char* reason)
//    {
//        ++checks;
//        if (!value) { ++errors; std::fprintf(stderr, "FAIL %s\n", reason); }
//    };
//    // 稳定单调时间，单位毫秒。
//    auto now = []() -> int64_t
//    {
//        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
//    };
//    // 测试等待有统一截止，睡眠期间不占CPU。
//    auto wait = [&](const std::function<bool()>& predicate, int32_t milliseconds) -> bool
//    {
//        const int64_t end = now() + milliseconds;
//        Semaphore sleep;
//        while (now() < end)
//        {
//            if (predicate()) { return true; }
//            sleep.wait(2);
//        }
//        return predicate();
//    };
//    // 测量进程CPU消耗，Windows下含所有CTask线程。
//    auto cpu = []() -> double
//    {
//#ifdef _WIN32
//        FILETIME created = {}, ended = {}, kernel = {}, user = {};
//        if (!::GetProcessTimes(::GetCurrentProcess(), &created, &ended, &kernel, &user)) { return -1; }
//        return ((static_cast<uint64_t>(kernel.dwHighDateTime) << 32) + kernel.dwLowDateTime +
//            (static_cast<uint64_t>(user.dwHighDateTime) << 32) + user.dwLowDateTime) / 10000000.0;
//#else
//        return static_cast<double>(std::clock()) / CLOCKS_PER_SEC;
//#endif
//    };
//    // 使用原生句柄快照观察重复启停是否持续增长。
//    auto handles = []() -> uint32_t
//    {
//#ifdef _WIN32
//        DWORD result = 0;
//        ::GetProcessHandleCount(::GetCurrentProcess(), &result);
//        return result;
//#else
//        return 0;
//#endif
//    };
//    const int32_t self = CSystem::currentProcessPid();
//#ifdef _WIN32
//    const std::string selfName = CSystem::GetCurrentExeName();
//#else
//    const std::string selfName = CSystem::GetCurrentExeFullName();
//#endif
//    // Full-position reference bytes avoid the old 256-byte periodic pattern hiding page/offset errors.
//    // Inputs are source/thread/sequence/byte offset; output is one deterministic reference byte.
//    auto payloadByte = [](int32_t source, int32_t thread, int32_t sequence, int32_t offset) -> unsigned char
//    {
//        uint32_t value = static_cast<uint32_t>(offset / 4) ^ (static_cast<uint32_t>(source) + 1) * 0x9e3779b9U ^
//            (static_cast<uint32_t>(thread) + 1) * 0x85ebca6bU ^ (static_cast<uint32_t>(sequence) + 1) * 0xc2b2ae35U;
//        value ^= value >> 16;
//        value *= 0x7feb352dU;
//        value ^= value >> 15;
//        value *= 0x846ca68bU;
//        value ^= value >> 16;
//        return static_cast<unsigned char>(value >> ((offset % 4) * 8));
//    };
//    const std::string action(argv[1]);
//    if (action == "overload-rejection-fifo")
//    {
//        // U70：全部重载在极小/最大容量、最长名称及拒绝消息夹杂时仍交付完整有序正文。
//        std::printf("CASE U70\n");
//        const int32_t capacities[] = { 0, 1, 6, 65536 };
//        const int32_t threads = 4, packets = 512;
//        int32_t total = 0, scenarios = 0;
//        for (int32_t mode = 0; mode < 2; ++mode)
//        {
//            for (int32_t c = 0; c < 4; ++c)
//            {
//                const int32_t capacity = capacities[c];
//                ProcessWorkTestReceiver receiver;
//                const std::string name = std::string(990, 'n') + std::string(33, 'z');
//                const std::string missing = "absent-" + std::to_string(self);
//                std::atomic<int32_t> received(0), bad(0);
//                std::vector<int32_t> next(threads, 0);
//                receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//                {
//                    const int32_t value = static_cast<int32_t>(type);
//                    const int32_t thread = value / 100000, sequence = value % 100000;
//                    const int32_t expected = sequence % 3 == 0 ? 0 : (sequence % 3 == 1 ? (std::min)(1, capacity) : capacity);
//                    bool valid = pid == self && value >= 0 && thread < threads && sequence < packets &&
//                        length == expected && (length == 0 ? data == nullptr : data != nullptr);
//                    if (valid)
//                    {
//                        valid = next[thread] == sequence;
//                        ++next[thread];
//                        for (int32_t i = 0; i < length; ++i)
//                        {
//                            if (static_cast<unsigned char>(data[i]) != payloadByte(c, thread, sequence, i))
//                            {
//                                valid = false;
//                                break;
//                            }
//                        }
//                    }
//                    if (!valid)
//                    {
//                        ++bad;
//                    }
//                    ++received;
//                };
//                check(receiver.setReceiveName(name), "U70 maximum legal logical name accepted");
//                check(receiver.initReceive(capacity + 12, 2, 1, 2, 1, 2), "U70 exact declared capacity initialized");
//                if (errors != 0)
//                {
//                    return 1;
//                }
//                if (mode == 1)
//                {
//                    receiver.initPostThread();
//                }
//                std::vector<std::shared_ptr<ProcessWorkTestTask>> jobs;
//                std::vector<uint32_t> ids;
//                for (int32_t thread = 0; thread < threads; ++thread)
//                {
//                    std::shared_ptr<ProcessWorkTestTask> job(new ProcessWorkTestTask);
//                    job->m_action = [&, thread]()
//                    {
//                        const std::string oversized(capacity + 1, 'x');
//                        for (int32_t sequence = 0; sequence < packets; ++sequence)
//                        {
//                            // 每十六个合法包插入参数拒绝、超长和目标不存在；不得毒化队列或占住共享槽。
//                            if (sequence % 16 == 0)
//                            {
//                                if (mode == 0)
//                                {
//                                    receiver.send(self, oversized, static_cast<MessageType>(INT32_MAX));
//                                    receiver.send(name, oversized, static_cast<MessageType>(INT32_MAX));
//                                    receiver.send(self, nullptr, 1);
//                                    receiver.send(name, "x", -1);
//                                    receiver.send(missing, "x", 1);
//                                }
//                                else
//                                {
//                                    receiver.post(self, oversized, static_cast<MessageType>(INT32_MAX));
//                                    receiver.post(name, oversized, static_cast<MessageType>(INT32_MAX));
//                                    receiver.post(self, nullptr, 1);
//                                    receiver.post(name, "x", -1);
//                                    receiver.post(missing, "x", 1);
//                                }
//                            }
//                            const int32_t length = sequence % 3 == 0 ? 0 : (sequence % 3 == 1 ? (std::min)(1, capacity) : capacity);
//                            std::string bytes(length, '\0');
//                            for (int32_t i = 0; i < length; ++i)
//                            {
//                                bytes[i] = static_cast<char>(payloadByte(c, thread, sequence, i));
//                            }
//                            const char* data = length == 0 ? nullptr : bytes.data();
//                            const MessageType type = static_cast<MessageType>(thread * 100000 + sequence);
//                            if (mode == 0)
//                            {
//                                switch (sequence % 4)
//                                {
//                                case 0: receiver.send(self, data, length, type); break;
//                                case 1: receiver.send(name, bytes, type); break;
//                                case 2: receiver.send(self, bytes, type); break;
//                                case 3: receiver.send(name, data, length, type); break;
//                                }
//                            }
//                            else
//                            {
//                                switch (sequence % 4)
//                                {
//                                case 0: receiver.post(self, data, length, type); break;
//                                case 1: receiver.post(name, bytes, type); break;
//                                case 2: receiver.post(self, bytes, type); break;
//                                case 3: receiver.post(name, data, length, type); break;
//                                }
//                            }
//                            std::fill(bytes.begin(), bytes.end(), static_cast<char>(0xa5));
//                        }
//                    };
//                    const uint32_t id = CTaskThreadManager::Instance().Init();
//                    std::shared_ptr<CTaskThread> worker = CTaskThreadManager::Instance().GetThreadInterface(id);
//                    if (!worker)
//                    {
//                        check(false, "U70 producer creation");
//                        break;
//                    }
//                    jobs.push_back(job);
//                    ids.push_back(id);
//                    worker->PostTask(job);
//                }
//                for (size_t i = 0; i < ids.size(); ++i)
//                {
//                    CTaskThreadManager::Instance().WaitForEnd(ids[i]);
//                    check(!jobs[i]->m_failed, "U70 producer completed normally");
//                }
//                if (mode == 1)
//                {
//                    receiver.waitForPostEnd();
//                }
//                check(wait([&]() { return received == threads * packets; }, 10000), "U70 all accepted packets delivered");
//                Semaphore quiet;
//                quiet.wait(50);
//                receiver.uninitReceive();
//                check(received == threads * packets && bad == 0, "U70 exact count payload FIFO and no rejected/duplicate callback");
//                for (int32_t thread = 0; thread < threads; ++thread)
//                {
//                    check(next[thread] == packets, "U70 each producer ledger complete");
//                }
//                total += received.load();
//                ++scenarios;
//            }
//        }
//        std::printf("RESULT {\"action\":\"overload-rejection-fifo\",\"scenarios\":%d,\"received\":%d,\"errors\":%d}\n", scenarios, total, errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "executable-name-model")
//    {
//        std::printf("CASE U69\n");
//        typedef std::string (*PathQuery)(const std::string&, bool);
//        const PathQuery current[] = { ProcessWorkTestWindowsGetCurrentExeName, ProcessWorkTestWindowsGetCurrentExeFullName,
//            ProcessWorkTestLinuxGetCurrentExeName, ProcessWorkTestLinuxGetCurrentExeFullName };
//        const PathQuery baseline[] = { ProcessWorkTestWindowsGetCurrentExeNameBaseline, ProcessWorkTestWindowsGetCurrentExeFullNameBaseline,
//            ProcessWorkTestLinuxGetCurrentExeNameBaseline, ProcessWorkTestLinuxGetCurrentExeFullNameBaseline };
//        const size_t lengths[] = { 0, 1, 17, 254, 1021, 1022, 1023, 1024, 2048, 65536 };
//        for (size_t function = 0; function < 4; ++function)
//        {
//            for (size_t sample = 0; sample < sizeof(lengths) / sizeof(lengths[0]); ++sample)
//            {
//                const size_t length = lengths[sample];
//                const std::string path = length < 12 ? std::string(length, 'x') : std::string(length - 10, 'd') + "/probe.exe";
//                const bool complete = length < (function < 2 ? 1024U : 1023U);
//                const std::string expected = complete ? CSystem::GetName(path, function % 2 == 0 ? 3 : 1) : std::string();
//                check(current[function](path, false) == expected, "U69 correct basename only from a complete OS path");
//                check(current[function](path, true).empty(), "U69 failed path query cannot publish a name");
//            }
//            const std::string truncated = std::string(1000, 'd') + "/abcdefghijklmnopqrstuvwxyz1234567890.exe";
//            check(!baseline[function](truncated, false).empty() && current[function](truncated, false).empty(),
//                "U69 original branch accepted a truncated incorrect basename");
//        }
//        std::printf("RESULT {\"action\":\"executable-name-model\",\"checks\":%d,\"errors\":%d}\n", checks, errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "linux-identity-model")
//    {
//        std::printf("CASE U68\n");
//        const char* names[] = { "worker", "a b", "a)b(c", "a\nb)c" };
//        const char* identities[] = { "1", "123456", "4294967296", "18446744073709551615" };
//        const uint64_t values[] = { 1, 123456, UINT64_C(4294967296), UINT64_MAX };
//        for (size_t name = 0; name < sizeof(names) / sizeof(names[0]); ++name)
//        {
//            std::string prefix = std::string("123 (") + names[name] + ") S";
//            for (int32_t field = 4; field < 22; ++field) { prefix += " 0"; }
//            for (size_t sample = 0; sample < sizeof(identities) / sizeof(identities[0]); ++sample)
//            {
//                const std::string record = prefix + " " + identities[sample] + " 0 0\n";
//                uint64_t identity = 0;
//                check(ProcessWorkTestLinuxIdentity(record, identity, false) && identity == values[sample], "U68 exact complete 64-bit identity");
//                const size_t delimiter = (prefix + " " + identities[sample]).size();
//                for (size_t length = 0; length <= delimiter; ++length)
//                {
//                    identity = 99;
//                    check(!ProcessWorkTestLinuxIdentity(record.substr(0, length), identity, false) && identity == 0, "U68 every truncation before identity delimiter is unknown with zero output");
//                }
//                check(!ProcessWorkTestLinuxIdentity(record, identity, true) && identity == 0, "U68 bad stream cannot establish identity from buffered data");
//            }
//            const char* invalid[] = { "0", "-1", "+1", "18446744073709551616", "12x", "x12" };
//            for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
//            {
//                uint64_t identity = 99;
//                check(!ProcessWorkTestLinuxIdentity(prefix + " " + invalid[i] + " 0\n", identity, false) && identity == 0, "U68 malformed/overflow/signed identity rejected with zero output");
//            }
//            uint64_t identity = 0;
//            check(ProcessWorkTestLinuxIdentityBaseline(prefix + " 123", identity, false) && identity == 123,
//                "U68 old actual branch accepts a truncated creation time");
//            check(ProcessWorkTestLinuxIdentityBaseline(prefix + " 123456 0\n", identity, true),
//                "U68 old actual branch ignores stream read failure");
//        }
//        std::printf("RESULT {\"action\":\"linux-identity-model\",\"checks\":%d,\"failure_outputs_zero\":true,\"errors\":%d}\n", checks, errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "unknown-live-writer")
//    {
//#if defined(_WIN32) && defined(PROCESS_WORK_TEST_ALLOCATION)
//        std::printf("CASE U66\n");
//        std::vector<char> body(65536);
//        for (size_t i = 0; i < body.size(); ++i) { body[i] = static_cast<char>(payloadByte(9, 0, 0, static_cast<int32_t>(i))); }
//        std::atomic<int32_t> received(0), bad(0), status(PROCESS_WORK_BUSY);
//        ProcessWorkTestReceiver receiver;
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            const int32_t position = received.load();
//            const int32_t expected = position == 0 ? 65536 : 6;
//            const char* bytes = position == 0 ? body.data() : "123456";
//            if (pid != self || static_cast<int32_t>(type) != position || length != expected || data == nullptr ||
//                (length == expected && std::memcmp(data, bytes, expected) != 0)) { ++bad; }
//            ++received;
//        };
//        check(receiver.initReceive(65548, 1), "U66 single-slot receiver");
//        ProcessWorkChannel channel;
//        check(channel.openSender(self), "U66 sender opened");
//        if (errors) { return 1; }
//        s_testCommitThread = 0; s_testCommitReached = 0; s_testCommitRelease = false;
//        s_testCommitExpired = false; s_testCommitTrace = false; s_testCommitPhase = 1;
//        std::shared_ptr<ProcessWorkTestTask> task(new ProcessWorkTestTask);
//        task->m_stopAction = [&]() { s_testCommitRelease = true; };
//        task->m_action = [&]()
//        {
//            s_testCommitThread = ::GetCurrentThreadId();
//            const int64_t deadline = now() + 10000;
//            do
//            {
//                status = channel.trySend(body.data(), static_cast<int32_t>(body.size()), 0);
//                if (status != PROCESS_WORK_BUSY) { break; }
//                CSystem::Sleep(1);
//            } while (now() < deadline);
//        };
//        const uint32_t worker = CTaskThreadManager::Instance().Init();
//        std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(worker);
//        check(thread != nullptr, "U66 worker exists");
//        if (!thread) { return 1; }
//        thread->PostTask(task);
//        check(wait([&]() { return s_testCommitReached == 1; }, 5000), "U66 paused after real memcpy before commit");
//        s_testUnknownIdentityHits = 0; s_testUnknownExitedHits = 0;
//        s_testUnknownPid = self;
//        // Observe complete maintenance sweeps; Windows timer granularity is not a 20ms guarantee.
//        check(wait([&]() { return s_testUnknownIdentityHits >= 3 && s_testUnknownExitedHits >= 3; }, 5000),
//            "U66 actual recovery repeatedly sees unknown identity");
//        check(channel.trySend("123456", 6, 1) == PROCESS_WORK_BUSY && received == 0, "U66 live writer slot never reclaimed or bypassed");
//        check(channel.waitForSpace() && channel.ready(), "U66 receiver query failure does not invalidate channel");
//        s_testUnknownPid = 0;
//        s_testCommitRelease = true;
//        CTaskThreadManager::Instance().WaitForEnd(worker);
//        check(!task->m_failed && !s_testCommitExpired && status == PROCESS_WORK_SENT, "U66 original writer commits normally");
//        check(wait([&]() { return received == 1; }, 5000), "U66 full original payload received");
//        int32_t next = PROCESS_WORK_BUSY;
//        check(wait([&]() { next = channel.trySend("123456", 6, 1); return next != PROCESS_WORK_BUSY; }, 5000) && next == PROCESS_WORK_SENT,
//            "U66 same slot reusable after original delivery");
//        check(wait([&]() { return received == 2; }, 5000), "U66 second packet delivered");
//        receiver.uninitReceive();
//        check(received == 2 && bad == 0, "U66 exact content, order and no duplicates");
//        std::printf("RESULT {\"action\":\"unknown-live-writer\",\"received\":%d,\"bad\":%d,\"identity_hits\":%d,\"exit_hits\":%d,\"errors\":%d}\n",
//            received.load(), bad.load(), s_testUnknownIdentityHits.load(), s_testUnknownExitedHits.load(), errors);
//        return errors == 0 ? 0 : 1;
//#else
//        return 77;
//#endif
//    }
//    if (action == "notification-blackout")
//    {
//        std::printf("CASE U67\n");
//        const int32_t sizes[] = { 0, 1, 6, 255, 4096, 65536 };
//        int32_t total = 0, injected = 0;
//        for (int32_t fault = 1; fault <= 3; ++fault)
//        {
//            std::atomic<int32_t> received(0), bad(0), sent(0), failed(0);
//            int32_t sequence[4] = {};
//            ProcessWorkTestReceiver receiver;
//            receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//            {
//                const int32_t value = static_cast<int32_t>(type);
//                if (value >= 9000)
//                {
//                    if (pid != self || value != 9000 || length != 6 || data == nullptr ||
//                        (length == 6 && std::memcmp(data, "123456", 6) != 0)) { ++bad; }
//                }
//                else
//                {
//                    const int32_t producer = value / 1000, number = value % 1000;
//                    if (pid != self || producer < 0 || producer >= 4 || number < 0 || number >= 64) { ++bad; }
//                    else
//                    {
//                        const int32_t expected = sizes[number % 6];
//                        if (sequence[producer]++ != number || length != expected || (expected != 0 && data == nullptr)) { ++bad; }
//                        else
//                        {
//                            for (int32_t i = 0; i < length; ++i)
//                            {
//                                if (static_cast<unsigned char>(data[i]) != payloadByte(10, producer, number, i)) { ++bad; break; }
//                            }
//                        }
//                    }
//                }
//                ++received;
//            };
//            check(receiver.initReceive(65548, 2), "U67 tiny pool forces both data and space waits");
//            ProcessWorkChannel channel;
//            check(channel.openSender(self), "U67 sender opened");
//            if (errors) { return 1; }
//            s_testSignalHits = 0; s_testSignalFault = fault;
//            std::vector<uint32_t> workers;
//            std::vector<std::shared_ptr<ProcessWorkTestTask> > tasks;
//            for (int32_t producer = 0; producer < 4; ++producer)
//            {
//                std::shared_ptr<ProcessWorkTestTask> task(new ProcessWorkTestTask);
//                task->m_action = [&, producer]()
//                {
//                    std::vector<char> body(65536);
//                    for (int32_t number = 0; number < 64; ++number)
//                    {
//                        const int32_t length = sizes[number % 6];
//                        for (int32_t i = 0; i < length; ++i) { body[i] = static_cast<char>(payloadByte(10, producer, number, i)); }
//                        int32_t status = PROCESS_WORK_BUSY;
//                        const int64_t deadline = now() + 10000;
//                        do
//                        {
//                            status = channel.trySend(length == 0 ? nullptr : body.data(), length, producer * 1000 + number);
//                            if (status != PROCESS_WORK_BUSY) { break; }
//                            if (!channel.waitForSpace()) { break; }
//                        } while (now() < deadline);
//                        if (status != PROCESS_WORK_SENT) { ++failed; break; }
//                        ++sent;
//                    }
//                };
//                const uint32_t worker = CTaskThreadManager::Instance().Init();
//                std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(worker);
//                check(thread != nullptr, "U67 producer thread exists");
//                if (!thread) { break; }
//                workers.push_back(worker); tasks.push_back(task); thread->PostTask(task);
//            }
//            for (size_t i = 0; i < workers.size(); ++i)
//            {
//                CTaskThreadManager::Instance().WaitForEnd(workers[i]);
//                check(!tasks[i]->m_failed, "U67 producer finished without exception");
//            }
//            check(wait([&]() { return received == 256; }, 5000), "U67 all committed packets arrive while every notification is suppressed");
//            check(sent == 256 && failed == 0 && s_testSignalHits >= 10, "U67 success ledger and sustained injected OS failure");
//            injected += s_testSignalHits.load(); s_testSignalFault = 0;
//            int32_t last = PROCESS_WORK_BUSY;
//            check(wait([&]() { last = channel.trySend("123456", 6, 9000); return last != PROCESS_WORK_BUSY; }, 5000) && last == PROCESS_WORK_SENT,
//                "U67 notification recovery leaves channel usable");
//            check(wait([&]() { return received == 257; }, 5000), "U67 recovery packet delivered");
//            receiver.uninitReceive();
//            for (int32_t i = 0; i < 4; ++i) { check(sequence[i] == 64, "U67 every producer preserves its complete order"); }
//            check(received == 257 && bad == 0, "U67 exact variable-length content and no duplicates");
//            total += received.load();
//        }
//        std::printf("RESULT {\"action\":\"notification-blackout\",\"received\":%d,\"signal_hits\":%d,\"errors\":%d}\n", total, injected, errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "linux-stat-model")
//    {
//        std::printf("CASE U64\n");
//        const char states[] = { 'R', 'S', 'D', 'T', 't', 'I', 'Z', 'X' };
//        const char* counts[] = { "1", "2", "32", "256", "0", "-1", "+1", "01", "1x", "18446744073709551616" };
//        const char* names[] = { "worker", "a b", "a)b(c", "a\nb)c" };
//        auto record = [](char state, const std::string& count, const std::string& name) -> std::string
//        {
//            std::string line = "123 (" + name + ") " + state;
//            for (int32_t field = 4; field < 20; ++field) { line += " 0"; }
//            return line + " " + count + " 0 123456 0\n";
//        };
//        for (size_t i = 0; i < sizeof(states); ++i)
//        {
//            for (size_t j = 0; j < sizeof(counts) / sizeof(counts[0]); ++j)
//            {
//                for (size_t k = 0; k < sizeof(names) / sizeof(names[0]); ++k)
//                {
//                    const bool expected = (states[i] == 'Z' || states[i] == 'X') && j == 0;
//                    check(ProcessWorkTestLinuxStatExited(record(states[i], counts[j], names[k])) == expected,
//                        "U64 only a dead leader with exactly one remaining thread is reclaimable");
//                }
//            }
//        }
//        const std::string valid = record('Z', "1", "worker");
//        check(ProcessWorkTestLinuxStatBaseline(record('Z', "2", "worker")), "U64 original production branch wrongly reclaims a live thread group");
//        check(!ProcessWorkTestLinuxStatExited(record('Z', "2", "worker")), "U64 repaired production branch preserves the live writer");
//        const size_t endCount = valid.find(" 1 0 123456") + 2;
//        for (size_t length = 0; length <= endCount; ++length)
//        {
//            check(!ProcessWorkTestLinuxStatExited(valid.substr(0, length)), "U64 incomplete stat cannot establish death");
//        }
//        check(!ProcessWorkTestLinuxStatExited(""), "U64 unreadable proc remains unknown");
//        std::printf("RESULT {\"action\":\"linux-stat-model\",\"checks\":%d,\"errors\":%d}\n", checks, errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "linux-zombie-leader")
//    {
//        std::printf("CASE U65\n");
//#if defined(__linux__)
//        // Fork before initializing IPC workers. Keep the child main thread's stack unborrowed.
//        int readyPipe[2] = {};
//        if (::pipe(readyPipe) != 0) { return 3; }
//        if (::fcntl(readyPipe[0], F_SETFL, O_NONBLOCK) != 0)
//        {
//            ::close(readyPipe[0]); ::close(readyPipe[1]); return 3;
//        }
//        const pid_t child = ::fork();
//        if (child < 0)
//        {
//            for (int32_t i = 0; i < 2; ++i) { ::close(readyPipe[i]); }
//            return 3;
//        }
//        if (child == 0)
//        {
//            ::close(readyPipe[0]);
//            const int output = readyPipe[1];
//            try
//            {
//                std::thread worker([output]()
//                {
//                    try
//                    {
//                    ProcessWorkTestReceiver receiver;
//                    std::atomic<int32_t> received(0), bad(0);
//                    receiver.m_message = [&](int32_t, const char* data, int32_t length, MessageType type)
//                    {
//                        if (length != 6 || data == nullptr || std::memcmp(data, "123456", 6) != 0 ||
//                            static_cast<int32_t>(type) != received.load()) { ++bad; }
//                        ++received;
//                    };
//                    if (!receiver.initReceive(18, 1)) { ::_exit(4); }
//                    if (::write(output, "R", 1) != 1) { ::_exit(5); }
//                    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
//                    while (received < 128 && std::chrono::steady_clock::now() < deadline) { CSystem::Sleep(2); }
//                    receiver.uninitReceive();
//                    CTaskThreadManager::Instance().UninitAll();
//                    ::_exit(received == 128 && bad == 0 ? 0 : 7);
//                    }
//                    catch (...) { ::_exit(9); }
//                });
//                worker.detach();
//            }
//            catch (...) { ::_exit(8); }
//            ::pthread_exit(nullptr);
//        }
//        ::close(readyPipe[1]);
//        // Parent owns and always reaps this child; there are no unrelated process operations.
//        char ready = 0;
//        check(wait([&]() { return ::read(readyPipe[0], &ready, 1) == 1; }, 5000) && ready == 'R', "U65 child worker initialized");
//        check(wait([&]()
//        {
//            std::ifstream input(("/proc/" + std::to_string(child) + "/stat").c_str());
//            const std::string line((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
//            const size_t end = line.rfind(')');
//            return end != std::string::npos && line.substr(end + 1, 3) == " Z ";
//        }, 5000), "U65 actual main thread is zombie while IPC workers remain live");
//        check(!CSystem::processDefinitelyExited(child), "U65 a zombie leader is not a dead process");
//        ProcessWorkTestReceiver sender;
//        for (int32_t i = 0; i < 128 && errors == 0; ++i)
//        {
//            int32_t result = PROCESS_WORK_BUSY;
//            check(wait([&]() { result = sender.trySend(child, "123456", 6, static_cast<MessageType>(i)); return result != PROCESS_WORK_BUSY; }, 3000) &&
//                result == PROCESS_WORK_SENT, "U65 continued real IPC after leader exit");
//        }
//        int status = 0;
//        const bool finished = wait([&]() { return ::waitpid(child, &status, WNOHANG) == child; }, 12000);
//        if (!finished) { ::kill(child, SIGKILL); ::waitpid(child, &status, 0); }
//        check(finished && WIFEXITED(status) && WEXITSTATUS(status) == 0, "U65 exact delivery and reaped child");
//        ::close(readyPipe[0]);
//        std::printf("RESULT {\"action\":\"linux-zombie-leader\",\"errors\":%d}\n", errors);
//        return errors == 0 ? 0 : 1;
//#else
//        std::printf("SKIP requires Linux/Android procfs and pthread_exit\n");
//        return 77;
//#endif
//    }
//    if (action == "commit-interleavings")
//    {
//#if defined(_WIN32) && defined(PROCESS_WORK_TEST_ALLOCATION)
//        std::printf("CASE U62\n");
//        int32_t delivered = 0;
//        for (int32_t scenario = 1; scenario <= 3; ++scenario)
//        {
//            std::vector<char> large(65536);
//            for (size_t i = 0; i < large.size(); ++i) { large[i] = static_cast<char>(payloadByte(7, scenario, 1, static_cast<int32_t>(i))); }
//            std::atomic<int32_t> received(0), bad(0), first(PROCESS_WORK_INVALID), second(PROCESS_WORK_INVALID);
//            ProcessWorkTestReceiver receiver;
//            receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//            {
//                const int32_t position = received.load(), value = static_cast<int32_t>(type);
//                const int32_t order[3] = { 2, 1, 3 };
//                const int32_t expected = scenario == 1 ? (position < 3 ? order[position] : -1) : (scenario == 2 ? position + 1 : 2);
//                const int32_t expectedSize = scenario == 3 ? 1 : (value == 1 ? 65536 : 6);
//                const char* bytes = scenario == 3 ? "Z" : (value == 1 ? large.data() : "123456");
//                if (pid != self || value != expected || length != expectedSize || data == nullptr ||
//                    (length == expectedSize && std::memcmp(data, bytes, expectedSize) != 0)) { ++bad; }
//                ++received;
//            };
//            check(receiver.initReceive(65548, scenario == 1 ? 2 : 1), "U62 receiver prepared");
//            ProcessWorkChannel channel;
//            check(channel.openSender(self), "U62 shared sending channel opened");
//            if (errors != 0) { return 1; }
//            s_testCommitThread = 0; s_testCommitReached = 0; s_testCommitRelease = false;
//            s_testCommitExpired = false; s_testCommitTrace = false;
//            s_testCommitPhase = scenario == 2 ? 2 : 1;
//            auto sendUntil = [&](ProcessWorkChannel& target, const char* data, int32_t length, int32_t type) -> int32_t
//            {
//                const int64_t end = now() + 10000;
//                int32_t status = PROCESS_WORK_BUSY;
//                do
//                {
//                    status = target.trySend(data, length, type);
//                    if (status != PROCESS_WORK_BUSY) { break; }
//                    CSystem::Sleep(1);
//                } while (now() < end);
//                return status;
//            };
//            std::shared_ptr<ProcessWorkTestTask> task(new ProcessWorkTestTask);
//            task->m_stopAction = [&]() { s_testCommitRelease = true; };
//            task->m_action = [&]()
//            {
//                s_testCommitThread = ::GetCurrentThreadId();
//                first = sendUntil(channel, large.data(), static_cast<int32_t>(large.size()), 1);
//                if (scenario != 3 && first == PROCESS_WORK_SENT)
//                {
//                    second = sendUntil(channel, "123456", 6, scenario == 1 ? 3 : 130);
//                }
//            };
//            const uint32_t worker = CTaskThreadManager::Instance().Init();
//            std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(worker);
//            check(thread != nullptr, "U62 task thread available");
//            if (!thread) { return 1; }
//            thread->PostTask(task);
//            check(wait([&]() { return s_testCommitReached != 0; }, 5000), "U62 actual commit instruction boundary reached");
//            if (scenario == 1)
//            {
//                check(sendUntil(channel, "123456", 6, 2) == PROCESS_WORK_SENT, "U62 another writer can commit before paused large writer");
//                check(wait([&]() { return received == 1; }, 3000), "U62 uncommitted writer leaves no FIFO hole");
//            }
//            else if (scenario == 2)
//            {
//                check(wait([&]() { return received == 1; }, 3000), "U62 committed packet arrives before sender returns or signals");
//                for (int32_t id = 2; id <= 129; ++id)
//                {
//                    check(sendUntil(channel, "123456", 6, id) == PROCESS_WORK_SENT, "U62 receiver helps tail and safely reuses single slot");
//                }
//                check(wait([&]() { return received == 129; }, 3000), "U62 128 subsequent packets progress while first sender is paused");
//            }
//            else
//            {
//                check(received == 0, "U62 no packet delivered before commit");
//                receiver.uninitReceive();
//                receiver.addProcessReceiveCallback(&receiver);
//                check(receiver.initReceive(13, 1), "U62 receiver rebuilt with one-byte capacity during old copy");
//            }
//            s_testCommitRelease = true;
//            CTaskThreadManager::Instance().WaitForEnd(worker);
//            check(!task->m_failed && !s_testCommitExpired, "U62 worker exited normally without watchdog release");
//            if (scenario == 3)
//            {
//                check(first == PROCESS_WORK_UNAVAILABLE && channel.payloadCapacity() == 0, "U62 stale copied packet rejected before publication");
//                ProcessWorkChannel replacement;
//                check(replacement.openSender(self) && replacement.payloadCapacity() == 1, "U62 fresh sender sees new capacity");
//                check(sendUntil(replacement, "Z", 1, 2) == PROCESS_WORK_SENT, "U62 replacement buffer remains usable");
//            }
//            else { check(first == PROCESS_WORK_SENT && second == PROCESS_WORK_SENT, "U62 both sequential sends report success exactly once"); }
//            const int32_t expected = scenario == 1 ? 3 : (scenario == 2 ? 130 : 1);
//            check(wait([&]() { return received == expected; }, 5000), "U62 exact delivery ledger completed");
//            receiver.uninitReceive();
//            check(received == expected && bad == 0, "U62 complete content and per-thread order retained");
//            delivered += received.load();
//        }
//        std::printf("RESULT {\"action\":\"commit-interleavings\",\"scenarios\":3,\"received\":%d,\"errors\":%d}\n", delivered, errors);
//        return errors == 0 ? 0 : 1;
//#else
//        return 77;
//#endif
//    }
//    if (action == "commit-death-server")
//    {
//        if (argc != 3) { return 2; }
//        const int32_t phase = std::atoi(argv[2]);
//        if (phase != 1 && phase != 2) { return 2; }
//        std::printf("CASE U63\n");
//        std::vector<char> large(65536);
//        for (size_t i = 0; i < large.size(); ++i) { large[i] = static_cast<char>(payloadByte(11, phase, 1, static_cast<int32_t>(i))); }
//        std::atomic<int32_t> received(0), bad(0);
//        ProcessWorkTestReceiver receiver;
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            const int32_t value = static_cast<int32_t>(type), expected = received.load() + (phase == 1 ? 2 : 1);
//            const int32_t size = value == 1 ? 65536 : 6;
//            const char* bytes = value == 1 ? large.data() : "123456";
//            if (pid <= 0 || value != expected || length != size || data == nullptr ||
//                (length == size && std::memcmp(data, bytes, size) != 0)) { ++bad; }
//            ++received;
//        };
//        check(receiver.initReceive(65548, 1), "U63 single-slot target initialized");
//        if (errors) { return 1; }
//        std::printf("READY %d\n", self);
//        std::string command;
//        std::getline(std::cin, command);
//        const int32_t expected = phase == 1 ? 128 : 129;
//        check(command == "finish" && wait([&]() { return received >= expected; }, 10000), "U63 recovery packets delivered after killed real sender");
//        receiver.uninitReceive();
//        check(received == expected && bad == 0, "U63 committed set and payload survive actual process kill");
//        std::printf("RESULT {\"action\":\"commit-death-server\",\"phase\":%d,\"received\":%d,\"expected\":%d,\"bad\":%d,\"errors\":%d}\n", phase, received.load(), expected, bad.load(), errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "commit-death-client")
//    {
//#if defined(_WIN32) && defined(PROCESS_WORK_TEST_ALLOCATION)
//        if (argc != 4) { return 2; }
//        const int32_t target = std::atoi(argv[2]), phase = std::atoi(argv[3]);
//        if (target <= 0 || phase < 0 || phase > 2) { return 2; }
//        ProcessWorkChannel channel;
//        check(channel.openSender(target), "U63 actual client channel opened");
//        if (errors) { return 1; }
//        s_testCommitThread = ::GetCurrentThreadId(); s_testCommitPhase = phase;
//        s_testCommitReached = 0; s_testCommitRelease = false; s_testCommitTrace = true; s_testCommitExpired = false;
//        std::vector<char> large(65536);
//        for (size_t i = 0; i < large.size(); ++i) { large[i] = static_cast<char>(payloadByte(11, phase, 1, static_cast<int32_t>(i))); }
//        const int64_t end = now() + 15000;
//        int32_t sent = 0;
//        for (int32_t id = phase == 0 ? 2 : 1; id <= (phase == 0 ? 129 : 1); ++id)
//        {
//            int32_t status = PROCESS_WORK_BUSY;
//            do
//            {
//                status = channel.trySend(phase == 0 ? "123456" : large.data(), phase == 0 ? 6 : 65536, id);
//                if (status != PROCESS_WORK_BUSY) { break; }
//                channel.waitForSpace();
//            } while (now() < end);
//            check(status == PROCESS_WORK_SENT, "U63 actual successful sends in recovering single-slot channel");
//            if (status == PROCESS_WORK_SENT) { ++sent; } else { break; }
//        }
//        check(phase == 0 && sent == 128 && !s_testCommitExpired, "U63 only recovery client exits voluntarily");
//        std::printf("RESULT {\"action\":\"commit-death-client\",\"sent\":%d,\"errors\":%d}\n", sent, errors);
//        return errors == 0 ? 0 : 1;
//#else
//        return 77;
//#endif
//    }
//    if (action == "darwin-notification-name")
//    {
//        std::printf("CASE U61\n");
//        std::set<std::string> unique;
//        const char kinds[] = { 'C', 'B', 'R', 'W' };
//        const std::string alphabet("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-");
//        for (uint32_t sample = 0; sample < 4096; ++sample)
//        {
//            const uint32_t pid = UINT32_MAX - sample;
//            const uint64_t identity = UINT64_MAX - static_cast<uint64_t>(sample) * UINT64_C(0x100000001);
//            const uint32_t session = sample == 0 ? 0xffffff : sample;
//            const uint32_t index = sample % 1024, generation = 0xffffff - sample;
//            for (size_t k = 0; k < sizeof(kinds); ++k)
//            {
//                const char kind = kinds[k];
//                const bool notification = kind == 'R' || kind == 'W';
//                const std::string name = ProcessWorkTestDarwinObjectName(kind, pid, identity, session, index, generation);
//                const size_t prefix = notification ? 2 : 3;
//                const bool sized = name.size() == prefix + 27;
//                check(sized, "U61 exact logical length");
//                check(name.size() + 2 <= (notification ? 31U : 32U), "U61 slash and terminating NUL fit separate Darwin kernel bounds");
//                check(unique.insert(name).second, "U61 unique full name");
//                if (!sized) { continue; }
//                check(name.substr(0, prefix) == std::string(notification ? "P" : "P2") + kind, "U61 distinct type namespace");
//                unsigned char decoded[20] = {};
//                uint32_t pending = 0;
//                int32_t bits = 0, count = 0;
//                bool valid = true;
//                for (size_t i = prefix; i < name.size(); ++i)
//                {
//                    const size_t value = alphabet.find(name[i]);
//                    if (value == std::string::npos) { valid = false; break; }
//                    pending = (pending << 6) | static_cast<uint32_t>(value); bits += 6;
//                    if (bits >= 8)
//                    {
//                        bits -= 8;
//                        if (count >= 20) { valid = false; break; }
//                        decoded[count++] = static_cast<unsigned char>(pending >> bits);
//                    }
//                }
//                uint32_t actualPid = 0, actualSession = 0, actualGeneration = 0;
//                uint64_t actualIdentity = 0;
//                for (int32_t i = 3; i >= 0; --i) { actualPid = (actualPid << 8) | decoded[i]; }
//                for (int32_t i = 7; i >= 0; --i) { actualIdentity = (actualIdentity << 8) | decoded[4 + i]; }
//                for (int32_t i = 2; i >= 0; --i) { actualSession = (actualSession << 8) | decoded[12 + i]; actualGeneration = (actualGeneration << 8) | decoded[17 + i]; }
//                const uint32_t actualIndex = static_cast<uint32_t>(decoded[15]) | (static_cast<uint32_t>(decoded[16]) << 8);
//                check(valid && count == 20 && actualPid == pid && actualIdentity == identity && actualSession == session && actualIndex == index && actualGeneration == generation, "U61 full identity and generation survive encoding");
//            }
//        }
//        std::printf("RESULT {\"action\":\"darwin-notification-name\",\"names\":%u,\"checks\":%d,\"errors\":%d}\n", static_cast<unsigned>(unique.size()), checks, errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "payload-capacity-restart")
//    {
//        std::printf("CASE U60\n");
//        ProcessWorkChannel stale;
//        check(stale.payloadCapacity() == 0, "U60 unopened capacity");
//        ProcessWorkTestReceiver receiver;
//        check(receiver.receivePayloadCapacity() == 0, "U60 facade before init");
//        std::atomic<int32_t> received(0), bad(0);
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            const int32_t expected = static_cast<int32_t>(type);
//            if (pid != self || length != expected || (length == 0 ? data != nullptr : data == nullptr)) { ++bad; }
//            for (int32_t i = 0; data != nullptr && i < length; ++i)
//            {
//                if (static_cast<unsigned char>(data[i]) != static_cast<unsigned char>((i * 73 + 19) % 256)) { ++bad; break; }
//            }
//            ++received;
//        };
//        check(receiver.initReceive(65548, 2), "U60 large receiver");
//        check(receiver.receivePayloadCapacity() == 65536 && stale.openSender(self) && stale.payloadCapacity() == 65536, "U60 exact declared capacity");
//        std::string large(65536, '\0');
//        for (int32_t i = 0; i < 65536; ++i) { large[i] = static_cast<char>((i * 73 + 19) % 256); }
//        receiver.send(self, large, static_cast<MessageType>(65536));
//        check(wait([&]() { return received == 1; }, 3000), "U60 maximum body delivered");
//        receiver.uninitReceive();
//        check(stale.payloadCapacity() == 0 && receiver.receivePayloadCapacity() == 0, "U60 revoked capacity");
//        receiver.addProcessReceiveCallback(&receiver);
//        check(receiver.initReceive(13, 2), "U60 same PID reopens smaller");
//        check(receiver.receivePayloadCapacity() == 1 && stale.payloadCapacity() == 0, "U60 stale mapping stays revoked");
//        check(stale.trySend(large.data(), 65536, 65536) == PROCESS_WORK_UNAVAILABLE, "U60 cannot reuse old large mapping");
//        check(receiver.trySend(self, large.data(), 2) == PROCESS_WORK_TOO_LARGE, "U60 new generation rejects excess without reservation");
//        receiver.send(selfName, large.data(), 1, static_cast<MessageType>(1));
//        check(wait([&]() { return received == 2; }, 3000), "U60 name reaches smaller generation");
//        check(receiver.receiveBufferCount() == 2 && bad == 0, "U60 rejected packet did not poison blocks");
//        receiver.uninitReceive();
//        receiver.addProcessReceiveCallback(&receiver);
//        check(receiver.initReceive(12, 1) && receiver.receivePayloadCapacity() == 0, "U60 empty-only capacity is zero");
//        receiver.send(self, nullptr, 0, static_cast<MessageType>(0));
//        check(wait([&]() { return received == 3; }, 3000), "U60 zero capacity still delivers empty body");
//        check(receiver.trySend(self, large.data(), 1) == PROCESS_WORK_TOO_LARGE, "U60 empty-only excess rejected");
//        std::printf("RESULT {\"action\":\"payload-capacity-restart\",\"received\":%d,\"bad\":%d,\"errors\":%d}\n", received.load(), bad.load(), errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "malformed-head-recovery")
//    {
//        std::printf("CASE U59\n");
//        const int32_t rounds = 112, maximum = 65536;
//        ProcessWorkChannel receiver, sender;
//        LockFreeQueue<char*> queue;
//        Semaphore notification;
//        std::atomic<int32_t> full(0);
//        check(receiver.openReceiver(maximum + 12, 2, 0, 0, 0, 0), "U59 two fixed slots");
//        receiver.publish();
//        SharedMemory discovery("P2D" + std::to_string(self));
//        ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(discovery.writeWithoutLock());
//        if (directory == nullptr) { check(false, "U59 discovery"); return 1; }
//        SharedMemory metadata(ProcessWorkChannel::objectName('C', self, directory->m_identity,
//            static_cast<uint32_t>(directory->m_session)));
//        ProcessWorkControl* control = static_cast<ProcessWorkControl*>(metadata.writeWithoutLock());
//        if (control == nullptr || !sender.openSender(self)) { check(false, "U59 sender opens"); return 1; }
//        receiver.pump(queue, notification, full);
//        std::vector<char> payload(maximum);
//        int32_t received = 0, damaged = 0, bad = 0;
//        const int32_t lengths[] = { 0, 1, 31, maximum };
//        for (int32_t round = 0; round < rounds; ++round)
//        {
//            const int32_t length = lengths[round % 4];
//            for (int32_t i = 0; i < length; ++i) { payload[i] = static_cast<char>(payloadByte(59, 0, round, i)); }
//            const int64_t head = control->m_head;
//            const int32_t first = sender.trySend("123456", 6, round * 2);
//            const int32_t second = sender.trySend(length == 0 ? nullptr : payload.data(), length, round * 2 + 1);
//            check(first == PROCESS_WORK_SENT && second == PROCESS_WORK_SENT, "U59 both source packets committed");
//            if (first != PROCESS_WORK_SENT || second != PROCESS_WORK_SENT) { return 1; }
//            const int64_t entry = AtomicMath::selfAdd(&control->m_ring[head % 2], static_cast<int64_t>(0));
//            const int32_t index = static_cast<int32_t>(entry & 2047) - 1;
//            if (index < 0 || index >= 2) { check(false, "U59 committed block index"); return 1; }
//            SharedMemory corruptible(ProcessWorkChannel::objectName('B', self, directory->m_identity,
//                static_cast<uint32_t>(directory->m_session), index, 1));
//            char* memory = static_cast<char*>(corruptible.writeWithoutLock());
//            if (memory == nullptr) { check(false, "U59 fault mapping"); return 1; }
//            int32_t header[3] = {};
//            std::memcpy(header, memory, sizeof(header));
//            // 本用例没有后台读者；只在确定的接收边界模拟外部头部损坏，不改环和owner。
//            switch (round % 7)
//            {
//            case 0: header[0] = 0; break;
//            case 1: header[0] = INT32_MIN; break;
//            case 2: header[1] = -1; break;
//            case 3: header[2] = -1; break;
//            case 4: header[2] = INT32_MIN; break;
//            case 5: header[2] = maximum + 1; break;
//            default: header[2] = INT32_MAX; break;
//            }
//            std::memcpy(memory, header, sizeof(header));
//            ++damaged;
//            receiver.pump(queue, notification, full);
//            char* raw = nullptr;
//            const bool popped = queue.pop(&raw);
//            std::unique_ptr<char[]> packet(raw);
//            int32_t actual[3] = {};
//            if (raw != nullptr) { std::memcpy(actual, raw, sizeof(actual)); }
//            bool valid = popped && raw != nullptr && actual[0] == self && actual[1] == round * 2 + 1 && actual[2] == length;
//            if (valid)
//            {
//                for (int32_t i = 0; i < length; ++i)
//                {
//                    if (static_cast<unsigned char>(raw[i + 12]) != payloadByte(59, 0, round, i)) { valid = false; break; }
//                }
//                valid = valid && raw[length + 12] == '\0';
//            }
//            if (!valid) { ++bad; }
//            else { ++received; }
//            check(valid && queue.empty(), "U59 only following valid packet delivered with exact bytes");
//            check(notification.wait(0) && !notification.wait(0), "U59 only one successful delivery notification");
//            check(control->m_head == head + 2 && control->m_tail == head + 2,
//                "U59 rejected head does not block later tickets");
//            for (int32_t i = 0; i < 2; ++i)
//            {
//                check(AtomicMath::selfAdd(&control->m_blocks[i].m_owner, static_cast<int64_t>(0)) == 0,
//                    "U59 both slots released and reusable");
//            }
//            if (errors != 0) { return 1; }
//        }
//        check(received == rounds && damaged == rounds && bad == 0 && receiver.bufferCount() == 2, "U59 final controlled-fault ledger");
//        std::printf("RESULT {\"action\":\"malformed-head-recovery\",\"damaged\":%d,\"received\":%d,\"bad\":%d,\"errors\":%d}\n", damaged, received, bad, errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "self-identity-failure")
//    {
//#ifdef PROCESS_WORK_TEST_ALLOCATION
//        std::printf("CASE U58\n");
//        std::atomic<int32_t> received(0), bad(0);
//        ProcessWorkTestReceiver receiver;
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            const int32_t sequence = received.load();
//            if (pid != self || static_cast<int32_t>(type) != sequence || length != 6 || data == nullptr || std::memcmp(data, "123456", 6) != 0) { ++bad; }
//            ++received;
//        };
//        const std::string name = "PW-self-identity-" + std::to_string(self);
//        check(receiver.setReceiveName(name), "U58 logical name");
//        const int32_t before = CTaskThreadManager::Instance().Count();
//        receiver.initPostThread();
//        check(CTaskThreadManager::Instance().Count() == before + 1, "U58 existing Post worker");
//        s_testSelfIdentityHits = 0;
//        s_testSelfIdentityFail = true;
//        const bool accepted = receiver.initReceive(128, 2);
//        s_testSelfIdentityFail = false;
//        check(s_testSelfIdentityHits == 1, "U58 intended self query failed exactly once");
//        check(!accepted, "U58 failed self identity rejects receiver initialization");
//        if (accepted)
//        {
//            // The baseline is allowed to expose the bug and still exit cleanly.
//            receiver.uninitReceive();
//            std::printf("RESULT {\"action\":\"self-identity-failure\",\"accepted_failed_identity\":1,\"received\":0,\"bad\":0,\"errors\":%d}\n", errors);
//            return 1;
//        }
//        check(receiver.receiveBufferCount() == 0 && CTaskThreadManager::Instance().Count() == before + 1,
//            "U58 rollback releases receiver and preserves existing Post worker");
//        check(receiver.trySend(self, "123456", 6) == PROCESS_WORK_UNAVAILABLE, "U58 failed receiver never published");
//        check(receiver.initReceive(128, 2), "U58 retry succeeds without replacing callback or Post task");
//        receiver.post(self, "123456", 6, static_cast<MessageType>(0));
//        receiver.post(name, "123456", 6, static_cast<MessageType>(1));
//        receiver.waitForPostEnd();
//        check(wait([&]() { return received == 2; }, 3000), "U58 both PID and name deliver after retry");
//        receiver.uninitReceive();
//        check(received == 2 && bad == 0 && CTaskThreadManager::Instance().Count() == before,
//            "U58 exact bytes FIFO and no surviving worker");
//        std::printf("RESULT {\"action\":\"self-identity-failure\",\"accepted_failed_identity\":0,\"received\":%d,\"bad\":%d,\"errors\":%d}\n", received.load(), bad.load(), errors);
//        return errors == 0 ? 0 : 1;
//#else
//        std::printf("SKIP U58 controlled identity failure requires test build\n");
//        return 77;
//#endif
//    }
//    if (action == "expanded-writer-boundary")
//    {
//#if defined(_WIN32) && defined(PROCESS_WORK_TEST_ALLOCATION)
//        std::printf("CASE U57\n");
//        int32_t delivered = 0, cancelled = 0, bad = 0;
//        const int32_t length = 65537;
//        std::vector<char> payload(length);
//        for (int32_t i = 0; i < length; ++i) { payload[i] = static_cast<char>(payloadByte(57, 0, 0, i)); }
//        for (int32_t round = 0; round < 3; ++round)
//        {
//            ProcessWorkChannel receiver, sender;
//            LockFreeQueue<char*> queue;
//            Semaphore notification, pause;
//            std::atomic<int32_t> full(0), result(PROCESS_WORK_BUSY);
//            check(receiver.openReceiver(length + 12, 1, 0, 2, 1, 1), "U57 receiver opens");
//            receiver.publish();
//            SharedMemory discovery("P2D" + std::to_string(self));
//            ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(discovery.writeWithoutLock());
//            if (directory == nullptr) { check(false, "U57 discovery"); return 1; }
//            SharedMemory metadata(ProcessWorkChannel::objectName('C', self, directory->m_identity,
//                static_cast<uint32_t>(directory->m_session)));
//            ProcessWorkControl* control = static_cast<ProcessWorkControl*>(metadata.writeWithoutLock());
//            if (control == nullptr || !sender.openSender(self)) { check(false, "U57 sender opens"); return 1; }
//            receiver.pump(queue, notification, full);
//            const int64_t token = AtomicMath::selfAdd(&control->m_peers[0].m_token, static_cast<int64_t>(0));
//            check(token > 0 && AtomicMath::compareAndSwap(&control->m_blocks[0].m_owner, static_cast<int64_t>(0), token), "U57 pin fixed block with live identity");
//            AtomicMath::compareAndSwap(&control->m_demand, 0, 1);
//            check(wait([&]() { receiver.pump(queue, notification, full); return receiver.bufferCount() == 3; }, 2000), "U57 grow two blocks");
//
//            // Pause the real sending call after reserving an expanded block, before its mapping allocation.
//            s_testAllocationSize = sizeof(SharedMemory);
//            s_testAllocationPaused = false;
//            s_testAllocationRelease = false;
//            s_testAllocationHits = 0;
//            std::shared_ptr<ProcessWorkTestTask> task(new ProcessWorkTestTask);
//            task->m_action = [&]()
//            {
//                s_testAllocationThread = ::GetCurrentThreadId();
//                s_testAllocationMode = 5;
//                result = sender.trySend(payload.data(), length, round);
//                s_testAllocationMode = 0;
//            };
//            task->m_stopAction = []() { s_testAllocationRelease = true; };
//            const uint32_t id = CTaskThreadManager::Instance().Init();
//            std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(id);
//            if (!thread) { check(false, "U57 worker"); return 1; }
//            thread->PostTask(task);
//            const bool reached = wait([&]() { return s_testAllocationPaused.load(); }, 2000);
//            check(reached && s_testAllocationHits == 1, "U57 real expanded reservation reached");
//            if (!reached)
//            {
//                CTaskThreadManager::Instance().Uninit(id);
//                s_testAllocationMode = 0;
//                return 1;
//            }
//            check(AtomicMath::selfAdd(&control->m_blocks[1].m_owner, static_cast<int64_t>(0)) == token, "U57 paused sender owns expanded block");
//            const int64_t until = now() + 1300;
//            while (now() < until) { receiver.pump(queue, notification, full); pause.wait(2); }
//            check(receiver.bufferCount() == 2 && AtomicMath::selfAdd(&control->m_blocks[2].m_owner, static_cast<int64_t>(0)) == -1,
//                "U57 unused sibling retires while busy expanded block remains");
//            check(AtomicMath::selfAdd(&control->m_blocks[1].m_owner, static_cast<int64_t>(0)) == token && queue.empty(),
//                "U57 live slow writer neither reclaimed nor prematurely delivered");
//            if (round == 2) { receiver.stop(); }
//            s_testAllocationRelease = true;
//            check(wait([&]() { return task->m_done.load(); }, 3000), "U57 writer completes after release");
//            CTaskThreadManager::Instance().Uninit(id);
//            s_testAllocationMode = 0;
//            check(!task->m_failed, "U57 writer has no exception");
//            if (round == 2)
//            {
//                check(result == PROCESS_WORK_UNAVAILABLE && queue.empty() && control->m_tail == 0,
//                    "U57 stop before commit returns unavailable with no published packet");
//                check(AtomicMath::selfAdd(&control->m_blocks[1].m_owner, static_cast<int64_t>(0)) == 0,
//                    "U57 cancelled reservation returns its expanded block");
//                ++cancelled;
//            }
//            else
//            {
//                check(result == PROCESS_WORK_SENT, "U57 delayed writer commits");
//                receiver.pump(queue, notification, full);
//                char* raw = nullptr;
//                const bool popped = queue.pop(&raw);
//                std::unique_ptr<char[]> packet(raw);
//                int32_t header[3] = {};
//                if (raw != nullptr) { std::memcpy(header, raw, sizeof(header)); }
//                bool valid = popped && raw != nullptr && header[0] == self && header[1] == round && header[2] == length;
//                if (valid)
//                {
//                    for (int32_t i = 0; i < length; ++i)
//                    {
//                        if (static_cast<unsigned char>(raw[i + 12]) != payloadByte(57, 0, 0, i)) { valid = false; break; }
//                    }
//                }
//                if (!valid) { ++bad; }
//                else { ++delivered; }
//                check(valid && queue.empty(), "U57 every byte exact with no duplicate");
//                check(wait([&]() { receiver.pump(queue, notification, full); return receiver.bufferCount() == 1; }, 3000),
//                    "U57 released expanded block becomes idle and retires");
//                // Commit requests maintenance: it may already have regrown the other slot before this drain.
//                // Compare against the actually retired generations, rather than assuming equal batch history.
//                const int64_t retiredFirst = control->m_blocks[1].m_generation;
//                const int64_t retiredSecond = control->m_blocks[2].m_generation;
//                AtomicMath::compareAndSwap(&control->m_demand, 0, 1);
//                check(wait([&]() { receiver.pump(queue, notification, full); return receiver.bufferCount() == 3; }, 2000),
//                    "U57 both retired blocks can grow again");
//                check(control->m_blocks[1].m_generation == retiredFirst + 1 && control->m_blocks[2].m_generation == retiredSecond + 1 &&
//                    control->m_blocks[0].m_generation == 1,
//                    "U57 both mapping generations advance");
//            }
//            check(AtomicMath::compareAndSwap(&control->m_blocks[0].m_owner, token, static_cast<int64_t>(0)),
//                "U57 fixed block survives entire idle period");
//            char* raw = nullptr;
//            while (queue.pop(&raw)) { delete[] raw; }
//        }
//        check(delivered == 2 && cancelled == 1 && bad == 0, "U57 final ledger");
//        std::printf("RESULT {\"action\":\"expanded-writer-boundary\",\"delivered\":%d,\"cancelled\":%d,\"bad\":%d,\"errors\":%d}\n", delivered, cancelled, bad, errors);
//        return errors == 0 ? 0 : 1;
//#else
//        std::printf("SKIP U57 controlled allocation pause requires Windows test build\n");
//        return 77;
//#endif
//    }
//    if (action == "task-id-exhaustion")
//    {
//#ifdef PROCESS_WORK_TEST_ALLOCATION
//        std::printf("CASE U56\n");
//        CTaskThreadManager& manager = CTaskThreadManager::Instance();
//        manager.UninitAll();
//        const uint32_t saved = ProcessWorkTestTaskIdAccess::count(manager);
//        ProcessWorkTestTaskIdAccess::set(manager, UINT32_MAX - 1);
//        const uint32_t last = manager.Init();
//        check(last == UINT32_MAX && manager.GetThreadInterface(last) != nullptr, "U56 last nonzero thread ID remains usable");
//        const int32_t before = manager.Count();
//        const uint32_t exhausted = manager.Init();
//        const int32_t after = manager.Count();
//        check(exhausted == 0 && before == 1 && after == before, "U56 exhausted ID creates no hidden thread registered under zero");
//        manager.UninitAll();
//        ProcessWorkTestTaskIdAccess::set(manager, saved);
//        if (errors != 0)
//        {
//            std::printf("RESULT {\"action\":\"task-id-exhaustion\",\"before\":%d,\"after\":%d,\"errors\":%d}\n", before, after, errors);
//            return 1;
//        }
//        const int32_t producers = 8, remaining = 4;
//        std::atomic<bool> start(false);
//        std::vector<uint32_t> workers, created(producers, 0);
//        std::vector<std::shared_ptr<ProcessWorkTestTask>> tasks;
//        for (int32_t i = 0; i < producers; ++i)
//        {
//            std::shared_ptr<ProcessWorkTestTask> task(new ProcessWorkTestTask);
//            task->m_action = [&, i]()
//            {
//                Semaphore delay;
//                while (!start) { delay.wait(1); }
//                created[i] = manager.Init();
//            };
//            task->m_stopAction = [&]() { start = true; };
//            const uint32_t id = manager.Init();
//            workers.push_back(id);
//            tasks.push_back(task);
//            manager.GetThreadInterface(id)->PostTask(task);
//        }
//        ProcessWorkTestTaskIdAccess::set(manager, UINT32_MAX - remaining);
//        start = true;
//        for (int32_t i = 0; i < producers; ++i) { manager.WaitForEnd(workers[i]); }
//        std::set<uint32_t> ids;
//        int32_t rejected = 0;
//        for (int32_t i = 0; i < producers; ++i)
//        {
//            check(tasks[i]->m_done && !tasks[i]->m_failed, "U56 concurrent allocation worker completes");
//            if (created[i] == 0) { ++rejected; }
//            else
//            {
//                check(created[i] > UINT32_MAX - remaining && ids.insert(created[i]).second &&
//                    manager.GetThreadInterface(created[i]) != nullptr, "U56 concurrent IDs unique and valid");
//            }
//        }
//        check(ids.size() == remaining && rejected == producers - remaining && manager.Count() == remaining &&
//            ProcessWorkTestTaskIdAccess::count(manager) == UINT32_MAX, "U56 concurrent exhaustion never wraps or replaces an existing thread");
//        check(manager.Init() == 0 && manager.Count() == remaining, "U56 repeated exhausted create remains side-effect free");
//        manager.UninitAll();
//        check(manager.Count() == 0, "U56 all real threads joined and removed");
//        ProcessWorkTestTaskIdAccess::set(manager, saved);
//        const uint32_t ordinary = manager.Init();
//        check(ordinary != 0 && manager.GetThreadInterface(ordinary) != nullptr, "U56 ordinary thread creation still usable");
//        manager.UninitAll();
//        std::printf("RESULT {\"action\":\"task-id-exhaustion\",\"created\":%u,\"rejected\":%d,\"errors\":%d}\n", static_cast<unsigned int>(ids.size()), rejected, errors);
//        return errors == 0 ? 0 : 1;
//#else
//        std::printf("SKIP U56 requires the test-only task ID boundary access\n");
//        return 77;
//#endif
//    }
//    if (action == "queue-count-boundary")
//    {
//#ifdef PROCESS_WORK_TEST_ALLOCATION
//        std::printf("CASE U55\n");
//        LockFreeQueue<int32_t> queue;
//        const uint64_t initial = static_cast<uint64_t>(INT32_MAX) - 1;
//        ProcessWorkTestQueueCountAccess::set(queue, initial);
//        queue.push(17);
//        queue.push(23);
//        const bool wronglyEmpty = queue.empty();
//        const uint64_t reported = queue.size();
//        check((std::is_same<decltype(queue.size()), uint64_t>::value), "U55 size return type is uint64_t");
//        check(!wronglyEmpty, "U55 count beyond INT32_MAX is not empty");
//        check(reported == initial + 2, "U55 size returns exact count above INT32_MAX");
//        int32_t value = 0;
//        check(queue.pop(&value) && value == 17, "U55 first real element remains accessible");
//        check(queue.pop(&value) && value == 23, "U55 second real element remains ordered");
//        check(ProcessWorkTestQueueCountAccess::count(queue) == initial, "U55 matching pops restore boundary count");
//        ProcessWorkTestQueueCountAccess::set(queue, 0);
//        check(queue.empty() && queue.size() == 0 && !queue.pop(&value), "U55 normal empty state restored");
//        if (errors == 0)
//        {
//            // 只注入统计快照，不虚构节点；每轮两个真实节点都消费后再恢复计数。
//            const uint64_t edges[] = { static_cast<uint64_t>(UINT32_MAX) - 1,
//                static_cast<uint64_t>(INT64_MAX) - 1 };
//            for (size_t i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i)
//            {
//                ProcessWorkTestQueueCountAccess::set(queue, edges[i]);
//                queue.push(31);
//                queue.push(47);
//                check(!queue.empty() && queue.size() == edges[i] + 2 &&
//                    ProcessWorkTestQueueCountAccess::count(queue) == edges[i] + 2, "U55 larger unsigned count remains exact");
//                check(queue.pop(&value) && value == 31, "U55 larger boundary first value");
//                check(queue.pop(&value) && value == 47, "U55 larger boundary second value");
//                check(ProcessWorkTestQueueCountAccess::count(queue) == edges[i], "U55 larger boundary decrements exactly");
//                ProcessWorkTestQueueCountAccess::set(queue, 0);
//            }
//            for (int32_t i = 0; i < 4096; ++i) { queue.push(i); }
//            check(!queue.empty() && queue.size() == 4096, "U55 ordinary count remains exact");
//            for (int32_t i = 0; i < 4096; ++i) { check(queue.pop(&value) && value == i, "U55 ordinary queue FIFO"); }
//            check(queue.empty() && queue.size() == 0, "U55 ordinary queue drains completely");
//        }
//        std::printf("RESULT {\"action\":\"queue-count-boundary\",\"wrongly_empty\":%d,\"reported\":%llu,\"errors\":%d}\n", wronglyEmpty ? 1 : 0, static_cast<unsigned long long>(reported), errors);
//        return errors == 0 ? 0 : 1;
//#else
//        std::printf("SKIP U55 requires test-only queue counter access\n");
//        return 77;
//#endif
//    }
//    if (action == "local-counter-boundary")
//    {
//#ifdef PROCESS_WORK_TEST_ALLOCATION
//        std::printf("CASE U53\n");
//        Semaphore notification;
//        const uint64_t initial = static_cast<uint64_t>(INT32_MAX) - 1;
//        ProcessWorkTestSemaphoreAccess::set(notification, initial);
//        notification.signal();
//        notification.signal();
//        const uint64_t observed = ProcessWorkTestSemaphoreAccess::count(notification);
//        check(observed == initial + 2, "U53 pending notification count crosses INT32_MAX exactly");
//        if (errors != 0)
//        {
//            notification.reset();
//            std::printf("RESULT {\"action\":\"local-counter-boundary\",\"observed\":%llu,\"errors\":%d}\n", static_cast<unsigned long long>(observed), errors);
//            return 1;
//        }
//        check(notification.wait(0) && notification.wait(0), "U53 large pending count remains consumable");
//        check(ProcessWorkTestSemaphoreAccess::count(notification) == initial, "U53 consumption preserves remaining permits");
//        const int32_t producers = 8, count = 4096;
//        std::vector<uint32_t> workers;
//        std::vector<std::shared_ptr<ProcessWorkTestTask>> tasks;
//        for (int32_t i = 0; i < producers; ++i)
//        {
//            std::shared_ptr<ProcessWorkTestTask> task(new ProcessWorkTestTask);
//            task->m_action = [&]() { for (int32_t j = 0; j < count; ++j) { notification.signal(); } };
//            const uint32_t id = CTaskThreadManager::Instance().Init();
//            workers.push_back(id);
//            tasks.push_back(task);
//            CTaskThreadManager::Instance().GetThreadInterface(id)->PostTask(task);
//        }
//        for (size_t i = 0; i < workers.size(); ++i)
//        {
//            CTaskThreadManager::Instance().WaitForEnd(workers[i]);
//            check(tasks[i]->m_done && !tasks[i]->m_failed, "U53 concurrent producers complete");
//        }
//        check(ProcessWorkTestSemaphoreAccess::count(notification) == initial + producers * count, "U53 exact concurrent permit count beyond signed 32-bit limit");
//        for (int32_t i = 0; i < producers * count; ++i) { check(notification.wait(0), "U53 every generated permit can be consumed"); }
//        check(ProcessWorkTestSemaphoreAccess::count(notification) == initial, "U53 no duplicated or lost permits");
//        ProcessWorkTestSemaphoreAccess::set(notification, UINT64_MAX - 1);
//        notification.signal();
//        bool rejected = false;
//        try { notification.signal(); }
//        catch (const std::overflow_error&) { rejected = true; }
//        check(rejected && ProcessWorkTestSemaphoreAccess::count(notification) == UINT64_MAX, "U53 uint64 exhaustion fails before altering count");
//        check(notification.wait(0) && ProcessWorkTestSemaphoreAccess::count(notification) == UINT64_MAX - 1, "U53 counter remains usable after overflow rejection");
//        notification.reset();
//        check(!notification.wait(0), "U53 reset removes all permits");
//        notification.signal();
//        check(notification.wait(0) && !notification.wait(0), "U53 normal behavior restored");
//        CTaskThreadManager::Instance().UninitAll();
//        std::printf("RESULT {\"action\":\"local-counter-boundary\",\"permits\":%d,\"semaphore_size\":%u,\"errors\":%d}\n", producers * count, static_cast<unsigned int>(sizeof(Semaphore)), errors);
//        return errors == 0 ? 0 : 1;
//#else
//        std::printf("SKIP U53 requires the test-only counter boundary access\n");
//        return 77;
//#endif
//    }
//    if (action == "post-publication-gap")
//    {
//#if defined(_WIN32) && defined(PROCESS_WORK_TEST_ALLOCATION)
//        std::printf("CASE U54\n");
//        const int32_t rounds = 32, total = rounds * 3;
//        std::vector<std::vector<char>> expected(total, std::vector<char>(127));
//        for (int32_t id = 0; id < total; ++id)
//        {
//            for (int32_t offset = 0; offset < 127; ++offset) { expected[id][offset] = static_cast<char>(payloadByte(7, id % 3, id, offset)); }
//        }
//        std::atomic<int32_t> received(0), bad(0);
//        ProcessWorkTestReceiver receiver;
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            const int32_t id = received.load();
//            if (id >= total || static_cast<int32_t>(type) != id || pid != self || length != 127 || data == nullptr ||
//                std::memcmp(data, expected[id].data(), 127) != 0) { ++bad; }
//            ++received;
//        };
//        check(receiver.initReceive(139, 2), "U54 receiver initialized");
//        if (errors != 0) { return 1; }
//        receiver.initPostThread();
//        s_testQueueAddress = reinterpret_cast<uintptr_t>(receiver.postQueueAddress());
//        for (int32_t round = 0; round < rounds && errors == 0; ++round)
//        {
//            const int32_t first = round * 3;
//            s_testQueueReached = false;
//            s_testQueueRelease = false;
//            std::shared_ptr<ProcessWorkTestTask> task(new ProcessWorkTestTask);
//            task->m_action = [&, first, round]()
//            {
//                std::vector<char> input(expected[first]);
//                s_testQueueThread = ::GetCurrentThreadId();
//                if (round % 2 == 0) { receiver.post(self, input.data(), 127, static_cast<MessageType>(first)); }
//                else { receiver.post(selfName, input.data(), 127, static_cast<MessageType>(first)); }
//                input = expected[first + 2];
//                receiver.post(self, input.data(), 127, static_cast<MessageType>(first + 2));
//                std::fill(input.begin(), input.end(), 0);
//            };
//            task->m_stopAction = []() { s_testQueueRelease = true; };
//            const uint32_t worker = CTaskThreadManager::Instance().Init();
//            CTaskThreadManager::Instance().GetThreadInterface(worker)->PostTask(task);
//            const bool reached = wait([&]() { return s_testQueueReached.load(); }, 3000);
//            check(reached, "U54 actual producer paused after tail exchange");
//            if (reached)
//            {
//                std::vector<char> input(expected[first + 1]);
//                receiver.post(selfName, input.data(), 127, static_cast<MessageType>(first + 1));
//                std::fill(input.begin(), input.end(), 0);
//                Semaphore quiet;
//                quiet.wait(25);
//                check(received == first, "U54 completed later producer cannot bypass unpublished head link");
//            }
//            s_testQueueRelease = true;
//            check(wait([&]() { return task->m_done.load(); }, 3000), "U54 producer resumes");
//            CTaskThreadManager::Instance().Uninit(worker);
//            check(!task->m_failed, "U54 producer no unexpected exception");
//            receiver.waitForPostEnd();
//            check(wait([&]() { return received == first + 3; }, 3000), "U54 complete ordered recovery after publication");
//        }
//        s_testQueueThread = 0;
//        s_testQueueAddress = 0;
//        Semaphore quiet;
//        quiet.wait(50);
//        receiver.uninitReceive();
//        CTaskThreadManager::Instance().UninitAll();
//        check(received == total && bad == 0, "U54 no loss, corruption, duplicate or order inversion");
//        std::printf("RESULT {\"action\":\"post-publication-gap\",\"rounds\":%d,\"received\":%d,\"bad\":%d,\"errors\":%d}\n", rounds, received.load(), bad.load(), errors);
//        return errors == 0 ? 0 : 1;
//#else
//        std::printf("SKIP U54 requires the Windows test-only publication pause\n");
//        return 77;
//#endif
//    }
//    if (action == "guarded-input")
//    {
//#ifdef _WIN32
//        std::printf("CASE U49\n");
//        SYSTEM_INFO information = {};
//        ::GetSystemInfo(&information);
//        const size_t page = information.dwPageSize;
//        const size_t accessible = ((65536 + page - 1) / page) * page;
//        void* allocation = ::VirtualAlloc(nullptr, accessible + page * 2, MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
//        check(allocation != nullptr, "U49 guarded allocation available");
//        if (allocation == nullptr) { return 1; }
//        // 所有退出路径均释放完整保留区；参数为VirtualAlloc返回的基址。
//        auto release = [](void* address) { ::VirtualFree(address, 0, MEM_RELEASE); };
//        std::unique_ptr<void, decltype(release)> scope(allocation, release);
//        char* memory = static_cast<char*>(allocation) + page;
//        DWORD previous = 0;
//        check(::VirtualProtect(memory, accessible, PAGE_READWRITE, &previous) != FALSE, "U49 accessible middle pages");
//        if (errors != 0) { return 1; }
//        const int32_t sizes[] = { 0, 1, 2, 3, 6, 7, 15, 16, 31, 32, 63, 64, 127, 128, 255, 256, 4095, 4096, 4097, 65535, 65536 };
//        const int32_t perRoute = static_cast<int32_t>(sizeof(sizes) / sizeof(sizes[0])) * 2;
//        const int32_t total = perRoute * 5;
//        std::vector<std::vector<char>> expected(total);
//        for (int32_t id = 0; id < total; ++id)
//        {
//            expected[id].resize(sizes[(id % perRoute) / 2]);
//            for (size_t offset = 0; offset < expected[id].size(); ++offset)
//            {
//                expected[id][offset] = static_cast<char>(payloadByte(3, id / perRoute, id, static_cast<int32_t>(offset)));
//            }
//        }
//        std::atomic<int32_t> received(0), bad(0);
//        ProcessWorkTestReceiver receiver;
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            const int32_t id = static_cast<int32_t>(type), next = received.load();
//            if (id < 0 || id >= total || id != next || pid != self) { ++bad; }
//            else if (length != static_cast<int32_t>(expected[id].size()) ||
//                (length == 0 ? data != nullptr : data == nullptr || std::memcmp(data, expected[id].data(), length) != 0)) { ++bad; }
//            ++received;
//        };
//        check(receiver.initReceive(65548, 2), "U49 receiver initialized");
//        if (errors != 0) { return 1; }
//        for (int32_t id = 0; id < total && errors == 0; ++id)
//        {
//            const int32_t mode = id / perRoute, length = static_cast<int32_t>(expected[id].size());
//            char* input = length == 0 ? nullptr : memory + (id % 2 == 0 ? 0 : accessible - length);
//            if (length > 0) { std::memcpy(input, expected[id].data(), length); }
//            const MessageType type = static_cast<MessageType>(id);
//            // Post先入队，再禁用调用者内存，最后启动后台线程，确定性排除延迟借用。
//            if (mode == 0) { receiver.send(self, input, length, type); }
//            else if (mode == 1) { receiver.send(selfName, input, length, type); }
//            else if (mode == 2) { receiver.post(self, input, length, type); }
//            else if (mode == 3) { receiver.post(selfName, input, length, type); }
//            else
//            {
//                check(wait([&]() { return receiver.trySend(self, input, length, type) == PROCESS_WORK_SENT; }, 3000), "U49 trySend committed");
//            }
//            check(::VirtualProtect(memory, accessible, PAGE_NOACCESS, &previous) != FALSE, "U49 source revoked immediately after return");
//            if (mode == 2 || mode == 3) { receiver.initPostThread(); }
//            check(wait([&]() { return received >= id + 1; }, 3000), "U49 complete delivery with inaccessible source");
//            if (mode == 2 || mode == 3)
//            {
//                receiver.waitForPostEnd();
//                receiver.uninitPostThread();
//            }
//            check(::VirtualProtect(memory, accessible, PAGE_READWRITE, &previous) != FALSE, "U49 restore source pages");
//        }
//        Semaphore quiet;
//        quiet.wait(50);
//        receiver.uninitReceive();
//        CTaskThreadManager::Instance().UninitAll();
//        check(received == total && bad == 0, "U49 exact bytes, FIFO and no duplicates");
//        scope.reset();
//        MEMORY_BASIC_INFORMATION released = {};
//        check(::VirtualQuery(allocation, &released, sizeof(released)) == sizeof(released) && released.State == MEM_FREE, "U49 guarded allocation fully released");
//        std::printf("RESULT {\"action\":\"guarded-input\",\"expected\":%d,\"received\":%d,\"bad\":%d,\"errors\":%d}\n", total, received.load(), bad.load(), errors);
//        return errors == 0 ? 0 : 1;
//#else
//        std::printf("SKIP U49 Windows guard-page test requires its native environment\n");
//        return 77;
//#endif
//    }
//    if (action == "type-domain")
//    {
//        std::printf("CASE U50\n");
//        check(sizeof(MessageType) == sizeof(int32_t) && std::is_same<std::underlying_type<MessageType>::type, int32_t>::value,
//            "U50 explicit signed 32-bit underlying domain");
//        check(INIT == 0 && MESSAGE == 10 && HEAD == 20 && LOGTEST_MESSAGE == 1000 && LOGTEST_SEND_MESSAGE == 1001 &&
//            LOGTEST_LOCAL_MESSAGE == 1002 && LOGTEST_FILE_MESSAGE == 1003, "U50 historical enum values unchanged");
//        const int32_t types[] = { 0, 1, 1000, 1003, 1023, 1024, 65535, 65536, 0x3fffffff, INT32_MAX - 1, INT32_MAX };
//        const int32_t perRoute = static_cast<int32_t>(sizeof(types) / sizeof(types[0]));
//        const int32_t total = perRoute * 5;
//        std::vector<std::vector<char>> expected(total, std::vector<char>(127));
//        for (int32_t id = 0; id < total; ++id)
//        {
//            for (size_t offset = 0; offset < expected[id].size(); ++offset)
//            {
//                expected[id][offset] = static_cast<char>(payloadByte(5, id / perRoute, id, static_cast<int32_t>(offset)));
//            }
//        }
//        std::atomic<int32_t> received(0), bad(0);
//        ProcessWorkTestReceiver receiver;
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            const int32_t id = received.load();
//            if (id >= total || pid != self || static_cast<int32_t>(type) != types[id % perRoute] ||
//                length != 127 || data == nullptr || std::memcmp(data, expected[id].data(), 127) != 0) { ++bad; }
//            ++received;
//        };
//        check(receiver.initReceive(139, 2), "U50 receiver initialized");
//        if (errors != 0) { return 1; }
//        receiver.initPostThread();
//        for (int32_t id = 0; id < total; ++id)
//        {
//            const int32_t mode = id / perRoute;
//            const MessageType type = static_cast<MessageType>(types[id % perRoute]);
//            std::vector<char> input(expected[id]);
//            if (mode == 0) { receiver.send(self, input.data(), 127, type); }
//            else if (mode == 1) { receiver.send(selfName, input.data(), 127, type); }
//            else if (mode == 2) { receiver.post(self, input.data(), 127, type); }
//            else if (mode == 3) { receiver.post(selfName, input.data(), 127, type); }
//            else
//            {
//                check(wait([&]() { return receiver.trySend(self, input.data(), 127, type) == PROCESS_WORK_SENT; }, 3000), "U50 boundary type committed");
//            }
//            std::fill(input.begin(), input.end(), 0);
//            check(wait([&]() { return received >= id + 1; }, 3000), "U50 type and bytes delivered");
//        }
//        const int32_t negativeTypes[] = { INT32_MIN, -2, -1 };
//        for (size_t i = 0; i < sizeof(negativeTypes) / sizeof(negativeTypes[0]); ++i)
//        {
//            const MessageType type = static_cast<MessageType>(negativeTypes[i]);
//            check(receiver.trySend(self, "123456", 6, type) == PROCESS_WORK_INVALID, "U50 negative type rejected");
//            receiver.send(self, "123456", 6, type);
//            receiver.send(selfName, "123456", 6, type);
//            receiver.post(self, "123456", 6, type);
//            receiver.post(selfName, "123456", 6, type);
//        }
//        receiver.waitForPostEnd();
//        Semaphore quiet;
//        quiet.wait(50);
//        receiver.uninitReceive();
//        CTaskThreadManager::Instance().UninitAll();
//        check(received == total && bad == 0, "U50 exact valid receipts and no negative deliveries");
//        std::printf("RESULT {\"action\":\"type-domain\",\"expected\":%d,\"received\":%d,\"bad\":%d,\"errors\":%d}\n", total, received.load(), bad.load(), errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "darwin-exit-model")
//    {
//#ifdef _WIN32
//        std::printf("CASE U51\n");
//        // Windows仅替代系统返回值；被测函数体由构建器从CSystem.cpp原样提取并选择Apple分支。
//        // 参数依次为sysctl结果、长度模式、进程状态、kill结果、errno；-1长度表示完整记录。
//        ProcessWorkTestDarwinResult(0, -1, 5, 0, 0);
//        check(!ProcessWorkTestDarwinExited(0) && !ProcessWorkTestDarwinExited(UINT32_MAX), "U51 invalid PID never classified dead");
//        check(ProcessWorkTestDarwinExited(123), "U51 zombie recognized although kill reports success");
//        ProcessWorkTestDarwinResult(0, -1, 2, 0, 0);
//        check(!ProcessWorkTestDarwinExited(123), "U51 running process retained");
//        ProcessWorkTestDarwinResult(0, -1, 4, 0, 0);
//        check(!ProcessWorkTestDarwinExited(123), "U51 stopped process retained");
//        ProcessWorkTestDarwinResult(-1, -1, 5, 0, EPERM);
//        check(!ProcessWorkTestDarwinExited(123), "U51 failed query is not death evidence");
//        ProcessWorkTestDarwinResult(0, 0, 5, 0, 0);
//        check(!ProcessWorkTestDarwinExited(123), "U51 empty query is not death evidence");
//        ProcessWorkTestDarwinResult(0, 1, 5, 0, 0);
//        check(!ProcessWorkTestDarwinExited(123), "U51 truncated zombie record ignored");
//        ProcessWorkTestDarwinResult(-1, -1, 5, -1, EPERM);
//        check(!ProcessWorkTestDarwinExited(123), "U51 permission failure retained");
//        ProcessWorkTestDarwinResult(-1, 0, 0, -1, ESRCH);
//        check(ProcessWorkTestDarwinExited(123), "U51 confirmed absent PID recovered");
//        std::printf("RESULT {\"action\":\"darwin-exit-model\",\"checks\":%d,\"errors\":%d}\n", checks, errors);
//        return errors == 0 ? 0 : 1;
//#else
//        std::printf("SKIP U51 Windows system-response model; use U52 on macOS\n");
//        return 77;
//#endif
//    }
//    if (action == "darwin-zombie")
//    {
//#if defined(__APPLE__) && !TARGET_OS_IPHONE
//        std::printf("CASE U52\n");
//        // 在任何IPC初始化和工作线程创建之前fork；子进程只等管道并正常退出，不继承活动IPC实例。
//        int descriptors[2] = { -1, -1 };
//        if (::pipe(descriptors) != 0) { return 3; }
//        const pid_t child = ::fork();
//        if (child < 0) { ::close(descriptors[0]); ::close(descriptors[1]); return 3; }
//        if (child == 0)
//        {
//            ::close(descriptors[1]);
//            char command = 0;
//            ::read(descriptors[0], &command, 1);
//            ::close(descriptors[0]);
//            ::_exit(0);
//        }
//        ::close(descriptors[0]);
//        check(!CSystem::processDefinitelyExited(static_cast<uint32_t>(child)), "U52 live child retained");
//        const char command = 1;
//        check(::write(descriptors[1], &command, 1) == 1, "U52 release child normal exit");
//        ::close(descriptors[1]);
//        check(wait([&]()
//        {
//            int query[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, static_cast<int>(child) };
//            struct kinfo_proc info = {};
//            size_t bytes = sizeof(info);
//            return ::sysctl(query, 4, &info, &bytes, nullptr, 0) == 0 && bytes == sizeof(info) && info.kp_proc.p_stat == SZOMB;
//        }, 3000), "U52 native zombie exists before waitpid");
//        check(CSystem::processDefinitelyExited(static_cast<uint32_t>(child)), "U52 library detects unreaped native zombie");
//        int status = 0;
//        pid_t reaped = -1;
//        do { reaped = ::waitpid(child, &status, 0); } while (reaped < 0 && errno == EINTR);
//        check(reaped == child && WIFEXITED(status) && WEXITSTATUS(status) == 0, "U52 child reaped normally");
//        std::printf("RESULT {\"action\":\"darwin-zombie\",\"errors\":%d}\n", errors);
//        return errors == 0 ? 0 : 1;
//#else
//        std::printf("SKIP U52 native macOS zombie test requires macOS\n");
//        return 77;
//#endif
//    }
//    if (action == "name-binding-boundary")
//    {
//#if defined(_WIN32) && defined(PROCESS_WORK_TEST_ALLOCATION)
//        std::printf("CASE U48\n");
//        std::atomic<int32_t> wrong(0);
//        for (int32_t mode = 0; mode < 2; ++mode)
//        {
//            const std::string name = "NameBoundary" + std::to_string(self) + "_" + std::to_string(mode);
//            ProcessWorkTestReceiver original, replacement, sender;
//            replacement.m_message = [&](int32_t, const char*, int32_t, MessageType) { ++wrong; };
//            const bool initialized = original.setReceiveName(name) && original.initReceive(18, 2);
//            check(initialized, "U48 original named receiver ready");
//            if (!initialized) { return 1; }
//            s_testNameReached = false;
//            s_testNameRelease = false;
//            s_testNamePause = true;
//            uint32_t worker = 0;
//            std::shared_ptr<ProcessWorkTestTask> job(new ProcessWorkTestTask);
//            if (mode == 0)
//            {
//                job->m_action = [&]() { sender.send(name, "123456", 6); };
//                worker = CTaskThreadManager::Instance().Init();
//                CTaskThreadManager::Instance().GetThreadInterface(worker)->PostTask(job);
//            }
//            else
//            {
//                sender.initPostThread();
//                sender.post(name, "123456", 6);
//            }
//            check(wait([&]() { return s_testNameReached.load(); }, 3000), "U48 paused after real identity-checked name lookup");
//            original.uninitReceive();
//            check(replacement.setReceiveName(name + "Other") && replacement.initReceive(18, 2), "U48 same PID now owns a different logical receiver");
//            s_testNameRelease = true;
//            if (mode == 0)
//            {
//                CTaskThreadManager::Instance().WaitForEnd(worker);
//                check(!job->m_failed, "U48 sender completed without exception");
//            }
//            else
//            {
//                sender.waitForPostEnd();
//            }
//            s_testNamePause = false;
//            Semaphore observation;
//            observation.wait(100);
//            sender.uninitReceive();
//            replacement.uninitReceive();
//            check(wrong == 0, "U48 resolved name must not cross into replacement receiver with another name");
//        }
//        std::printf("RESULT {\"action\":\"name-binding-boundary\",\"wrong_receiver\":%d,\"errors\":%d}\n", wrong.load(), errors);
//        return errors == 0 ? 0 : 1;
//#else
//        std::printf("SKIP U48 deterministic test-build boundary unavailable\n");
//        return 77;
//#endif
//    }
//
//    if (action == "peer-capacity-server")
//    {
//        std::printf("CASE U46\n");
//        std::atomic<int32_t> received(0), bad(0);
//        std::vector<int32_t> seen(257, 0), pids(257, 0);
//        ProcessWorkTestReceiver receiver;
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            const int32_t index = static_cast<int32_t>(type);
//            if (index < 0 || index >= 257 || pid <= 0 || length != 6 || data == nullptr || std::memcmp(data, "123456", 6) != 0)
//            {
//                ++bad;
//            }
//            else
//            {
//                if (++seen[index] != 1) { ++bad; }
//                pids[index] = pid;
//            }
//            ++received;
//        };
//        check(receiver.initReceive(18, 8), "U46 receiver ready for 256 distinct live peers");
//        if (errors != 0) { return 1; }
//        std::printf("READY %d\n", self);
//        std::string command;
//        while (std::getline(std::cin, command))
//        {
//            if (command == "finish") { break; }
//            const int32_t target = std::atoi(command.c_str());
//            check((target == 256 || target == 257) && wait([&]() { return received == target; }, 10000), "U46 exact committed count at controlled boundary");
//            std::printf("COUNT %d BAD %d\n", received.load(), bad.load());
//        }
//        receiver.uninitReceive();
//        check(received == 257 && bad == 0, "U46 capacity recovery preserves every committed packet");
//        for (size_t i = 0; i < seen.size(); ++i)
//        {
//            check(seen[i] == 1, "U46 every successful peer delivered once");
//            for (size_t j = 0; j < i; ++j) { check(pids[i] != pids[j], "U46 real distinct sender PIDs"); }
//        }
//        std::printf("RESULT {\"action\":\"peer-capacity-server\",\"received\":%d,\"bad\":%d,\"errors\":%d}\n", received.load(), bad.load(), errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "peer-capacity-client")
//    {
//        if (argc != 4) { return 2; }
//        const int32_t target = std::atoi(argv[2]), index = std::atoi(argv[3]);
//        if (target <= 0 || index < 0 || index >= 257) { return 2; }
//        ProcessWorkTestReceiver sender;
//        int32_t accepted = 0, attempt = 0;
//        std::string command = "first";
//        do
//        {
//            if (command == "quit") { break; }
//            int32_t status = PROCESS_WORK_UNAVAILABLE;
//            const int64_t end = now() + 10000;
//            Semaphore delay;
//            do
//            {
//                status = sender.trySend(target, "123456", 6, static_cast<MessageType>(index));
//                if (status == PROCESS_WORK_SENT || (status != PROCESS_WORK_BUSY && command != "retry")) { break; }
//                delay.wait(2);
//            } while (now() < end);
//            if (status == PROCESS_WORK_SENT) { ++accepted; }
//            std::printf("ATTEMPT %d STATUS %d PID %d\n", ++attempt, status, self);
//        } while (std::getline(std::cin, command));
//        sender.clear();
//        check(accepted == 1, "U46 one accepted message per live client");
//        std::printf("RESULT {\"action\":\"peer-capacity-client\",\"index\":%d,\"accepted\":%d,\"errors\":%d}\n", index, accepted, errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "try-result-ledger")
//    {
//        std::printf("CASE U47\n");
//        const int32_t threads = 8, attempts = 2048, total = threads * attempts;
//        std::vector<std::vector<char>> expected(threads);
//        std::vector<int32_t> status(total, PROCESS_WORK_INVALID), seen(total, 0), last(threads, -1);
//        for (int32_t thread = 0; thread < threads; ++thread)
//        {
//            expected[thread].resize(65536);
//            for (size_t offset = 0; offset < expected[thread].size(); ++offset)
//            {
//                expected[thread][offset] = static_cast<char>((offset * 71 + thread * 193 + offset / 251) & 255);
//            }
//        }
//        std::atomic<int32_t> received(0), bad(0);
//        ProcessWorkTestReceiver receiver;
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            const int32_t id = static_cast<int32_t>(type), thread = id / attempts, sequence = id % attempts;
//            if (pid != self || id < 0 || id >= total) { ++bad; return; }
//            const int32_t lengths[8] = { 0, 1, 6, 127, 4095, 4096, 65535, 65536 };
//            if (++seen[id] != 1 || sequence <= last[thread] || length != lengths[sequence % 8] ||
//                (length == 0 ? data != nullptr : data == nullptr || std::memcmp(data, expected[thread].data(), length) != 0)) { ++bad; }
//            last[thread] = sequence;
//            ++received;
//        };
//        check(receiver.initReceive(65548, 1), "U47 single-slot receiver initialized");
//        if (errors != 0) { return 1; }
//        // 用真实空报文完成登记，随后暂停读取以确定性制造一个成功和六个BUSY。
//        check(wait([&]() { return receiver.trySend(self, nullptr, 0, static_cast<MessageType>(0)) == PROCESS_WORK_SENT; }, 3000), "U47 warmup committed");
//        check(wait([&]() { return received == 1; }, 3000), "U47 warmup delivered");
//        receiver.pauseReader();
//        status[0] = PROCESS_WORK_SENT;
//        for (int32_t thread = 1; thread < threads; ++thread)
//        {
//            status[thread * attempts] = receiver.trySend(self, nullptr, 0, static_cast<MessageType>(thread * attempts));
//        }
//        check(status[attempts] == PROCESS_WORK_SENT, "U47 paused slot accepted once");
//        for (int32_t thread = 2; thread < threads; ++thread) { check(status[thread * attempts] == PROCESS_WORK_BUSY, "U47 BUSY is uncommitted"); }
//        check(receiver.resumeReader(), "U47 real reader resumed");
//        std::atomic<bool> start(false);
//        std::vector<uint32_t> workers;
//        std::vector<std::shared_ptr<ProcessWorkTestTask>> jobs;
//        for (int32_t thread = 0; thread < threads; ++thread)
//        {
//            std::shared_ptr<ProcessWorkTestTask> job(new ProcessWorkTestTask);
//            job->m_action = [&, thread]()
//            {
//                Semaphore delay;
//                while (!start) { delay.wait(1); }
//                std::vector<char> input(65538);
//                const int32_t lengths[8] = { 0, 1, 6, 127, 4095, 4096, 65535, 65536 };
//                for (int32_t sequence = 1; sequence < attempts; ++sequence)
//                {
//                    const int32_t id = thread * attempts + sequence, length = lengths[sequence % 8];
//                    std::memcpy(input.data() + 1, expected[thread].data(), length);
//                    const char* data = length == 0 ? nullptr : input.data() + 1;
//                    if (sequence % 37 == 0)
//                    {
//                        status[id] = receiver.trySend(self, data, -1, static_cast<MessageType>(id));
//                    }
//                    else if (sequence % 41 == 0)
//                    {
//                        status[id] = receiver.trySend(self, input.data(), 65537, static_cast<MessageType>(id));
//                    }
//                    else
//                    {
//                        status[id] = receiver.trySend(self, data, length, static_cast<MessageType>(id));
//                    }
//                    std::memset(input.data(), 0xa5, input.size());
//                }
//            };
//            const uint32_t worker = CTaskThreadManager::Instance().Init();
//            workers.push_back(worker);
//            jobs.push_back(job);
//            CTaskThreadManager::Instance().GetThreadInterface(worker)->PostTask(job);
//        }
//        start = true;
//        for (size_t i = 0; i < workers.size(); ++i) { CTaskThreadManager::Instance().WaitForEnd(workers[i]); }
//        int32_t sent = 0, busy = 0, invalid = 0, tooLarge = 0;
//        for (int32_t id = 0; id < total; ++id)
//        {
//            if (status[id] == PROCESS_WORK_SENT) { ++sent; }
//            else if (status[id] == PROCESS_WORK_BUSY) { ++busy; }
//            else if (status[id] == PROCESS_WORK_INVALID) { ++invalid; }
//            else if (status[id] == PROCESS_WORK_TOO_LARGE) { ++tooLarge; }
//            else { check(false, "U47 unexpected return code"); }
//        }
//        check(wait([&]() { return received == sent; }, 10000), "U47 receive count matches independently recorded successful returns");
//        receiver.uninitReceive();
//        for (int32_t id = 0; id < total; ++id) { check(seen[id] == (status[id] == PROCESS_WORK_SENT ? 1 : 0), "U47 exact set equality: only SENT is delivered once"); }
//        for (size_t i = 0; i < jobs.size(); ++i) { check(!jobs[i]->m_failed, "U47 worker completed without exception"); }
//        check(busy > 0 && invalid > 0 && tooLarge > 0 && bad == 0 && received == sent, "U47 all rejection classes and FIFO bytes verified");
//        std::printf("RESULT {\"action\":\"try-result-ledger\",\"attempts\":%d,\"sent\":%d,\"received\":%d,\"busy\":%d,\"invalid\":%d,\"too_large\":%d,\"bad\":%d,\"errors\":%d}\n", total, sent, received.load(), busy, invalid, tooLarge, bad.load(), errors);
//        return errors == 0 ? 0 : 1;
//    }
//
//    if (action == "linux-shm-quota")
//    {
//        std::printf("CASE U45\n");
//#if defined(__linux__) && !defined(__ANDROID__)
//        // 只供宿主已隔离挂载的8MiB /dev/shm；不得在日常共享挂载上制造资源耗尽。
//        const char* isolated = std::getenv("PROCESS_WORK_TEST_ISOLATED_SHM");
//        struct statvfs info = {};
//        if (isolated == nullptr || std::strcmp(isolated, "8MiB-private") != 0 || ::statvfs("/dev/shm", &info) != 0 ||
//            static_cast<uint64_t>(info.f_blocks) * info.f_frsize != 8U * 1024U * 1024U ||
//            static_cast<uint64_t>(info.f_bavail) * info.f_frsize < 2U * 1024U * 1024U)
//        {
//            std::printf("SKIP U45 requires explicitly isolated 8MiB tmpfs\n");
//            return 77;
//        }
//        const std::string name = "P2Quota" + std::to_string(self);
//        {
//            SharedMemory missing(name);
//            check(missing.writeWithoutLock() == nullptr, "U45 fixture name initially absent");
//        }
//        if (errors != 0) { return 1; }
//        {
//            SharedMemory oversized(name, 16U * 1024U * 1024U, false);
//            // 改前也不访问未预留的映射页，避免用SIGBUS当测试断言。
//            check(oversized.isFailed(), "U45 quota failure is reported during creation");
//        }
//        {
//            SharedMemory missing(name);
//            check(missing.writeWithoutLock() == nullptr, "U45 failed creation leaves no name");
//        }
//        {
//            SharedMemory valid(name, 1024U * 1024U, false);
//            char* bytes = static_cast<char*>(valid.writeWithoutLock());
//            check(!valid.isFailed() && bytes != nullptr && valid.realSize() == 1024U * 1024U, "U45 smaller retry succeeds after rollback");
//            if (bytes != nullptr)
//            {
//                std::memset(bytes, 0x5a, 1024U * 1024U);
//                bool correct = true;
//                for (size_t offset = 0; offset < 1024U * 1024U; ++offset)
//                {
//                    if (bytes[offset] != 0x5a) { correct = false; break; }
//                }
//                check(correct, "U45 all reserved bytes are writable and readable");
//            }
//        }
//        std::printf("RESULT {\"action\":\"linux-shm-quota\",\"checks\":%d,\"errors\":%d}\n", checks, errors);
//        return errors == 0 ? 0 : 1;
//#else
//        std::printf("SKIP U45 requires Linux native tmpfs environment\n");
//        return 77;
//#endif
//    }
//
//    if (action == "seeded-payload")
//    {
//        std::printf("CASE U41\n");
//        if (argc != 4) { return 2; }
//        const uint32_t seed = static_cast<uint32_t>(std::strtoul(argv[2], nullptr, 10));
//        const int32_t mode = std::atoi(argv[3]);
//        if (mode != 0 && mode != 4) { return 2; }
//        const int32_t threads = 8, count = 512, total = threads * count;
//        // 先生成独立保留的参考正文；发送者只改写自己的副本，观察者不读取发送缓冲。
//        std::vector<std::vector<std::vector<char>>> expected(threads);
//        uint32_t random = seed;
//        for (int32_t thread = 0; thread < threads; ++thread)
//        {
//            expected[thread].resize(count);
//            for (int32_t sequence = 0; sequence < count; ++sequence)
//            {
//                random = random * 1664525U + 1013904223U;
//                const int32_t lengths[8] = { 0, 1, 6, 127, 4095, 4096, 65536, static_cast<int32_t>(random % 65537) };
//                std::vector<char>& body = expected[thread][sequence];
//                body.resize(lengths[sequence % 8]);
//                for (size_t offset = 0; offset < body.size(); ++offset)
//                {
//                    random ^= random << 13;
//                    random ^= random >> 17;
//                    random ^= random << 5;
//                    body[offset] = static_cast<char>(random >> 8);
//                }
//            }
//        }
//        ProcessWorkTestReceiver receiver;
//        std::atomic<int32_t> received(0), bad(0);
//        std::vector<int32_t> next(threads, 0);
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            const int32_t value = static_cast<int32_t>(type), thread = value / 100000, sequence = value % 100000;
//            if (pid != self || thread < 0 || thread >= threads || sequence < 0 || sequence >= count) { ++bad; return; }
//            const std::vector<char>& body = expected[thread][sequence];
//            if (sequence != next[thread]++ || length != body.size() ||
//                (length == 0 ? data != nullptr : data == nullptr || std::memcmp(data, body.data(), body.size()) != 0)) { ++bad; }
//            ++received;
//        };
//        check(receiver.initReceive(65548, 3, 1, 2, 1, 4), "U41 initialize exact 65536 body capacity");
//        if (errors != 0) { return 1; }
//        if (mode == 4) { receiver.initPostThread(); }
//        std::atomic<bool> start(false);
//        std::vector<std::shared_ptr<ProcessWorkTestTask>> jobs;
//        std::vector<uint32_t> workers;
//        for (int32_t thread = 0; thread < threads; ++thread)
//        {
//            std::shared_ptr<ProcessWorkTestTask> job(new ProcessWorkTestTask);
//            job->m_action = [&, thread]()
//            {
//                Semaphore delay;
//                while (!start) { delay.wait(1); }
//                std::vector<char> buffer(65538);
//                for (int32_t sequence = 0; sequence < count; ++sequence)
//                {
//                    const std::vector<char>& body = expected[thread][sequence];
//                    if (!body.empty()) { std::memcpy(buffer.data() + 1, body.data(), body.size()); }
//                    const char* data = body.empty() ? nullptr : buffer.data() + 1;
//                    const MessageType type = static_cast<MessageType>(thread * 100000 + sequence);
//                    if (mode == 0) { receiver.send(self, data, static_cast<int32_t>(body.size()), type); }
//                    else if (sequence % 2 == 0) { receiver.post(self, data, static_cast<int32_t>(body.size()), type); }
//                    else { receiver.post(selfName, data, static_cast<int32_t>(body.size()), type); }
//                    std::memset(buffer.data(), 0xa5, buffer.size());
//                }
//            };
//            const uint32_t worker = CTaskThreadManager::Instance().Init();
//            jobs.push_back(job);
//            workers.push_back(worker);
//            CTaskThreadManager::Instance().GetThreadInterface(worker)->PostTask(job);
//        }
//        start = true;
//        for (size_t i = 0; i < workers.size(); ++i) { CTaskThreadManager::Instance().WaitForEnd(workers[i]); }
//        if (mode == 4) { receiver.waitForPostEnd(); }
//        check(wait([&]() { return received == total; }, 10000), "U41 all seeded packets delivered");
//        for (size_t i = 0; i < jobs.size(); ++i) { check(!jobs[i]->m_failed, "U41 producer did not throw"); }
//        check(received == total && bad == 0, "U41 all lengths bytes and per-thread order match independent reference");
//        check(wait([&]() { return receiver.receiveBufferCount() == 3; }, 4000), "U41 shrink returns to initial capacity");
//        receiver.uninitReceive();
//        check(received == total && bad == 0, "U41 no late duplicates after joined shutdown");
//        std::printf("RESULT {\"action\":\"seeded-payload\",\"seed\":%u,\"mode\":%d,\"received\":%d,\"bad\":%d,\"errors\":%d}\n", seed, mode, received.load(), bad.load(), errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "post-drain-cycles")
//    {
//        std::printf("CASE U42\n");
//        const int32_t cycles = 64, perCycle = 257, total = cycles * perCycle;
//        const int32_t originalThreads = CTaskThreadManager::Instance().Count();
//        ProcessWorkTestReceiver receiver;
//        std::atomic<int32_t> received(0), bad(0);
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            const int32_t sequence = received.load(), expectedLength = sequence % 3 == 0 ? 0 : 6;
//            if (pid != self || static_cast<int32_t>(type) != sequence || length != expectedLength ||
//                (length == 0 ? data != nullptr : data == nullptr || std::memcmp(data, "123456", 6) != 0)) { ++bad; }
//            ++received;
//        };
//        check(receiver.initReceive(18, 1), "U42 single-slot receiver initialized");
//        receiver.initPostThread();
//        const uint32_t beforeHandles = handles();
//        for (int32_t cycle = 0; cycle < cycles && errors == 0; ++cycle)
//        {
//            for (int32_t i = 0; i < perCycle; ++i)
//            {
//                const int32_t sequence = cycle * perCycle + i, length = sequence % 3 == 0 ? 0 : 6;
//                if (sequence % 2 == 0) { receiver.post(self, length == 0 ? nullptr : "123456", length, static_cast<MessageType>(sequence)); }
//                else { receiver.post(selfName, length == 0 ? nullptr : "123456", length, static_cast<MessageType>(sequence)); }
//            }
//            receiver.waitForPostEnd();
//            check(wait([&]() { return received == (cycle + 1) * perCycle; }, 4000), "U42 drained batch delivered before next lifecycle");
//            check(CTaskThreadManager::Instance().Count() == originalThreads + 3, "U42 exactly two receive tasks and one replacement post task");
//        }
//        const uint32_t afterHandles = handles();
//        receiver.uninitReceive();
//        check(received == total && bad == 0, "U42 complete ordered stream across 64 drain/restart cycles");
//        check(CTaskThreadManager::Instance().Count() == originalThreads, "U42 all managed threads released");
//        check(afterHandles <= beforeHandles + 12, "U42 no cumulative handle growth");
//        std::printf("RESULT {\"action\":\"post-drain-cycles\",\"received\":%d,\"bad\":%d,\"handles_before\":%u,\"handles_after\":%u,\"errors\":%d}\n", received.load(), bad.load(), beforeHandles, afterHandles, errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "callback-reentrant")
//    {
//        std::printf("CASE U43\n");
//        const int32_t total = 10000;
//        ProcessWorkTestReceiver receiver;
//        std::atomic<int32_t> received(0), bad(0), depth(0);
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            const int32_t sequence = received.load();
//            if (++depth != 1 || pid != self || static_cast<int32_t>(type) != sequence || length != 6 || data == nullptr || std::memcmp(data, "123456", 6) != 0) { ++bad; }
//            ++received;
//            if (sequence + 1 < total) { receiver.send(self, "123456", 6, static_cast<MessageType>(sequence + 1)); }
//            --depth;
//        };
//        check(receiver.initReceive(18, 1), "U43 single-slot receiver initialized");
//        receiver.send(self, "123456", 6, static_cast<MessageType>(0));
//        check(wait([&]() { return received == total; }, 20000), "U43 callback can send without blocking own reader");
//        receiver.uninitReceive();
//        check(received == total && bad == 0 && depth == 0, "U43 nonrecursive callbacks correct and completely joined");
//        std::printf("RESULT {\"action\":\"callback-reentrant\",\"received\":%d,\"bad\":%d,\"errors\":%d}\n", received.load(), bad.load(), errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "full-callback-exception")
//    {
//        std::printf("CASE U44\n");
//        ProcessWorkTestReceiver receiver, observer;
//        std::atomic<int32_t> received(0), observed(0), bad(0), fullFirst(0), fullSecond(0);
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            if (pid != self || static_cast<int32_t>(type) != received || length != 6 || data == nullptr || std::memcmp(data, "123456", 6) != 0) { ++bad; }
//            ++received;
//        };
//        observer.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            if (pid != self || static_cast<int32_t>(type) != observed || length != 6 || data == nullptr || std::memcmp(data, "123456", 6) != 0) { ++bad; }
//            ++observed;
//        };
//        receiver.m_full = [&](int32_t count)
//        {
//            if (count != 2) { ++bad; }
//            ++fullFirst;
//            throw std::runtime_error("expected full callback exception");
//        };
//        observer.m_full = [&](int32_t count) { if (count != 2) { ++bad; } ++fullSecond; };
//        receiver.addProcessReceiveCallback(&observer);
//        check(receiver.initReceive(18, 2), "U44 receiver initialized");
//        uint64_t identity = 0;
//        check(CSystem::processIdentity(self, identity), "U44 process identity available");
//        SharedMemory directoryMemory("P2D" + std::to_string(self));
//        ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//        check(directory != nullptr, "U44 directory mapped");
//        if (directory == nullptr) { receiver.uninitReceive(); return 1; }
//        const int64_t session = AtomicMath::selfAdd(&directory->m_session, static_cast<int64_t>(0));
//        SharedMemory controlMemory(ProcessWorkChannel::objectName('C', self, identity, static_cast<uint32_t>(session)));
//        ProcessWorkControl* control = static_cast<ProcessWorkControl*>(controlMemory.writeWithoutLock());
//        check(control != nullptr, "U44 control mapped");
//        if (control == nullptr) { receiver.uninitReceive(); return 1; }
//        receiver.send(self, "123456", 6, static_cast<MessageType>(0));
//        check(wait([&]() { return observed == 1; }, 3000), "U44 registration warmup delivered");
//        for (int32_t cycle = 0; cycle < 10 && errors == 0; ++cycle)
//        {
//            receiver.pauseReader();
//            const int32_t first = 1 + cycle * 3;
//            check(receiver.trySend(self, "123456", 6, static_cast<MessageType>(first)) == PROCESS_WORK_SENT, "U44 first slot committed");
//            check(receiver.trySend(self, "123456", 6, static_cast<MessageType>(first + 1)) == PROCESS_WORK_SENT, "U44 second slot committed");
//            check(receiver.trySend(self, "123456", 6, static_cast<MessageType>(first + 2)) == PROCESS_WORK_BUSY, "U44 full attempt is uncommitted");
//            check(receiver.resumeReader(), "U44 reader resumes");
//            check(wait([&]() { return fullFirst > cycle && fullSecond > cycle && observed == first + 2; }, 3000), "U44 throwing first observer cannot suppress full event or packets for second");
//            receiver.send(self, "123456", 6, static_cast<MessageType>(first + 2));
//            check(wait([&]() { return observed == first + 3; }, 3000), "U44 subsequent send remains usable");
//            // 等到维护确认上一满载阶段结束，下一轮才要求一个新的合并事件。
//            check(wait([&]() { return AtomicMath::selfAdd(&control->m_full, 0) == 0; }, 3000), "U44 full episode resets after capacity returns");
//        }
//        receiver.uninitReceive();
//        check(received == 31 && observed == 31 && fullFirst == 10 && fullSecond == 10 && bad == 0, "U44 exact packets snapshots and observer isolation");
//        std::printf("RESULT {\"action\":\"full-callback-exception\",\"received\":%d,\"observed\":%d,\"bad\":%d,\"full_first\":%d,\"full_second\":%d,\"errors\":%d}\n", received.load(), observed.load(), bad.load(), fullFirst.load(), fullSecond.load(), errors);
//        return errors == 0 ? 0 : 1;
//    }
//
//    if (action == "close-lease")
//    {
//#ifdef _WIN32
//        // 旧接收器真实close，与下一初始化者的0->-1原子认领交错；不修改生产实现插入停顿。
//        std::printf("CASE U40\n");
//        const int32_t rounds = argc > 2 ? std::atoi(argv[2]) : 20000;
//        if (rounds < 1 || rounds > 200000) { return 2; }
//        ProcessWorkChannel receiver;
//        check(receiver.openReceiver(64, 1, 0, 0, 0, 0), "U40 initial receiver prepared");
//        if (errors != 0) { return 1; }
//        receiver.publish();
//        SharedMemory directoryMemory("P2D" + std::to_string(self));
//        ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//        check(directory != nullptr, "U40 retain directory across close");
//        if (directory == nullptr) { return 1; }
//        std::atomic<int32_t> request(0), entered(0), acquired(0);
//        std::atomic<bool> finished(false);
//        std::shared_ptr<ProcessWorkTestTask> contender(new ProcessWorkTestTask);
//        contender->m_action = [&]()
//        {
//            int32_t round = 0;
//            while (!finished && !contender->m_stop)
//            {
//                const int32_t requested = request.load();
//                if (requested == round) { continue; }
//                round = requested;
//                entered = round;
//                while (!finished && !contender->m_stop)
//                {
//                    if (AtomicMath::compareAndSwap(&directory->m_session, static_cast<int64_t>(0), static_cast<int64_t>(-1)))
//                    {
//                        acquired = round;
//                        break;
//                    }
//                }
//            }
//        };
//        const uint32_t worker = CTaskThreadManager::Instance().Init();
//        CTaskThreadManager::Instance().GetThreadInterface(worker)->PostTask(contender);
//        int32_t completed = 0;
//        for (int32_t round = 1; round <= rounds && errors == 0; ++round)
//        {
//            if (round != 1)
//            {
//                check(receiver.openReceiver(64, 1, 0, 0, 0, 0), "U40 reopen old owner");
//                receiver.publish();
//            }
//            check(receiver.ready(), "U40 old owner ready before handover");
//            request = round;
//            const int64_t deadline = now() + 2000;
//            while (entered != round && now() < deadline) {}
//            check(entered == round, "U40 contender entered before close");
//            receiver.close();
//            while (acquired != round && now() < deadline) {}
//            check(acquired == round, "U40 new owner acquired released lease");
//            const int64_t session = AtomicMath::selfAdd(&directory->m_session, static_cast<int64_t>(0));
//            check(session == -1, "U40 old close must not clear new preparation lease");
//            AtomicMath::compareAndSwap(&directory->m_session, static_cast<int64_t>(-1), static_cast<int64_t>(0));
//            ++completed;
//        }
//        finished = true;
//        CTaskThreadManager::Instance().Uninit(worker);
//        check(!contender->m_failed, "U40 contender completed without exception");
//        receiver.close();
//        std::printf("RESULT {\"action\":\"close-lease\",\"completed\":%d,\"checks\":%d,\"errors\":%d}\n", completed, checks, errors);
//        return errors == 0 ? 0 : 1;
//#else
//        // POSIX成功撤名后应重新打开新入口，不在旧映射上模拟Windows的名称保留行为。
//        return 77;
//#endif
//    }
//
//    if (action == "task-post-oom" || action == "task-try-oom")
//    {
//#if defined(_WIN32) && defined(PROCESS_WORK_TEST_ALLOCATION)
//        if (argc != 3) { return 2; }
//        const int32_t point = std::atoi(argv[2]);
//        if (point <= 0) { return 2; }
//        const bool useTry = action == "task-try-oom";
//        std::printf("CASE %s\n", useTry ? "U34" : "U33");
//        const int32_t originalThreads = CTaskThreadManager::Instance().Count();
//        std::atomic<bool> entered(false), released(false), probeAccepted(false);
//        std::atomic<int32_t> originalCalls(0), recoveredCalls(0);
//        std::shared_ptr<ProcessWorkTestTask> blocker(new ProcessWorkTestTask);
//        std::shared_ptr<ProcessWorkTestTask> original(new ProcessWorkTestTask);
//        std::shared_ptr<ProcessWorkTestTask> recovered(new ProcessWorkTestTask);
//        blocker->m_action = [&]()
//        {
//            entered = true;
//            Semaphore sleep;
//            while (!released && !blocker->m_stop) { sleep.wait(2); }
//        };
//        original->m_action = [&]() { ++originalCalls; };
//        recovered->m_action = [&]() { ++recoveredCalls; };
//        const uint32_t worker = CTaskThreadManager::Instance().Init();
//        std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(worker);
//        thread->PostTask(blocker, 3);
//        check(wait([&]() { return entered.load(); }, 2000), "task-oom blocker started");
//        s_testAllocationThread = CSystem::SystemThreadId();
//        s_testAllocationRemaining = point;
//        s_testAllocationAttempts = 0;
//        s_testAllocationHits = 0;
//        s_testAllocationMode = 6;
//        bool rejected = false;
//        bool accepted = false;
//        try
//        {
//            if (useTry) { accepted = thread->TryPostTask(original, 1); }
//            else { thread->PostTask(original, 1); accepted = true; }
//        }
//        catch (const std::bad_alloc&) { rejected = true; }
//        s_testAllocationMode = 0;
//        const int32_t hits = s_testAllocationHits.load();
//        std::printf("AFTER_INJECTION point=%d hits=%d rejected=%d\n", point, hits, rejected ? 1 : 0);
//        check(rejected == (hits == 1), "task-oom allocation exception propagated");
//        std::printf("STAGE before_probe\n");
//        if (useTry && rejected)
//        {
//            std::shared_ptr<ProcessWorkTestTask> probe(new ProcessWorkTestTask);
//            probe->m_action = [&]() { probeAccepted = thread->TryPostTask(recovered, 2); };
//            const uint32_t probeId = CTaskThreadManager::Instance().Init();
//            CTaskThreadManager::Instance().GetThreadInterface(probeId)->PostTask(probe);
//            check(wait([&]() { return probe->m_done.load(); }, 2000), "U34 another thread completed nonblocking probe");
//            CTaskThreadManager::Instance().Uninit(probeId);
//            check(probeAccepted && !probe->m_failed, "U34 exception released existing queue mutex");
//            // 旧实现漏解锁时，下面真实查询/关闭也会阻塞；外部控制器记录超时并终止本测试子进程。
//        }
//        std::printf("STAGE before_snapshot\n");
//        std::map<int32_t, int32_t> waiting;
//        thread->GetWaitTaskInfo(waiting);
//        std::printf("STAGE snapshot size=%u\n", static_cast<unsigned int>(waiting.size()));
//        for (auto it = waiting.begin(); it != waiting.end(); ++it)
//        {
//            check(it->second > 0, "U33 no empty priority queue survives failed append");
//        }
//        check(originalCalls == 0 && recoveredCalls == 0, "task-oom blocked consumer did not run queued work");
//        if (rejected && !useTry) { thread->PostTask(recovered, 2); }
//        released = true;
//        check(wait([&]() { return rejected ? recoveredCalls == 1 : originalCalls == 1; }, 2000), "task-oom subsequent valid work executes");
//        std::printf("STAGE before_join\n");
//        CTaskThreadManager::Instance().WaitForEnd(worker);
//        check(originalCalls == (accepted ? 1 : 0) && recoveredCalls == (rejected ? 1 : 0), "task-oom rejected task never executes or duplicates");
//        check(CTaskThreadManager::Instance().Count() == originalThreads, "task-oom no retained managed threads");
//        std::printf("RESULT {\"action\":\"%s\",\"point\":%d,\"hits\":%d,\"checks\":%d,\"errors\":%d}\n",
//            action.c_str(), point, hits, checks, errors);
//        return errors == 0 ? 0 : 1;
//#else
//        return 77;
//#endif
//    }
//
//    if (action == "generation-boundary")
//    {
//        std::printf("CASE U35\n");
//        LockFreeQueue<char*> queue;
//        Semaphore notification;
//        std::atomic<int32_t> full(0);
//        {
//            ProcessWorkChannel oldReceiver, oldSender;
//            check(oldReceiver.openReceiver(64, 1, 0, 0, 0, 0), "U35 initial receiver");
//            oldReceiver.publish();
//            SharedMemory directoryMemory("P2D" + std::to_string(self));
//            ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//            check(directory != nullptr && oldSender.openSender(self), "U35 retained old directory and sender");
//            oldReceiver.pump(queue, notification, full);
//            oldReceiver.close();
//            AtomicMath::compareAndSwap(&directory->m_nextSession,
//                AtomicMath::selfAdd(&directory->m_nextSession, static_cast<int64_t>(0)), static_cast<int64_t>(0xfffffe));
//            ProcessWorkChannel finalReceiver, finalSender;
//            check(finalReceiver.openReceiver(64, 1, 0, 0, 0, 0), "U35 final valid receiver session opens");
//            finalReceiver.publish();
//            check(directory->m_session == 0xffffff, "U35 session retains all 24 bits");
//            check(!oldSender.ready() && oldSender.trySend("old", 3, 0) == PROCESS_WORK_UNAVAILABLE, "U35 old sender cannot enter final session");
//            check(finalSender.openSender(self), "U35 final sender opens");
//            finalReceiver.pump(queue, notification, full);
//            check(finalSender.trySend("123456", 6, 0) == PROCESS_WORK_SENT, "U35 final session commits");
//            finalReceiver.pump(queue, notification, full);
//            char* packet = nullptr;
//            check(queue.pop(&packet) && packet != nullptr && std::memcmp(packet + 12, "123456", 6) == 0, "U35 final session exact delivery");
//            delete[] packet;
//            finalReceiver.close();
//            ProcessWorkChannel exhausted;
//            check(!exhausted.openReceiver(64, 1, 0, 0, 0, 0), "U35 exhausted session rejected");
//            check(directory->m_session == 0 && directory->m_nextSession == 0xffffff, "U35 failure clears preparation but never wraps counter");
//        }
//        {
//            ProcessWorkChannel receiver, sender;
//            check(receiver.openReceiver(64, 1, 1, 1, 1, 1), "U35 generation receiver");
//            receiver.publish();
//            SharedMemory directoryMemory("P2D" + std::to_string(self));
//            ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//            SharedMemory controlMemory(ProcessWorkChannel::objectName('C', self, directory->m_identity, static_cast<uint32_t>(directory->m_session)));
//            ProcessWorkControl* control = static_cast<ProcessWorkControl*>(controlMemory.writeWithoutLock());
//            check(sender.openSender(self), "U35 generation sender");
//            receiver.pump(queue, notification, full);
//            const int64_t token = AtomicMath::selfAdd(&control->m_peers[0].m_token, static_cast<int64_t>(0));
//            check(token > 0 && AtomicMath::compareAndSwap(&control->m_blocks[0].m_owner, static_cast<int64_t>(0), token), "U35 pin initial block under real live token");
//            AtomicMath::compareAndSwap(&control->m_blocks[1].m_generation, static_cast<int64_t>(0), static_cast<int64_t>(0xfffffe));
//            AtomicMath::compareAndSwap(&control->m_demand, 0, 1);
//            check(wait([&]() { receiver.pump(queue, notification, full); return receiver.bufferCount() == 2; }, 1000), "U35 last generation allocated");
//            check(control->m_blocks[1].m_generation == 0xffffff, "U35 last generation encoded without alias");
//            check(sender.trySend("123456", 6, 1) == PROCESS_WORK_SENT, "U35 final generation sends");
//            receiver.pump(queue, notification, full);
//            char* packet = nullptr;
//            check(queue.pop(&packet) && packet != nullptr && std::memcmp(packet + 12, "123456", 6) == 0, "U35 final generation exact bytes");
//            delete[] packet;
//            check(wait([&]() { receiver.pump(queue, notification, full); return receiver.bufferCount() == 1; }, 2500), "U35 last generation retires after idle interval");
//            AtomicMath::compareAndSwap(&control->m_demand, 0, 1);
//            Semaphore delay;
//            delay.wait(30);
//            receiver.pump(queue, notification, full);
//            check(receiver.bufferCount() == 1 && control->m_blocks[1].m_owner == -1 && control->m_blocks[1].m_generation == 0xffffff,
//                "U35 exhausted generation not reactivated or wrapped");
//            AtomicMath::compareAndSwap(&control->m_blocks[0].m_owner, token, static_cast<int64_t>(0));
//            check(sender.trySend("123456", 6, 2) == PROCESS_WORK_SENT, "U35 fixed capacity remains usable");
//            receiver.pump(queue, notification, full);
//            packet = nullptr;
//            check(queue.pop(&packet) && packet != nullptr && std::memcmp(packet + 12, "123456", 6) == 0, "U35 initial block still exact");
//            delete[] packet;
//        }
//        std::printf("RESULT {\"action\":\"generation-boundary\",\"checks\":%d,\"errors\":%d}\n", checks, errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "maximum-capacity")
//    {
//        std::printf("CASE U36\n");
//        ProcessWorkTestReceiver receiver;
//        std::atomic<int32_t> received(0), bad(0);
//        std::string name = "ipc.maximum." + std::to_string(self);
//        name.append(1023 - name.size(), 'n');
//        check(!receiver.setReceiveName(name + "x"), "U36 over-limit name rejected");
//        check(receiver.setReceiveName(name), "U36 exact maximum name accepted");
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            const int32_t sequence = static_cast<int32_t>(type);
//            const int32_t expectedLength = sequence == 0 ? 0 : (sequence <= 1024 ? (sequence % 3 == 0 ? 0 : (sequence % 3 == 1 ? 1 : 64)) : 64);
//            bool valid = pid == self && sequence == received && length == expectedLength;
//            if (length == 0) { valid = valid && data == nullptr; }
//            else if (data == nullptr) { valid = false; }
//            else
//            {
//                for (int32_t i = 0; i < length; ++i)
//                {
//                    if (static_cast<unsigned char>(data[i]) != payloadByte(0, 0, sequence, i)) { valid = false; break; }
//                }
//            }
//            if (!valid) { ++bad; }
//            ++received;
//        };
//        check(receiver.initReceive(76, 1024), "U36 maximum block count initialized");
//        receiver.send(self, nullptr, 0, static_cast<MessageType>(0));
//        check(wait([&]() { return received == 1; }, 2000), "U36 registration warmup");
//        receiver.pauseReader();
//        char bytes[65];
//        for (int32_t sequence = 1; sequence <= 1024; ++sequence)
//        {
//            const int32_t length = sequence % 3 == 0 ? 0 : (sequence % 3 == 1 ? 1 : 64);
//            for (int32_t i = 0; i < length; ++i) { bytes[i] = static_cast<char>(payloadByte(0, 0, sequence, i)); }
//            check(receiver.trySend(self, length == 0 ? nullptr : bytes, length, static_cast<MessageType>(sequence)) == PROCESS_WORK_SENT,
//                "U36 each of 1024 reservations commits");
//        }
//        check(receiver.trySend(self, bytes, 1) == PROCESS_WORK_BUSY, "U36 exactly full after 1024 packets");
//        check(receiver.trySend(self, bytes, 65) == PROCESS_WORK_TOO_LARGE, "U36 declared bound checked even when full");
//        check(receiver.receiveBufferCount() == 1024 && receiver.resumeReader(), "U36 unchanged capacity resumes");
//        check(wait([&]() { return received == 1025; }, 4000), "U36 all maximum-capacity packets received");
//        receiver.initPostThread();
//        for (int32_t sequence = 1025; sequence < 1125; ++sequence)
//        {
//            for (int32_t i = 0; i < 64; ++i) { bytes[i] = static_cast<char>(payloadByte(0, 0, sequence, i)); }
//            if (sequence % 2 == 0) { receiver.post(self, bytes, 64, static_cast<MessageType>(sequence)); }
//            else { receiver.post(name, bytes, 64, static_cast<MessageType>(sequence)); }
//        }
//        receiver.waitForPostEnd();
//        check(wait([&]() { return received == 1125; }, 3000) && bad == 0, "U36 maximal name and PID share exact FIFO");
//        receiver.uninitReceive();
//        std::printf("RESULT {\"action\":\"maximum-capacity\",\"received\":%d,\"bad\":%d,\"checks\":%d,\"errors\":%d}\n", received.load(), bad.load(), checks, errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "slow-callback" || action == "shared-channel")
//    {
//        const bool direct = action == "shared-channel";
//        const int32_t mode = direct ? 0 : (argc == 3 ? std::atoi(argv[2]) : -1);
//        if (mode != 0 && mode != 4) { return 2; }
//        std::printf("CASE %s\n", direct ? "U39" : "U38");
//        const int32_t threads = direct ? 8 : 4;
//        const int32_t count = direct ? 10000 : 1500;
//        const int32_t total = threads * count;
//        ProcessWorkTestReceiver receiver, observer;
//        ProcessWorkChannel channel;
//        std::atomic<int32_t> received(0), observed(0), bad(0), fullEvents(0), failed(0);
//        std::vector<int32_t> next(threads, 0), nextObserved(threads, 0);
//        // 按来源线程和序号逐字节核对，返回后输入可被覆盖；回调期间不保留借用地址。
//        auto validate = [&](int32_t pid, const char* data, int32_t length, MessageType type, std::vector<int32_t>& expected) -> bool
//        {
//            const int32_t value = static_cast<int32_t>(type), thread = value / 2000000, sequence = value % 2000000;
//            const int32_t lengths[5] = { 0, 1, 6, 257, 4096 };
//            if (pid != self || thread < 0 || thread >= threads || sequence >= count || length != lengths[sequence % 5] ||
//                sequence != expected[thread] || (length == 0 ? data != nullptr : data == nullptr)) { return false; }
//            ++expected[thread];
//            for (int32_t i = 0; i < length; ++i)
//            {
//                if (static_cast<unsigned char>(data[i]) != payloadByte(0, thread, sequence, i)) { return false; }
//            }
//            return true;
//        };
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            if (!validate(pid, data, length, type, next)) { ++bad; }
//            const int32_t current = ++received;
//            if (!direct && current % 17 == 0) { Semaphore delay; delay.wait(2); }
//            if (!direct && current % 97 == 0) { throw std::runtime_error("expected observer exception"); }
//        };
//        observer.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            if (!validate(pid, data, length, type, nextObserved)) { ++bad; }
//            ++observed;
//        };
//        receiver.m_full = [&](int32_t size) { if (size < 3 || size > 9) { ++bad; } ++fullEvents; };
//        receiver.addProcessReceiveCallback(&observer);
//        check(receiver.initReceive(4108, 3, 1, 2, 1, 3), "concurrent receiver initialized");
//        if (mode == 4) { receiver.initPostThread(); }
//        if (direct) { check(channel.openSender(self), "U39 one shared channel opened before producers"); }
//        std::vector<std::shared_ptr<ProcessWorkTestTask>> jobs;
//        std::vector<uint32_t> workers;
//        std::atomic<bool> start(false);
//        const int64_t deadline = now() + 30000;
//        for (int32_t index = 0; index < threads; ++index)
//        {
//            std::shared_ptr<ProcessWorkTestTask> job(new ProcessWorkTestTask);
//            job->m_action = [&, index]()
//            {
//                Semaphore delay;
//                while (!start) { delay.wait(1); }
//                char bytes[4097];
//                const int32_t lengths[5] = { 0, 1, 6, 257, 4096 };
//                for (int32_t sequence = 0; sequence < count; ++sequence)
//                {
//                    const int32_t length = lengths[sequence % 5], type = index * 2000000 + sequence;
//                    for (int32_t i = 0; i < length; ++i) { bytes[i + 1] = static_cast<char>(payloadByte(0, index, sequence, i)); }
//                    const char* data = length == 0 ? nullptr : bytes + 1;
//                    if (direct)
//                    {
//                        int32_t status;
//                        do
//                        {
//                            status = channel.trySend(data, length, type);
//                            if (status == PROCESS_WORK_BUSY && (!channel.waitForSpace() || now() >= deadline)) { ++failed; return; }
//                        }
//                        while (status == PROCESS_WORK_BUSY);
//                        if (status != PROCESS_WORK_SENT) { ++failed; return; }
//                    }
//                    else if (mode == 0) { receiver.send(self, data, length, static_cast<MessageType>(type)); }
//                    else if (sequence % 2 == 0) { receiver.post(self, data, length, static_cast<MessageType>(type)); }
//                    else { receiver.post(selfName, data, length, static_cast<MessageType>(type)); }
//                    std::memset(bytes, 0, sizeof(bytes));
//                }
//            };
//            const uint32_t worker = CTaskThreadManager::Instance().Init();
//            jobs.push_back(job);
//            workers.push_back(worker);
//            CTaskThreadManager::Instance().GetThreadInterface(worker)->PostTask(job);
//        }
//        start = true;
//        for (size_t i = 0; i < workers.size(); ++i) { CTaskThreadManager::Instance().WaitForEnd(workers[i]); }
//        if (mode == 4) { receiver.waitForPostEnd(); }
//        check(wait([&]() { return observed == total; }, 15000), "concurrent observers drain all messages");
//        for (size_t i = 0; i < jobs.size(); ++i) { check(!jobs[i]->m_failed, "concurrent producer did not throw"); }
//        check(received == total && observed == total && bad == 0 && failed == 0, "concurrent exact bytes counts per-thread order and observer isolation");
//        check(wait([&]() { return receiver.receiveBufferCount() == 3; }, 3000), "concurrent expanded buffers return to baseline");
//        channel.close();
//        receiver.uninitReceive();
//        std::printf("RESULT {\"action\":\"%s\",\"mode\":%d,\"received\":%d,\"observed\":%d,\"bad\":%d,\"full_events\":%d,\"errors\":%d}\n",
//            action.c_str(), mode, received.load(), observed.load(), bad.load(), fullEvents.load(), errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "restart-server")
//    {
//        if (argc != 3) { return 2; }
//        ProcessWorkTestReceiver receiver;
//        std::atomic<int32_t> received(0), bad(0);
//        receiver.m_message = [&](int32_t, const char* data, int32_t length, MessageType type)
//        {
//            if (length != 6 || data == nullptr || std::memcmp(data, "123456", 6) != 0 || static_cast<int32_t>(type) != received) { ++bad; }
//            ++received;
//        };
//        if (!receiver.setReceiveName(argv[2]) || !receiver.initReceive(64, 2)) { return 3; }
//        std::printf("READY %d\n", self);
//        std::string command;
//        while (std::cin >> command)
//        {
//            if (command == "pause") { check(wait([&]() { return received == 1; }, 3000), "U37 warmup drained before pause"); receiver.pauseReader(); std::printf("PAUSED\n"); }
//            else if (command == "finish") { break; }
//            else { return 2; }
//        }
//        check(wait([&]() { return received == 1; }, 3000) && bad == 0, "U37 replacement receives only its new packet");
//        receiver.uninitReceive();
//        std::printf("RESULT {\"action\":\"restart-server\",\"received\":%d,\"bad\":%d,\"errors\":%d}\n", received.load(), bad.load(), errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "restart-client")
//    {
//        if (argc != 4) { return 2; }
//        std::printf("CASE U37\n");
//        const std::string name(argv[2]);
//        const int32_t oldPid = std::atoi(argv[3]);
//        ProcessWorkTestReceiver sender;
//        sender.send(name, "123456", 6, static_cast<MessageType>(0));
//        std::printf("REGISTERED\n");
//        std::string command;
//        if (!(std::cin >> command) || command != "fill") { return 2; }
//        check(sender.trySend(oldPid, "123456", 6, static_cast<MessageType>(1)) == PROCESS_WORK_SENT, "U37 first paused packet committed");
//        check(sender.trySend(oldPid, "123456", 6, static_cast<MessageType>(2)) == PROCESS_WORK_SENT, "U37 second paused packet committed");
//        check(sender.trySend(oldPid, "123456", 6, static_cast<MessageType>(3)) == PROCESS_WORK_BUSY, "U37 receiver full before death");
//        std::printf("BLOCKED\n");
//        const int64_t stoppedAt = now();
//        sender.send(oldPid, "123456", 6, static_cast<MessageType>(3));
//        check(now() - stoppedAt < 4000, "U37 blocking send exits after confirmed receiver death");
//        check(sender.trySend(oldPid, "123456", 6) == PROCESS_WORK_UNAVAILABLE, "U37 old destination becomes unavailable");
//        std::printf("UNBLOCKED\n");
//        int32_t newPid = 0;
//        if (!(std::cin >> newPid) || newPid <= 0 || newPid == oldPid) { return 2; }
//        sender.initPostThread();
//        sender.post(name, "123456", 6, static_cast<MessageType>(0));
//        sender.waitForPostEnd();
//        sender.uninitReceive();
//        std::printf("RESULT {\"action\":\"restart-client\",\"checks\":%d,\"errors\":%d}\n", checks, errors);
//        return errors == 0 ? 0 : 1;
//    }
//
//    if (action == "token-boundary")
//    {
//        std::printf("CASE U32\n");
//        ProcessWorkChannel receiver, sender;
//        LockFreeQueue<char*> queue;
//        Semaphore notification;
//        std::atomic<int32_t> full(0);
//        check(receiver.openReceiver(64, 2, 0, 0, 0, 0), "U32 receiver");
//        receiver.publish();
//        SharedMemory directoryMemory("P2D" + std::to_string(self));
//        ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//        if (directory != nullptr)
//        {
//            SharedMemory controlMemory(ProcessWorkChannel::objectName('C', self, directory->m_identity, static_cast<uint32_t>(directory->m_session)));
//            ProcessWorkControl* control = static_cast<ProcessWorkControl*>(controlMemory.writeWithoutLock());
//            if (control != nullptr)
//            {
//                // White-box boundary setup before any registration, with no concurrent pump.
//                // Actual open/registration/send remain the production entry points.
//                AtomicMath::compareAndSwap(&control->m_nextToken, static_cast<int64_t>(0), static_cast<int64_t>(INT64_MAX));
//                check(sender.openSender(self), "U32 exhausted registry can be observed");
//                receiver.pump(queue, notification, full);
//                check(sender.trySend("123456", 6, 0) == PROCESS_WORK_UNAVAILABLE, "U32 permanent exhaustion is not retryable BUSY");
//                check(AtomicMath::selfAdd(&control->m_nextToken, static_cast<int64_t>(0)) == INT64_MAX, "U32 registry never wraps");
//                check(queue.empty(), "U32 exhausted registration never commits a packet");
//                sender.close();
//                receiver.close();
//            }
//            else { check(false, "U32 control mapping"); }
//        }
//        else { check(false, "U32 directory mapping"); }
//        {
//            ProcessWorkChannel fresh, lastSender;
//            check(fresh.openReceiver(64, 2, 0, 0, 0, 0), "U32 last-token receiver");
//            fresh.publish();
//            SharedMemory currentDirectory("P2D" + std::to_string(self));
//            ProcessWorkDirectory* current = static_cast<ProcessWorkDirectory*>(currentDirectory.writeWithoutLock());
//            if (current != nullptr)
//            {
//                SharedMemory controlMemory(ProcessWorkChannel::objectName('C', self, current->m_identity, static_cast<uint32_t>(current->m_session)));
//                ProcessWorkControl* control = static_cast<ProcessWorkControl*>(controlMemory.writeWithoutLock());
//                if (control != nullptr)
//                {
//                    AtomicMath::compareAndSwap(&control->m_nextToken, static_cast<int64_t>(0), static_cast<int64_t>(INT64_MAX - 1));
//                    check(lastSender.openSender(self), "U32 final registration request");
//                    fresh.pump(queue, notification, full);
//                    check(lastSender.trySend("123456", 6, 1) == PROCESS_WORK_SENT, "U32 last valid token still sends");
//                    fresh.pump(queue, notification, full);
//                    char* packet = nullptr;
//                    check(queue.pop(&packet) && packet != nullptr && std::memcmp(packet + 12, "123456", 6) == 0, "U32 last-token exact delivery");
//                    delete[] packet;
//                }
//                else { check(false, "U32 last-token control"); }
//            }
//            else { check(false, "U32 last-token directory"); }
//        }
//        std::printf("RESULT {\"action\":\"token-boundary\",\"checks\":%d,\"errors\":%d}\n", checks, errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "cache-stress")
//    {
//        if (argc != 3) { return 2; }
//        std::printf("CASE U30\n");
//        const int32_t mode = std::atoi(argv[2]);
//        if (mode != 0 && mode != 4) { return 2; }
//        const int32_t threads = 8, count = 3000;
//        std::atomic<int32_t> received(0), bad(0), clears(0);
//        std::vector<int32_t> next(threads, 0);
//        ProcessWorkTestReceiver receiver;
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            const int32_t value = static_cast<int32_t>(type), thread = value / 2000000, sequence = value % 2000000;
//            bool valid = pid == self && thread >= 0 && thread < threads && sequence >= 0 && sequence < count && length == 257 && data != nullptr;
//            if (valid)
//            {
//                valid = next[thread] == sequence;
//                ++next[thread];
//                for (int32_t i = 0; i < length; ++i)
//                {
//                    if (static_cast<unsigned char>(data[i]) != payloadByte(0, thread, sequence, i)) { valid = false; break; }
//                }
//            }
//            if (!valid) { ++bad; }
//            ++received;
//        };
//        check(receiver.initReceive(269, 4, 1, 2, 1, 2), "U30 receiver");
//        if (mode == 4) { receiver.initPostThread(); }
//        std::shared_ptr<ProcessWorkTestTask> cleaner(new ProcessWorkTestTask);
//        cleaner->m_action = [&]()
//        {
//            Semaphore pause;
//            while (!cleaner->m_stop)
//            {
//                receiver.clearDestPid(self);
//                receiver.clearUseless();
//                receiver.clear();
//                ++clears;
//                pause.wait(1);
//            }
//        };
//        const uint32_t cleanerId = CTaskThreadManager::Instance().Init();
//        CTaskThreadManager::Instance().GetThreadInterface(cleanerId)->PostTask(cleaner);
//        std::vector<uint32_t> ids;
//        std::vector<std::shared_ptr<ProcessWorkTestTask> > tasks;
//        for (int32_t thread = 0; thread < threads; ++thread)
//        {
//            std::shared_ptr<ProcessWorkTestTask> task(new ProcessWorkTestTask);
//            ProcessWorkTestTask* raw = task.get();
//            task->m_action = [&, thread, raw]()
//            {
//                char bytes[258];
//                for (int32_t sequence = 0; sequence < count && !raw->m_stop; ++sequence)
//                {
//                    for (int32_t i = 0; i < 257; ++i) { bytes[i + 1] = static_cast<char>(payloadByte(0, thread, sequence, i)); }
//                    const MessageType type = static_cast<MessageType>(thread * 2000000 + sequence);
//                    if (mode == 0) { receiver.send(self, bytes + 1, 257, type); }
//                    else if (sequence % 2 == 0) { receiver.post(self, bytes + 1, 257, type); }
//                    else { receiver.post(selfName, bytes + 1, 257, type); }
//                    std::memset(bytes, 0, sizeof(bytes));
//                }
//            };
//            const uint32_t id = CTaskThreadManager::Instance().Init();
//            CTaskThreadManager::Instance().GetThreadInterface(id)->PostTask(task);
//            tasks.push_back(task);
//            ids.push_back(id);
//        }
//        check(wait([&]() { for (size_t i = 0; i < tasks.size(); ++i) { if (!tasks[i]->m_done) { return false; } } return true; }, 30000), "U30 producer deadline");
//        for (size_t i = 0; i < ids.size(); ++i) { CTaskThreadManager::Instance().Uninit(ids[i]); check(!tasks[i]->m_failed, "U30 sender task succeeded"); }
//        if (mode == 4) { receiver.waitForPostEnd(); }
//        check(wait([&]() { return received == threads * count; }, 30000), "U30 every message delivered");
//        CTaskThreadManager::Instance().Uninit(cleanerId);
//        check(!cleaner->m_failed && clears > 0, "U30 concurrent cache retirement actually ran");
//        Semaphore pause;
//        pause.wait(100);
//        check(received == threads * count && bad == 0, "U30 exact bytes FIFO and no late duplicates");
//        receiver.uninitReceive();
//        std::printf("RESULT {\"action\":\"cache-stress\",\"mode\":%d,\"received\":%d,\"bad\":%d,\"clears\":%d,\"errors\":%d}\n", mode, received.load(), bad.load(), clears.load(), errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "peer-churn-server")
//    {
//        if (argc != 3) { return 2; }
//        std::printf("CASE U31\n");
//        const int32_t count = std::atoi(argv[2]);
//        if (count <= 0 || count > 4096) { return 2; }
//        std::vector<int32_t> seen(count, 0);
//        std::atomic<int32_t> received(0), bad(0);
//        ProcessWorkTestReceiver receiver;
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            const int32_t sequence = static_cast<int32_t>(type);
//            bool valid = pid > 0 && sequence >= 0 && sequence < count && length == 32 && data != nullptr;
//            if (valid)
//            {
//                if (++seen[sequence] != 1) { valid = false; }
//                for (int32_t i = 0; i < length; ++i) { if (static_cast<unsigned char>(data[i]) != payloadByte(0, 0, sequence, i)) { valid = false; break; } }
//            }
//            if (!valid) { ++bad; }
//            ++received;
//        };
//        check(receiver.initReceive(44, 8, 2, 8, 1, 4), "U31 receiver");
//        std::printf("READY %d\n", self);
//        check(wait([&]() { return received >= count; }, 120000), "U31 all short-lived processes delivered committed packets");
//        SharedMemory directoryMemory("P2D" + std::to_string(self));
//        ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//        if (directory != nullptr)
//        {
//            SharedMemory controlMemory(ProcessWorkChannel::objectName('C', self, directory->m_identity, static_cast<uint32_t>(directory->m_session)));
//            ProcessWorkControl* control = static_cast<ProcessWorkControl*>(controlMemory.writeWithoutLock());
//            check(control != nullptr, "U31 control observation");
//            if (control != nullptr)
//            {
//                check(wait([&]()
//                {
//                    for (int32_t i = 0; i < 256; ++i) { if (AtomicMath::selfAdd(&control->m_peers[i].m_pid, 0) != 0) { return false; } }
//                    return true;
//                }, 10000), "U31 all exited peer registrations reusable");
//                check(AtomicMath::selfAdd(&control->m_nextToken, static_cast<int64_t>(0)) >= count, "U31 registration tokens never reused");
//            }
//        }
//        else { check(false, "U31 directory observation"); }
//        check(received == count && bad == 0, "U31 exact counts bytes and no duplicates after peer cleanup");
//        receiver.uninitReceive();
//        std::printf("RESULT {\"action\":\"peer-churn-server\",\"expected\":%d,\"received\":%d,\"bad\":%d,\"errors\":%d}\n", count, received.load(), bad.load(), errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "peer-churn-child")
//    {
//        if (argc != 4) { return 2; }
//        const int32_t target = std::atoi(argv[2]), sequence = std::atoi(argv[3]);
//        char bytes[32];
//        for (int32_t i = 0; i < 32; ++i) { bytes[i] = static_cast<char>(payloadByte(0, 0, sequence, i)); }
//        ProcessWorkChannel sender;
//        const int64_t deadline = now() + 15000;
//        Semaphore pause;
//        int32_t result = PROCESS_WORK_UNAVAILABLE;
//        while (now() < deadline)
//        {
//            if (!sender.ready()) { sender.close(); sender.openSender(target); }
//            result = sender.trySend(bytes, 32, sequence);
//            if (result == PROCESS_WORK_SENT) { return 0; }
//            if (result == PROCESS_WORK_BUSY) { sender.waitForSpace(); }
//            else { pause.wait(2); }
//        }
//        std::fprintf(stderr, "peer child timed out sequence=%d result=%d\n", sequence, result);
//        return 1;
//    }
//    if (action == "oom-post-init" || action == "oom-receive-init")
//    {
//#if defined(_WIN32) && defined(PROCESS_WORK_TEST_ALLOCATION)
//        if (argc != 3) { return 2; }
//        std::printf("CASE %s\n", action == "oom-post-init" ? "U28" : "U29");
//        const int32_t point = std::atoi(argv[2]);
//        if (point <= 0 || point > 1000) { return 2; }
//        std::atomic<int32_t> received(0), bad(0);
//        ProcessWorkTestReceiver receiver;
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            if (pid != self || length != 6 || data == nullptr || std::memcmp(data, "123456", 6) != 0) { ++bad; }
//            ++received;
//        };
//        const int32_t before = CTaskThreadManager::Instance().Count();
//        s_testAllocationThread = ::GetCurrentThreadId();
//        s_testAllocationRemaining = point;
//        s_testAllocationAttempts = 0;
//        s_testAllocationHits = 0;
//        s_testAllocationMode = 6;
//        bool initialized = false;
//        try
//        {
//            if (action == "oom-post-init") { receiver.initPostThread(); initialized = true; }
//            else { initialized = receiver.initReceive(64, 1); }
//        }
//        catch (const std::bad_alloc&) { }
//        catch (...) { check(false, "unexpected initialization exception"); }
//        s_testAllocationMode = 0;
//        s_testAllocationThread = 0;
//        const int32_t hits = s_testAllocationHits.load();
//        const int32_t attempts = s_testAllocationAttempts.load();
//        receiver.uninitReceive();
//        check(CTaskThreadManager::Instance().Count() == before, "failed initialization leaves no managed worker");
//        receiver.addProcessReceiveCallback(&receiver);
//        check(receiver.initReceive(64, 2), "initialization succeeds after resource recovery");
//        receiver.initPostThread();
//        receiver.post(self, "123456", 6);
//        receiver.waitForPostEnd();
//        check(wait([&]() { return received.load() >= 1; }, 3000), "post recovery actually delivered");
//        receiver.uninitReceive();
//        check(received == 1 && bad == 0, "recovery exact bytes and no duplicate callbacks");
//        check(CTaskThreadManager::Instance().Count() == before, "recovered instance fully released");
//        std::printf("RESULT {\"action\":\"%s\",\"point\":%d,\"attempts\":%d,\"hits\":%d,\"initialized\":%d,\"errors\":%d}\n",
//            action.c_str(), point, attempts, hits, initialized ? 1 : 0, errors);
//        return errors == 0 ? 0 : 1;
//#else
//        return 2;
//#endif
//    }
//    if (action == "expert-unit")
//    {
//#if defined(_WIN32) && defined(PROCESS_WORK_TEST_ALLOCATION)
//        // U26: A slow successful copy must not count its busy interval as idle time.
//        {
//            std::printf("CASE U26\n");
//            ProcessWorkChannel receiver, sender;
//            LockFreeQueue<char*> queue;
//            Semaphore notification, pause;
//            std::atomic<int32_t> full(0);
//            check(receiver.openReceiver(64, 1, 1, 1, 1, 1), "U26 receiver");
//            receiver.publish();
//            check(sender.openSender(self), "U26 sender");
//            receiver.pump(queue, notification, full);
//            check(sender.trySend("a", 1, 0) == PROCESS_WORK_SENT, "U26 base submitted");
//            pause.wait(25);
//            receiver.pump(queue, notification, full);
//            char* packet = nullptr;
//            check(queue.pop(&packet) && packet != nullptr && packet[12] == 'a', "U26 base delivered");
//            delete[] packet;
//            check(receiver.bufferCount() == 2, "U26 actual demand expanded");
//            check(sender.trySend("123456", 6, 1) == PROCESS_WORK_SENT, "U26 expanded block submitted");
//            s_testAllocationHits = 0;
//            s_testAllocationThread = ::GetCurrentThreadId();
//            s_testAllocationMode = 4;
//            const int64_t copyStart = now();
//            receiver.pump(queue, notification, full);
//            s_testAllocationMode = 0;
//            s_testAllocationThread = 0;
//            check(s_testAllocationHits == 1 && now() - copyStart >= 1100, "U26 real copy crossed idle duration");
//            packet = nullptr;
//            check(queue.pop(&packet) && packet != nullptr && std::memcmp(packet + 12, "123456", 6) == 0, "U26 exact slow-copy delivery");
//            delete[] packet;
//            receiver.pump(queue, notification, full);
//            check(receiver.bufferCount() == 2, "U26 recently released block not prematurely retired");
//            pause.wait(250);
//            receiver.pump(queue, notification, full);
//            check(receiver.bufferCount() == 2, "U26 full idle interval measured after release");
//            pause.wait(1000);
//            receiver.pump(queue, notification, full);
//            check(receiver.bufferCount() == 1, "U26 idle extension eventually retired");
//            while (queue.pop(&packet)) { delete[] packet; }
//        }
//        // U27: A receiver stopping while openSender maps blocks must leave a retryable object.
//        {
//            std::printf("CASE U27\n");
//            ProcessWorkChannel receiver, sender;
//            check(receiver.openReceiver(64, 37, 0, 0, 0, 0), "U27 first receiver");
//            receiver.publish();
//            std::atomic<bool> opened(false);
//            std::shared_ptr<ProcessWorkTestTask> task(new ProcessWorkTestTask);
//            s_testAllocationHits = 0;
//            s_testAllocationPaused = false;
//            s_testAllocationRelease = false;
//            task->m_action = [&]()
//            {
//                s_testAllocationThread = ::GetCurrentThreadId();
//                s_testAllocationMode = 7;
//                opened = sender.openSender(self);
//                s_testAllocationMode = 0;
//                s_testAllocationThread = 0;
//            };
//            task->m_stopAction = [&]() { s_testAllocationRelease = true; };
//            const uint32_t id = CTaskThreadManager::Instance().Init();
//            CTaskThreadManager::Instance().GetThreadInterface(id)->PostTask(task);
//            check(wait([&]() { return s_testAllocationPaused.load(); }, 3000), "U27 paused after ready validation before block maps");
//            receiver.stop();
//            s_testAllocationRelease = true;
//            check(wait([&]() { return task->m_done.load(); }, 3000), "U27 open returns after stop");
//            CTaskThreadManager::Instance().Uninit(id);
//            check(!opened && !task->m_failed, "U27 closing receiver rejected");
//            receiver.close();
//            check(receiver.openReceiver(64, 2, 0, 0, 0, 0), "U27 replacement receiver");
//            receiver.publish();
//            check(sender.openSender(self), "U27 same failed sender can retry without manual close");
//        }
//#else
//        check(false, "expert-unit requires allocation-enabled Windows test build");
//#endif
//        CTaskThreadManager::Instance().UninitAll();
//        std::printf("RESULT {\"action\":\"expert-unit\",\"checks\":%d,\"errors\":%d}\n", checks, errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "unit" || action == "clock-regression")
//    {
//        if (action == "unit")
//        {
//        // U00：测试等待器成功后不再次执行有副作用的条件；超时能返回false。
//        {
//            std::printf("CASE U00\n");
//            int32_t evaluations = 0;
//            check(wait([&]() { ++evaluations; return true; }, 100) && evaluations == 1, "U00 wait predicate evaluated once after success");
//            check(!wait([]() { return false; }, 5), "U00 false predicate times out");
//        }
//        // U01：原生共享对象不允许独占创建覆盖；大小查询和反复映射不泄漏。
//        {
//            std::printf("CASE U01\n");
//            const std::string name = "PTM" + std::to_string(self);
//            SharedMemory first(name, 64, false);
//            char* data = static_cast<char*>(first.writeWithoutLock());
//            check(data != nullptr, "U01 shared memory create");
//            if (data != nullptr) { data[0] = 73; }
//            SharedMemory collision(name, 64, false);
//            check(collision.isFailed() && collision.writeWithoutLock() == nullptr, "U01 exclusive create refuses collision");
//            SharedMemory peer(name);
//            check(peer.realSize() >= 64, "U01 capacity query");
//            char* joined = static_cast<char*>(peer.writeWithoutLock());
//            check(joined != nullptr && joined[0] == 73, "U01 same bytes after size query");
//            const uint32_t baseline = handles();
//            for (int32_t i = 0; i < 1000; ++i) { peer.realSize(); peer.close(); peer.writeWithoutLock(); }
//            check(handles() <= baseline + 2, "U01 repeated map/query handle count");
//        }
//        // U02：缺失端点不能由发送方伪造；通知计数精确并可有限等待。
//        {
//            std::printf("CASE U02\n");
//            Semaphore owner;
//            Semaphore peer;
//            const std::string name = "PTS" + std::to_string(self);
//            check(!peer.tryOpenProcessSemaphore(name), "U02 missing notify refused");
//            owner.createProcessSemaphore(name, 3);
//            check(peer.tryOpenProcessSemaphore(name), "U02 existing notify opened");
//            check(peer.tryProcessSignal() == 0 && peer.tryProcessSignal() == 0, "U02 two notifications");
//            check(owner.processWait(0) && owner.processWait(0) && !owner.processWait(0), "U02 exact notification consumption");
//            const int64_t start = now();
//            check(!owner.processWait(25) && now() - start >= 15 && now() - start < 1000, "U02 timed blocking");
//        }
//        // U03：初始化边界与失败后直接重试；重复init不改变已运行接收器。
//        {
//            std::printf("CASE U03\n");
//            ProcessWorkTestReceiver receiver;
//            check(!receiver.initReceive(11, 1), "U03 tiny header refused");
//            check(!receiver.initReceive(INT32_MAX, 1), "U03 maximum signed bytes refused");
//            check(!receiver.initReceive(64, 0) && !receiver.initReceive(64, 1025), "U03 count range");
//            check(!receiver.initReceive(64, 10, 1, INT32_MAX, 1, INT32_MAX), "U03 growth overflow refused");
//            check(!receiver.initReceive(64, 2, -1, 1, 1, 1), "U03 negative threshold refused");
//            check(receiver.initReceive(64, 2), "U03 retry succeeds");
//            check(!receiver.initReceive(1024, 5) && receiver.receiveBufferCount() == 2, "U03 duplicate preserves receiver");
//        }
//        // U04：0/极短/跨页/最大正文均逐字节验证；大于声明容量明确拒绝且不占块。
//        {
//            std::printf("CASE U04\n");
//            const int32_t sizes[] = { 0, 1, 2, 3, 5, 6, 7, 15, 16, 17, 255, 4096, 65535, 65536 };
//            std::atomic<int32_t> received(0), bad(0);
//            ProcessWorkTestReceiver receiver;
//            receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//            {
//                const int32_t sequence = static_cast<int32_t>(type);
//                if (pid != self || sequence != received.load() || sequence < 0 || sequence >= 14 || length != sizes[sequence]) { ++bad; }
//                else
//                {
//                    if (length == 0 && data != nullptr) { ++bad; }
//                    for (int32_t i = 0; i < length; ++i)
//                    {
//                        if (static_cast<unsigned char>(data[i]) != static_cast<unsigned char>(sequence * 13 + i * 7)) { ++bad; break; }
//                    }
//                }
//                ++received;
//            };
//            check(receiver.initReceive(65548, 3), "U04 receiver ready");
//            check(receiver.trySend(self, nullptr, 1) == PROCESS_WORK_INVALID, "U04 null nonempty rejected");
//            check(receiver.trySend(self, nullptr, -1) == PROCESS_WORK_INVALID, "U04 negative length rejected");
//            std::vector<char> payload(65537);
//            check(receiver.trySend(self, payload.data(), 65537) == PROCESS_WORK_TOO_LARGE, "U04 over declared capacity rejected");
//            for (int32_t n = 0; n < 14; ++n)
//            {
//                for (int32_t i = 0; i < sizes[n]; ++i) { payload[i] = static_cast<char>(n * 13 + i * 7); }
//                receiver.send(self, sizes[n] == 0 ? nullptr : payload.data(), sizes[n], static_cast<MessageType>(n));
//            }
//            check(wait([&]() { return received == 14; }, 5000), "U04 all packets received");
//            check(bad == 0, "U04 exact contents and order");
//        }
//        // U05：PID与名称Post混排仍有序；返回后立即修改原输入不能改变接收正文。
//        {
//            std::printf("CASE U05\n");
//            std::atomic<int32_t> received(0), bad(0);
//            ProcessWorkTestReceiver receiver;
//            receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//            {
//                const int32_t sequence = static_cast<int32_t>(type);
//                const char expected = static_cast<char>(sequence & 255);
//                if (pid != self || sequence != received.load() || length != 7 || data == nullptr) { ++bad; }
//                else { for (int32_t i = 0; i < length; ++i) { if (data[i] != expected) { ++bad; break; } } }
//                ++received;
//            };
//            check(receiver.initReceive(1024, 3), "U05 receiver ready");
//            receiver.initPostThread();
//            for (int32_t n = 0; n < 10000; ++n)
//            {
//                char data[7];
//                std::memset(data, n & 255, sizeof(data));
//                if (n % 2 == 0) { receiver.post(self, data, sizeof(data), static_cast<MessageType>(n)); }
//                else { receiver.post(selfName, data, sizeof(data), static_cast<MessageType>(n)); }
//                std::memset(data, 99, sizeof(data));
//            }
//            receiver.waitForPostEnd();
//            check(wait([&]() { return received == 10000; }, 10000), "U05 unified Post drains");
//            check(bad == 0, "U05 name/PID FIFO and input copied");
//        }
//        // U06：暂停真实读任务使缓冲满；同步send阻塞但CPU低，恢复后原包按序到达。
//        {
//            std::printf("CASE U06\n");
//            std::atomic<int32_t> received(0), bad(0), full(0);
//            ProcessWorkTestReceiver receiver;
//            receiver.m_message = [&](int32_t, const char* data, int32_t length, MessageType type)
//            {
//                if (length != 6 || std::memcmp(data, "123456", 6) != 0 || static_cast<int32_t>(type) != received.load()) { ++bad; }
//                ++received;
//            };
//            receiver.m_full = [&](int32_t count) { if (count != 1) { ++bad; } ++full; };
//            check(receiver.initReceive(64, 1), "U06 receiver ready");
//            receiver.send(self, "123456", 6, static_cast<MessageType>(0));
//            check(wait([&]() { return received == 1; }, 2000), "U06 warmup registered");
//            receiver.pauseReader();
//            receiver.send(self, "123456", 6, static_cast<MessageType>(1));
//            std::shared_ptr<ProcessWorkTestTask> task(new ProcessWorkTestTask);
//            task->m_action = [&]() { receiver.send(self, "123456", 6, static_cast<MessageType>(2)); };
//            const uint32_t id = CTaskThreadManager::Instance().Init();
//            CTaskThreadManager::Instance().GetThreadInterface(id)->PostTask(task);
//            const double before = cpu();
//            Semaphore sleep;
//            sleep.wait(1200);
//            const double used = cpu() - before;
//            check(!task->m_done, "U06 full sender actually blocks");
//            check(used >= 0 && used < 0.35, "U06 blocked CPU below 0.35 core-seconds/1.2s");
//            std::printf("CPU full_wait_seconds=1.2 cpu_seconds=%.6f\n", used);
//            check(receiver.resumeReader(), "U06 reader resumed");
//            check(wait([&]() { return task->m_done.load() && received == 3; }, 5000), "U06 all committed packets preserved");
//            CTaskThreadManager::Instance().Uninit(id);
//            check(!task->m_failed && bad == 0 && full > 0, "U06 order and full callback");
//        }
//        // U07：保留旧发送映射跨同PID接收器重建，旧代拒绝，新代可以发送。
//        {
//            std::printf("CASE U07\n");
//            ProcessWorkChannel old;
//            {
//                ProcessWorkTestReceiver receiver;
//                check(receiver.initReceive(64, 2) && old.openSender(self), "U07 open old generation");
//            }
//            check(!old.ready() && old.trySend("x", 1, 0) == PROCESS_WORK_UNAVAILABLE, "U07 old generation revoked");
//            std::atomic<int32_t> received(0);
//            ProcessWorkTestReceiver receiver;
//            receiver.m_message = [&](int32_t, const char* data, int32_t length, MessageType)
//            {
//                if (length == 1 && data[0] == 'x') { ++received; }
//            };
//            check(receiver.initReceive(64, 2), "U07 restart while old maps retained");
//            receiver.send(selfName, "x", 1);
//            check(wait([&]() { return received == 1; }, 3000), "U07 restarted receiver reached by name");
//        }
//        // U08：NetSender有界适配使用相同v2共享区，不另建发送协议或后台线程。
//        {
//            std::printf("CASE U08\n");
//            std::atomic<int32_t> received(0), bad(0);
//            ProcessWorkTestReceiver receiver;
//            receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//            {
//                const unsigned char expected[] = { 0, 255, 128, 7, 0 };
//                if (pid != self || length != 5 || static_cast<int32_t>(type) != 42 || std::memcmp(data, expected, 5) != 0) { ++bad; }
//                ++received;
//            };
//            check(receiver.initReceive(64, 1), "U08 receiver ready");
//            NetSenderIpc ipc;
//            check(ipc.open(self, self + 1, 52) == NET_SENDER_IPC_INVALID, "U08 forged local PID rejected");
//            check(ipc.open(self, self, 52) == NET_SENDER_IPC_ACCEPTED, "U08 native adapter opens v2");
//            const uint8_t body[] = { 0, 255, 128, 7, 0 };
//            int32_t result = NET_SENDER_IPC_BUSY;
//            check(wait([&]() { result = ipc.trySend(body, sizeof(body), 42); return result != NET_SENDER_IPC_BUSY; }, 3000) &&
//                result == NET_SENDER_IPC_ACCEPTED, "U08 bounded registration and commit");
//            check(wait([&]() { return received == 1; }, 3000) && bad == 0, "U08 adapter exact binary content");
//            check(ipc.pendingNotifications() == 0 && ipc.flushNotifications() == NET_SENDER_IPC_ACCEPTED, "U08 compatible notify queries");
//        }
//        // U10：两轮各10块扩容全部纳入空闲回收；存活写者不回收，满载回调允许重入send。
//        {
//            std::printf("CASE U10\n");
//            std::atomic<int32_t> received(0), bad(0), full(0), fullDone(0);
//            ProcessWorkTestReceiver receiver;
//            receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType)
//            {
//                if (pid != self || length != 6 || std::memcmp(data, "123456", 6) != 0) { ++bad; }
//                ++received;
//            };
//            receiver.m_full = [&](int32_t count)
//            {
//                if (count != 22) { ++bad; }
//                ++full;
//                receiver.send(self, "123456", 6, static_cast<MessageType>(99));
//                ++fullDone;
//            };
//            check(receiver.initReceive(4096, 2, 1, 10, 1, 2), "U10 dynamic receiver ready");
//            receiver.send(self, "123456", 6);
//            check(wait([&]() { return received == 1; }, 2000), "U10 warmup");
//            SharedMemory directoryMemory("P2D" + std::to_string(self));
//            ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//            check(directory != nullptr, "U10 discovery fixture");
//            if (directory == nullptr) { return 1; }
//            SharedMemory controlMemory(ProcessWorkChannel::objectName('C', self, directory->m_identity, static_cast<uint32_t>(directory->m_session)));
//            ProcessWorkControl* control = static_cast<ProcessWorkControl*>(controlMemory.writeWithoutLock());
//            check(control != nullptr, "U10 shared control fixture");
//            if (control == nullptr) { return 1; }
//            int64_t token = 0;
//            for (int32_t i = 0; i < 256; ++i)
//            {
//                if (control->m_peers[i].m_pid == self && control->m_peers[i].m_token > 0) { token = control->m_peers[i].m_token; break; }
//            }
//            check(token > 0, "U10 real identity granted");
//            check(wait([&]() { return AtomicMath::selfAdd(&control->m_blocks[0].m_owner, static_cast<int64_t>(0)) == 0 &&
//                AtomicMath::selfAdd(&control->m_blocks[1].m_owner, static_cast<int64_t>(0)) == 0; }, 2000), "U10 initial blocks free");
//            for (int32_t i = 0; i < 2; ++i)
//            {
//                check(AtomicMath::compareAndSwap(&control->m_blocks[i].m_owner, static_cast<int64_t>(0), token), "U10 pin initial block as live sender");
//            }
//            AtomicMath::compareAndSwap(&control->m_demand, 0, 1);
//            check(wait([&]() { return receiver.receiveBufferCount() == 12; }, 2000), "U10 first growth adds10");
//            for (int32_t i = 2; i < 12; ++i)
//            {
//                check(AtomicMath::compareAndSwap(&control->m_blocks[i].m_owner, static_cast<int64_t>(0), token), "U10 pin first grown batch");
//            }
//            AtomicMath::compareAndSwap(&control->m_demand, 0, 1);
//            check(wait([&]() { return receiver.receiveBufferCount() == 22; }, 2000), "U10 second growth adds10");
//            for (int32_t i = 12; i < 22; ++i)
//            {
//                check(AtomicMath::compareAndSwap(&control->m_blocks[i].m_owner, static_cast<int64_t>(0), token), "U10 pin second grown batch");
//            }
//            AtomicMath::compareAndSwap(&control->m_demand, 0, 1);
//            AtomicMath::compareAndSwap(&control->m_full, 0, 1);
//            check(wait([&]() { return full > 0; }, 2000), "U10 full snapshot dispatched");
//            Semaphore sleep;
//            sleep.wait(400);
//            check(receiver.receiveBufferCount() == 22, "U10 growth cap reached");
//            for (int32_t i = 0; i < 22; ++i)
//            {
//                check(AtomicMath::selfAdd(&control->m_blocks[i].m_owner, static_cast<int64_t>(0)) == token, "U10 live writer not reclaimed");
//                AtomicMath::compareAndSwap(&control->m_blocks[i].m_owner, token, static_cast<int64_t>(0));
//            }
//            check(wait([&]() { return fullDone == 1 && received == 2; }, 3000), "U10 callback reentrant send completes");
//            check(wait([&]() { return receiver.receiveBufferCount() == 2; }, 3500), "U10 all20 grown blocks released");
//            for (int32_t i = 2; i < 22; ++i)
//            {
//                check(AtomicMath::selfAdd(&control->m_blocks[i].m_owner, static_cast<int64_t>(0)) == -1, "U10 retired metadata unavailable");
//                SharedMemory retired(ProcessWorkChannel::objectName('B', self, directory->m_identity,
//                    static_cast<uint32_t>(directory->m_session), i, 1));
//                check(retired.writeWithoutLock() == nullptr, "U10 retired mapping actually released");
//            }
//            for (int32_t i = 0; i < 2; ++i)
//            {
//                check(AtomicMath::compareAndSwap(&control->m_blocks[i].m_owner, static_cast<int64_t>(0), token), "U10 repin base");
//            }
//            AtomicMath::compareAndSwap(&control->m_demand, 0, 1);
//            check(wait([&]() { return receiver.receiveBufferCount() == 12; }, 2000), "U10 shrink returns growth budget");
//            check(control->m_blocks[2].m_generation == 2, "U10 regrowth uses new generation");
//            for (int32_t i = 0; i < 2; ++i) { AtomicMath::compareAndSwap(&control->m_blocks[i].m_owner, token, static_cast<int64_t>(0)); }
//            check(full == 1 && bad == 0, "U10 full event coalescing and contents");
//        }
//        // U11：靠近票号上界仍正确提交，耗尽明确拒绝，不发生有符号回绕或遗留占用。
//        {
//            std::printf("CASE U11\n");
//            std::atomic<int32_t> received(0), bad(0);
//            ProcessWorkTestReceiver receiver;
//            receiver.m_message = [&](int32_t, const char* data, int32_t length, MessageType type)
//            {
//                if (length != 1 || data[0] != 'x' || static_cast<int32_t>(type) != received.load()) { ++bad; }
//                ++received;
//            };
//            check(receiver.initReceive(64, 3), "U11 receiver ready");
//            receiver.send(self, "x", 1, static_cast<MessageType>(0));
//            check(wait([&]() { return received == 1; }, 2000), "U11 warmup");
//            receiver.pauseReader();
//            SharedMemory directoryMemory("P2D" + std::to_string(self));
//            ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//            if (directory == nullptr) { return 1; }
//            SharedMemory controlMemory(ProcessWorkChannel::objectName('C', self, directory->m_identity, static_cast<uint32_t>(directory->m_session)));
//            ProcessWorkControl* control = static_cast<ProcessWorkControl*>(controlMemory.writeWithoutLock());
//            if (control == nullptr) { return 1; }
//            const int64_t ticket = (INT64_MAX >> 11) - 3 - 2;
//            control->m_tail = control->m_head = ticket;
//            for (int32_t offset = 0; offset < 3; ++offset) { control->m_ring[(ticket + offset) % 3] = (ticket + offset) << 11; }
//            check(receiver.resumeReader(), "U11 reader resumed at boundary");
//            for (int32_t i = 1; i <= 3; ++i) { receiver.send(self, "x", 1, static_cast<MessageType>(i)); }
//            check(wait([&]() { return received == 4; }, 2000) && bad == 0, "U11 final valid tickets delivered in order");
//            check(receiver.trySend(self, "x", 1) == PROCESS_WORK_UNAVAILABLE, "U11 exhausted ticket refuses commit");
//            for (int32_t i = 0; i < 3; ++i)
//            {
//                check(AtomicMath::selfAdd(&control->m_blocks[i].m_owner, static_cast<int64_t>(0)) == 0, "U11 failed ticket releases block");
//            }
//        }
//        // U12：满载中取消Post任务有界退出，取消的是尚未提交的包；已提交包保留。
//        {
//            std::printf("CASE U12\n");
//            std::atomic<int32_t> received(0), bad(0);
//            ProcessWorkTestReceiver receiver;
//            receiver.m_message = [&](int32_t, const char* data, int32_t length, MessageType type)
//            {
//                if (length != 1 || data[0] != 'x' || static_cast<int32_t>(type) != received.load()) { ++bad; }
//                ++received;
//            };
//            check(receiver.initReceive(64, 1), "U12 receiver ready");
//            receiver.send(self, "x", 1, static_cast<MessageType>(0));
//            check(wait([&]() { return received == 1; }, 2000), "U12 warmup");
//            receiver.pauseReader();
//            receiver.send(self, "x", 1, static_cast<MessageType>(1));
//            receiver.initPostThread();
//            receiver.post(self, "x", 1, static_cast<MessageType>(2));
//            Semaphore sleep;
//            sleep.wait(100);
//            const int64_t start = now();
//            receiver.uninitPostThread();
//            check(now() - start < 1000, "U12 full Post worker cancels within1s");
//            check(receiver.resumeReader(), "U12 reader resumed");
//            check(wait([&]() { return received == 2; }, 2000), "U12 pre-cancel committed packet delivered");
//            sleep.wait(50);
//            check(received == 2 && bad == 0, "U12 uncommitted cancellation explicit");
//        }
//        // U13：单个接收者抛异常不能截断其他回调或泄漏后续消息。
//        {
//            std::printf("CASE U13\n");
//            std::atomic<int32_t> received(0), bad(0);
//            ProcessWorkTestReceiver observer;
//            observer.m_message = [&](int32_t, const char* data, int32_t length, MessageType type)
//            {
//                if (length != 1 || data[0] != 'x' || static_cast<int32_t>(type) != received.load()) { ++bad; }
//                ++received;
//            };
//            ProcessWorkTestReceiver receiver;
//            receiver.m_message = [](int32_t, const char*, int32_t, MessageType) { throw std::runtime_error("test callback failure"); };
//            receiver.addProcessReceiveCallback(&observer);
//            check(receiver.initReceive(64, 3), "U13 receiver ready");
//            for (int32_t i = 0; i < 1000; ++i) { receiver.send(self, "x", 1, static_cast<MessageType>(i)); }
//            check(wait([&]() { return received == 1000; }, 3000) && bad == 0, "U13 observer exceptions isolated");
//        }
//
//#if defined(_WIN32) && defined(PROCESS_WORK_TEST_ALLOCATION)
//        // U14：真实读任务的new[]或本地队列new失败时保持头包，恢复后两包有序且不重复。
//        {
//            std::printf("CASE U14\n");
//            for (int32_t mode = 1; mode <= 2; ++mode)
//            {
//                std::atomic<int32_t> received(0), bad(0);
//                ProcessWorkTestReceiver receiver;
//                receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//                {
//                    const int32_t sequence = static_cast<int32_t>(type);
//                    if (pid != self || length != 255 || sequence != received.load()) { ++bad; }
//                    else
//                    {
//                        for (int32_t i = 0; i < length; ++i) { if (static_cast<unsigned char>(data[i]) != static_cast<unsigned char>(sequence * 31 + i)) { ++bad; break; } }
//                    }
//                    ++received;
//                };
//                check(receiver.initReceive(512, 2), "U14 receiver ready");
//                char payload[255];
//                for (int32_t i = 0; i < 255; ++i) { payload[i] = static_cast<char>(i); }
//                receiver.send(self, payload, 255, static_cast<MessageType>(0));
//                check(wait([&]() { return received == 1; }, 2000), "U14 warmup");
//                receiver.pauseReader();
//                std::shared_ptr<ReadTask> inner = receiver.readerTask();
//                std::shared_ptr<ProcessWorkTestTask> wrapper(new ProcessWorkTestTask);
//                wrapper->m_action = [inner]()
//                {
//                    // 读任务结束时总是解除注入，不能影响线程库自身的退出回收。
//                    auto clear = [](std::atomic<uint32_t>*) { s_testAllocationThread = 0; };
//                    std::unique_ptr<std::atomic<uint32_t>, decltype(clear)> scope(&s_testAllocationThread, clear);
//                    s_testAllocationThread = ::GetCurrentThreadId();
//                    inner->DoTask();
//                };
//                wrapper->m_stopAction = [inner]() { inner->StopTask(); };
//                s_testAllocationHits = 0;
//                s_testAllocationMode = mode;
//                check(receiver.resumeReaderWithTask(wrapper), "U14 instrumented real reader running");
//                check(wait([&]() { return s_testAllocationThread.load() != 0; }, 2000), "U14 selected actual worker");
//                for (int32_t n = 1; n <= 2; ++n)
//                {
//                    for (int32_t i = 0; i < 255; ++i) { payload[i] = static_cast<char>(n * 31 + i); }
//                    receiver.send(self, payload, 255, static_cast<MessageType>(n));
//                }
//                check(wait([&]() { return s_testAllocationHits.load() > 0; }, 2000), "U14 actual allocation failed");
//                Semaphore sleep;
//                sleep.wait(150);
//                check(received == 1, "U14 later packet cannot overtake failed copy");
//                s_testAllocationMode = 0;
//                check(wait([&]() { return received == 3; }, 3000), "U14 preserved messages delivered after recovery");
//                sleep.wait(50);
//                check(received == 3 && bad == 0, "U14 retry does not duplicate or corrupt");
//                receiver.uninitReceive();
//                check(!wrapper->m_failed && s_testAllocationThread == 0, "U14 reader stopped with injection cleared");
//            }
//        }
//#endif
//
//        // U15：名称缓存不能把旧进程代次误投给复用PID的新进程。
//        {
//            std::printf("CASE U15\n");
//            ProcessWorkTestReceiver receiver;
//            std::atomic<int32_t> received(0);
//            receiver.m_message = [&](int32_t, const char*, int32_t, MessageType) { ++received; };
//            check(receiver.initReceive(64, 2), "U15 init");
//#ifdef _WIN32
//            const std::string name = CSystem::GetCurrentExeName();
//#else
//            const std::string name = CSystem::GetCurrentExeFullName();
//#endif
//            uint64_t hash = 14695981039346656037ULL;
//            for (size_t i = 0; i < name.size(); ++i) { hash ^= static_cast<unsigned char>(name[i]); hash *= 1099511628211ULL; }
//            SharedMemory names("P2N" + std::to_string(hash));
//            ProcessWorkNameDirectory* directory = static_cast<ProcessWorkNameDirectory*>(names.writeWithoutLock());
//            check(directory != nullptr, "U15 discovery");
//            if (directory != nullptr)
//            {
//                const uint64_t identity = directory->m_identity;
//                directory->m_identity = identity + 1;
//                receiver.send(name, "old", 3);
//                Semaphore pause; pause.wait(50);
//                check(received == 0, "U15 mismatched name identity never misdelivered");
//                directory->m_identity = identity;
//                receiver.send(name, "new", 3);
//                check(wait([&]() { return received == 1; }, 2000), "U15 matching identity delivered");
//            }
//        }
//
//        // U16：同进程多个独立通道同时打开目标，复用一个进程登记。
//        {
//            std::printf("CASE U16\n");
//            for (int32_t round = 0; round < 8; ++round)
//            {
//                ProcessWorkChannel receiver;
//                check(receiver.openReceiver(64, 2, 0, 0, 0, 0), "U16 receiver");
//                receiver.publish();
//                std::atomic<int32_t> started(0), opened(0);
//                std::atomic<bool> go(false);
//                std::vector<std::shared_ptr<ProcessWorkChannel> > channels;
//                std::vector<std::shared_ptr<ProcessWorkTestTask> > tasks;
//                std::vector<uint32_t> ids;
//                for (int32_t n = 0; n < 32; ++n)
//                {
//                    std::shared_ptr<ProcessWorkChannel> sender(new ProcessWorkChannel);
//                    std::shared_ptr<ProcessWorkTestTask> task(new ProcessWorkTestTask);
//                    task->m_action = [&, sender]()
//                    {
//                        ++started;
//                        if (!wait([&]() { return go.load(); }, 3000)) { return; }
//                        if (sender->openSender(self)) { ++opened; }
//                    };
//                    const uint32_t id = CTaskThreadManager::Instance().Init();
//                    CTaskThreadManager::Instance().GetThreadInterface(id)->PostTask(task);
//                    channels.push_back(sender); tasks.push_back(task); ids.push_back(id);
//                }
//                check(wait([&]() { return started == 32; }, 3000), "U16 producers ready");
//                go = true;
//                check(wait([&]() { return opened == 32; }, 3000), "U16 concurrent opens");
//                for (size_t i = 0; i < ids.size(); ++i) { CTaskThreadManager::Instance().Uninit(ids[i]); }
//                SharedMemory directoryMemory("P2D" + std::to_string(self));
//                ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//                if (directory == nullptr) { check(false, "U16 directory"); continue; }
//                SharedMemory controlMemory(ProcessWorkChannel::objectName('C', self, directory->m_identity, static_cast<uint32_t>(directory->m_session)));
//                ProcessWorkControl* control = static_cast<ProcessWorkControl*>(controlMemory.writeWithoutLock());
//                int32_t registrations = 0;
//                if (control != nullptr)
//                {
//                    for (int32_t i = 0; i < 256; ++i) { if (control->m_peers[i].m_pid == self) { ++registrations; } }
//                }
//                check(control != nullptr && registrations == 1, "U16 one registration per process");
//            }
//        }
//
//        // U17：只读查询持有最后引用时，升级写权限和反复重映射不能丢失对象。
//        {
//            std::printf("CASE U17\n");
//            const std::string name = "P2Last" + std::to_string(self);
//            std::unique_ptr<SharedMemory> creator(new SharedMemory(name, 64, false));
//            char* initial = static_cast<char*>(creator->writeWithoutLock());
//            check(initial != nullptr, "U17 creator mapped");
//            if (initial != nullptr) { initial[0] = 91; }
//            SharedMemory peer(name);
//            check(peer.realSize() >= 64, "U17 retain read handle");
//            creator.reset();
//            char* data = static_cast<char*>(peer.writeWithoutLock());
//            check(data != nullptr && data[0] == 91, "U17 upgrade last reference preserves data");
//            for (int32_t round = 0; round < 100; ++round)
//            {
//                peer.close();
//                data = static_cast<char*>(peer.writeWithoutLock());
//                check(data != nullptr && data[0] == 91, "U17 remap last reference");
//            }
//        }
//        // U18：stop不能交出仍在清理的发现入口，也不能重新publish复活旧代。
//        {
//            std::printf("CASE U18\n");
//            ProcessWorkChannel old;
//            check(old.openReceiver(64, 2, 0, 0, 0, 0), "U18 open old");
//            old.publish();
//            ProcessWorkChannel held;
//            check(held.openSender(self), "U18 retain old sender");
//            old.stop();
//            old.stop();
//            old.publish();
//            check(!old.ready() && !held.ready(), "U18 stopped generation cannot republish");
//            ProcessWorkChannel premature;
//            check(!premature.openReceiver(64, 2, 0, 0, 0, 0), "U18 cleanup lease not stolen");
//            old.close();
//            ProcessWorkChannel next;
//            check(next.openReceiver(64, 2, 0, 0, 0, 0), "U18 new generation after close");
//            next.publish();
//            old.close();
//            old.stop();
//            old.publish();
//            ProcessWorkChannel fresh;
//            check(next.ready() && fresh.openSender(self) && !held.ready(), "U18 old cleanup cannot revoke new receiver");
//        }
//        // U19：生命周期清理本地通知，既没有旧计数空转，也不破坏之后的新通知。
//        {
//            std::printf("CASE U19\n");
//            Semaphore local;
//            for (int32_t i = 0; i < 10000; ++i) { local.signal(); }
//            local.reset();
//            check(!local.wait(0), "U19 stale count removed");
//            local.signal();
//            check(local.wait(0) && !local.wait(0), "U19 future notification retained");
//        }
//        // U20：有上限的通知发布必须报告饱和，不溢出为负数或永久失效。
//        {
//            std::printf("CASE U20\n");
//            Semaphore owner, peer;
//            const std::string name = "P2Max" + std::to_string(self);
//            owner.createProcessSemaphore(name, 2);
//            check(peer.tryOpenProcessSemaphore(name), "U20 open");
//            check(peer.tryProcessSignal() == 0 && peer.tryProcessSignal() == 0, "U20 fill");
//#if defined(_WIN32) || defined(SHARED_MEMORY_MOBILE) || defined(PROCESS_WORK_PORTABLE_NOTIFY)
//            check(peer.tryProcessSignal() == 1, "U20 saturated result");
//#endif
//            check(owner.processWait(0) && owner.processWait(0) && !owner.processWait(0), "U20 exact drain");
//            check(!owner.processWait(-2), "U20 invalid timeout");
//            check(peer.tryProcessSignal() == 0 && owner.processWait(0), "U20 reusable after saturation");
//        }
//        // U21：损坏的公开元数据在读头前拒绝；不把非法上限用于数组下标。
//        {
//            std::printf("CASE U21\n");
//            ProcessWorkChannel receiver;
//            check(receiver.openReceiver(64, 2, 0, 0, 0, 0), "U21 receiver");
//            receiver.publish();
//            SharedMemory directoryMemory("P2D" + std::to_string(self));
//            ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//            if (directory != nullptr)
//            {
//                SharedMemory controlMemory(ProcessWorkChannel::objectName('C', self, directory->m_identity, static_cast<uint32_t>(directory->m_session)));
//                ProcessWorkControl* control = static_cast<ProcessWorkControl*>(controlMemory.writeWithoutLock());
//                check(control != nullptr, "U21 control");
//                if (control != nullptr)
//                {
//                    const int32_t limit = control->m_limit;
//                    control->m_limit = 1025;
//                    ProcessWorkChannel sender;
//                    check(!sender.openSender(self), "U21 invalid limit refused");
//                    control->m_limit = limit;
//                    control->m_blockBytes = 11;
//                    check(!sender.openSender(self), "U21 invalid body bound refused");
//                    control->m_blockBytes = 64;
//                    check(sender.openSender(self), "U21 valid metadata still usable");
//                }
//            }
//            else { check(false, "U21 directory"); }
//        }
//
//        // U22：逻辑名称独立于宿主可执行文件，拒绝运行中变更以及非法长度。
//        {
//            std::printf("CASE U22\n");
//            ProcessWorkTestReceiver receiver;
//            const std::string name = "app.worker:" + std::to_string(self);
//            check(!receiver.setReceiveName(std::string(1024, 'x')), "U22 oversized name refused");
//            check(!receiver.setReceiveName(std::string("a\0b", 3)), "U22 NUL name refused");
//            check(receiver.setReceiveName(name), "U22 configured name");
//            std::atomic<int32_t> received(0), bad(0);
//            receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//            {
//                if (pid != self || length != 6 || data == nullptr || std::memcmp(data, "123456", 6) != 0 || static_cast<int32_t>(type) != received.load()) { ++bad; }
//                ++received;
//            };
//            check(receiver.initReceive(64, 4), "U22 initialize named receiver");
//            check(!receiver.setReceiveName("other"), "U22 live rename refused");
//            receiver.initPostThread();
//            for (int32_t i = 0; i < 1000; ++i)
//            {
//                if (i % 2 == 0) { receiver.post(name, "123456", 6, static_cast<MessageType>(i)); }
//                else { receiver.post(self, "123456", 6, static_cast<MessageType>(i)); }
//            }
//            receiver.waitForPostEnd();
//            check(wait([&]() { return received == 1000; }, 3000) && bad == 0, "U22 mixed PID/custom-name FIFO exact bytes");
//        }
//        // U23：并发初始化同一PID时仅一个接收器获胜，失败者析构不撤销胜者入口。
//        {
//            std::printf("CASE U23\n");
//            for (int32_t round = 0; round < 16; ++round)
//            {
//                std::atomic<int32_t> started(0), finished(0), winners(0);
//                std::atomic<bool> go(false);
//                std::vector<std::shared_ptr<ProcessWorkChannel> > channels;
//                std::vector<uint32_t> ids;
//                for (int32_t n = 0; n < 16; ++n)
//                {
//                    std::shared_ptr<ProcessWorkChannel> channel(new ProcessWorkChannel);
//                    std::shared_ptr<ProcessWorkTestTask> task(new ProcessWorkTestTask);
//                    task->m_action = [&, channel]()
//                    {
//                        ++started;
//                        if (wait([&]() { return go.load(); }, 3000) && channel->openReceiver(64, 2, 0, 0, 0, 0))
//                        {
//                            channel->publish();
//                            ++winners;
//                        }
//                        ++finished;
//                    };
//                    const uint32_t id = CTaskThreadManager::Instance().Init();
//                    CTaskThreadManager::Instance().GetThreadInterface(id)->PostTask(task);
//                    channels.push_back(channel);
//                    ids.push_back(id);
//                }
//                check(wait([&]() { return started == 16; }, 3000), "U23 initializers ready");
//                go = true;
//                check(wait([&]() { return finished == 16; }, 3000) && winners == 1, "U23 exactly one winner");
//                for (size_t i = 0; i < ids.size(); ++i) { CTaskThreadManager::Instance().Uninit(ids[i]); }
//                for (size_t i = 0; i < channels.size(); ++i) { if (!channels[i]->ready()) { channels[i]->close(); } }
//                ProcessWorkChannel sender;
//                check(sender.openSender(self), "U23 failed owners do not unlink winner");
//            }
//        }
//
//        // U24：扩容映射分配持续失败时，有空槽也必须退让，恢复后正文仍能提交。
//        {
//            std::printf("CASE U24\n");
//#if defined(_WIN32) && defined(PROCESS_WORK_TEST_ALLOCATION)
//            ProcessWorkChannel receiver, sender;
//            check(receiver.openReceiver(64, 1, 1, 1, 0, 1), "U24 receiver");
//            receiver.publish();
//            check(sender.openSender(self), "U24 sender");
//            LockFreeQueue<char*> queue;
//            Semaphore notification;
//            std::atomic<int32_t> full(0);
//            receiver.pump(queue, notification, full);
//            SharedMemory directoryMemory("P2D" + std::to_string(self));
//            ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//            if (directory != nullptr)
//            {
//                SharedMemory controlMemory(ProcessWorkChannel::objectName('C', self, directory->m_identity, static_cast<uint32_t>(directory->m_session)));
//                ProcessWorkControl* control = static_cast<ProcessWorkControl*>(controlMemory.writeWithoutLock());
//                if (control != nullptr)
//                {
//                    AtomicMath::compareAndSwap(&control->m_demand, 0, 1);
//                    Semaphore sleep;
//                    sleep.wait(25);
//                    receiver.pump(queue, notification, full);
//                    check(receiver.bufferCount() == 2, "U24 expanded block available");
//                    const int64_t token = control->m_peers[0].m_token;
//                    check(token > 0 && AtomicMath::compareAndSwap(&control->m_blocks[0].m_owner, static_cast<int64_t>(0), token), "U24 occupy initial block");
//                    const int64_t start = now();
//                    s_testAllocationHits = 0;
//                    s_testAllocationThread = ::GetCurrentThreadId();
//                    s_testAllocationMode = 2;
//                    for (int32_t i = 0; i < 10; ++i)
//                    {
//                        check(sender.trySend("123456", 6, 0) == PROCESS_WORK_BUSY, "U24 allocation failure retryable");
//                        check(sender.waitForSpace(), "U24 receiver remains available");
//                    }
//                    s_testAllocationMode = 0;
//                    s_testAllocationThread = 0;
//                    check(s_testAllocationHits >= 10 && now() - start >= 8, "U24 repeated OOM does not busy-spin");
//                    check(sender.trySend("123456", 6, 0) == PROCESS_WORK_SENT, "U24 allocation recovery submits");
//                    receiver.pump(queue, notification, full);
//                    char* packet = nullptr;
//                    check(queue.pop(&packet) && packet != nullptr && std::memcmp(packet + 12, "123456", 6) == 0, "U24 exact recovered payload");
//                    delete[] packet;
//                    AtomicMath::compareAndSwap(&control->m_blocks[0].m_owner, token, static_cast<int64_t>(0));
//                }
//                else { check(false, "U24 control mapping"); }
//            }
//            else { check(false, "U24 directory mapping"); }
//#endif
//        }
//
//        }
//        // U25：维护快照早于本轮新块的创建时间，不能发生无符号下溢并立即缩容。
//        {
//            std::printf("CASE U25\n");
//#if defined(_WIN32) && defined(PROCESS_WORK_TEST_ALLOCATION)
//            ProcessWorkChannel receiver;
//            check(receiver.openReceiver(64, 1, 1, 2, 1, 1), "U25 receiver");
//            receiver.publish();
//            SharedMemory directoryMemory("P2D" + std::to_string(self));
//            ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//            if (directory != nullptr)
//            {
//                SharedMemory controlMemory(ProcessWorkChannel::objectName('C', self, directory->m_identity, static_cast<uint32_t>(directory->m_session)));
//                ProcessWorkControl* control = static_cast<ProcessWorkControl*>(controlMemory.writeWithoutLock());
//                if (control != nullptr)
//                {
//                    LockFreeQueue<char*> queue;
//                    Semaphore notification;
//                    std::atomic<int32_t> full(0);
//                    AtomicMath::compareAndSwap(&control->m_demand, 0, 1);
//                    s_testAllocationHits = 0;
//                    s_testAllocationThread = ::GetCurrentThreadId();
//                    s_testAllocationMode = 3;
//                    receiver.pump(queue, notification, full);
//                    s_testAllocationMode = 0;
//                    s_testAllocationThread = 0;
//                    check(s_testAllocationHits > 0, "U25 actual creation crossed timestamp snapshot");
//                    check(receiver.bufferCount() == 3, "U25 newly grown blocks survive same maintenance");
//                    Semaphore sleep;
//                    sleep.wait(25);
//                    receiver.pump(queue, notification, full);
//                    check(receiver.bufferCount() == 3, "U25 no retirement before idle interval");
//                    sleep.wait(1100);
//                    receiver.pump(queue, notification, full);
//                    check(receiver.bufferCount() == 1, "U25 all grown blocks retire after real idle interval");
//                }
//                else { check(false, "U25 control mapping"); }
//            }
//            else { check(false, "U25 directory mapping"); }
//#endif
//        }
//        if (action == "unit")
//        {
//        // U09：重复创建/关闭后线程与句柄回落，基线先包含运行库的首次惰性初始化。
//        {
//            std::printf("CASE U09\n");
//            const uint32_t before = handles();
//            const int32_t threads = CTaskThreadManager::Instance().Count();
//            for (int32_t round = 0; round < 30; ++round)
//            {
//                ProcessWorkTestReceiver receiver;
//                check(receiver.initReceive(4096, 4), "U09 repeated receiver init");
//                receiver.initPostThread();
//                receiver.post(self, "123456", 6);
//                receiver.waitForPostEnd();
//            }
//            Semaphore sleep;
//            sleep.wait(100);
//            check(CTaskThreadManager::Instance().Count() == threads, "U09 no retained CTask threads");
//            check(handles() <= before + 4, "U09 no handle growth over30cycles");
//            std::printf("RESOURCES before=%u after=%u threads=%d\n", before, handles(), CTaskThreadManager::Instance().Count());
//        }
//        }
//        CTaskThreadManager::Instance().UninitAll();
//        std::printf("RESULT {\"action\":\"unit\",\"checks\":%d,\"errors\":%d}\n", checks, errors);
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "network")
//    {
//        try
//        {
//            NetInterfaceIpcTestCases::notifications();
//            NetInterfaceIpcTestCases::stalledReceiver();
//            NetInterfaceIpcTestCases::publication();
//            NetInterfaceIpcTestCases::queueLifecycle();
//            NetInterfaceIpcTestCases::localPackets();
//        }
//        catch (const std::exception& error) { std::fprintf(stderr, "NETWORK FAIL %s\n", error.what()); return 1; }
//        std::printf("RESULT network5 PASS\n");
//        return 0;
//    }
//#ifdef _WIN32
//    if (action == "network-audit")
//    {
//        if (argc != 3) { return 2; }
//        try { return NetInterfaceIpcTestCases::auditChild(argv[2]); }
//        catch (const std::exception& error) { std::fprintf(stderr, "NETWORK AUDIT FAIL %s\n", error.what()); return 1; }
//    }
//#endif
//    if (action == "fault-server")
//    {
//        if (argc != 3) { return 2; }
//        const int32_t phase = std::atoi(argv[2]);
//        if (phase < 0 || phase > 3) { return 2; }
//        std::atomic<int32_t> received(0), bad(0), own(0), source(0);
//        ProcessWorkTestReceiver receiver;
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            if (length != 6 || data == nullptr || std::memcmp(data, "123456", 6) != 0) { ++bad; }
//            if (static_cast<int32_t>(type) == 1000000000)
//            {
//                if (pid != self) { ++bad; }
//                ++own;
//                return;
//            }
//            int32_t expectedSource = 0;
//            source.compare_exchange_strong(expectedSource, pid);
//            if (pid != source || static_cast<int32_t>(type) != received.load()) { ++bad; }
//            ++received;
//        };
//        if (!receiver.initReceive(4096, 4)) { return 3; }
//        SharedMemory directoryMemory("P2D" + std::to_string(self));
//        ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//        if (directory == nullptr) { return 3; }
//        SharedMemory controlMemory(ProcessWorkChannel::objectName('C', self, directory->m_identity, static_cast<uint32_t>(directory->m_session)));
//        ProcessWorkControl* control = static_cast<ProcessWorkControl*>(controlMemory.writeWithoutLock());
//        if (control == nullptr) { return 3; }
//        std::printf("READY %d\n", self);
//        std::fflush(stdout);
//        int32_t child = 0;
//        if (!(std::cin >> child) || child <= 0) { return 4; }
//        check(wait([&]() { return received >= (phase == 3 ? 1000 : 1); }, 3000), "F warmup received before kill");
//        if (phase < 3)
//        {
//            Semaphore sleep;
//            sleep.wait(450);
//            int32_t held = 0;
//            for (int32_t i = 0; i < 4; ++i)
//            {
//                if (AtomicMath::selfAdd(&control->m_blocks[i].m_owner, static_cast<int64_t>(0)) > 0) { ++held; }
//            }
//            check(held == (phase == 2 ? 3 : 4), "F live paused writer remains reserved");
//        }
//        std::printf("ALIVE_OK\n");
//        std::fflush(stdout);
//        std::string command;
//        if (!(std::cin >> command) || command != "killed") { return 5; }
//        const int64_t recoveryStart = now();
//        check(CSystem::processDefinitelyExited(child), "F owned child really exited");
//        check(wait([&]()
//        {
//            for (int32_t i = 0; i < 4; ++i)
//            {
//                if (AtomicMath::selfAdd(&control->m_blocks[i].m_owner, static_cast<int64_t>(0)) != 0) { return false; }
//            }
//            return true;
//        }, 4000), "F every abandoned block recovered");
//        const int64_t recoveryMs = now() - recoveryStart;
//        receiver.send(self, "123456", 6, static_cast<MessageType>(1000000000));
//        check(wait([&]() { return own == 1; }, 3000), "F new writer can use recovered receiver");
//        const int32_t expected = phase == 2 ? 2 : 1;
//        check(phase == 3 ? received >= 1000 : received == expected, "F committed count preserved, uncommitted not delivered");
//        check(bad == 0 && source == child, "F data and committed sequence prefix exact");
//        std::printf("RESULT {\"action\":\"fault-server\",\"phase\":%d,\"received\":%d,\"bad\":%d,\"errors\":%d,\"recovery_ms\":%lld}\n",
//            phase, received.load(), bad.load(), errors, static_cast<long long>(recoveryMs));
//        receiver.uninitReceive();
//        CTaskThreadManager::Instance().UninitAll();
//        return errors == 0 ? 0 : 1;
//    }
//    if (action == "fault-child")
//    {
//        if (argc != 4) { return 2; }
//        const int32_t destination = std::atoi(argv[2]), phase = std::atoi(argv[3]);
//        if (destination <= 0 || phase < 0 || phase > 3) { return 2; }
//        ProcessWorkChannel sender;
//        if (!sender.openSender(destination)) { return 3; }
//        int32_t result = PROCESS_WORK_BUSY;
//        if (!wait([&]() { result = sender.trySend("123456", 6, 0); return result != PROCESS_WORK_BUSY; }, 5000) || result != PROCESS_WORK_SENT) { return 4; }
//        if (phase == 3)
//        {
//            for (int32_t sequence = 1; sequence < 2000000; ++sequence)
//            {
//                do
//                {
//                    result = sender.trySend("123456", 6, sequence);
//                    if (result == PROCESS_WORK_BUSY && !sender.waitForSpace()) { return 5; }
//                }
//                while (result == PROCESS_WORK_BUSY);
//                if (result != PROCESS_WORK_SENT) { return 5; }
//                if (sequence == 1000) { std::printf("RUNNING %d\n", self); std::fflush(stdout); }
//            }
//            return 6;
//        }
//        // 状态注入只在测试main内；借用真实登记号构造发送者被终止的三个边界。
//        SharedMemory directoryMemory("P2D" + std::to_string(destination));
//        ProcessWorkDirectory* directory = static_cast<ProcessWorkDirectory*>(directoryMemory.writeWithoutLock());
//        if (directory == nullptr) { return 3; }
//        SharedMemory controlMemory(ProcessWorkChannel::objectName('C', destination, directory->m_identity, static_cast<uint32_t>(directory->m_session)));
//        ProcessWorkControl* control = static_cast<ProcessWorkControl*>(controlMemory.writeWithoutLock());
//        if (control == nullptr) { return 3; }
//        int64_t token = 0;
//        for (int32_t i = 0; i < 256; ++i)
//        {
//            if (control->m_peers[i].m_pid == self && control->m_peers[i].m_token > 0) { token = control->m_peers[i].m_token; break; }
//        }
//        if (token <= 0) { return 4; }
//        int32_t claimed = 0;
//        bool held[4] = {};
//        if (!wait([&]()
//        {
//            for (int32_t i = 0; i < 4; ++i)
//            {
//                if (!held[i] && AtomicMath::compareAndSwap(&control->m_blocks[i].m_owner, static_cast<int64_t>(0), token))
//                {
//                    held[i] = true;
//                    ++claimed;
//                }
//            }
//            return claimed == 4;
//        }, 3000)) { return 7; }
//        if (phase > 0)
//        {
//            for (int32_t i = 0; i < 4; ++i)
//            {
//                SharedMemory memory(ProcessWorkChannel::objectName('B', destination, directory->m_identity,
//                    static_cast<uint32_t>(directory->m_session), i, 1));
//                char* data = static_cast<char*>(memory.writeWithoutLock());
//                if (data == nullptr) { return 8; }
//                const int32_t header[] = { self, i + 1, 6 };
//                std::memcpy(data, header, sizeof(header));
//                std::memcpy(data + 12, "123456", 6);
//            }
//        }
//        if (phase == 2)
//        {
//            const int64_t ticket = AtomicMath::selfAdd(&control->m_tail, static_cast<int64_t>(0));
//            if (!AtomicMath::compareAndSwap(&control->m_ring[ticket % 4], ticket << 11, (ticket << 11) | 1)) { return 9; }
//            // 故意不推进尾，也不通知；实际接收任务必须自己完成剩余步骤。
//        }
//        std::printf("CLAIMED %d\n", self);
//        std::fflush(stdout);
//        Semaphore sleep;
//        for (;;) { sleep.wait(1000); }
//    }
//
//#ifdef _WIN32
//    if (action == "resources")
//    {
//        size_t baseline = 0;
//        size_t finalBytes = 0;
//        uint32_t baselineHandles = 0;
//        for (int32_t round = 0; round < 120; ++round)
//        {
//            {
//                ProcessWorkTestReceiver receiver;
//                std::atomic<int32_t> received(0), bad(0);
//                receiver.m_message = [&](int32_t, const char* data, int32_t length, MessageType type)
//                {
//                    if (length != 4096 || static_cast<int32_t>(type) != received) { ++bad; }
//                    for (int32_t i = 0; i < length; ++i) { if (data[i] != static_cast<char>(i + static_cast<int32_t>(type))) { ++bad; break; } }
//                    ++received;
//                };
//                check(receiver.initReceive(4108, 4, 1, 4, 1, 4), "resource receiver init");
//                receiver.initPostThread();
//                char bytes[4096];
//                for (int32_t n = 0; n < 1000; ++n)
//                {
//                    for (int32_t i = 0; i < 4096; ++i) { bytes[i] = static_cast<char>(n + i); }
//                    receiver.post(self, bytes, 4096, static_cast<MessageType>(n));
//                }
//                receiver.waitForPostEnd();
//                check(wait([&]() { return received == 1000; }, 3000) && bad == 0, "resource exact body and sequence");
//            }
//            if ((round + 1) % 20 == 0)
//            {
//                Semaphore pause; pause.wait(100);
//                PROCESS_MEMORY_COUNTERS_EX info = {};
//                info.cb = sizeof(info);
//                check(::GetProcessMemoryInfo(::GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&info), sizeof(info)) != FALSE, "resource native query");
//                finalBytes = info.PrivateUsage;
//                const uint32_t currentHandles = handles();
//                if (round == 19) { baseline = finalBytes; baselineHandles = currentHandles; }
//                std::printf("RESOURCE cycle=%d private_bytes=%llu handles=%u ctask=%d\n", round + 1,
//                    static_cast<unsigned long long>(finalBytes), currentHandles, CTaskThreadManager::Instance().Count());
//                check(CTaskThreadManager::Instance().Count() == 0, "resource no retained workers");
//                check(currentHandles <= baselineHandles + 4, "resource stable handles after warmup");
//            }
//        }
//        // 单批正文约4MiB；允许分配器缓存一批，不允许100次生命周期线性保留。
//        check(finalBytes <= baseline + 4096U * 1000U, "resource private growth bounded by one batch");
//        std::printf("RESULT resources checks=%d errors=%d\n", checks, errors);
//        return errors == 0 ? 0 : 1;
//    }
//#endif
//    if (action == "dead-server")
//    {
//        ProcessWorkTestReceiver receiver;
//        if (!receiver.initReceive(64, 2)) { return 3; }
//        std::printf("READY %d\n", self); std::fflush(stdout);
//        std::string command;
//        std::cin >> command;
//        receiver.pauseReader();
//        std::printf("PAUSED\n"); std::fflush(stdout);
//        std::cin >> command;
//        return 0;
//    }
//    if (action == "dead-client")
//    {
//        if (argc != 3) { return 2; }
//        ProcessWorkChannel sender;
//        if (!sender.openSender(std::atoi(argv[2]))) { return 3; }
//        if (!wait([&]() { return sender.trySend("123456", 6, 1) == PROCESS_WORK_SENT; }, 2000)) { return 4; }
//        std::printf("REGISTERED\n"); std::fflush(stdout);
//        std::string command; std::cin >> command;
//        for (int32_t i = 0; i < 2; ++i)
//        {
//            if (sender.trySend("123456", 6, 2) != PROCESS_WORK_SENT) { return 5; }
//        }
//        if (sender.trySend("123456", 6, 3) != PROCESS_WORK_BUSY) { return 6; }
//        std::printf("BLOCKED\n"); std::fflush(stdout);
//        const int64_t start = now();
//        while (sender.waitForSpace() && now() - start < 5000) {}
//        const bool stopped = !sender.ready() && sender.trySend("123456", 6, 4) == PROCESS_WORK_UNAVAILABLE;
//        std::printf("RESULT dead-receiver stopped=%d elapsed_ms=%lld\n", stopped ? 1 : 0, static_cast<long long>(now() - start));
//        return stopped ? 0 : 7;
//    }
//    if (action == "mesh" || action == "mesh-growth" || action == "mesh-corrupt")
//    {
//        if (argc != 11) { return 2; }
//        const std::string batch(argv[2]);
//        const int32_t node = std::atoi(argv[3]), nodes = std::atoi(argv[4]), threads = std::atoi(argv[5]);
//        const int32_t count = std::atoi(argv[6]), slots = std::atoi(argv[7]), body = std::atoi(argv[8]);
//        const int32_t mode = std::atoi(argv[9]), rounds = std::atoi(argv[10]);
//        if (node < 0 || node >= nodes || nodes < 2 || nodes > 8 || threads < 1 || threads > 16 || count < 1 ||
//            static_cast<int64_t>(count) * rounds > 1999999 || rounds < 1 || slots < 1 || slots > 1024 ||
//            body < -1 || body > 4194304 || mode < 0 || mode > 4 || batch.size() > 20) { return 2; }
//        const int32_t total = (nodes - 1) * threads * count * rounds;
//        const int32_t sizes[] = { 0, 1, 2, 3, 5, 6, 7, 15, 16, 17, 255, 4096, 65536 };
//        std::atomic<int32_t> received(0), bad(0);
//        std::atomic<int64_t> lastReceived(0);
//        int32_t next[8][16] = {};
//        SharedMemory coordination("PTC" + batch, 8 * 3 * sizeof(int32_t), true);
//        int32_t* state = static_cast<int32_t*>(coordination.writeWithoutLock());
//        if (state == nullptr) { return 3; }
//        ProcessWorkTestReceiver receiver;
//        receiver.m_message = [&](int32_t pid, const char* data, int32_t length, MessageType type)
//        {
//            int32_t source = -1;
//            for (int32_t i = 0; i < nodes; ++i) { if (state[i] == pid) { source = i; break; } }
//            const int32_t value = static_cast<int32_t>(type), thread = value / 2000000, sequence = value % 2000000;
//            const int32_t expectedSize = sequence < 0 ? -1 : (body < 0 ? sizes[sequence % 13] : body);
//            bool valid = source >= 0 && source != node && value >= 0 && thread < threads && sequence < count * rounds &&
//                length == expectedSize && (length == 0 ? data == nullptr : data != nullptr);
//            if (valid)
//            {
//                if (next[source][thread] != sequence) { valid = false; }
//                next[source][thread] = sequence + 1;
//                if (body == 6) { valid = valid && std::memcmp(data, "123456", 6) == 0; }
//                else
//                {
//                    for (int32_t i = 0; i < length; ++i)
//                    {
//                        if (static_cast<unsigned char>(data[i]) != payloadByte(source, thread, sequence, i))
//                        { valid = false; break; }
//                    }
//                }
//            }
//            if (!valid) { ++bad; }
//            if (received.fetch_add(1) + 1 == total) { lastReceived = now(); }
//        };
//        const bool growth = action == "mesh-growth";
//        if (!receiver.initReceive((std::max)(65536, body) + 12, slots, growth ? 1 : 0, growth ? 4 : 0, 1, growth ? 4 : 0)) { std::fprintf(stderr, "mesh init failed\n"); return 3; }
//        if (mode >= 2) { receiver.initPostThread(); }
//        AtomicMath::compareAndSwap(state + node, 0, self);
//        if (!wait([&]() { for (int32_t i = 0; i < nodes; ++i) { if (AtomicMath::selfAdd(state + i, 0) <= 0) { return false; } } return true; }, 10000)) { return 4; }
//        const int64_t start = now();
//        const double cpuStart = cpu();
//        std::vector<std::shared_ptr<ProcessWorkTestTask> > tasks;
//        std::vector<uint32_t> ids;
//        for (int32_t thread = 0; thread < threads; ++thread)
//        {
//            std::shared_ptr<ProcessWorkTestTask> task(new ProcessWorkTestTask);
//            ProcessWorkTestTask* raw = task.get();
//            task->m_action = [&, thread, raw]()
//            {
//                std::vector<char> bytes((std::max)(65536, body));
//                for (int32_t sequence = 0; sequence < count * rounds && !raw->m_stop; ++sequence)
//                {
//                    if (growth && sequence != 0 && sequence % count == 0) { Semaphore pause; pause.wait(1250); }
//                    const int32_t length = body < 0 ? sizes[sequence % 13] : body;
//                    if (body == 6) { std::memcpy(bytes.data(), "123456", 6); }
//                    else { for (int32_t i = 0; i < length; ++i) { bytes[i] = static_cast<char>(payloadByte(node, thread, sequence, i)); } }
//                    // 测试设施负例：只篡改一包，控制器必须观测内容校验失败和非零退出。
//                    if (action == "mesh-corrupt" && node == 0 && thread == 0 && sequence == 0 && length > 0) { bytes[0] ^= 1; }
//                    for (int32_t target = 0; target < nodes; ++target)
//                    {
//                        if (target == node) { continue; }
//                        const MessageType type = static_cast<MessageType>(thread * 2000000 + sequence);
//                        const std::string name = "IpcNode" + std::to_string(target);
//                        if (mode == 0) { receiver.send(state[target], bytes.data(), length, type); }
//                        else if (mode == 1) { receiver.send(name, bytes.data(), length, type); }
//                        else if (mode == 2 || (mode == 4 && sequence % 2 == 0)) { receiver.post(state[target], bytes.data(), length, type); }
//                        else { receiver.post(name, bytes.data(), length, type); }
//                    }
//                    if (mode >= 2) { std::memset(bytes.data(), 99, length); }
//                }
//            };
//            const uint32_t id = CTaskThreadManager::Instance().Init();
//            CTaskThreadManager::Instance().GetThreadInterface(id)->PostTask(task);
//            tasks.push_back(task);
//            ids.push_back(id);
//        }
//        const bool finished = wait([&]() { for (size_t i = 0; i < tasks.size(); ++i) { if (!tasks[i]->m_done) { return false; } } return true; }, 120000);
//        for (size_t i = 0; i < ids.size(); ++i) { CTaskThreadManager::Instance().Uninit(ids[i]); if (tasks[i]->m_failed) { ++bad; } }
//        if (mode >= 2) { receiver.waitForPostEnd(); }
//        const int64_t sentAt = now();
//        check(finished, "mesh producer deadline");
//        AtomicMath::compareAndSwap(state + 8 + node, 0, 1);
//        check(wait([&]() { return received >= total; }, 120000), "mesh receive deadline");
//        check(received == total && bad == 0, "mesh exact totals, bytes and per-thread FIFO");
//        for (int32_t source = 0; source < nodes; ++source)
//        {
//            if (source == node) { continue; }
//            for (int32_t thread = 0; thread < threads; ++thread) { check(next[source][thread] == count * rounds, "mesh final per-thread sequence"); }
//        }
//        AtomicMath::compareAndSwap(state + 16 + node, 0, 1);
//        check(wait([&]() { for (int32_t i = 0; i < nodes; ++i) { if (AtomicMath::selfAdd(state + 16 + i, 0) != 1) { return false; } } return true; }, 10000), "mesh final barrier");
//        Semaphore quiet;
//        quiet.wait(50);
//        check(received == total && bad == 0, "mesh no late duplicates after final barrier");
//        const int64_t receivedAt = lastReceived.load();
//        const double sendSeconds = (std::max)(static_cast<int64_t>(1), sentAt - start) / 1000.0;
//        const double receiveSeconds = (std::max)(static_cast<int64_t>(1), receivedAt - start) / 1000.0;
//        std::printf("RESULT {\"action\":\"mesh\",\"node\":%d,\"mode\":%d,\"expected\":%d,\"received\":%d,\"bad\":%d,\"errors\":%d,\"send_seconds\":%.3f,\"receive_seconds\":%.3f,\"send_pps\":%.1f,\"receive_pps\":%.1f,\"cpu_seconds\":%.6f}\n",
//            node, mode, total, received.load(), bad.load(), errors, sendSeconds, receiveSeconds, total / sendSeconds, total / receiveSeconds, cpu() - cpuStart);
//        receiver.uninitReceive();
//        CTaskThreadManager::Instance().UninitAll();
//        return errors == 0 ? 0 : 1;
//    }
//    return 2;
//}
// PROCESS_WORK_V2_TESTS_END

// PROCESS_WORK_PUBLIC_TESTS_BEGIN
//#include "ProcessWork/ProcessWorkAPI.h"
//#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
//#include "CSystem/CSystemAPI.h"
//#include <atomic>
//#include <chrono>
//#include <cstdio>
//#include <cstdlib>
//#include <iostream>
//#include <memory>
//#include <vector>
//#include <string>
//
///** External consumer checks the public LIB/DLL boundary using the same message ledger. */
//class ProcessWorkPublicReceiver : public ProcessReceiveCallback
//{
//public:
//    ProcessWorkPublicReceiver(): threads(0), packets(0), fixed(-1), received(0), bad(0) {}
//    void receive(int32_t pid, const char* data, int32_t length, MessageType messageType)
//    {
//        const int32_t value = static_cast<int32_t>(messageType);
//        const int32_t stream = value / packets, sequence = value % packets;
//        const int32_t sizes[] = { 0, 1, 6, 64, 4096, 65536 };
//        const int32_t expected = fixed < 0 ? sizes[sequence % 6] : fixed;
//        if (stream < 0 || static_cast<size_t>(stream) >= next.size() ||
//            pids[stream / threads] != pid || next[stream] != sequence || length != expected ||
//            (length == 0 ? data != nullptr : data == nullptr))
//        {
//            ++bad;
//        }
//        else
//        {
//            ++next[stream];
//            for (int32_t i = 0; i < length; ++i)
//            {
//                const unsigned char reference = fixed == 6 ? static_cast<unsigned char>('1' + i) :
//                    static_cast<unsigned char>((static_cast<uint32_t>(value) * 17U + static_cast<uint32_t>(i) * 31U + static_cast<uint32_t>(i / 256)) % 251U);
//                if (static_cast<unsigned char>(data[i]) != reference) { ++bad; break; }
//            }
//        }
//        ++received;
//    }
//    int32_t threads, packets, fixed;
//    std::vector<int32_t> pids, next;
//    std::atomic<int32_t> received, bad;
//};
//
///** Real public API producer; each task owns one ordered stream to every other process. */
//class ProcessWorkPublicSender : public CTask
//{
//public:
//    ProcessWorkPublicSender(): node(0), thread(0), threads(0), packets(0), mode(0), fixed(-1), stopped(false), failed(false) {}
//    void StopTask() { stopped = true; }
//    void DoTask()
//    {
//        try
//        {
//            const int32_t sizes[] = { 0, 1, 6, 64, 4096, 65536 };
//            for (int32_t sequence = 0; sequence < packets && !stopped; ++sequence)
//            {
//                const int32_t length = fixed < 0 ? sizes[sequence % 6] : fixed;
//                const int32_t value = (node * threads + thread) * packets + sequence;
//                std::string body(length, '\0');
//                for (int32_t i = 0; i < length; ++i)
//                {
//                    body[i] = fixed == 6 ? static_cast<char>('1' + i) :
//                        static_cast<char>((static_cast<uint32_t>(value) * 17U + static_cast<uint32_t>(i) * 31U + static_cast<uint32_t>(i / 256)) % 251U);
//                }
//                for (size_t peer = 0; peer < pids.size(); ++peer)
//                {
//                    if (peer == node) { continue; }
//                    ProcessWork& ipc = ProcessWork::instance();
//                    const MessageType type = static_cast<MessageType>(value);
//                    const std::string name = batch + "_" + std::to_string(peer);
//                    if (mode == 0) { ipc.send(pids[peer], body, type); }
//                    else if (mode == 1) { ipc.send(name, body, type); }
//                    else if (mode == 2 || (mode == 4 && sequence % 2 == 0)) { ipc.post(pids[peer], body, type); }
//                    else { ipc.post(name, body, type); }
//                }
//            }
//        }
//        catch (...) { failed = true; }
//    }
//    int32_t node, thread, threads, packets, mode, fixed;
//    std::string batch;
//    std::vector<int32_t> pids;
//    std::atomic<bool> stopped, failed;
//};
//
///** Stored in ProcessWork.cpp; extracted verbatim for an external import-library consumer. */
//int main(int argc, char** argv)
//{
//    if (argc != 8 && argc != 9) { return 2; }
//    const std::string batch(argv[1]);
//    const int32_t node = std::atoi(argv[2]), nodes = std::atoi(argv[3]), threads = std::atoi(argv[4]);
//    const int32_t packets = std::atoi(argv[5]), mode = std::atoi(argv[6]), fixed = std::atoi(argv[7]);
//    if (nodes < 2 || nodes > 8 || node < 0 || node >= nodes || threads < 1 || threads > 8 || packets < 1 || packets > 1000000 || mode < 0 || mode > 4 || (fixed != -1 && fixed != 6)) { return 2; }
//    const int32_t initial = argc == 9 ? std::atoi(argv[8]) : 8;
//    if (initial != 8 && initial != 128) { return 2; }
//    ProcessWorkPublicReceiver receiver;
//    receiver.threads = threads; receiver.packets = packets; receiver.fixed = fixed;
//    receiver.pids.resize(nodes); receiver.next.resize(nodes * threads, 0);
//    ProcessWork& ipc = ProcessWork::instance();
//    ipc.addProcessReceiveCallback(&receiver);
//    if (!ipc.setReceiveName(batch + "_" + std::to_string(node)) || !ipc.initReceive(65548, initial, initial == 8 ? 2 : 0, initial == 8 ? 4 : 0, 1, initial == 8 ? 3 : 0)) { return 3; }
//    if (ipc.receivePayloadCapacity() != 65536) { ipc.uninitReceive(); return 4; }
//    if (mode >= 2) { ipc.initPostThread(); }
//    std::printf("READY %d\n", CSystem::currentProcessPid()); std::fflush(stdout);
//    for (int32_t i = 0; i < nodes; ++i)
//    {
//        if (!(std::cin >> receiver.pids[i]) || receiver.pids[i] <= 0) { ipc.uninitReceive(); return 5; }
//    }
//    // Publish the immutable ledger before any peer is allowed to send.
//    std::printf("ARMED\n"); std::fflush(stdout);
//    int32_t begin = 0;
//    if (!(std::cin >> begin) || begin != 1) { ipc.uninitReceive(); return 5; }
//    std::vector<uint32_t> ids;
//    std::vector<std::shared_ptr<ProcessWorkPublicSender> > tasks;
//    const auto start = std::chrono::steady_clock::now();
//    for (int32_t thread = 0; thread < threads; ++thread)
//    {
//        std::shared_ptr<ProcessWorkPublicSender> task(new ProcessWorkPublicSender);
//        task->batch = batch; task->node = node; task->thread = thread; task->threads = threads;
//        task->packets = packets; task->mode = mode; task->fixed = fixed; task->pids = receiver.pids;
//        const uint32_t id = CTaskThreadManager::Instance().Init();
//        std::shared_ptr<CTaskThread> worker = CTaskThreadManager::Instance().GetThreadInterface(id);
//        if (!worker) { ipc.uninitReceive(); return 6; }
//        ids.push_back(id); tasks.push_back(task); worker->PostTask(task);
//    }
//    int32_t errors = 0;
//    for (size_t i = 0; i < ids.size(); ++i)
//    {
//        CTaskThreadManager::Instance().WaitForEnd(ids[i]);
//        if (tasks[i]->failed || tasks[i]->stopped) { ++errors; }
//    }
//    if (mode >= 2) { ipc.waitForPostEnd(); }
//    const int32_t expected = (nodes - 1) * threads * packets;
//    while (receiver.received < expected && std::chrono::steady_clock::now() - start < std::chrono::seconds(60)) { CSystem::Sleep(2); }
//    const double seconds = std::chrono::duration_cast<std::chrono::duration<double> >(std::chrono::steady_clock::now() - start).count();
//    if (receiver.received != expected || receiver.bad != 0) { ++errors; }
//    // All peers signal delivery before any receiver is closed; the controller owns this barrier.
//    std::printf("DELIVERED %d %d\n", receiver.received.load(), errors); std::fflush(stdout);
//    int32_t finish = 0;
//    if (!(std::cin >> finish) || finish != 1) { ++errors; }
//    ipc.uninitReceive();
//    if (ipc.receivePayloadCapacity() != 0 || ipc.receiveBufferCount() != 0) { ++errors; }
//    std::printf("RESULT {\"received\":%d,\"expected\":%d,\"bad\":%d,\"seconds\":%.6f,\"errors\":%d}\n", receiver.received.load(), expected, receiver.bad.load(), seconds, errors);
//    return errors == 0 ? 0 : 1;
//}
// PROCESS_WORK_PUBLIC_TESTS_END
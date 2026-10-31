#include "ProcessWorkChannel.h"
#include "AtomicMath/AtomicMathAPI.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>
#include <cstring>
#include <limits.h>
#include <cstddef>
#include <utility>

static_assert(sizeof(ProcessWorkDirectory) == 24, "v2 directory layout");
static_assert(sizeof(ProcessWorkPeer) == 24, "v2 peer layout");
static_assert(sizeof(ProcessWorkControl) == 30800, "v2 control layout");
static_assert(offsetof(ProcessWorkControl, m_ring) % 8 == 0, "v2 ring alignment");

/** 取得本进程单调毫秒。
@return 不受墙上时钟调整影响的时间
*/
static uint64_t ChannelNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

/** 通过已验证跨进程原语读取64位值。
@param [in] value 8字节对齐共享字段
@return 原子快照
*/
static int64_t ChannelLoad64(int64_t* value)
{
    return AtomicMath::selfAdd(value, static_cast<int64_t>(0));
}

/** 通过跨进程原语读取32位值。
@param [in] value 对齐共享字段
@return 原子快照
*/
static int32_t ChannelLoad32(int32_t* value)
{
    return AtomicMath::selfAdd(value, 0);
}

ProcessWorkChannel::ProcessWorkChannel():
    m_receiver(false), m_stopped(false), m_pid(0), m_identity(0), m_selfIdentity(0), m_session(0),
    m_selfPid(0), m_peerIndex(-1), m_directory(nullptr), m_control(nullptr),
    m_nextMaintenance(0), m_peerCursor(0), m_cursor(0), m_retryDelay(false)
{
}

ProcessWorkChannel::~ProcessWorkChannel()
{
    close();
}

std::string ProcessWorkChannel::objectName(char kind, uint32_t pid, uint64_t identity,
    uint32_t session, uint32_t index, uint32_t generation)
{
    // 20字节固定线格式，base64url无填充为27字符，加P2及种类恰好30。
    unsigned char bytes[20] = {};
    for (int32_t i = 0; i < 4; ++i)
    {
        bytes[i] = static_cast<unsigned char>(pid >> (i * 8));
    }
    for (int32_t i = 0; i < 8; ++i)
    {
        bytes[4 + i] = static_cast<unsigned char>(identity >> (i * 8));
    }
    for (int32_t i = 0; i < 3; ++i)
    {
        bytes[12 + i] = static_cast<unsigned char>(session >> (i * 8));
    }
    bytes[15] = static_cast<unsigned char>(index);
    bytes[16] = static_cast<unsigned char>(index >> 8);
    for (int32_t i = 0; i < 3; ++i)
    {
        bytes[17 + i] = static_cast<unsigned char>(generation >> (i * 8));
    }
    const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";
    // Darwin sem_open把末尾NUL计入31字节限制；Semaphore还会添加一个前导斜杠。
    // 通知名因此只用2字节前缀，保留完整27字节编码；共享内存名保持原协议。
#if defined(__APPLE__) && !TARGET_OS_IPHONE
    std::string result(kind == 'R' || kind == 'W' ? "P" : "P2");
#else
    std::string result("P2");
#endif
    result += kind;
    uint32_t pending = 0;
    int32_t bits = 0;
    for (int32_t i = 0; i < 20; ++i)
    {
        pending = (pending << 8) | bytes[i];
        bits += 8;
        while (bits >= 6)
        {
            bits -= 6;
            result += alphabet[(pending >> bits) & 63];
        }
    }
    if (bits != 0)
    {
        result += alphabet[(pending << (6 - bits)) & 63];
    }
    return result;
}

bool ProcessWorkChannel::openReceiver(int32_t bytes, int32_t initial, int32_t threshold,
    int32_t increment, int32_t idleSeconds, int32_t rounds)
{
    if (m_control != nullptr || bytes < 12 || bytes == INT32_MAX || initial < 1 || initial > 1024 ||
        threshold < 0 || increment < 0 || idleSeconds < 0 || rounds < 0 ||
        static_cast<int64_t>(initial) + static_cast<int64_t>(increment) * rounds > 1024 ||
        ((increment == 0) != (rounds == 0)))
    {
        return false;
    }
    try
    {
        std::atomic<int64_t> atomicProbe(0);
        if (!atomicProbe.is_lock_free())
        {
            return false;
        }
        m_pid = CSystem::currentProcessPid();
        if (!CSystem::processIdentity(m_pid, m_identity))
        {
            return false;
        }
        m_directoryMemory.reset(new SharedMemory("P2D" + std::to_string(m_pid), sizeof(ProcessWorkDirectory), true));
        m_directoryMemory->preserveName();
        m_directory = static_cast<ProcessWorkDirectory*>(m_directoryMemory->writeWithoutLock());
        if (m_directory == nullptr || m_directoryMemory->realSize() < sizeof(ProcessWorkDirectory))
        {
            close();
            return false;
        }
        int64_t* directoryIdentity = reinterpret_cast<int64_t*>(&m_directory->m_identity);
        const int64_t previous = ChannelLoad64(&m_directory->m_session);
        const int64_t previousIdentity = ChannelLoad64(directoryIdentity);
        if (static_cast<uint64_t>(previousIdentity) != m_identity)
        {
            // 先发布本进程身份再争用准备标志，禁止两个初始化者把尚未写身份的-1同时认领。
            if (!AtomicMath::compareAndSwap(directoryIdentity, previousIdentity, static_cast<int64_t>(m_identity)))
            {
                close();
                return false;
            }
            if (previous != 0)
            {
                AtomicMath::compareAndSwap(&m_directory->m_session, previous, static_cast<int64_t>(0));
            }
        }
        if (!AtomicMath::compareAndSwap(&m_directory->m_session, static_cast<int64_t>(0), static_cast<int64_t>(-1)))
        {
            close();
            return false;
        }
        m_receiver = true;
        if (ChannelLoad64(&m_directory->m_nextSession) < 0 || ChannelLoad64(&m_directory->m_nextSession) >= 0xffffff)
        {
            close();
            return false;
        }
        m_session = AtomicMath::selfAddOne(&m_directory->m_nextSession);
        if (m_session <= 0 || m_session > 0xffffff)
        {
            close();
            return false;
        }
        m_controlMemory.reset(new SharedMemory(objectName('C', m_pid, m_identity, static_cast<uint32_t>(m_session)),
            sizeof(ProcessWorkControl), false));
        m_control = static_cast<ProcessWorkControl*>(m_controlMemory->writeWithoutLock());
        if (m_control == nullptr || m_controlMemory->isFailed() || m_controlMemory->realSize() < sizeof(ProcessWorkControl))
        {
            close();
            return false;
        }
        std::memset(m_control, 0, sizeof(ProcessWorkControl));
        m_control->m_version = 2;
        m_control->m_bytes = sizeof(ProcessWorkControl);
        m_control->m_blockBytes = bytes;
        m_control->m_initial = initial;
        m_control->m_limit = initial + increment * rounds;
        m_control->m_threshold = threshold;
        m_control->m_increment = increment;
        m_control->m_idleSeconds = idleSeconds;
        m_memories.resize(m_control->m_limit);
        m_idleSince.resize(m_control->m_limit, ChannelNow());
        for (int32_t i = 0; i < m_control->m_limit; ++i)
        {
            m_control->m_blocks[i].m_owner = -1;
            m_control->m_ring[i] = static_cast<int64_t>(i) << 11;
        }
        m_dataReady.createProcessSemaphore(objectName('R', m_pid, m_identity, static_cast<uint32_t>(m_session)));
        m_spaceReady.createProcessSemaphore(objectName('W', m_pid, m_identity, static_cast<uint32_t>(m_session)));
        if (m_dataReady.tryProcessSignal() != 0 || !m_dataReady.processWait(0) ||
            m_spaceReady.tryProcessSignal() != 0 || !m_spaceReady.processWait(0))
        {
            close();
            return false;
        }
        for (int32_t i = 0; i < initial; ++i)
        {
            if (!createBlock(i))
            {
                close();
                return false;
            }
        }
        return true;
    }
    catch (...)
    {
        close();
        return false;
    }
}

bool ProcessWorkChannel::createBlock(int32_t index)
{
    ProcessWorkBlock& block = m_control->m_blocks[index];
    if (ChannelLoad64(&block.m_owner) != -1 || block.m_generation >= 0xffffff)
    {
        return false;
    }
    const int64_t generation = block.m_generation + 1;
    std::unique_ptr<SharedMemory> memory(new SharedMemory(objectName('B', m_pid, m_identity,
        static_cast<uint32_t>(m_session), index, static_cast<uint32_t>(generation)), m_control->m_blockBytes, false));
    if (memory->isFailed() || memory->writeWithoutLock() == nullptr ||
        memory->realSize() < static_cast<uint32_t>(m_control->m_blockBytes))
        {
            return false;
        }
    m_memories[index] = std::move(memory);
    block.m_generation = generation;
    m_idleSince[index] = ChannelNow();
    AtomicMath::selfAddOne(&m_control->m_active);
    AtomicMath::compareAndSwap(&block.m_owner, static_cast<int64_t>(-1), static_cast<int64_t>(0));
    signalSpace();
    return true;
}

void ProcessWorkChannel::publish()
{
    if (m_receiver && !m_stopped && m_control != nullptr)
    {
        AtomicMath::compareAndSwap(&m_control->m_ready, 0, 1);
        AtomicMath::compareAndSwap(&m_directory->m_session, static_cast<int64_t>(-1), m_session);
    }
}

void ProcessWorkChannel::stop()
{
    if (!m_receiver)
    {
        return;
    }
    m_stopped = true;
    if (m_control != nullptr)
    {
        AtomicMath::compareAndSwap(&m_control->m_ready, 1, 0);
    }
    signalData();
    signalSpace();
}

void ProcessWorkChannel::close()
{
    stop();
    m_memories.clear();
    m_idleSince.clear();
    m_control = nullptr;
    m_controlMemory.reset();
    m_dataReady.closeProcessSemaphore();
    m_spaceReady.closeProcessSemaphore();
    if (m_receiver && m_directory != nullptr)
    {
        // POSIX先撤入口，旧映射保留占用标志，禁止迟到的打开者在已撤名对象上发布。
        // Windows依靠内核引用管理名称，原入口保留代次计数并解除占用。
        if (!m_directoryMemory->unlinkName())
        {
            // 只释放一次；发布0后新实例可能立即认领-1，旧实例不能再次清除它。
            const int64_t ownedSession = ChannelLoad64(&m_directory->m_session);
            if (ownedSession == m_session || ownedSession == -1)
            {
                AtomicMath::compareAndSwap(&m_directory->m_session, ownedSession, static_cast<int64_t>(0));
            }
        }
    }
    m_directory = nullptr;
    m_directoryMemory.reset();
    m_receiver = false;
    m_stopped = false;
    m_session = 0;
    m_peerIndex = -1;
}

bool ProcessWorkChannel::openSender(int32_t pid)
{
    if (pid <= 0 || m_control != nullptr)
    {
        return false;
    }
    try
    {
        m_pid = pid;
        m_selfPid = CSystem::currentProcessPid();
        if (!CSystem::processIdentity(pid, m_identity) || !CSystem::processIdentity(m_selfPid, m_selfIdentity))
        {
            return false;
        }
        m_directoryMemory.reset(new SharedMemory("P2D" + std::to_string(pid)));
        m_directory = static_cast<ProcessWorkDirectory*>(m_directoryMemory->writeWithoutLock());
        if (m_directory == nullptr || m_directoryMemory->realSize() < sizeof(ProcessWorkDirectory))
        {
            close(); return false;
        }
        m_session = ChannelLoad64(&m_directory->m_session);
        if (m_session <= 0 || m_session > 0xffffff || static_cast<uint64_t>(ChannelLoad64(reinterpret_cast<int64_t*>(&m_directory->m_identity))) != m_identity)
        {
            close(); return false;
        }
        m_controlMemory.reset(new SharedMemory(objectName('C', pid, m_identity, static_cast<uint32_t>(m_session))));
        m_control = static_cast<ProcessWorkControl*>(m_controlMemory->writeWithoutLock());
        if (m_control == nullptr || m_controlMemory->realSize() < sizeof(ProcessWorkControl) || !ready() ||
            m_control->m_version != 2 || m_control->m_bytes != sizeof(ProcessWorkControl) ||
            m_control->m_initial < 1 || m_control->m_limit < m_control->m_initial || m_control->m_limit > 1024 ||
            m_control->m_blockBytes < 12 || m_control->m_blockBytes == INT32_MAX)
        {
            close(); return false;
        }
        if (!m_dataReady.tryOpenProcessSemaphore(objectName('R', pid, m_identity, static_cast<uint32_t>(m_session))) ||
            !m_spaceReady.tryOpenProcessSemaphore(objectName('W', pid, m_identity, static_cast<uint32_t>(m_session))))
        {
            close(); return false;
        }
        m_memories.resize(m_control->m_initial);
        for (int32_t i = 0; i < m_control->m_initial; ++i)
        {
            std::unique_ptr<SharedMemory> memory(new SharedMemory(objectName('B', pid, m_identity,
                static_cast<uint32_t>(m_session), i, 1)));
            if (memory->writeWithoutLock() == nullptr || memory->realSize() < static_cast<uint32_t>(m_control->m_blockBytes))
            {
                close(); return false;
            }
            m_memories[i] = std::move(memory);
        }
        // 登记只在open阶段选择下标，发送热路径不修改本地共享指针或容器。
        for (int32_t pass = 0; pass < 2 && m_peerIndex < 0; ++pass)
        {
            for (int32_t i = 0; i < 256; ++i)
            {
                ProcessWorkPeer& peer = m_control->m_peers[i];
                // 申请轮也复查同PID：另一个本进程通道可能刚在此处赢得CAS。
                if (pass == 1)
                {
                    AtomicMath::compareAndSwap(&peer.m_pid, 0, m_selfPid);
                }
                if (ChannelLoad32(&peer.m_pid) == m_selfPid &&
                    (ChannelLoad64(&peer.m_token) == 0 || static_cast<uint64_t>(ChannelLoad64(reinterpret_cast<int64_t*>(&peer.m_identity))) == m_selfIdentity) &&
                    ChannelLoad32(&peer.m_pid) == m_selfPid)
                {
                    m_peerIndex = i;
                    break;
                }
            }
        }
        if (m_peerIndex < 0)
        {
            close();
            return false;
        }
        signalData();
        if (!ready())
        {
            // 打开末尾与接收器停止交错时，失败对象也必须回滚，允许原对象重试。
            close();
            return false;
        }
        return true;
    }
    catch (...)
    {
        close();
        return false;
    }
}

bool ProcessWorkChannel::ready() const
{
    return m_directory != nullptr && m_control != nullptr && ChannelLoad32(&m_control->m_ready) == 1 &&
        ChannelLoad64(&m_directory->m_session) == m_session && static_cast<uint64_t>(ChannelLoad64(reinterpret_cast<int64_t*>(&m_directory->m_identity))) == m_identity;
}

int32_t ProcessWorkChannel::payloadCapacity() const
{
    const int32_t bytes = ready() ? m_control->m_blockBytes : 0;
    return bytes >= 12 && bytes < INT32_MAX ? bytes - 12 : 0;
}

bool ProcessWorkChannel::matchesIdentity(uint64_t identity) const
{
    return identity == m_identity && ready();
}

int64_t ProcessWorkChannel::senderToken()
{
    if (m_peerIndex < 0)
    {
        return 0;
    }
    ProcessWorkPeer& peer = m_control->m_peers[m_peerIndex];
    const int64_t token = ChannelLoad64(&peer.m_token);
    return token > 0 && static_cast<uint64_t>(ChannelLoad64(reinterpret_cast<int64_t*>(&peer.m_identity))) == m_selfIdentity &&
        ChannelLoad32(&peer.m_pid) == m_selfPid ? token : 0;
}

int32_t ProcessWorkChannel::trySend(const char* data, int32_t length, int32_t type)
{
    if (length < 0 || (length != 0 && data == nullptr) || type < 0)
    {
        return PROCESS_WORK_INVALID;
    }
    if (m_receiver || !ready())
    {
        return PROCESS_WORK_UNAVAILABLE;
    }
    if (length > m_control->m_blockBytes - 12)
    {
        return PROCESS_WORK_TOO_LARGE;
    }
    int64_t token = senderToken();
    if (token == 0)
    {
        if (ChannelLoad64(&m_control->m_nextToken) == INT64_MAX)
        {
            // 最后一个token先发布再标记耗尽，复查避免错拒刚获准的最后一名发送者。
            token = senderToken();
            if (token == 0)
            {
                return PROCESS_WORK_UNAVAILABLE;
            }
        }
        else
        {
            signalData();
            return PROCESS_WORK_BUSY;
        }
    }
    const int32_t limit = m_control->m_limit;
    const uint32_t start = m_cursor.fetch_add(1, std::memory_order_relaxed);
    int32_t index = -1;
    for (int32_t step = 0; step < limit; ++step)
    {
        const int32_t candidate = static_cast<int32_t>((start % limit + static_cast<uint32_t>(step)) % limit);
        if (AtomicMath::compareAndSwap(&m_control->m_blocks[candidate].m_owner, static_cast<int64_t>(0), token))
        {
            index = candidate;
            break;
        }
    }
    if (index < 0)
    {
        AtomicMath::compareAndSwap(&m_control->m_full, 0, 1);
        AtomicMath::compareAndSwap(&m_control->m_demand, 0, 1);
        signalData();
        return PROCESS_WORK_BUSY;
    }
    // 只撤销本次未提交占位；提交成功后release禁止析构回收已交付数据。
    auto releaseBlock = [this, token](int32_t* reserved)
    {
        AtomicMath::compareAndSwap(&m_control->m_blocks[*reserved].m_owner, token, static_cast<int64_t>(0));
        signalSpace();
    };
    std::unique_ptr<int32_t, decltype(releaseBlock)> reservation(&index, releaseBlock);
    try
    {
        // 通道持有固定块；调用方持有通道，借用期间映射不会销毁。
        // 扩容块只在本次占位期间独占映射，不增加共享引用计数。
        std::unique_ptr<SharedMemory> expanded;
        SharedMemory* memory = nullptr;
        if (index < m_control->m_initial)
        {
            memory = m_memories[index].get();
        }
        else
        {
            // 扩容映射不进入发送缓存，缩容后即使发送进程长期空闲也不会保留旧映射。
            expanded.reset(new SharedMemory(objectName('B', m_pid, m_identity, static_cast<uint32_t>(m_session),
                index, static_cast<uint32_t>(m_control->m_blocks[index].m_generation))));
            memory = expanded.get();
            if (memory->writeWithoutLock() == nullptr || memory->realSize() < static_cast<uint32_t>(m_control->m_blockBytes))
            {
                return PROCESS_WORK_UNAVAILABLE;
            }
        }
        char* target = static_cast<char*>(memory->memory());
        if (target == nullptr || !ready())
        {
            return PROCESS_WORK_UNAVAILABLE;
        }
        const int32_t header[3] = { m_selfPid, type, length };
        std::memcpy(target, header, sizeof(header));
        if (length != 0)
        {
            std::memcpy(target + 12, data, length);
        }
        // 唯一提交点是包含票号与块号的64位CAS。之前被kill没有环洞，之后被kill正文已完整。
        for (int32_t attempt = 0; attempt <= limit; ++attempt)
        {
            if (!ready())
            {
                return PROCESS_WORK_UNAVAILABLE;
            }
            const int64_t ticket = ChannelLoad64(&m_control->m_tail);
            if (ticket < 0 || ticket > (INT64_MAX >> 11) - limit)
            {
                return PROCESS_WORK_UNAVAILABLE;
            }
            int64_t* cell = &m_control->m_ring[ticket % limit];
            const int64_t empty = ticket << 11;
            const int64_t current = ChannelLoad64(cell);
            if (current == empty && AtomicMath::compareAndSwap(cell, empty, empty | (index + 1)))
            {
                reservation.release();
                AtomicMath::compareAndSwap(&m_control->m_tail, ticket, ticket + 1);
                AtomicMath::compareAndSwap(&m_control->m_demand, 0, 1);
                signalData();
                return PROCESS_WORK_SENT;
            }
            if ((current >> 11) == ticket && (current & 2047) != 0)
            {
                AtomicMath::compareAndSwap(&m_control->m_tail, ticket, ticket + 1);
                continue;
            }
            if (current != empty)
            {
                break;
            }
        }
        return PROCESS_WORK_BUSY;
    }
    catch (...)
    {
        m_retryDelay = true;
        return PROCESS_WORK_BUSY;
    }
}

bool ProcessWorkChannel::waitForSpace()
{
    if (!ready())
    {
        return false;
    }
    if (m_retryDelay.exchange(false))
    {
        // 有空槽但本地分配持续失败时，不能跳过等待而成为忙循环。
        CSystem::Sleep(1);
    }
    AtomicMath::compareAndSwap(&m_control->m_spaceSignal, 0, 1);
    bool available = false;
    if (senderToken() > 0)
    {
        for (int32_t i = 0; i < m_control->m_limit; ++i)
        {
            if (ChannelLoad64(&m_control->m_blocks[i].m_owner) == 0)
            {
                available = true;
                break;
            }
        }
    }
    if (!available)
    {
        m_spaceReady.processWait(20);
    }
    uint64_t current = 0;
    const bool known = CSystem::processIdentity(m_pid, current);
    if ((known && current != m_identity) || CSystem::processDefinitelyExited(m_pid))
    {
        AtomicMath::compareAndSwap(&m_control->m_ready, 1, 0);
        try
        {
            cleanupExitedReceiver();
        }
        catch (...)
        {
            /* 撤名失败不影响退出等待。 */
        }
        return false;
    }
    return ready();
}

void ProcessWorkChannel::cleanupExitedReceiver()
{
#ifndef _WIN32
    // 只含创建标识和实例号的名称不会属于PID复用后的新接收器。
    // 不解除任何缓存映射，也不撤销可被新进程重新发布的P2D/P2N固定入口。
    if (m_control == nullptr || m_receiver)
    {
        return;
    }
    for (int32_t i = 0; i < m_control->m_limit; ++i)
    {
        const int64_t generation = ChannelLoad64(&m_control->m_blocks[i].m_generation);
        if (generation <= 0 || generation > 0xffffff)
        {
            continue;
        }
        SharedMemory memory(objectName('B', m_pid, m_identity, static_cast<uint32_t>(m_session),
            i, static_cast<uint32_t>(generation)));
        memory.unlinkName();
    }
    // 多个发送线程可能同时到达；仅以独立临时句柄撤名，不修改共享缓存对象成员。
    SharedMemory control(objectName('C', m_pid, m_identity, static_cast<uint32_t>(m_session)));
    control.unlinkName();
    Semaphore data;
    if (data.tryOpenProcessSemaphore(objectName('R', m_pid, m_identity, static_cast<uint32_t>(m_session))))
    {
        data.unlinkProcessName();
    }
    Semaphore space;
    if (space.tryOpenProcessSemaphore(objectName('W', m_pid, m_identity, static_cast<uint32_t>(m_session))))
    {
        space.unlinkProcessName();
    }
#endif
}

void ProcessWorkChannel::signalData()
{
    if (m_control != nullptr && AtomicMath::compareAndSwap(&m_control->m_dataSignal, 0, 1))
    {
        m_dataReady.tryProcessSignal();
    }
}

void ProcessWorkChannel::signalSpace()
{
    if (m_control != nullptr && AtomicMath::compareAndSwap(&m_control->m_spaceSignal, 1, 0))
    {
        m_spaceReady.tryProcessSignal();
    }
}

void ProcessWorkChannel::wake()
{
    m_dataReady.tryProcessSignal();
}

void ProcessWorkChannel::waitForData()
{
    if (m_control != nullptr)
    {
        AtomicMath::compareAndSwap(&m_control->m_dataSignal, 1, 0);
    }
    m_dataReady.processWait(20);
}

int32_t ProcessWorkChannel::bufferCount() const
{
    return m_control == nullptr ? 0 : ChannelLoad32(&m_control->m_active);
}

int32_t ProcessWorkChannel::pump(LockFreeQueue<char*>& queue, Semaphore& notification, std::atomic<int32_t>& fullCount)
{
    if (!m_receiver || m_control == nullptr)
    {
        return 0;
    }
    const uint64_t now = ChannelNow();
    if (now >= m_nextMaintenance)
    {
        maintain(notification, fullCount);
        m_nextMaintenance = now + 20;
    }
    int32_t copied = 0;
    for (int32_t step = 0; step < m_control->m_limit; ++step)
    {
        const int64_t ticket = m_control->m_head;
        int64_t* cell = &m_control->m_ring[ticket % m_control->m_limit];
        const int64_t entry = ChannelLoad64(cell);
        if ((entry >> 11) != ticket || (entry & 2047) == 0)
        {
            break;
        }
        const int32_t index = static_cast<int32_t>(entry & 2047) - 1;
        if (index < 0 || index >= m_control->m_limit || !m_memories[index])
        {
            break;
        }
        ProcessWorkBlock& block = m_control->m_blocks[index];
        const int64_t owner = ChannelLoad64(&block.m_owner);
        if (owner > 0)
        {
            AtomicMath::compareAndSwap(&block.m_owner, owner, static_cast<int64_t>(-2));
        }
        if (ChannelLoad64(&block.m_owner) != -2)
        {
            break;
        }
        char* source = static_cast<char*>(m_memories[index]->memory());
        int32_t header[3] = {};
        std::memcpy(header, source, sizeof(header));
        // 正常发送者已校验；损坏头不会导致越界分配或复制，也不影响后续合法槽。
        if (header[0] > 0 && header[1] >= 0 && header[2] >= 0 && header[2] <= m_control->m_blockBytes - 12)
        {
            try
            {
                std::unique_ptr<char[]> packet(new char[static_cast<size_t>(header[2]) + 13]);
                std::memcpy(packet.get(), header, sizeof(header));
                if (header[2] != 0)
                {
                    std::memcpy(packet.get() + 12, source + 12, header[2]);
                }
                packet[static_cast<size_t>(header[2]) + 12] = '\0';
                queue.push(packet.get());
                packet.release();
                try
                {
                    notification.signal();
                }
                catch (...)
                {
                    /* 接收任务有限等待补查队列。 */
                }
            }
            catch (...)
            {
                // 内存或本地队列暂时失败：保留当前头槽和正文，后续包不得越过。
                break;
            }
        }
        // 帮助完成生产游标后才回收环槽，否则提交后退出的发送者会留下旧尾。
        AtomicMath::compareAndSwap(&m_control->m_tail, ticket, ticket + 1);
        AtomicMath::compareAndSwap(cell, entry, (ticket + m_control->m_limit) << 11);
        m_control->m_head = ticket + 1;
        // 刚归还尚未开始空闲计时；由下轮维护取完成之后的时间，不把复制耗时计入空闲。
        m_idleSince[index] = 0;
        AtomicMath::compareAndSwap(&block.m_owner, static_cast<int64_t>(-2), static_cast<int64_t>(0));
        signalSpace();
        ++copied;
    }
    return copied;
}

void ProcessWorkChannel::maintain(Semaphore& notification, std::atomic<int32_t>& fullCount)
{
    approveSenders();
    recoverExitedSenders();
    resizeBuffers(ChannelNow(), notification, fullCount);
}

void ProcessWorkChannel::approveSenders()
{
    for (int32_t i = 0; i < 256; ++i)
    {
        ProcessWorkPeer& peer = m_control->m_peers[i];
        const int32_t pid = ChannelLoad32(&peer.m_pid);
        if (pid <= 0 || ChannelLoad64(&peer.m_token) != 0)
        {
            continue;
        }
        uint64_t identity = 0;
        const int64_t previousToken = ChannelLoad64(&m_control->m_nextToken);
        if (CSystem::processIdentity(pid, identity) && previousToken < INT64_MAX)
        {
            int64_t* publishedIdentity = reinterpret_cast<int64_t*>(&peer.m_identity);
            AtomicMath::compareAndSwap(publishedIdentity, ChannelLoad64(publishedIdentity), static_cast<int64_t>(identity));
            AtomicMath::compareAndSwap(&peer.m_token, static_cast<int64_t>(0), previousToken + 1);
            // 唯一登记写者；先批准token再发布计数，耗尽状态不能早于最后一次批准。
            AtomicMath::compareAndSwap(&m_control->m_nextToken, previousToken, previousToken + 1);
            signalSpace();
        }
        else if (CSystem::processDefinitelyExited(pid))
        {
            AtomicMath::compareAndSwap(&peer.m_pid, pid, 0);
        }
    }
}

void ProcessWorkChannel::recoverExitedSenders()
{
    // 系统存活查询分摊到每轮，避免一次扫描256个进程阻塞收件。
    for (int32_t step = 0; step < 16; ++step)
    {
        ProcessWorkPeer& peer = m_control->m_peers[m_peerCursor];
        m_peerCursor = (m_peerCursor + 1) % 256;
        const int32_t pid = ChannelLoad32(&peer.m_pid);
        const int64_t token = ChannelLoad64(&peer.m_token);
        if (pid <= 0 || token <= 0)
        {
            continue;
        }
        uint64_t identity = 0;
        const bool known = CSystem::processIdentity(pid, identity);
        const bool exited = CSystem::processDefinitelyExited(pid);
        if (!exited && (!known || identity == static_cast<uint64_t>(ChannelLoad64(reinterpret_cast<int64_t*>(&peer.m_identity)))))
        {
            continue;
        }
        // 确认此进程已死亡之后才能取快照；活进程暂停不能成为回收理由。
        // 单一接收任务尚未消费这些槽，死进程也不会再发布新槽。
        bool committed[1024] = {};
        for (int32_t i = 0; i < m_control->m_limit; ++i)
        {
            const int32_t block = static_cast<int32_t>(ChannelLoad64(&m_control->m_ring[i]) & 2047) - 1;
            if (block >= 0 && block < m_control->m_limit)
            {
                committed[block] = true;
            }
        }
        bool referenced = false;
        for (int32_t i = 0; i < m_control->m_limit; ++i)
        {
            if (ChannelLoad64(&m_control->m_blocks[i].m_owner) != token)
            {
                continue;
            }
            if (committed[i])
            {
                referenced = true;
                continue;
            }
            if (AtomicMath::compareAndSwap(&m_control->m_blocks[i].m_owner, token, static_cast<int64_t>(0)))
            {
                m_idleSince[i] = 0;
                signalSpace();
            }
        }
        if (!referenced)
        {
            // 先隐藏旧登记，防止PID复用的新发送者绑定正在撤销的表项。
            AtomicMath::compareAndSwap(&peer.m_pid, pid, -pid);
            AtomicMath::compareAndSwap(&peer.m_token, token, static_cast<int64_t>(0));
            AtomicMath::compareAndSwap(&peer.m_pid, -pid, 0);
        }
    }
}

void ProcessWorkChannel::resizeBuffers(uint64_t now, Semaphore& notification, std::atomic<int32_t>& fullCount)
{
    // 先取走本轮请求再扫描空闲：扫描期间的新请求保留给下一轮，不能用旧快照消费它。
    const bool demand = AtomicMath::compareAndSwap(&m_control->m_demand, 1, 0);
    int32_t free = 0;
    for (int32_t i = 0; i < m_control->m_limit; ++i)
    {
        if (ChannelLoad64(&m_control->m_blocks[i].m_owner) == 0)
        {
            ++free;
            if (m_idleSince[i] == 0)
            {
                m_idleSince[i] = now;
            }
        }
        else
        {
            m_idleSince[i] = now;
        }
    }
    if (AtomicMath::compareAndSwap(&m_control->m_full, 1, 2))
    {
        if (fullCount.exchange(bufferCount()) == 0)
        {
            notification.signal();
        }
    }
    if (demand && free <= m_control->m_threshold && m_control->m_increment > 0)
    {
        int32_t added = 0;
        for (int32_t i = m_control->m_initial; i < m_control->m_limit && added < m_control->m_increment; ++i)
        {
            if (ChannelLoad64(&m_control->m_blocks[i].m_owner) == -1)
            {
                try
                {
                    if (createBlock(i))
                    {
                        ++added;
                        ++free;
                    }
                }
                catch (...)
                {
                    break;
                }
            }
        }
    }
    if (free > 0)
    {
        AtomicMath::compareAndSwap(&m_control->m_full, 2, 0);
    }
    if (m_control->m_idleSeconds == 0)
    {
        return;
    }
    for (int32_t i = m_control->m_initial; i < m_control->m_limit; ++i)
    {
        if (now < m_idleSince[i] || now - m_idleSince[i] < static_cast<uint64_t>(m_control->m_idleSeconds) * 1000)
        {
            continue;
        }
        if (AtomicMath::compareAndSwap(&m_control->m_blocks[i].m_owner, static_cast<int64_t>(0), static_cast<int64_t>(-1)))
        {
            m_memories[i].reset();
            AtomicMath::selfSubOne(&m_control->m_active);
        }
    }
}
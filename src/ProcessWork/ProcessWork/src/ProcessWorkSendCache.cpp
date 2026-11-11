#include "ProcessWorkSendCache.h"
#include "ProcessWorkChannel.h"
#include <limits>
#include <new>
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
#include <Windows.h>
#include <intrin.h>
#endif

static std::atomic<uint64_t> s_cacheIdentity(0);
static std::atomic<uint64_t> s_cacheThread(0);
#ifdef _MSC_VER
static __declspec(thread) uint64_t s_cacheCurrentThread = 0;
static __declspec(thread) uint64_t s_cacheCurrentIdentity = 0;
static __declspec(thread) ProcessWorkSendCacheRecord* s_cacheCurrentRecord = nullptr;
#else
static thread_local uint64_t s_cacheCurrentThread = 0;
static thread_local uint64_t s_cacheCurrentIdentity = 0;
static thread_local ProcessWorkSendCacheRecord* s_cacheCurrentRecord = nullptr;
#endif

/** 取得不会回绕的进程内身份，耗尽返回0并关闭相应快路径
@param [in,out] counter 单调身份计数
@return 新身份或0
*/
static uint64_t SendCacheIdentity(std::atomic<uint64_t>& counter)
{
    uint64_t value = counter.load(std::memory_order_relaxed);
    while (value != (std::numeric_limits<uint64_t>::max)())
    {
        if (counter.compare_exchange_weak(value, value + 1, std::memory_order_relaxed))
        {
            return value + 1;
        }
    }
    return 0;
}

ProcessWorkSendCacheNode::ProcessWorkSendCacheNode(int32_t pid, uint64_t epoch,
    const std::shared_ptr<ProcessWorkChannel>& channel) :
m_pid(pid), m_epoch(epoch), m_channel(channel), m_next(nullptr)
{

}

ProcessWorkSendCacheRecord::ProcessWorkSendCacheRecord() :
m_thread(0), m_pointer(nullptr)
{

}

ProcessWorkSendCacheLease::ProcessWorkSendCacheLease() :
m_cache(nullptr), m_record(nullptr)
{

}

ProcessWorkSendCacheLease::~ProcessWorkSendCacheLease()
{
    reset();
}

void ProcessWorkSendCacheLease::reset()
{
    if (m_record != nullptr)
    {
        m_record->m_pointer.store(nullptr, std::memory_order_release);
        m_cache->collect();
        m_record = nullptr;
        m_cache = nullptr;
    }
}

ProcessWorkSendCache::ProcessWorkSendCache() :
m_identity(SendCacheIdentity(s_cacheIdentity)), m_epoch(1), m_retired(nullptr)
{
    for (size_t i = 0; i < 64; ++i)
    {
        m_slots[i].store(nullptr, std::memory_order_relaxed);
    }
}

ProcessWorkSendCache::~ProcessWorkSendCache()
{
    clear();
    collect();
}

uint64_t ProcessWorkSendCache::epoch() const
{
    return m_identity == 0 ? 0 : m_epoch.load(std::memory_order_acquire);
}

ProcessWorkSendCacheRecord* ProcessWorkSendCache::record()
{
    if (s_cacheCurrentIdentity == m_identity && m_identity != 0)
    {
        return s_cacheCurrentRecord != nullptr && s_cacheCurrentRecord->m_pointer.load(std::memory_order_relaxed) == nullptr ?
            s_cacheCurrentRecord : nullptr;
    }
    if (s_cacheCurrentThread == 0)
    {
        s_cacheCurrentThread = SendCacheIdentity(s_cacheThread);
    }
    if (s_cacheCurrentThread == 0 || m_identity == 0)
    {
        return nullptr;
    }
    ProcessWorkSendCacheRecord* found = nullptr;
    for (size_t i = 0; i < 128; ++i)
    {
        if (m_records[i].m_thread.load(std::memory_order_relaxed) == s_cacheCurrentThread)
        {
            found = &m_records[i];
            break;
        }
    }
    for (size_t i = 0; found == nullptr && i < 128; ++i)
    {
        uint64_t empty = 0;
        if (m_records[i].m_thread.compare_exchange_strong(empty, s_cacheCurrentThread, std::memory_order_relaxed))
        {
            found = &m_records[i];
        }
    }
    s_cacheCurrentIdentity = m_identity;
    s_cacheCurrentRecord = found;
    return found != nullptr && found->m_pointer.load(std::memory_order_relaxed) == nullptr ? found : nullptr;
}

ProcessWorkChannel* ProcessWorkSendCache::find(int32_t pid, ProcessWorkSendCacheLease& lease)
{
    if (pid <= 0 || lease.m_record != nullptr || epoch() == 0)
    {
        return nullptr;
    }
    const size_t index = static_cast<uint32_t>(pid) & 63;
    ProcessWorkSendCacheNode* node = m_slots[index].load(std::memory_order_seq_cst);
    if (node == nullptr)
    {
        return nullptr;
    }
    ProcessWorkSendCacheRecord* protection = record();
    if (protection == nullptr)
    {
        return nullptr;
    }
    // Windows把昂贵的进程写屏障移到少见的回收端；编译器不能把来源复查移到保护发布之前。
    // 其他平台使用标准的顺序一致保护发布，不依赖未实现的系统屏障。
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    protection->m_pointer.store(node, std::memory_order_release);
    _ReadWriteBarrier();
#else
    protection->m_pointer.store(node, std::memory_order_seq_cst);
#endif
    lease.m_cache = this;
    lease.m_record = protection;
    if (node != m_slots[index].load(std::memory_order_seq_cst))
    {
        return nullptr;
    }
    if (node->m_epoch != epoch())
    {
        // clear与首次填充交错时，旧代候选可能晚发布；摘除它，让下次正常查找重新填充。
        ProcessWorkSendCacheNode* expected = node;
        if (m_slots[index].compare_exchange_strong(expected, nullptr, std::memory_order_seq_cst))
        {
            retire(node);
        }
        return nullptr;
    }
    return node->m_pid == pid ? node->m_channel.get() : nullptr;
}

void ProcessWorkSendCache::remember(int32_t pid, uint64_t expectedEpoch,
    const std::shared_ptr<ProcessWorkChannel>& channel)
{
    if (pid <= 0 || !channel || expectedEpoch == 0 || expectedEpoch != epoch())
    {
        return;
    }
    const size_t index = static_cast<uint32_t>(pid) & 63;
    if (m_slots[index].load(std::memory_order_acquire) != nullptr)
    {
        // 碰撞目标直接使用权威Map，不能把每次未命中变成堆分配和进程写屏障。
        return;
    }
    try
    {
        std::unique_ptr<ProcessWorkSendCacheNode> candidate(new (std::nothrow) ProcessWorkSendCacheNode(pid, expectedEpoch, channel));
        if (!candidate)
        {
            return;
        }
        ProcessWorkSendCacheNode* empty = nullptr;
        if (m_slots[index].compare_exchange_strong(empty, candidate.get(), std::memory_order_seq_cst))
        {
            candidate.release();
        }
    }
    catch (...)
    {
        // 可选缓存准备失败不改变原Map取得的通道或发送结果。
    }
}

void ProcessWorkSendCache::retire(ProcessWorkSendCacheNode* node)
{
    if (node == nullptr)
    {
        return;
    }
    ProcessWorkSendCacheNode* head = m_retired.load(std::memory_order_relaxed);
    do
    {
        node->m_next = head;
    } while (!m_retired.compare_exchange_weak(head, node, std::memory_order_release, std::memory_order_relaxed));
}

void ProcessWorkSendCache::clear()
{
    uint64_t current = m_epoch.load(std::memory_order_acquire);
    while (current != 0 && !m_epoch.compare_exchange_weak(current,
        current == (std::numeric_limits<uint64_t>::max)() ? 0 : current + 1, std::memory_order_seq_cst))
    {

    }
    for (size_t i = 0; i < 64; ++i)
    {
        retire(m_slots[i].exchange(nullptr, std::memory_order_seq_cst));
    }
    collect();
}

void ProcessWorkSendCache::collect()
{
    if (m_retired.load(std::memory_order_acquire) == nullptr)
    {
        return;
    }
    ProcessWorkSendCacheNode* node = m_retired.exchange(nullptr, std::memory_order_seq_cst);
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    // 节点已经从来源槽摘除。让每个处理器完成写屏障后才扫描保护：
    // 复查见到旧来源的读者，其先行保护在扫描前可见；晚发布保护的读者复查会失败。
    // 不等待某个发送线程完成，正在借用的节点继续退役保留。
    if (node != nullptr)
    {
        ::FlushProcessWriteBuffers();
    }
#endif
    while (node != nullptr)
    {
        ProcessWorkSendCacheNode* next = node->m_next;
        bool protectedNow = false;
        for (size_t i = 0; i < 128; ++i)
        {
            if (m_records[i].m_pointer.load(std::memory_order_seq_cst) == node)
            {
                protectedNow = true;
                break;
            }
        }
        if (protectedNow)
        {
            retire(node);
        }
        else
        {
            delete node;
        }
        node = next;
    }
}
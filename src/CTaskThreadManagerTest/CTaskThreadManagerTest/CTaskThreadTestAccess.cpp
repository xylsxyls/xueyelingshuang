#include "CTaskThreadTestAccess.h"
#ifndef CTTM_TEST_DLL
#include "CTaskPrivateTestAccess.h"
#include <climits>
#include <stdexcept>

CTaskTestThreadState::Type CTaskTestThreadState::s_member = nullptr;
CTaskTestManagerState::Type CTaskTestManagerState::s_member = nullptr;
CTaskTestManagerId::Type CTaskTestManagerId::s_member = nullptr;
CTaskTestManagerThreads::Type CTaskTestManagerThreads::s_member = nullptr;
CTaskTestSequence::Type CTaskTestSequence::s_member = nullptr;
CTaskTestCurrent::Type CTaskTestCurrent::s_member = nullptr;
CTaskTestOutstanding::Type CTaskTestOutstanding::s_member = nullptr;
CTaskTestReleased::Type CTaskTestReleased::s_member = nullptr;
CTaskTestQueued::Type CTaskTestQueued::s_member = nullptr;
CTaskTestDequeued::Type CTaskTestDequeued::s_member = nullptr;
CTaskTestObservers::Type CTaskTestObservers::s_member = nullptr;
CTaskTestBuckets::Type CTaskTestBuckets::s_member = nullptr;
CTaskTestDomain::Type CTaskTestDomain::s_member = nullptr;
CTaskTestStorage::Type CTaskTestStorage::s_member = nullptr;
CTaskTestPreempt::Type CTaskTestPreempt::s_member = nullptr;

// C++ 显式模板实例化允许指定私有成员；只保存指针，不依赖对象布局偏移。
template struct CTaskPrivateBinding<CTaskTestThreadState, &CTaskThread::m_state>;
template struct CTaskPrivateBinding<CTaskTestManagerState, &CTaskThreadManager::m_state>;
template struct CTaskPrivateBinding<CTaskTestManagerId, &CTaskManagerState::m_threadId>;
template struct CTaskPrivateBinding<CTaskTestManagerThreads, &CTaskManagerState::m_threads>;
template struct CTaskPrivateBinding<CTaskTestSequence, &CTaskThreadState::m_sequence>;
template struct CTaskPrivateBinding<CTaskTestCurrent, &CTaskThreadState::m_current>;
template struct CTaskPrivateBinding<CTaskTestOutstanding, &CTaskThreadState::m_outstanding>;
template struct CTaskPrivateBinding<CTaskTestReleased, &CTaskThreadState::m_releasedTasks>;
template struct CTaskPrivateBinding<CTaskTestQueued, &CTaskThreadState::m_queued>;
template struct CTaskPrivateBinding<CTaskTestDequeued, &CTaskThreadState::m_dequeuedTasks>;
template struct CTaskPrivateBinding<CTaskTestObservers, &CTaskThreadState::m_observers>;
template struct CTaskPrivateBinding<CTaskTestBuckets, &CTaskThreadState::m_buckets>;
template struct CTaskPrivateBinding<CTaskTestDomain, &CTaskThreadState::m_readDomain>;
template struct CTaskPrivateBinding<CTaskTestStorage, &CTaskThreadState::m_storage>;
template struct CTaskPrivateBinding<CTaskTestPreempt, &CTaskThreadState::preempt>;

/** 从生产门面取得状态，只在测试工程使用
@param [in,out] thread 已成功初始化的线程
@return 线程生命周期内有效的私有状态
*/
static CTaskThreadState& TestState(CTaskThread& thread)
{
    return *(thread.*CTaskTestThreadState::s_member);
}

/** 读取已经初始化的管理器，不调用其私有初始化流程
@return 管理器状态或 nullptr
*/
static CTaskManagerState* TestManagerState()
{
    if ((CTaskThreadManager::instance().*CTaskTestManagerState::s_member).load(std::memory_order_acquire) == nullptr)
    {
        CTaskThreadManager::instance().configure(CTaskThreadManagerConfig());
    }
    return (CTaskThreadManager::instance().*CTaskTestManagerState::s_member).load(std::memory_order_acquire);
}

bool CTaskThreadTestAccess::holdManagerRead(const std::function<void()>& held)
{
    CTaskManagerState* state = TestManagerState();
    bool called = false;
    if (state != nullptr)
    {
        (state->*CTaskTestManagerThreads::s_member).forEach([&held, &called](const uint32_t&, const std::shared_ptr<CTaskThread>&)
        {
            if (!called)
            {
                called = true;
                held();
            }
        });
    }
    return called;
}

void CTaskThreadTestAccess::setTaskSequence(CTaskThread& thread, uint64_t sequence)
{
    const uint64_t block = sequence / 512;
    (TestState(thread).*CTaskTestSequence::s_member).store(block + (sequence % 512 != 0 ? 1 : 0), std::memory_order_relaxed);
    CTaskReadDomain& domain = TestState(thread).*CTaskTestDomain::s_member;
    CTaskStorage& storage = TestState(thread).*CTaskTestStorage::s_member;
    for (size_t i = 0; i <= domain.capacity(); ++i)
    {
        storage.cache(i).m_nextSequence = 0;
        storage.cache(i).m_lastSequence = 0;
    }
    if (sequence % 512 != 0 && sequence != UINT64_MAX)
    {
        // 边界用例随后由同一测试线程投递；部分尾段只交给一个位置，不能复制给所有缓存。
        CTaskReadPosition* position = domain.enter();
        if (position == nullptr)
        {
            throw std::runtime_error("sequence fixture cannot acquire idle read position");
        }
        CTaskStorageCache& cache = storage.cache(domain.indexOf(*position));
        cache.m_nextSequence = sequence;
        cache.m_lastSequence = block == UINT64_MAX / 512 ? UINT64_MAX : (block + 1) * 512;
        domain.leave(*position);
    }
}

void CTaskThreadTestAccess::seedPendingCounters(CTaskThread& thread)
{
    (TestState(thread).*CTaskTestOutstanding::s_member).store(UINT64_MAX - 1, std::memory_order_relaxed);
    (TestState(thread).*CTaskTestReleased::s_member).store(UINT64_MAX - 2, std::memory_order_release);
    (TestState(thread).*CTaskTestQueued::s_member).store(UINT64_MAX - 1, std::memory_order_relaxed);
    (TestState(thread).*CTaskTestDequeued::s_member).store(UINT64_MAX - 1, std::memory_order_release);
}

void CTaskThreadTestAccess::setManagerSequence(uint32_t sequence)
{
    CTaskManagerState* state = TestManagerState();
    if (state != nullptr)
    {
        (state->*CTaskTestManagerId::s_member).store(sequence, std::memory_order_relaxed);
    }
}

size_t CTaskThreadTestAccess::recordCount(CTaskThread& thread)
{
    if (!(TestState(thread).*CTaskTestObservers::s_member).enter())
    {
        return 0;
    }
    CTaskReadDomain& domain = TestState(thread).*CTaskTestDomain::s_member;
    CTaskReadPosition* reader = domain.enter();
    size_t count = 0;
    if (reader != nullptr)
    {
        (TestState(thread).*CTaskTestStorage::s_member).forEach([&count](CTaskStorageSlot&)
        {
            ++count;
        });
        domain.leave(*reader);
    }
    (TestState(thread).*CTaskTestObservers::s_member).leave();
    return count;
}

size_t CTaskThreadTestAccess::bucketCount(CTaskThread& thread)
{
    if (!(TestState(thread).*CTaskTestObservers::s_member).enter())
    {
        return 0;
    }
    const size_t count = (TestState(thread).*CTaskTestBuckets::s_member).size();
    (TestState(thread).*CTaskTestObservers::s_member).leave();
    return count;
}

bool CTaskThreadTestAccess::closeIdleBucket(CTaskThread& thread, int32_t level)
{
    if (!(TestState(thread).*CTaskTestObservers::s_member).enter())
    {
        return false;
    }
    std::shared_ptr<CTaskPriorityBucket> bucket;
    LockFreeMapToken token;
    bool closed = false;
    if ((TestState(thread).*CTaskTestBuckets::s_member).lockFreeMapGet(level, bucket, token) == LOCK_FREE_MAP_OK && bucket->m_queued.load() == 0)
    {
        closed = bucket->m_writers.closeIfEmpty();
    }
    (TestState(thread).*CTaskTestObservers::s_member).leave();
    return closed;
}

CTaskThreadResult CTaskThreadTestAccess::latePreempt(CTaskThread& thread)
{
    CTaskReadDomain& domain = TestState(thread).*CTaskTestDomain::s_member;
    CTaskReadPosition* reader = domain.enter();
    if (reader == nullptr)
    {
        return CTaskThreadResult(CTaskThreadRetry);
    }
    CTaskStorageSlot posted;
    CTaskStorageState postedState;
    posted.m_state = &postedState;
    posted.m_state->m_bits.store(3);
    const CTaskThreadResult result = (TestState(thread).*CTaskTestPreempt::s_member)(INT_MAX, &posted);
    domain.leave(*reader);
    return result;
}

CTaskThreadResult CTaskThreadTestAccess::holdDirectoryRead(CTaskThread& thread, const std::function<void()>& held)
{
    CTaskAdmission& observers = TestState(thread).*CTaskTestObservers::s_member;
    if (!observers.enter())
    {
        return CTaskThreadResult(CTaskThreadClosed);
    }
    CTaskReadDomain& domain = TestState(thread).*CTaskTestDomain::s_member;
    CTaskReadPosition* reader = domain.enter();
    if (reader == nullptr)
    {
        observers.leave();
        return CTaskThreadResult(CTaskThreadRetry);
    }
    CTaskThreadResult result;
    try
    {
        held();
    }
    catch (...)
    {
        result = CTaskThreadResult(CTaskThreadCallbackFailure);
    }
    domain.leave(*reader);
    observers.leave();
    return result;
}

#endif
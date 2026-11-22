#pragma once
#include "CTaskThreadTestConfig.h"
#ifndef CTTM_TEST_DLL
#include "CTaskThreadManager/CTaskThreadState.h"
#include "CTaskThreadManager/CTaskThreadManager.h"
#include "CTaskThreadManager/CTaskManagerState.h"
#include "CTaskThreadManager/CTaskPoolState.h"

/** 测试专用显式实例化绑定器；生产类不声明测试友元，也不改变访问修饰符
@param Tag 测试成员指针存放类型
@param Member 仅在显式实例化位置引用的实际成员
*/
template <class Tag, typename Tag::Type Member>
struct CTaskPrivateBinding
{
public:
    // 显式实例化时完成绑定，不进入生产库编译或导出表
    static const bool m_bound;
};

/** 白盒测试使用的 ThreadState 成员指针，不拥有对应对象
*/
struct CTaskTestThreadState
{
public:
    typedef std::unique_ptr<CTaskThreadState> CTaskThread::* Type;
    // 由测试翻译单元中的显式实例化初始化
    static Type m_member;
};

/** 白盒测试使用的 ManagerState 成员指针，不拥有对应对象
*/
struct CTaskTestManagerState
{
public:
    typedef std::atomic<CTaskManagerState*> CTaskThreadManager::* Type;
    // 由测试翻译单元中的显式实例化初始化
    static Type m_member;
};

/** 白盒测试使用的 ManagerId 成员指针，不拥有对应对象
*/
struct CTaskTestManagerId
{
public:
    typedef std::atomic<uint64_t> CTaskManagerState::* Type;
    // 由测试翻译单元中的显式实例化初始化
    static Type m_member;
};

/** 白盒测试使用的 ManagerThreads 成员指针，不拥有对应对象
*/
struct CTaskTestManagerThreads
{
public:
    typedef LockFreeMap<uint32_t, std::shared_ptr<CTaskThread>> CTaskManagerState::* Type;
    // 由测试翻译单元中的显式实例化初始化
    static Type m_member;
};

/** 白盒测试使用的 Sequence 成员指针，不拥有对应对象
*/
struct CTaskTestSequence
{
public:
    typedef std::atomic<uint64_t> CTaskThreadState::* Type;
    // 由测试翻译单元中的显式实例化初始化
    static Type m_member;
};

/** 白盒测试观察最近选择的记录；记录所有权仍由测试夹具维持
*/
struct CTaskTestCurrent
{
public:
    typedef std::atomic<CTaskStorageSlot*> CTaskThreadState::* Type;
    // 仅测试翻译单元显式实例化绑定
    static Type m_member;
};

/** 白盒测试使用的 Released 成员指针，不拥有对应对象
*/
struct CTaskTestReleased
{
public:
    typedef std::atomic<uint64_t> CTaskThreadState::* Type;
    // 由测试翻译单元中的显式实例化初始化
    static Type m_member;
};

/** 白盒测试使用的 Queued 成员指针，不拥有对应对象
*/
struct CTaskTestQueued
{
public:
    typedef std::atomic<uint64_t> CTaskThreadState::* Type;
    // 由测试翻译单元中的显式实例化初始化
    static Type m_member;
};

/** 白盒测试使用的 Dequeued 成员指针，不拥有对应对象
*/
struct CTaskTestDequeued
{
public:
    typedef std::atomic<uint64_t> CTaskThreadState::* Type;
    // 由测试翻译单元中的显式实例化初始化
    static Type m_member;
};

/** 只读观察真实容量重试留下的压力提示，不伪造库内状态
*/
struct CTaskTestCapacityPressure
{
public:
    typedef std::atomic<uint32_t> CTaskThreadState::* Type;
    // 由测试翻译单元中的显式实例化初始化
    static Type m_member;
};

/** 白盒测试使用的 Observers 成员指针，不拥有对应对象
*/
struct CTaskTestObservers
{
public:
    typedef CTaskAdmission CTaskThreadState::* Type;
    // 由测试翻译单元中的显式实例化初始化
    static Type m_member;
};

/** 白盒测试使用的 Buckets 成员指针，不拥有对应对象
*/
struct CTaskTestBuckets
{
public:
    typedef LockFreeMap<int32_t, std::shared_ptr<CTaskPriorityBucket>> CTaskThreadState::* Type;
    // 由测试翻译单元中的显式实例化初始化
    static Type m_member;
};

/** 白盒测试使用的 Domain 成员指针，不拥有对应对象
*/
struct CTaskTestDomain
{
public:
    typedef CTaskReadDomain CTaskThreadState::* Type;
    // 由测试翻译单元中的显式实例化初始化
    static Type m_member;
};

/** 白盒测试使用的 Storage 成员指针，不拥有对应对象
*/
struct CTaskTestStorage
{
public:
    typedef CTaskStorage CTaskThreadState::* Type;
    // 由测试翻译单元中的显式实例化初始化
    static Type m_member;
};

/** 白盒测试使用的 Preempt 成员指针，不拥有对应对象
*/
struct CTaskTestPreempt
{
public:
    typedef CTaskThreadStatus (CTaskThreadState::*Type)(int32_t, CTaskStorageSlot*);

    // 由测试翻译单元中的显式实例化初始化
    static Type m_member;
};

/** 静止故障注入借用工作线程的启动通知对象，不更改生产类声明
*/
struct CTaskTestWakeup
{
public:
    typedef Semaphore CTaskThreadState::* Type;
    // 测试翻译单元显式绑定的通知成员
    static Type m_member;
};

/** 静止故障注入借用工作线程的退出通知对象
*/
struct CTaskTestCompletion
{
public:
    typedef Semaphore CTaskThreadState::* Type;
    // 测试翻译单元显式绑定的通知成员
    static Type m_member;
};

/** 静止故障注入借用线程池的控制通知对象
*/
struct CTaskTestPoolChanged
{
public:
    typedef Semaphore CTaskPoolState::* Type;
    // 测试翻译单元显式绑定的通知成员
    static Type m_member;
};

/** 仅测试工程设置原生等待后端的初始化失败状态，不修改基础库源码
*/
struct CTaskTestSemaphoreWake
{
public:
    typedef SemaphoreWake Semaphore::* Type;
    // 未启动并发调用前使用的原生后端成员
    static Type m_member;
};

/** 绑定 FIFO 配置成员，供静止夹具验证配置已传入基础库。
@param Value 底层字段类型
@param Index 区分同一类型的不同字段
@param Owner 字段所属的队列或其发布通道
*/
template <class Value, unsigned Index, class Owner = LockFreeQueue<CTaskStorageSlot*>>
struct CTaskTestQueueSetting
{
public:
    typedef Value Owner::* Type;
    // 初始化后只读，引用完整队列配置。
    static Type m_member;
};

#include "CTaskPrivateTestAccess.inl"
#endif
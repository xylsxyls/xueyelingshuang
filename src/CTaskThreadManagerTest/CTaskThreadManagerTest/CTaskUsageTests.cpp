#include "CTaskUsageTests.h"
#include "Semaphore/SemaphoreAPI.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

UsageTask::UsageTask(int32_t propertyId, const std::function<void(UsageTask&)>& body, bool redo) :
CTask(propertyId),
m_body(body),
m_redo(redo),
m_stopped(false),
m_runs(0),
m_stops(0)
{

}

void UsageTask::doTask()
{
    m_runs.fetch_add(1);
    m_body(*this);
}

void UsageTask::stopTask()
{
    m_stops.fetch_add(1);
    m_stopped.store(true);
}

bool UsageTask::reExecute()
{
    return m_redo;
}

bool UsageTask::stopped() const
{
    return m_stopped.load();
}

uint32_t UsageTask::runs() const
{
    return m_runs.load();
}

uint32_t UsageTask::stops() const
{
    return m_stops.load();
}

UsagePoolPolicy::UsagePoolPolicy() :
m_target(2)
{

}

void UsagePoolPolicy::setTarget(size_t target)
{
    m_target.store(target);
}

CTaskPoolDecision UsagePoolPolicy::evaluate(const CTaskPoolLoad& load)
{
    size_t target = m_target.load();
    if (load.m_queuedTasks != 0)
    {
        target = (std::max)(target, load.m_currentThreads);
    }
    return CTaskPoolDecision(target, 20);
}

/** 核对示例结果，失败转入用例入口的统一收尾，不使用 assert/abort
@param [in] condition 预期是否成立
@param [in] message 失败说明
*/
static void Check(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

/** 有界等待示例条件；用于演示协调，不是业务热路径写法
@param [in] ready 条件观察或未提交操作的一次重试；返回 true 后不再调用
@param [in] milliseconds 最大等待毫秒数
@return 条件是否在期限内满足
*/
static bool WaitUntil(const std::function<bool()>& ready, uint32_t milliseconds = 5000)
{
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
    Semaphore pause;
    while (!ready())
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        const SemaphoreResult result = pause.waitResult(1);
        Check(result.status == SEMAPHORE_TIMEOUT, "coordinator wait failed");
    }
    return true;
}

/** 普通投递内部处理竞争；此处只检查最终接纳及真实错误
@param [in] target 已取得的线程强引用
@param [in] task 已构造并配置完成的业务对象
@param [in] level 调度优先级，大值优先
*/
static void Post(const std::shared_ptr<CTaskThread>& target, const std::shared_ptr<CTask>& task, int32_t level = 1)
{
    const CTaskThreadResult result = target->postTask(task, level);
    Check(result.m_committed, taskThreadStatusText(result.m_status));
    // 已接纳后即使附带错误也绝不重发；示例要求回调没有错误。
    Check(result.m_status == CTaskThreadOk, "accepted task has notification error");
}

/** 创建线程并缓存接口，所有权仍由管理器登记
@param [in] config 冻结的初始化配置
@return 已启动线程，调用方最后须关闭并注销
*/
static std::shared_ptr<CTaskThread> Open(const CTaskThreadConfig& config)
{
    uint32_t id = 0;
    CTaskThreadResult result = CTaskThreadManager::instance().initialize(config, id);
    Check(result.m_status == CTaskThreadOk && result.m_committed && id != 0, "initialize failed");
    std::shared_ptr<CTaskThread> target = CTaskThreadManager::instance().getThreadInterface(id);
    Check(target != nullptr, "thread lookup failed; main will reclaim registered id");
    return target;
}

/** 创建有界合作等待任务，用于确定其它任务排队的时机
@param [in] propertyId 属性 ID
@param [in] release 外部正常放行标志
@return 收到 stopTask 也会提前返回的任务
*/
static std::shared_ptr<UsageTask> Gate(int32_t propertyId, const std::shared_ptr<std::atomic<bool>>& release)
{
    return std::make_shared<UsageTask>(propertyId, [release](UsageTask& self)
    {
        Check(WaitUntil([&self, release]() -> bool
        {
            return self.stopped() || release->load();
        }), "gate timed out");
    });
}

/** 创建只向单消费者账本追加标记的任务
@param [in] propertyId 属性 ID
@param [in] mark 执行标记
@param [in] order 只在该线程执行时写，主线程在退出后读取
@return 普通任务，停止通知不删除该排队项
*/
static std::shared_ptr<UsageTask> Append(int32_t propertyId, int32_t mark, const std::shared_ptr<std::vector<int32_t>>& order)
{
    return std::make_shared<UsageTask>(propertyId, [mark, order](UsageTask&)
    {
        order->push_back(mark);
    });
}

/** 展示配置、查询、优先级、FIFO、系统优先级和两种排空接口
@param [in] config 工作线程配置
*/
static void ExamplePriority(const CTaskThreadConfig& config)
{
    CTaskThreadManager& manager = CTaskThreadManager::instance();
    std::shared_ptr<CTaskThread> target = Open(config);
    Check(manager.count() == 1, "manager count");
    Check(target->setSystemPriority(CTaskPriorityNormal).m_status == CTaskThreadOk, "system priority");
    std::shared_ptr<std::atomic<bool>> release = std::make_shared<std::atomic<bool>>(false);
    std::shared_ptr<UsageTask> gate = Gate(900, release);
    Post(target, gate, 16);
    Check(WaitUntil([gate]() -> bool
    {
        return gate->runs() == 1;
    }), "gate not started");
    std::shared_ptr<std::vector<int32_t>> order = std::make_shared<std::vector<int32_t>>();
    std::shared_ptr<UsageTask> first = Append(42, 1, order);
    Check(first->getPropertyId() == 42, "property id");
    Check(first->clone(std::function<CTask*()>()) && first->cloneFactory() == nullptr, "clear factory");
    Post(target, first, 1);
    Post(target, Append(42, 2, order), 1);
    Post(target, Append(7, 5, order), 5);
    Check(target->tryPostTask(Append(8, 3, order), 3), "tryPostTask refused");
    Check(!target->postTask(first, 0).m_committed, "invalid level accepted");
    Check(!target->postTask(std::shared_ptr<CTask>()).m_committed, "null task accepted");
    std::map<int32_t, int32_t> waiting;
    Check(target->getWaitTaskInfo(waiting).m_status == CTaskThreadOk, "wait info");
    Check(target->hasTask() && target->getWaitTaskCount() == 4 && waiting[1] == 2, "queued counts");
    Check(target->getCurTaskLevel() == 16 && target->statistics().m_outstanding == 5, "running counts");
    release->store(true);
    Check(target->waitForEnd().m_status == CTaskThreadOk, "direct drain");
    const int32_t expected[] = {5, 3, 1, 2};
    Check(order->size() == 4 && std::equal(order->begin(), order->end(), expected), "priority/FIFO");
    Check(target->statistics().m_errors == 0 && target->statistics().m_executed == 5, "execution statistics");
    Check(manager.count() == 1, "direct waitForEnd must retain registration");
    Check(manager.waitForEnd(target->threadId()).m_status == CTaskThreadOk, "manager unregister");
    const CTaskThreadResult closed = target->postTask(first);
    Check(!closed.m_committed && closed.m_status == CTaskThreadClosed && manager.count() == 0, "old handle");
    std::cout << "PASS priority, FIFO, property, queries, system priority, drain" << std::endl;
}

/** 展示 A 被 H 打断、H 又被 X 打断，重做仍先于同级普通队列
@param [in] config 工作线程配置
*/
static void ExampleRedo(const CTaskThreadConfig& config)
{
    std::shared_ptr<CTaskThread> target = Open(config);
    std::shared_ptr<std::vector<int32_t>> order = std::make_shared<std::vector<int32_t>>();
    std::shared_ptr<std::atomic<uint32_t>> factories = std::make_shared<std::atomic<uint32_t>>(0);
    std::shared_ptr<UsageTask> low = std::make_shared<UsageTask>(10, [order](UsageTask& self)
    {
        order->push_back(1);
        Check(WaitUntil([&self]() -> bool
        {
            return self.stopped();
        }), "low task not stopped");
    }, true);
    std::shared_ptr<UsageTask> high = std::make_shared<UsageTask>(20, [order](UsageTask& self)
    {
        order->push_back(5);
        Check(WaitUntil([&self]() -> bool
        {
            return self.stopped();
        }), "high task not stopped");
    }, true);
    // lambda 只在实际重做时调用；返回独立裸指针，所有权交给库。
    Check(low->clone([order, factories]() -> CTask*
    {
        factories->fetch_add(1);
        return new UsageTask(10, [order](UsageTask&)
        {
            order->push_back(11);
        });
    }), "low factory configuration");
    Check(high->clone([order, factories]() -> CTask*
    {
        factories->fetch_add(1);
        return new UsageTask(20, [order](UsageTask&)
        {
            order->push_back(55);
        });
    }), "high factory configuration");
    Check(low->cloneFactory() != nullptr && factories->load() == 0, "factory must be lazy");
    Post(target, low, 1);
    Check(WaitUntil([low]() -> bool
    {
        return low->runs() == 1;
    }), "low not running");
    Post(target, Append(10, 2, order), 1);
    Post(target, high, 5);
    Check(WaitUntil([high]() -> bool
    {
        return high->runs() == 1;
    }), "high not running");
    Post(target, Append(30, 10, order), 10);
    Check(CTaskThreadManager::instance().waitForEnd(target->threadId()).m_status == CTaskThreadOk, "redo drain");
    const int32_t expected[] = {1, 5, 10, 55, 11, 2};
    Check(order->size() == 6 && std::equal(order->begin(), order->end(), expected), "nested redo order");
    Check(factories->load() == 2 && target->statistics().m_errors == 0, "redo factory count/errors");
    std::cout << "PASS nested redo: A, H, X, H-redo, A-redo, same-level-next" << std::endl;
}

/** 展示按属性停止、全部停止和当前停止，排队项不会因此删除
@param [in] config 工作线程配置
*/
static void ExampleStopping(const CTaskThreadConfig& config)
{
    std::shared_ptr<CTaskThread> target = Open(config);
    std::shared_ptr<std::atomic<bool>> release = std::make_shared<std::atomic<bool>>(false);
    std::shared_ptr<UsageTask> gate = Gate(900, release);
    Post(target, gate, 16);
    Check(WaitUntil([gate]() -> bool
    {
        return gate->runs() == 1;
    }), "stop gate not started");
    std::shared_ptr<std::vector<int32_t>> order = std::make_shared<std::vector<int32_t>>();
    std::shared_ptr<UsageTask> a = Append(7, 1, order);
    std::shared_ptr<UsageTask> b = Append(7, 2, order);
    std::shared_ptr<UsageTask> c = Append(8, 3, order);
    Post(target, a, 1);
    Post(target, b, 2);
    Post(target, c, 2);
    Check(target->stopTask(7, 1).m_status == CTaskThreadOk, "property stop");
    Check(a->stops() == 1 && b->stops() == 0 && c->stops() == 0, "property/level filtering");
    Check(target->stopAllTask().m_status == CTaskThreadOk, "stop all");
    Check(CTaskThreadManager::instance().waitForEnd(target->threadId()).m_status == CTaskThreadOk, "stop drain");
    Check(a->runs() == 1 && b->runs() == 1 && c->runs() == 1 && order->size() == 3, "stop must not erase queued tasks");
    Check(b->stops() >= 1 && c->stops() >= 1 && gate->stopped(), "stop all coverage");
    target = Open(config);
    gate = Gate(901, release);
    Post(target, gate);
    Check(WaitUntil([gate]() -> bool
    {
        return gate->runs() == 1;
    }), "current gate not started");
    Check(target->stopCurTask().m_status == CTaskThreadOk, "stop current");
    Check(CTaskThreadManager::instance().waitForEnd(target->threadId()).m_status == CTaskThreadOk && gate->stopped(), "current stop completion");
    std::cout << "PASS property stop, stopAllTask, stopCurTask; stop does not erase" << std::endl;
}

/** 展示物理池增长/缩容回调和逻辑容量拒绝，观察不是额度预留
@param [in] source 常规线程配置
*/
static void ExampleResources(const CTaskThreadConfig& source)
{
    CTaskThreadConfig config = source;
    std::shared_ptr<std::atomic<uint32_t>> mapGrows = std::make_shared<std::atomic<uint32_t>>(0);
    std::shared_ptr<std::atomic<uint32_t>> taskGrows = std::make_shared<std::atomic<uint32_t>>(0);
    std::shared_ptr<std::atomic<uint32_t>> queueGrows = std::make_shared<std::atomic<uint32_t>>(0);
    std::shared_ptr<std::atomic<uint32_t>> shrinks = std::make_shared<std::atomic<uint32_t>>(0);
    config.m_priorityMap.nodes.initialCapacity = 2;
    config.m_priorityMap.values.initialCapacity = 2;
    // 三类增长上下文都提供 initialCapacity、successfulGrowCount、currentCapacity。
    // 回调只返回增量，上限由池截断；缩容时间为 0 表示禁用。
    config.m_priorityMap.nodes.grow = [mapGrows](const LockFreeMapGrowContext& context) -> size_t
    {
        mapGrows->fetch_add(1);
        return (std::max)(size_t(8), context.currentCapacity / 2);
    };
    config.m_priorityMap.values.grow = config.m_priorityMap.nodes.grow;
    config.m_priorityMap.nodes.shrink = [shrinks](const LockFreeMapShrinkContext& context) -> LockFreeMapShrinkDecision
    {
        shrinks->fetch_add(1);
        return LockFreeMapShrinkDecision(context.reusableCapacity / 2, 20);
    };
    config.m_priorityMap.values.shrink = config.m_priorityMap.nodes.shrink;
    config.m_taskMap.initialCapacity = 64;
    config.m_taskMap.maximumCapacity = 1024;
    config.m_taskMap.growPolicy = [taskGrows](const LockFreeHashMapGrowContext& context) -> size_t
    {
        taskGrows->fetch_add(1);
        return (std::max)(size_t(64), context.currentCapacity / 2);
    };
    config.m_taskMap.shrinkPolicy = [shrinks](const LockFreeHashMapShrinkContext& context) -> LockFreeHashMapShrinkDecision
    {
        shrinks->fetch_add(1);
        return LockFreeHashMapShrinkDecision(context.reusableFreeCapacity / 2, 20);
    };
    config.m_queue.initialCapacity = 8;
    config.m_queue.maximumCapacity = 1024;
    config.m_queueGrow = [queueGrows](const LockFreeQueueGrowContext& context) -> size_t
    {
        queueGrows->fetch_add(1);
        return (std::max)(size_t(32), context.currentCapacity / 2);
    };
    config.m_queueShrink = [shrinks](const LockFreeQueueShrinkContext& context) -> LockFreeQueueShrinkDecision
    {
        shrinks->fetch_add(1);
        LockFreeQueueShrinkDecision decision = {context.reusableFreeCapacity / 2, 20};
        return decision;
    };
    config.m_maximumTasks = -1;
    std::shared_ptr<CTaskThread> target = Open(config);
    std::shared_ptr<std::atomic<bool>> release = std::make_shared<std::atomic<bool>>(false);
    std::shared_ptr<UsageTask> gate = Gate(900, release);
    Post(target, gate, 16);
    Check(WaitUntil([gate]() -> bool
    {
        return gate->runs() == 1;
    }), "growth gate not started");
    std::shared_ptr<UsageTask> work = std::make_shared<UsageTask>(1, [](UsageTask&)
    {

    });
    for (int32_t i = 0; i < 200; ++i)
    {
        Post(target, work, i < 190 ? 1 : i - 188);
    }
    release->store(true);
    Check(WaitUntil([work, shrinks]() -> bool
    {
        return work->runs() == 200 && shrinks->load() != 0;
    }), "growth/shrink callbacks");
    Check(CTaskThreadManager::instance().waitForEnd(target->threadId()).m_status == CTaskThreadOk, "growth drain");
    Check(mapGrows->load() != 0 && taskGrows->load() != 0 && queueGrows->load() != 0 && target->statistics().m_errors == 0, "physical pool growth");
    // 上面只通过公开行为核对缩容策略被调用，不把它说成实际释放了精确数量。
    config = source;
    config.m_maximumTasks = 2;
    target = Open(config);
    release->store(false);
    gate = Gate(900, release);
    Post(target, gate, 16);
    Check(WaitUntil([gate]() -> bool
    {
        return gate->runs() == 1;
    }), "capacity gate not started");
    Post(target, work);
    const CTaskThreadResult full = target->postTask(work);
    Check(!full.m_committed && full.m_status == CTaskThreadCapacity, "logical capacity rejection");
    release->store(true);
    Check(CTaskThreadManager::instance().waitForEnd(target->threadId()).m_status == CTaskThreadOk, "capacity drain");
    std::cout << "PASS memory policy callbacks, growth, logical capacity rejection" << std::endl;
}

/** 展示从任务请求排空、自等待拒绝、异常隔离和丢弃关闭
@param [in] config 工作线程配置
*/
static void ExampleLifecycle(const CTaskThreadConfig& config)
{
    CTaskThreadManager& manager = CTaskThreadManager::instance();
    std::shared_ptr<CTaskThread> target = Open(config);
    std::weak_ptr<CTaskThread> weak = target;
    std::shared_ptr<std::atomic<bool>> release = std::make_shared<std::atomic<bool>>(false);
    std::shared_ptr<UsageTask> gate = Gate(900, release);
    Post(target, gate, 16);
    Check(WaitUntil([gate]() -> bool
    {
        return gate->runs() == 1;
    }), "lifecycle gate");
    std::shared_ptr<UsageTask> closing = std::make_shared<UsageTask>(1, [weak](UsageTask&)
    {
        std::shared_ptr<CTaskThread> self = weak.lock();
        Check(self && self->waitForEnd().m_status == CTaskThreadSelfWait, "self wait must be refused");
        Check(self->requestShutdown(true).m_committed, "request drain from task");
    });
    Post(target, closing, 5);
    std::shared_ptr<UsageTask> bad = std::make_shared<UsageTask>(2, [](UsageTask&)
    {
        throw std::runtime_error("intentional business failure");
    });
    std::shared_ptr<UsageTask> after = std::make_shared<UsageTask>(3, [](UsageTask&)
    {

    });
    Post(target, bad);
    Post(target, after);
    release->store(true);
    Check(manager.waitForEnd(target->threadId()).m_status == CTaskThreadOk, "requested drain");
    Check(closing->runs() == 1 && after->runs() == 1 && target->statistics().m_errors == 1, "exception isolation");
    // uninit 关闭接纳并丢弃排队项；当前执行仍靠 stopTask 合作返回。
    target = Open(config);
    release->store(false);
    gate = Gate(900, release);
    Post(target, gate, 16);
    Check(WaitUntil([gate]() -> bool
    {
        return gate->runs() == 1;
    }), "discard gate");
    std::shared_ptr<UsageTask> discarded = std::make_shared<UsageTask>(4, [](UsageTask&)
    {

    });
    Post(target, discarded);
    Check(target->requestShutdown(false).m_committed, "request discard");
    Check(manager.uninit(target->threadId()).m_status == CTaskThreadOk, "discard close");
    Check(gate->stopped() && discarded->runs() == 0, "queued task must be discarded");
    const uint32_t firstId = manager.init();
    const uint32_t secondId = manager.init(config);
    Check(firstId != 0 && secondId != 0 && firstId != secondId, "init overloads");
    Check(manager.uninitAll().m_status == CTaskThreadOk && manager.count() == 0, "uninitAll");
    std::cout << "PASS self wait, requestShutdown, exception isolation, uninit, uninitAll" << std::endl;
}

/** 用两条管理器工作线程充当生产者，并发向同一目标投递
@param [in] source 常规线程配置
*/
static void ExampleConcurrent(const CTaskThreadConfig& source)
{
    CTaskThreadConfig config = source;
    config.m_maximumTasks = -1;
    std::shared_ptr<CTaskThread> target = Open(config);
    std::shared_ptr<UsageTask> work = std::make_shared<UsageTask>(1, [](UsageTask&)
    {

    });
    std::shared_ptr<std::atomic<bool>> start = std::make_shared<std::atomic<bool>>(false);
    std::shared_ptr<std::atomic<uint32_t>> ready = std::make_shared<std::atomic<uint32_t>>(0);
    std::vector<std::shared_ptr<CTaskThread>> producers;
    for (int32_t i = 0; i < 2; ++i)
    {
        std::shared_ptr<CTaskThread> producer = Open(config);
        producers.push_back(producer);
        Post(producer, std::make_shared<UsageTask>(2, [target, work, start, ready](UsageTask& self)
        {
            ready->fetch_add(1);
            Check(WaitUntil([start, &self]() -> bool
            {
                return start->load() || self.stopped();
            }), "producer start timed out");
            Check(!self.stopped(), "producer cancelled before start");
            for (int32_t n = 0; n < 100; ++n)
            {
                Post(target, work);
            }
        }));
    }
    // 两个生产者都准备好才同时放行，避免任务太短导致实际串行投递。
    Check(WaitUntil([ready]() -> bool
    {
        return ready->load() == 2;
    }), "producers not ready");
    start->store(true);
    // 必须先等生产结束，再关闭消费者，不能反过来。
    for (size_t i = 0; i < producers.size(); ++i)
    {
        Check(CTaskThreadManager::instance().waitForEnd(producers[i]->threadId()).m_status == CTaskThreadOk, "producer drain");
        Check(producers[i]->statistics().m_errors == 0, "producer error");
    }
    Check(CTaskThreadManager::instance().waitForEnd(target->threadId()).m_status == CTaskThreadOk, "consumer drain");
    Check(work->runs() == 200 && target->statistics().m_errors == 0, "concurrent exact count");
    std::cout << "PASS two concurrent producers: accepted/executed 200" << std::endl;
}

/** 展示默认池、自定义扩缩容、分配器两种接口及上层投递
@param [in] config 工作线程配置
*/
static void ExamplePool(const CTaskThreadConfig& config)
{
    CTaskThreadPoolConfig poolConfig;
    poolConfig.m_initialThreads = 2;
    poolConfig.m_maximumThreads = 4;
    poolConfig.m_checkMilliseconds = 10;
    poolConfig.m_worker = config;
    // 默认策略无需提供对象：全忙且积压时增长，持续空闲后缩容。
    // 初始化和销毁期间必须停止外部 allocator 调用。
    CTaskThreadPool defaultPool;
    Check(defaultPool.initialize(poolConfig).m_committed, "default pool initialize");
    CTaskThreadAllocator defaultAllocator(defaultPool);
    uint32_t id = 0;
    Check(defaultAllocator.select(id).m_status == CTaskThreadOk, "select id");
    std::shared_ptr<CTaskThread> target = CTaskThreadManager::instance().getThreadInterface(id);
    Check(target != nullptr, "pool target lookup");
    std::shared_ptr<UsageTask> work = std::make_shared<UsageTask>(1, [](UsageTask&)
    {

    });
    Post(target, work);
    Check(defaultPool.shutdown(true).m_status == CTaskThreadOk && work->runs() == 1, "default pool drain");

    std::shared_ptr<UsagePoolPolicy> policy = std::make_shared<UsagePoolPolicy>();
    poolConfig.m_policy = policy;
    CTaskThreadPool pool;
    Check(pool.initialize(poolConfig).m_committed, "custom pool initialize");
    CTaskThreadAllocator allocator(pool);
    policy->setTarget(99);
    Check(WaitUntil([&pool]() -> bool
    {
        return pool.statistics().m_threads == 4;
    }), "pool growth/clamp");
    std::vector<uint32_t> ids;
    Check(pool.threadIds(ids).m_status == CTaskThreadOk && ids.size() == 4, "pool threadIds");
    Check(CTaskThreadManager::instance().count() == 5, "pool controller not counted in worker limit");
    for (int32_t i = 0; i < 40; ++i)
    {
        // 选择不预留接纳；缩容竞争时只有未接纳项可以重新选取。
        Check(WaitUntil([&allocator, work]() -> bool
        {
            uint32_t selectedId = 0;
            std::shared_ptr<CTaskThread> selected;
            CTaskThreadResult choice = allocator.select(selectedId, selected);
            if (choice.m_status != CTaskThreadOk)
            {
                Check(choice.m_status == CTaskThreadCapacity || choice.m_status == CTaskThreadClosed, "allocator closed/error");
                return false;
            }
            const CTaskThreadResult posted = selected->postTask(work);
            if (posted.m_committed)
            {
                Check(posted.m_status == CTaskThreadOk, "pool accepted with error");
                return true;
            }
            Check(posted.m_status == CTaskThreadClosed || posted.m_status == CTaskThreadCapacity, "pool post failure");
            return false;
        }), "allocator retry timed out");
    }
    Check(WaitUntil([work]() -> bool
    {
        return work->runs() == 41;
    }), "pool execution entries");
    policy->setTarget(0);
    Check(WaitUntil([&pool]() -> bool
    {
        const CTaskPoolStatistics state = pool.statistics();
        return state.m_threads == 2 && state.m_shrinks == 2;
    }), "pool shrink/floor");
    const CTaskPoolStatistics state = pool.statistics();
    Check(state.m_grows == 2 && state.m_shrinks == 2 && state.m_errors == 0, "pool resize statistics");
    Check(pool.shutdown(false).m_status == CTaskThreadOk, "pool discard shutdown");
    Check(!pool.statistics().m_accepting && CTaskThreadManager::instance().count() == 0, "pool final cleanup");
    std::cout << "PASS default/custom pool, allocator, 2->4->2 workers, upper/lower bounds" << std::endl;
}

/** 公开接口用法测试入口；全部业务线程由本库创建，未使用白盒或故障注入
@return 0 为所有预期成立，非零为失败；外部运行器另设进程超时
*/
int32_t runTaskUsageTests(size_t scenario)
{
    // 唯一实例串行预热；所有构造、策略复制和调用前表达式也在 try 范围内。
    CTaskThreadManager& manager = CTaskThreadManager::instance();
    try
    {
        CTaskThreadManagerConfig managerConfig;
        managerConfig.m_threadMap.nodes.initialCapacity = 32;
        managerConfig.m_threadMap.values.initialCapacity = 64;
        Check(manager.configure(managerConfig).m_status == CTaskThreadOk, "configure manager");
        CTaskThreadConfig config;
        config.m_priorityMap.nodes.initialCapacity = 32;
        config.m_priorityMap.values.initialCapacity = 64;
        config.m_priorityMap.nodes.maximumCapacity = -1;
        config.m_priorityMap.values.maximumCapacity = -1;
        config.m_taskMap.initialCapacity = 1024;
        config.m_taskMap.maximumCapacity = 4096;
        config.m_taskMap.growFreeThreshold = 0;
        config.m_queue.initialCapacity = 512;
        config.m_queue.maximumCapacity = 2048;
        config.m_queue.growThreshold = 0;
        config.m_queue.maximumProducers = 16;
        config.m_maximumTasks = 64;
        config.m_systemPriority = CTaskPriorityNormal;
        Check(scenario <= 7, "unknown usage scenario");
        if (scenario == 0 || scenario == 1)
        {
            ExamplePriority(config);
        }
        if (scenario == 0 || scenario == 2)
        {
            ExampleRedo(config);
        }
        if (scenario == 0 || scenario == 3)
        {
            ExampleStopping(config);
        }
        if (scenario == 0 || scenario == 4)
        {
            ExampleResources(config);
        }
        if (scenario == 0 || scenario == 5)
        {
            ExampleLifecycle(config);
        }
        if (scenario == 0 || scenario == 6)
        {
            ExampleConcurrent(config);
        }
        if (scenario == 0 || scenario == 7)
        {
            ExamplePool(config);
        }
        Check(manager.count() == 0, "registered threads leaked");
        std::cout << "PASS usage scenario=" << scenario << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "FAIL: unexpected exception" << std::endl;
    }
    // 示例拥有本进程创建的所有线程；任务捕获共享对象，不借用已退栈的局部资源。
    const CTaskThreadResult cleanup = manager.uninitAll();
    if (cleanup.m_status != CTaskThreadOk)
    {
        std::cerr << "cleanup failed: " << taskThreadStatusText(cleanup.m_status) << std::endl;
    }
    return 1;
}
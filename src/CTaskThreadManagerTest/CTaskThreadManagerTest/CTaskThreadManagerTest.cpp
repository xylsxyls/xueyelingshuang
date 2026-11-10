#include "CTaskThreadManagerTest.h"
#include "CTaskThreadBenchmark.h"
#include "CTaskPreparedBenchmark.h"
#include "CTaskThreadRegressionTests.h"
#include "CTaskThreadModuleTests.h"
#include "CTaskV3ModuleTests.h"
#include "CTaskV5Tests.h"
#include "CTaskFairnessTests.h"
#include "CTaskThreadWhiteBoxTests.h"
#include "CTaskThreadStressTests.h"
#include "CTaskTestAutomation.h"
#include "CTaskIntrusivePrototype.h"
#include "CTaskUsageTests.h"
#include <cstdlib>
#include <algorithm>
#include <climits>
#include <iostream>
#include <set>
#include <stdexcept>
#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

/** 中文场景集中登记；与稳定 ID、实际用例和本头文件说明共用同一顺序
*/
static const std::map<int32_t, CTaskTestCaseDescription> g_taskCaseDescriptions =
{
    // 1 生命周期
    {CTaskTestLifecycle, {"lifecycle", "生命周期", "创建、查询及注销线程；ID 不重复，无效 ID 安全，最终注册数为零。"}},
    // 2 非法投递边界
    {CTaskTestInvalidPost, {"invalid-post", "非法投递边界", "空任务和非正优先级被拒绝，INT_MAX 优先级可执行，失败不得接纳。"}},
    // 3 优先级与同级先进先出
    {CTaskTestPriorityFifoSnapshots, {"priority-fifo-snapshots", "优先级与同级先进先出", "门控积压 3、2、1 级任务；按 3>2>1 执行，同级 FIFO，快照数量准确。"}},
    // 4 重做插在原级队首
    {CTaskTestRedoFront, {"redo-front", "重做插在原级队首", "低级任务被高优先级打断；高任务先执行，原级重做先于同级积压。"}},
    // 5 合并并发重做意图
    {CTaskTestRedoOnce, {"redo-once", "合并并发重做意图", "连续高优先级投递打断同一代；只生成一份重做任务。"}},
    // 6 禁止重做的任务
    {CTaskTestNoRedo, {"no-redo", "禁止重做的任务", "reExecute 返回 false；收到通知后不重放，后续任务正常。"}},
    // 7 空重做工厂结果
    {CTaskTestNullClone, {"null-clone", "空重做工厂结果", "工厂返回空指针；不生成重做，也不丢失新投递任务。"}},
    // 8 同级及低级不抢占
    {CTaskTestEqualLowerNoPreempt, {"equal-lower-no-preempt", "同级及低级不抢占", "当前任务在门控中，投递同级和更低级；不得产生抢占通知。"}},
    // 9 按属性及级别停止
    {CTaskTestStopPropertyLevel, {"stop-property-level", "按属性及级别停止", "队列含重复指针和不同属性；只通知匹配项，同一指针只通知一次。"}},
    // 10 跨级按属性停止
    {CTaskTestStopPropertyAllLevels, {"stop-property-all-levels", "跨级按属性停止", "同一属性分布在多个级别；全部匹配项收到通知，其他属性不受影响。"}},
    // 11 停止当前任务
    {CTaskTestStopCurrent, {"stop-current", "停止当前任务", "执行体仍在运行时请求停止；收到合作停止通知而非强行终止线程。"}},
    // 12 停止全部任务
    {CTaskTestStopAll, {"stop-all", "停止全部任务", "运行及排队任务都收到通知；排队项不因停止通知被直接删除。"}},
    // 13 停止回调重入
    {CTaskTestStopCallbackReentry, {"stop-callback-reentry", "停止回调重入", "stopTask 回调再次查询、投递；不得死锁，后续任务完成。"}},
    // 14 注销与过期句柄
    {CTaskTestUninitAndStaleHandle, {"uninit-and-stale-handle", "注销与过期句柄", "运行中关闭并丢弃积压；线程退出，旧句柄投递被拒绝，不泄漏任务。"}},
    // 15 多生产者先进先出
    {CTaskTestMultiProducerFifo, {"multi-producer-fifo", "多生产者先进先出", "并发普通投递；全部已接纳项恰执行一次，每个生产者同级 FIFO。"}},
    // 16 尝试投递竞争
    {CTaskTestTryPostContention, {"try-post-contention", "尝试投递竞争", "try 投递只重试未接纳项；无重复、遗漏，已提交项不得重发。"}},
    // 17 多线程派生分发
    {CTaskTestLegacy20ThreadDispatch, {"legacy-20-thread-dispatch", "多线程派生分发", "20 条工作线程、10 类任务有界派生 200 个子任务；全部完成并退出。"}},
    // 18 异步投递后显式排空
    {CTaskTestPostAndExplicitDrain, {"post-and-explicit-drain", "异步投递后显式排空", "投递后调用排空接口；返回时已接纳任务全部完成。"}},
    // 19 批量注销
    {CTaskTestBulkUninitAll, {"bulk-uninit-all", "批量注销", "20 条运行线程批量停止并 join；管理器最终为空。"}},
    // 20 空闲唤醒
    {CTaskTestIdleWakeup, {"idle-wakeup", "空闲唤醒", "100 次空闲转工作；每轮都能及时执行，无丢失唤醒。"}},
    // 21 反复生命周期与所有权
    {CTaskTestRepeatedLifecycleOwnership, {"repeated-lifecycle-ownership", "反复生命周期与所有权", "100 次创建、投递和注销；任务存活数及注册数回到零。"}},
    // 22 普通投递压力
    {CTaskTestProducerStress, {"producer-stress", "普通投递压力", "8 个生产者各 5000 项；精确数量，无重复、遗漏和同级倒序。"}},
    // 23 尝试投递压力
    {CTaskTestTryPostStress, {"try-post-stress", "尝试投递压力", "8 个生产者各 2000 个成功项；未提交才重试，精确交付。"}},
    // 24 反复抢占重做
    {CTaskTestRedoRepeat, {"redo-repeat", "反复抢占重做", "50 轮受控抢占；每轮原级重做及同级队列顺序正确。"}},
    // 25 固定种子优先级模型
    {CTaskTestSeededPriorityModel, {"seeded-priority-model", "固定种子优先级模型", "种子 0x5a17、2048 项；执行序列与独立稳定优先级排序相同。"}},
    // 26 独立工作线程进展
    {CTaskTestIndependentWorkers, {"independent-workers", "独立工作线程进展", "阻塞一条工作线程；另一条仍能完成自己的任务。"}},
    // 27 停止查询与完成竞争
    {CTaskTestStopCompletionRace, {"stop-completion-race", "停止查询与完成竞争", "4 个生产者共 2000 项与停止、查询并发；完整交付且无崩溃。"}},
    // 28 配置门控与系统优先级
    {CTaskTestModelsGatesNativePriority, {"models-gates-native-priority", "配置门控与系统优先级", "配置验证、并发接纳及实际 OS 线程优先级；非法参数不破坏状态。"}},
    // 29 有界任务接纳
    {CTaskTestBoundedTaskAdmission, {"bounded-task-admission", "有界任务接纳", "运行项及在途项共同占用硬上限；拒绝的新项不得触发抢占。"}},
    // 30 业务异常隔离
    {CTaskTestCallbackFailureIsolation, {"callback-failure-isolation", "业务异常隔离", "执行及工厂抛异常、工厂返回自身；工作线程继续，不重复释放。"}},
    // 31 回调内自等待
    {CTaskTestCallbackSelfWait, {"callback-self-wait", "回调内自等待", "回调直接、经管理器或批量 join 自己；明确 SelfWait，不死锁。"}},
    // 32 运行期系统优先级
    {CTaskTestRuntimeSystemPriority, {"runtime-system-priority", "运行期系统优先级", "创建及运行期设置，失败反馈和关闭句柄拒绝；不伪报设置成功。"}},
    // 33 动态线程池与分配器
    {CTaskTestDynamicPoolAllocator, {"dynamic-pool-allocator", "动态线程池与分配器", "虚策略返回规模，内部裁剪最小最大值；扩缩容、分配和排空正确。"}},
    // 34 任务序号耗尽
    {CTaskTestWhiteTaskSequence, {"white-task-sequence", "任务序号耗尽", "白盒安排 UINT64_MAX 边界，最后合法序号成功，后续 SequenceExhausted；不回绕或泄漏额度。"}},
    // 35 管理器序号耗尽
    {CTaskTestWhiteManagerSequence, {"white-manager-sequence", "管理器序号耗尽", "白盒安排 UINT32_MAX 边界，最后 ID 可创建，耗尽及注销后均不复用 ID。"}},
    // 36 帮助关闭优先级桶
    {CTaskTestWhiteHelpClosedBucket, {"white-help-closed-bucket", "帮助关闭优先级桶", "桶关闭与访问交错；可继续回收，不保留坏死桶或丢任务。"}},
    // 37 迟到抢占通知隔离
    {CTaskTestWhiteLatePreemption, {"white-late-preemption", "迟到抢占通知隔离", "已完成投递的抢占通知迟到；不得停止后来无关的低优先级任务。"}},
    // 38 读保护耗尽及恢复
    {CTaskTestWhiteProtectionExhaustion, {"white-protection-exhaustion", "读保护耗尽及恢复", "保护记录耗尽；错误语义正确，归还后恢复正常操作。"}},
    // 39 初始化分配故障扫点
    {CTaskTestFaultInitAllocationSweep, {"fault-init-allocation-sweep", "初始化分配故障扫点", "逐个真实分配位置注入失败；部分初始化回滚，清理后可重建。"}},
    // 40 热投递分配故障扫点
    {CTaskTestFaultHotPostAllocationSweep, {"fault-hot-post-allocation-sweep", "热投递分配故障扫点", "热级别投递逐点故障；提交结果、所有权和后续执行一致。"}},
    // 41 冷投递分配故障扫点
    {CTaskTestFaultColdPostAllocationSweep, {"fault-cold-post-allocation-sweep", "冷投递分配故障扫点", "首次级别创建逐点故障；未发布项不得残留或触发抢占。"}},
    // 42 未发布级别不能抢占
    {CTaskTestWhiteUnpublishedPriority, {"white-unpublished-priority", "未发布级别不能抢占", "新高优先级创建失败；运行任务不被未接纳请求打断。"}},
    // 43 有限级别容量复用
    {CTaskTestWhiteBoundedPriorityReuse, {"white-bounded-priority-reuse", "有限级别容量复用", "有限容量反复切换 20 个优先级；已释放容量可复用，桶不无限残留。"}},
    // 44 投递与排空竞争
    {CTaskTestPostDrainRace, {"post-drain-race", "投递与排空竞争", "生产者仍在投递时关闭排空；每个成功接纳项恰执行一次。"}},
    // 45 并发等待同一线程
    {CTaskTestConcurrentJoin, {"concurrent-join", "并发等待同一线程", "两个调用者同时 join；完成时均成功，不对外返回 Busy 或 Retry。"}},
    // 46 并发线程池关闭
    {CTaskTestConcurrentPoolShutdown, {"concurrent-pool-shutdown", "并发线程池关闭", "排空和丢弃关闭交错；请求能完成，无自旋死等或残留工作线程。"}},
    // 47 流量下线程池扩缩容
    {CTaskTestPoolResizeTraffic, {"pool-resize-traffic", "流量下线程池扩缩容", "10 波共 10000 项、规模 2 到 6；缩容不丢任务，不低于初始值。"}},
    // 48 脱离心跳的唤醒
    {CTaskTestWakeWithoutHeartbeat, {"wake-without-heartbeat", "脱离心跳的唤醒", "心跳设为 60 秒，100 次空闲突发仍在短等待窗口内完成。"}},
    // 49 持续有界压力
    {CTaskTestSustainedPressure, {"sustained-pressure", "持续有界压力", "默认 60 秒、8 生产者、4096 容量；已接纳项精确交付，退出后资源回落。"}},
    // 50 管理器遍历保护耗尽
    {CTaskTestManagerTraversalExhaustion, {"manager-traversal-exhaustion", "管理器遍历保护耗尽", "遍历保护占满时执行接口；反馈正确，释放保护后可恢复。"}},
    // 51 嵌套停止回调自等待
    {CTaskTestNestedStopSelfWait, {"nested-stop-self-wait", "嵌套停止回调自等待", "受控 A 到 B 再到 A 的回调；识别自等待，不形成闭环死锁。"}},
    // 52 线程池部分初始化回滚
    {CTaskTestPartialPoolInitRollback, {"partial-pool-init-rollback", "线程池部分初始化回滚", "中途资源不足；已建线程全部收回，失败池不残留注册项。"}},
    // 53 单例预热后并发访问
    {CTaskTestSingletonAfterPrewarm, {"singleton-after-prewarm", "单例预热后并发访问", "启动线程串行预热，再并发访问；单例身份一致且无重复构造。"}},
    // 54 属性 ID 边界
    {CTaskTestPropertyIdentifiers, {"property-identifiers", "属性 ID 边界", "默认、重复、零、负数及极值；按属性停止符合非唯一 ID 语义。"}},
    // 55 并发重做顺序
    {CTaskTestConcurrentRedoOrdering, {"concurrent-redo-ordering", "并发重做顺序", "8 个线程请求重做、20 轮；代次不重复，高级先于原级重做。"}},
    // 56 优先级缓存回退
    {CTaskTestCachedPriorityFallback, {"cached-priority-fallback", "优先级缓存回退", "17 级及 INT_MAX 等超出快路径的级别；正确查询、抢占和执行。"}},
    // 57 分离计数器回绕
    {CTaskTestSplitCountersWrap, {"split-counters-wrap", "分离计数器回绕", "UINT64 边界的生产消费计数；净数量及收尾状态正确。"}},
    // 58 有界并发统计
    {CTaskTestConcurrentBoundedStatistics, {"concurrent-bounded-statistics", "有界并发统计", "4 个生产者、容量 3、20000 项；逻辑上限、提交和完成计数一致。"}},
    // 59 读域寿命及延迟回收
    {CTaskTestV3ReadDomain, {"v3-read-domain", "读域寿命及延迟回收", "读租约持有时关闭或回收；不能提前释放，租约结束后可回收。"}},
    // 60 稳定任务槽位池
    {CTaskTestV3TaskStorage, {"v3-task-storage", "稳定任务槽位池", "容量、持有引用、扩缩容及并发分配；槽位不重用在途对象。"}},
    // 61 槽位失败与缩容边界
    {CTaskTestV3StorageFailure, {"v3-storage-failure", "槽位失败与缩容边界", "分配失败、策略拒绝或异常、旧读者暂停；回滚并守住初始容量。"}},
    // 62 队列跨段就绪
    {CTaskTestV3QueueReady, {"v3-queue-ready", "队列跨段就绪", "跨段 FIFO、空检查及显式回收；在途上下文不被释放，静止数量归零。"}},
    // 63 最高优先级发布竞争
    {CTaskTestV3ReadyIndex, {"v3-ready-index", "最高优先级发布竞争", "8 生产者、16 级、160000 次发布和消费下调交错；最高级提示不漏项。"}},
    // 64 队列分布计数压力
    {CTaskTestV3QueueCounters, {"v3-queue-counters", "队列分布计数压力", "独立生产者上下文、有界池与同步消费；精确计数、同线程 FIFO、收尾归零。"}},
    // 65 有界前缀与异常提交
    {CTaskTestV3QueueBatch, {"v3-queue-batch", "有界前缀与异常提交", "0 预算、最大 16 项、跨段和第二次赋值异常；只提交成功前缀。"}},
    // 66 预取后抢占重做停止
    {CTaskTestPrefetchedPriorityRedoStop, {"prefetched-priority-redo-stop", "预取后抢占重做停止", "已预取同级任务再来高级任务；维持优先级、重做队首及属性停止覆盖。"}},
    // 67 高级任务即时唤醒
    {CTaskTestPriorityWakeWithoutHeartbeat, {"priority-wake-without-heartbeat", "高级任务即时唤醒", "200 次高级空闲突发、60 秒心跳；不能依赖心跳才能执行。"}},
    // 68 外部队列条目
    {CTaskTestExternalQueueEntry, {"external-queue-entry", "外部队列条目", "借用寿命、混合 FIFO、异常前缀和并发交付；无提前释放或重复消费。"}},
    // 69 Map 缓冲维护
    {CTaskTestMapBufferMaintenance, {"map-buffer-maintenance", "Map 缓冲维护", "并发维护、查询和释放复用缓冲；键值不丢失，安全回收。"}},
    // 70 性能校验器自检
    {CTaskTestPreparedBenchmarkValidator, {"prepared-benchmark-validator", "性能校验器自检", "注入数量相等的重复及缺失、级内乱序；必须拒绝伪正确样本。"}},
    // 71 外部条目额度压力
    {CTaskTestExternalEntryQuota, {"external-entry-quota", "外部条目额度压力", "额度、扩缩容、失败、寿命和并发 FIFO；前缀预算生效，资源可归还。"}},
    // 72 延迟重做工厂顺序
    {CTaskTestLazyRedoFactoryOrder, {"lazy-redo-factory-order", "延迟重做工厂顺序", "普通执行不建副本，仅真正重做才取参数创建；原级重做先于同级积压。"}},
    // 73 延迟工厂并发合并
    {CTaskTestLazyRedoFactoryConcurrency, {"lazy-redo-factory-concurrency", "延迟工厂并发合并", "8 个请求同时抢占同一代；只创建一次，继承工厂可再次重做。"}},
    // 74 延迟工厂失败
    {CTaskTestLazyRedoFactoryFailure, {"lazy-redo-factory-failure", "延迟工厂失败", "普通及分配异常、空或自身返回、关闭和禁重做；后续任务可执行且不泄漏。"}},
    // 75 工厂配置与记录资源
    {CTaskTestLazyRedoFactoryResources, {"lazy-redo-factory-resources", "工厂配置与记录资源", "配置复制失败保留旧值，继承失败释放新任务，记录耗尽反馈正确。"}},
    // 76 工厂控制块分配失败
    {CTaskTestLazyRedoFactoryAllocation, {"lazy-redo-factory-allocation", "工厂控制块分配失败", "接管返回裸指针时失败；对象必须释放，旧配置及后续任务不受破坏。"}},
    // 77 工厂与关闭生命周期
    {CTaskTestLazyRedoFactoryLifecycle, {"lazy-redo-factory-lifecycle", "工厂与关闭生命周期", "子任务自有工厂优先；在途工厂返回时已关闭，不得再提交或泄漏。"}},
    // 78 待重做项与高级任务
    {CTaskTestV5PendingRedoPriority, {"v5-pending-redo-priority", "待重做项与高级任务", "原级重做已准备但尚未执行时来高级任务；高级先，原级同级 FIFO。"}},
    // 79 嵌套抢占
    {CTaskTestV5NestedPreemption, {"v5-nested-preemption", "嵌套抢占", "两个优先级先后被打断；各在原级重做一次，最高级优先。"}},
    // 80 已排队的更高级任务
    {CTaskTestV5QueuedHigherPriority, {"v5-queued-higher-priority", "已排队的更高级任务", "高级项已在队列后续位置；重做不能越过它。"}},
    // 81 工厂发布竞争
    {CTaskTestV5FactoryPublicationRace, {"v5-factory-publication-race", "工厂发布竞争", "工厂执行过程中再投递高级或丢弃关闭；顺序和任务所有权正确。"}},
    // 82 多代连续重做
    {CTaskTestV5RepeatedRedo, {"v5-repeated-redo", "多代连续重做", "64 个继承工厂代次；延迟构造、原级和执行顺序正确。"}},
    // 83 重做成本基准
    {CTaskTestV5RedoPerformance, {"v5-redo-performance", "重做成本基准", "百万普通执行与 100 次合作打断成对计时；工厂次数准确，同时输出开销。"}},
    // 84 首次高级发布与交接
    {CTaskTestV5FirstPriorityRace, {"v5-first-priority-race", "首次高级发布与交接", "第一个高级桶发布恰遇当前任务交接；不能丢抢占或执行项。"}},
    // 85 关闭与最高级交接
    {CTaskTestV5ShutdownHandoff, {"v5-shutdown-handoff", "关闭与最高级交接", "丢弃关闭恰遇高级任务交接；运行任务不能漏 stopTask。"}},
    // 86 回收批次帮助
    {CTaskTestV5ReclaimBatches, {"v5-reclaim-batches", "回收批次帮助", "回收邮箱满且帮助者暂停；其他批次仍能推进，无重复释放。"}},
    // 87 当前任务属性停止
    {CTaskTestV5CurrentPropertyStop, {"v5-current-property-stop", "当前任务属性停止", "当前项属性匹配不受队列级别过滤影响；排队匹配通知去重且不删除。"}},
    // 88 多线程并发注销
    {CTaskTestV5ConcurrentRemoval, {"v5-concurrent-removal", "多线程并发注销", "16 个不同 ID 同时 join；成功返回的线程不再残留注册表。"}},
    // 89 重入读保护耗尽
    {CTaskTestReentrantProtectionExhaustion, {"reentrant-protection-exhaustion", "重入读保护耗尽", "回调占最后保护记录时重入；返回 SelfWait，退出回调后恢复。"}},
    // 90 投递结果契约
    {CTaskTestPostResultContract, {"post-result-contract", "投递结果契约", "逻辑及物理容量、非法或关闭、try 竞争分别判定；普通 post 不返回竞争错误。"}},
    // 91 普通投递竞争契约
    {CTaskTestPostContentionContract, {"post-contention-contract", "普通投递竞争契约", "32 生产者正常投递；内部消化竞争，不外露 Busy 或 Retry，交付精确。"}},
    // 92 动态额度归还竞争
    {CTaskTestDynamicCreditReturnRace, {"dynamic-credit-return-race", "动态额度归还竞争", "100 轮默认小池突发，每轮 8 乘 5000；可归还或扩容，FIFO 和总数准确。"}},
    // 93 原生测试调度器自检
    {CTaskTestNativeAutomationSelfTest, {"native-automation-self-test", "原生测试调度器自检", "合法及损坏 CSV、中位数、比较比例、非法 ID、超时和自有子进程清理；拒绝坏报告。"}},
    // 94 公开用法之优先级
    {CTaskTestUsagePriority, {"usage-priority", "公开用法之优先级", "使用例演示投递、查询和级别排序；独立账本核对并正常退出。"}},
    // 95 公开用法之重做
    {CTaskTestUsageRedo, {"usage-redo", "公开用法之重做", "注册延迟工厂并受控打断；高任务和原级重做顺序准确。"}},
    // 96 公开用法之停止
    {CTaskTestUsageStopping, {"usage-stopping", "公开用法之停止", "当前、按属性和全部停止；合作停止及不删除队列语义成立。"}},
    // 97 公开用法之资源
    {CTaskTestUsageResources, {"usage-resources", "公开用法之资源", "设置 Map、Queue 和任务容量；失败反馈、快照及资源恢复正确。"}},
    // 98 公开用法之生命周期
    {CTaskTestUsageLifecycle, {"usage-lifecycle", "公开用法之生命周期", "创建、查询、排空、关闭和旧句柄；无悬挂或已关闭误接纳。"}},
    // 99 公开用法之并发
    {CTaskTestUsageConcurrent, {"usage-concurrent", "公开用法之并发", "多线程公开接口投递；执行数量和每生产者顺序正确。"}},
    // 100 公开用法之线程池
    {CTaskTestUsagePool, {"usage-pool", "公开用法之线程池", "公开策略、线程池和分配器；扩缩容下限、选择线程及关闭成立。"}},
    // 101 公平性四生产者一级
    {CTaskTestFairness41, {"fairness-4-1", "公平性四生产者一级", "4 生产者、1 级、不限制 CPU，队列生产者记录 64。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 102 公平性四生产者十级
    {CTaskTestFairness410, {"fairness-4-10", "公平性四生产者十级", "4 生产者、10 级、不限制 CPU，队列生产者记录 64。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 103 公平性八生产者一级
    {CTaskTestFairness81, {"fairness-8-1", "公平性八生产者一级", "8 生产者、1 级、不限制 CPU，队列生产者记录 64。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 104 公平性八生产者十级
    {CTaskTestFairness810, {"fairness-8-10", "公平性八生产者十级", "8 生产者、10 级、不限制 CPU，队列生产者记录 64。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 105 公平性十六生产者一级
    {CTaskTestFairness161, {"fairness-16-1", "公平性十六生产者一级", "16 生产者、1 级、不限制 CPU，队列生产者记录 64。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 106 公平性十六生产者十级
    {CTaskTestFairness1610, {"fairness-16-10", "公平性十六生产者十级", "16 生产者、10 级、不限制 CPU，队列生产者记录 64。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 107 公平性三十二生产者一级
    {CTaskTestFairness321, {"fairness-32-1", "公平性三十二生产者一级", "32 生产者、1 级、不限制 CPU，队列生产者记录 64。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 108 公平性三十二生产者十级
    {CTaskTestFairness3210, {"fairness-32-10", "公平性三十二生产者十级", "32 生产者、10 级、不限制 CPU，队列生产者记录 64。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 109 公平性四核三十二生产者一级
    {CTaskTestFairness321Cpu4, {"fairness-32-1-cpu4", "公平性四核三十二生产者一级", "32 生产者、1 级、4 CPU，队列生产者记录 64。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 110 公平性四核三十二生产者十级
    {CTaskTestFairness3210Cpu4, {"fairness-32-10-cpu4", "公平性四核三十二生产者十级", "32 生产者、10 级、4 CPU，队列生产者记录 64。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 111 公平性四核六十四生产者一级
    {CTaskTestFairness641Cpu4, {"fairness-64-1-cpu4", "公平性四核六十四生产者一级", "64 生产者、1 级、4 CPU，队列生产者记录 64。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 112 公平性四核六十四生产者十级
    {CTaskTestFairness6410Cpu4, {"fairness-64-10-cpu4", "公平性四核六十四生产者十级", "64 生产者、10 级、4 CPU，队列生产者记录 64。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 113 公平性单核八生产者一级
    {CTaskTestFairness81Cpu1, {"fairness-8-1-cpu1", "公平性单核八生产者一级", "8 生产者、1 级、1 CPU，队列生产者记录 64。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 114 公平性单核八生产者十级
    {CTaskTestFairness810Cpu1, {"fairness-8-10-cpu1", "公平性单核八生产者十级", "8 生产者、10 级、1 CPU，队列生产者记录 64。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 115 单记录公平性八生产者十级
    {CTaskTestFairnessSingleRecord810, {"fairness-single-record-8-10", "单记录公平性八生产者十级", "8 生产者、10 级、4 CPU，仅 1 个队列生产者记录。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 116 单记录公平性三十二生产者一级
    {CTaskTestFairnessSingleRecord321, {"fairness-single-record-32-1", "单记录公平性三十二生产者一级", "32 生产者、1 级、4 CPU，仅 1 个队列生产者记录。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 117 单记录公平性三十二生产者十级
    {CTaskTestFairnessSingleRecord3210, {"fairness-single-record-32-10", "单记录公平性三十二生产者十级", "32 生产者、10 级、4 CPU，仅 1 个队列生产者记录。 每线程至少1000项、FIFO、无外露竞争、进展间隔小于2000ms。"}},
    // 118 值队列空扫后归还
    {CTaskTestQueueEmptyScanReturnValue, {"queue-empty-scan-return-value", "值队列空扫后归还", "私有源码夹具暂停空扫描，另一线程归还可用块；正确接纳并保持 FIFO。"}},
    // 119 条目队列空扫后归还
    {CTaskTestQueueEmptyScanReturnEntry, {"queue-empty-scan-return-entry", "条目队列空扫后归还", "外部条目路径在空扫描后归还额度；已可用资源不能误判耗尽。"}},
    // 120 部分归还发布竞争
    {CTaskTestQueuePartialReturnPublication, {"queue-partial-return-publication", "部分归还发布竞争", "归还数量与可见发布交错；必须看到真实归还，最终 FIFO 及空状态正确。"}},
};

std::atomic<int> TestTask::live(0);
std::atomic<int> TestTask::errors(0);
TestTask::TestTask(int propertyId, const std::function<void(TestTask&)>& body) :
CTask(propertyId),
stopped(false),
stops(0),
m_body(body)
{
    ++live;
}

TestTask::~TestTask()
{
    --live;
}

void TestTask::doTask()
{
    try
    {
        m_body(*this);
    }
    catch (const std::exception& error)
    {
        ++errors;
        std::cerr << "task: " << error.what() << "\n";
    }
    catch (...)
    {
        ++errors;
    }
}

void TestTask::stopTask()
{
    stopped = true;
    ++stops;
    try
    {
        if (onStop)
        {
            onStop();
        }
    }
    catch (...)
    {
        ++errors;
    }
}

TestWorkers::~TestWorkers()
{
    for (size_t i = 0; i < ids.size(); ++i)
    {
        try
        {
            CTaskThreadManager::instance().uninit(ids[i]);
        }
        catch (...)
        {
            ++TestTask::errors;
        }
    }
}

std::shared_ptr<CTaskThread> TestWorkers::add()
{
    const uint32_t id = CTaskThreadManager::instance().init();
    bbCheck(id != 0, "init returned invalid ID");
    ids.push_back(id);
    std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
    bbCheck(worker != nullptr, "init has no interface");
    return worker;
}

void TestWorkers::drain()
{
    for (size_t i = 0; i < ids.size(); ++i)
    {
        const uint32_t id = ids[i];
        const CTaskThreadResult result = CTaskThreadManager::instance().waitForEnd(id);
        bbCheck(result.m_status == CTaskThreadOk || result.m_status == CTaskThreadNotFound,
            "ordinary drain failed or leaked contention");
    }
    ids.clear();
    bbCheck(TestTask::errors == 0, "worker fixture reported an error");
}

void TaskLedger::add(int value)
{
    std::lock_guard<std::mutex> lock(mutex);
    values.push_back(value);
}

std::vector<int> TaskLedger::snapshot()
{
    std::lock_guard<std::mutex> lock(mutex);
    return values;
}

RedoState::RedoState() :
entered(false),
release(false),
stops(0)
{

}

RedoTask::RedoTask(const std::shared_ptr<RedoState>& state, int generation, bool redo, bool clone) :
CTask(700),
m_state(state),
m_generation(generation),
m_redo(redo),
m_clone(clone)
{
    this->clone([state, generation, redo, clone]() -> CTask*
    {
        return clone ? new RedoTask(state, generation + 1, redo, clone) : nullptr;
    });
}

void RedoTask::doTask()
{
    try
    {
        m_state->ledger.add(100 + m_generation);
        if (!m_generation)
        {
            m_state->entered = true;
            bbWait([this]()
            {
                return m_state->release.load();
            }, "release original task");
        }
    }
    catch (...)
    {
        ++TestTask::errors;
    }
}

void RedoTask::stopTask()
{
    ++m_state->stops;
}

bool RedoTask::reExecute()
{
    return m_redo;
}

/** 创建向顺序账本追加标记的任务
@param [in,out] ledger 生命周期覆盖任务执行的账本
@param [in] value 执行标记
@param [in] propertyId 非唯一属性编号
@return 新任务强引用
*/
static std::shared_ptr<TestTask> Record(TaskLedger& ledger, int value, int propertyId = 1)
{
    return std::make_shared<TestTask>(propertyId, [&ledger, value](TestTask&)
    {
        ledger.add(value);
    });
}

/** 创建并等待进入门控任务
@param [in] worker 非空目标线程
@param [in,out] entered 进入通知，借用至任务结束
@param [in,out] release 放行标志，借用至任务结束
@param [in] level 正数任务优先级
@param [in] propertyId 非唯一属性编号
@return 已进入执行体的任务强引用
*/
static std::shared_ptr<TestTask> Block(const std::shared_ptr<CTaskThread>& worker,
    std::atomic<bool>& entered, std::atomic<bool>& release, int level = INT_MAX, int propertyId = 999)
{
    std::shared_ptr<TestTask> task = std::make_shared<TestTask>(propertyId, [&entered, &release](TestTask&)
    {
        entered = true;
        bbWait([&release]()
        {
            return release.load();
        }, "release task gate");
    });
    worker->postTask(task, level);
    bbWait([&entered]()
    {
        return entered.load();
    }, "task gate entered");
    return task;
}

/** 验证线程创建、查询、注销和重复关闭
*/
static void Lifecycle()
{
    CTaskThreadManager& manager = CTaskThreadManager::instance();
    bbCheck(manager.count() == 0, "fresh manager not empty");
    bbCheck(!manager.getThreadInterface(0) && !manager.getThreadInterface(UINT32_MAX), "unknown ID has interface");
    manager.uninit(0);
    manager.waitForEnd(UINT32_MAX);
    manager.uninitAll();
    TestWorkers workers;
    std::shared_ptr<CTaskThread> a = workers.add(), b = workers.add();
    bbCheck(workers.ids[0] != workers.ids[1] && manager.count() == 2, "IDs/count invalid");
    bbCheck(!a->hasTask() && a->getWaitTaskCount() == 0 && a->getCurTaskLevel() == 0, "idle snapshot invalid");
    manager.uninit(workers.ids[0]);
    manager.uninit(workers.ids[0]);
    bbCheck(!manager.getThreadInterface(workers.ids[0]) && manager.count() == 1, "uninit did not unregister");
    workers.drain();
    bbCheck(manager.count() == 0, "drain did not unregister");
}

/** 验证空任务、非法优先级拒绝与最高合法优先级
*/
static void InvalidPost()
{
    TaskLedger ledger;
    TestWorkers workers;
    std::shared_ptr<CTaskThread> w = workers.add();
    std::shared_ptr<TestTask> task = Record(ledger, 1);
    w->postTask(nullptr);
    w->postTask(task, 0);
    w->postTask(task, -1);
    bbCheck(!w->tryPostTask(nullptr) && !w->tryPostTask(task, 0) && !w->tryPostTask(task, INT_MIN), "invalid TryPost accepted");
    w->postTask(task, INT_MAX);
    workers.drain();
    bbCheck(ledger.snapshot() == std::vector<int>(1, 1), "invalid tasks executed or maximum priority lost");
}

/** 验证优先级选择、同级先进先出和等待计数
*/
static void PriorityFifo()
{
    TaskLedger ledger;
    std::atomic<bool> entered(false), release(false);
    TestWorkers workers;
    std::shared_ptr<CTaskThread> w = workers.add();
    Block(w, entered, release);
    const int levels[] = {1, 3, 2, 3, 1, 2};
    for (int i = 0; i < 6; ++i)
    {
        w->postTask(Record(ledger, i + 1), levels[i]);
    }
    std::map<int32_t, int32_t> info;
    info[-10] = 99;
    w->getWaitTaskInfo(info);
    bbCheck(info.size() == 3 && info[1] == 2 && info[2] == 2 && info[3] == 2, "wait info not exact/old entry not cleared");
    bbCheck(w->hasTask() && w->getWaitTaskCount() == 6 && w->getCurTaskLevel() == INT_MAX, "current included in pending count");
    release = true;
    workers.drain();
    const int expected[] = {2, 4, 3, 6, 1, 5};
    bbCheck(ledger.snapshot() == std::vector<int>(expected, expected + 6), "highest priority / same-level FIFO violated");
    info[42] = 9;
    w->getWaitTaskInfo(info);
    bbCheck(info.empty() && !w->hasTask() && !w->getWaitTaskCount(), "queue did not drain");
}

/** 验证高优先级抢占后的重做与原队列相对顺序
@param [in] redo 原任务是否要求重做
@param [in] clone 工厂是否生成下一代
@param [in] repeated 是否连续发布多条高优先级任务
*/
static void Preempt(bool redo, bool clone, bool repeated)
{
    std::shared_ptr<RedoState> state = std::make_shared<RedoState>();
    TestWorkers workers;
    std::shared_ptr<CTaskThread> w = workers.add();
    w->postTask(std::make_shared<RedoTask>(state, 0, redo, clone), 1);
    bbWait([state]()
    {
        return state->entered.load();
    }, "original started");
    w->postTask(Record(state->ledger, 11), 1);
    w->postTask(Record(state->ledger, 21), 2);
    if (repeated)
    {
        w->postTask(Record(state->ledger, 22), 2);
        w->postTask(Record(state->ledger, 31), 3);
    }
    bbCheck(state->stops > 0, "higher priority did not request stop");
    bbCheck(state->ledger.snapshot().size() == 1, "cooperative preemption ran next before original returned");
    state->release = true;
    workers.drain();
    std::vector<int> expected(1, 100);
    if (repeated)
    {
        expected.push_back(31);
    }
    expected.push_back(21);
    if (repeated)
    {
        expected.push_back(22);
    }
    if (redo && clone)
    {
        expected.push_back(101);
    }
    expected.push_back(11);
    bbCheck(state->ledger.snapshot() == expected, "redo duplicated/lost or wrong original priority/front position");
}

/** 验证同级投递不会通知当前任务停止
*/
static void EqualDoesNotStop()
{
    TaskLedger ledger;
    std::atomic<bool> entered(false), release(false);
    TestWorkers workers;
    std::shared_ptr<CTaskThread> w = workers.add();
    std::shared_ptr<TestTask> current = Block(w, entered, release, 2);
    w->postTask(Record(ledger, 1), 2);
    w->postTask(Record(ledger, 2), 1);
    bbCheck(current->stops == 0, "equal/lower priority preempted current");
    release = true;
    workers.drain();
    const int expected[] = {1, 2};
    bbCheck(ledger.snapshot() == std::vector<int>(expected, expected + 2), "equal/lower order wrong");
}

/** 验证按属性和优先级筛选停止通知
@param [in] allLevels 是否不限制等待任务优先级
*/
static void StopFiltered(bool allLevels)
{
    TaskLedger ledger;
    std::atomic<bool> entered(false), release(false);
    TestWorkers workers;
    std::shared_ptr<CTaskThread> w = workers.add();
    Block(w, entered, release);
    std::shared_ptr<TestTask> a = Record(ledger, 1, 77), b = Record(ledger, 2, 77), c = Record(ledger, 3, 88);
    w->postTask(a, 1);
    w->postTask(a, 1);
    w->postTask(b, 2);
    w->postTask(c, 1);
    w->stopTask(77, allLevels ? 0 : 1);
    bbCheck(a->stops == 1 && b->stops == (allLevels ? 1 : 0) && c->stops == 0, "stopTask selection/dedup wrong");
    bbCheck(w->getWaitTaskCount() == 4, "stopTask silently removed queued tasks");
    release = true;
    workers.drain();
    bbCheck(ledger.snapshot().size() == 4, "stopTask changed execution contract");
}

/** 验证当前任务停止通知及空闲调用
*/
static void StopCurrent()
{
    std::atomic<bool> entered(false), stopped(false);
    TestWorkers workers;
    std::shared_ptr<CTaskThread> w = workers.add();
    std::shared_ptr<TestTask> task = std::make_shared<TestTask>(4, [&entered, &stopped](TestTask& task)
    {
        entered = true;
        bbWait([&task]()
        {
            return task.stopped.load();
        }, "current stopTask");
        stopped = true;
    });
    w->postTask(task);
    bbWait([&entered]()
    {
        return entered.load();
    }, "current started");
    w->stopCurTask();
    workers.drain();
    bbCheck(stopped && task->stops > 0, "current did not observe stop");
}

/** 验证全部任务停止通知覆盖当前任务和等待任务
*/
static void StopAll()
{
    TaskLedger ledger;
    std::atomic<bool> entered(false), release(false);
    TestWorkers workers;
    std::shared_ptr<CTaskThread> w = workers.add();
    std::shared_ptr<TestTask> current = Block(w, entered, release);
    std::vector<std::shared_ptr<TestTask>> tasks;
    for (int i = 0; i < 10; ++i)
    {
        tasks.push_back(Record(ledger, i));
        w->postTask(tasks.back(), i + 1);
    }
    w->stopAllTask();
    bbCheck(current->stops > 0, "StopAll missed current");
    for (size_t i = 0; i < tasks.size(); ++i)
    {
        bbCheck(tasks[i]->stops > 0, "StopAll missed queued task");
    }
    release = true;
    workers.drain();
    bbCheck(ledger.snapshot().size() == 10, "StopAll unexpectedly removed tasks");
}

/** 验证停止回调重入查询和投递不会死锁
*/
static void StopReentry()
{
    TaskLedger ledger;
    std::atomic<bool> entered(false), release(false);
    TestWorkers workers;
    std::shared_ptr<CTaskThread> w = workers.add();
    Block(w, entered, release);
    std::shared_ptr<TestTask> task = Record(ledger, 1, 77);
    task->onStop = [w, &ledger]()
    {
        bbCheck(w->getWaitTaskCount() > 0, "callback query failed");
        w->postTask(Record(ledger, 2));
    };
    w->postTask(task);
    w->stopTask(77);
    release = true;
    workers.drain();
    bbCheck(ledger.snapshot().size() == 2, "stopTask reentry failed");
}

/** 验证立即关闭丢弃等待任务并释放引用
*/
static void UninitDiscards()
{
    TaskLedger ledger;
    std::atomic<bool> entered(false);
    TestWorkers workers;
    std::shared_ptr<CTaskThread> w = workers.add();
    w->postTask(std::make_shared<TestTask>(1, [&entered](TestTask& task)
    {
        entered = true;
        bbWait([&task]()
        {
            return task.stopped.load();
        }, "uninit stop current");
    }),
        INT_MAX);
    bbWait([&entered]()
    {
        return entered.load();
    }, "shutdown gate");
    std::shared_ptr<TestTask> queued = Record(ledger, 1);
    std::weak_ptr<TestTask> weak = queued;
    w->postTask(queued);
    queued.reset();
    CTaskThreadManager::instance().uninit(workers.ids[0]);
    bbCheck(ledger.snapshot().empty() && weak.expired(), "uninit executed or retained pending task");
    w->postTask(Record(ledger, 2));
    bbCheck(!w->tryPostTask(Record(ledger, 3)), "stale handle accepted TryPost");
    bbCheck(w->getWaitTaskCount() == 0, "stale handle accepted Post");
}

/** 验证并发投递的精确数量与生产者内部同级顺序
@param [in] tryPost 是否使用布尔返回的尝试投递接口
@param [in] count 各生产者投递数量
*/
static void ManyProducers(bool tryPost, int count)
{
    TaskLedger ledger;
    TestWorkers consumers, producers;
    std::shared_ptr<CTaskThread> target = consumers.add();
    const int producerCount = 8;
    for (int p = 0; p < producerCount; ++p)
    {
        producers.add()->postTask(std::make_shared<TestTask>(p, [target, &ledger, p, count, tryPost](TestTask&)
        {
            for (int i = 0; i < count; ++i)
            {
                std::shared_ptr<TestTask> task = Record(ledger, p * count + i);
                if (tryPost)
                {
                    bbWait([target, task]()
                    {
                        return target->tryPostTask(task);
                    }, "TryPost retry");
                }
                else
                {
#ifdef CTTM_TEST_NO_LEGACY_SEND_TASK
                    bbWait([target, task]()
                    {
                        const CTaskThreadResult result = target->postTask(task);
                        bbCheck(result.m_status == CTaskThreadOk && result.m_committed,
                            "postTask rejected with status=" + bbNumber(result.m_status));
                        return result.m_committed;
                    }, "ordinary postTask handles contention internally");
#else
                    target->postTask(task);
#endif
                }
            }
        }));
    }
    producers.drain();
    consumers.drain();
    std::vector<int> values = ledger.snapshot();
    bbCheck(values.size() == static_cast<size_t>(producerCount * count), "lost/extra concurrent tasks");
    std::vector<int> next(producerCount, 0);
    for (size_t i = 0; i < values.size(); ++i)
    {
        const int producer = values[i] / count, sequence = values[i] % count;
        bbCheck(producer >= 0 && producer < producerCount && sequence == next[producer]++, "duplicate or per-producer FIFO violation");
    }
}

/** 验证任务执行期间继续投递和跨线程转发
*/
static void RecursiveDispatch()
{
    TaskLedger ledger;
    std::atomic<int> parentsDone(0);
    TestWorkers workers;
    std::vector<std::shared_ptr<CTaskThread>> targets;
    for (int i = 0; i < 20; ++i)
    {
        targets.push_back(workers.add());
    }
    // 旧TestTask1..10行为相同；保留10类并将递归扇出限制为固定200个子任务。
    for (int type = 0; type < 10; ++type)
    {
        targets[type]->postTask(std::make_shared<TestTask>(type + 1, [targets, type, &ledger, &parentsDone](TestTask&)
        {
            for (int child = 0; child < 20; ++child)
            {
                targets[(type + child) % 20]->postTask(Record(ledger, type * 20 + child, type + 1), child % 10 + 1);
            }
            ++parentsDone;
        }),
            INT_MAX);
    }
    bbWait([&parentsDone]()
    {
        return parentsDone == 10;
    }, "all dispatching tasks finished");
    workers.drain();
    std::vector<int> values = ledger.snapshot();
    std::sort(values.begin(), values.end());
    bbCheck(values.size() == 200, "recursive dispatch count wrong");
    for (int i = 0; i < 200; ++i)
    {
        bbCheck(values[i] == i, "recursive dispatch duplicate/missing task");
    }
}

/** 验证批量关闭释放全部工作线程
*/
static void BulkExit()
{
    std::atomic<int> entered(0), exited(0);
    TestWorkers workers;
    for (int i = 0; i < 20; ++i)
    {
        workers.add()->postTask(std::make_shared<TestTask>(i, [&entered, &exited](TestTask& task)
        {
            ++entered;
            bbWait([&task]()
            {
                return task.stopped.load();
            }, "bulk stop");
            ++exited;
        }));
    }
    bbWait([&entered]()
    {
        return entered == 20;
    }, "20 workers running");
    CTaskThreadManager::instance().uninitAll();
    bbCheck(exited == 20 && CTaskThreadManager::instance().count() == 0, "uninitAll failed to join all threads");
}

/** 验证重复创建和关闭不残留管理器注册
*/
static void RepeatLifecycle()
{
    std::set<uint32_t> seen;
    for (int i = 0; i < 100; ++i)
    {
        TaskLedger ledger;
        TestWorkers workers;
        std::shared_ptr<CTaskThread> w = workers.add();
        bbCheck(seen.insert(workers.ids[0]).second, "thread ID reused");
        for (int j = 0; j < 20; ++j)
        {
            w->postTask(Record(ledger, j), j % 3 + 1);
        }
        workers.drain();
        w.reset();
        bbCheck(ledger.snapshot().size() == 20 && TestTask::live == 0, "lost task or retained TestTask object");
    }
}

/** 验证线程空闲后的再次投递能够唤醒
*/
static void IdleWake()
{
    std::atomic<int> completed(0);
    TestWorkers workers;
    std::shared_ptr<CTaskThread> w = workers.add();
    for (int i = 1; i <= 100; ++i)
    {
        w->postTask(std::make_shared<TestTask>(1, [&completed](TestTask&)
        {
            ++completed;
        }));
        bbWait([&completed, i]()
        {
            return completed == i;
        }, "wake idle worker");
        bbSleep(1);
    }
    workers.drain();
}

/** 仅在旧版基线验证已移除的同步投递接口
*/
static void SynchronousLegacy()
{
#ifndef CTTM_TEST_NO_LEGACY_SEND_TASK
    std::atomic<int> completed(0);
    TestWorkers workers;
    std::shared_ptr<CTaskThread> w = workers.add();
    w->SendTask(std::make_shared<TestTask>(2, [&completed](TestTask&)
    {
        completed = 1;
    }),
        2);
    bbCheck(completed == 1, "SendTask returned before execution");
    workers.drain();
#else
    // SendTask 已按本轮明确需求移除，保留 ID 18 作为取消同步发送后的替代行为验证。
    std::atomic<int> completed(0);
    TestWorkers workers;
    std::shared_ptr<CTaskThread> worker = workers.add();
    bbCheck(worker->postTask(std::make_shared<TestTask>(2, [&completed](TestTask&)
    {
        ++completed;
    })).m_committed,
        "asynchronous replacement rejected");
    workers.drain();
    bbCheck(completed == 1, "postTask plus explicit drain did not complete");
#endif
}

/** 按顺序模型验证多优先级任务的选择结果
*/
static void PriorityModel()
{
    TaskLedger ledger;
    std::atomic<bool> entered(false), release(false);
    TestWorkers workers;
    std::shared_ptr<CTaskThread> w = workers.add();
    Block(w, entered, release);
    std::vector<std::pair<int, int>> submitted;
    uint32_t random = 0x5a17u;
    for (int i = 0; i < 2048; ++i)
    {
        random = random * 1664525u + 1013904223u;
        int level = static_cast<int>(random % 10) + 1;
        submitted.push_back(std::make_pair(level, i));
        w->postTask(Record(ledger, i), level);
    }
    std::stable_sort(submitted.begin(), submitted.end(),
        [](const std::pair<int, int>& a, const std::pair<int, int>& b)
    {
        return a.first > b.first;
    });
    release = true;
    workers.drain();
    const std::vector<int> actual = ledger.snapshot();
    bbCheck(actual.size() == submitted.size(), "model task count mismatch");
    for (size_t i = 0; i < actual.size(); ++i)
    {
        bbCheck(actual[i] == submitted[i].second, "seed=0x5a17 priority model mismatch");
    }
}

/** 验证不同工作线程互不阻塞
*/
static void IndependentWorkers()
{
    TaskLedger ledger;
    std::atomic<bool> entered(false), release(false), otherDone(false);
    TestWorkers workers;
    std::shared_ptr<CTaskThread> a = workers.add(), b = workers.add();
    Block(a, entered, release);
    a->postTask(Record(ledger, 1));
    b->postTask(std::make_shared<TestTask>(2, [&otherDone](TestTask&)
    {
        otherDone = true;
    }));
    bbWait([&otherDone]()
    {
        return otherDone.load();
    }, "independent worker progress");
    bbCheck(ledger.snapshot().empty() && a->getWaitTaskCount() == 1, "blocked worker unexpectedly advanced");
    release = true;
    workers.drain();
    bbCheck(ledger.snapshot().size() == 1, "blocked worker did not recover");
}

/** 验证任务完成与停止通知并发时的生命周期
*/
static void StopWhileCompleting()
{
    TaskLedger ledger;
    std::atomic<int> notified(0);
    TestWorkers consumers, stoppers;
    std::shared_ptr<CTaskThread> target = consumers.add();
    for (int p = 0; p < 4; ++p)
    {
        stoppers.add()->postTask(std::make_shared<TestTask>(p, [target, p, &ledger, &notified](TestTask&)
        {
            for (int i = 0; i < 500; ++i)
            {
                std::shared_ptr<TestTask> task = Record(ledger, p * 500 + i, 10);
                task->onStop = [&notified]()
                {
                    ++notified;
                };
                target->postTask(task);
                target->stopTask(10);
                target->stopCurTask();
                target->getWaitTaskCount();
            }
        }));
    }
    stoppers.drain();
    consumers.drain();
    target.reset();
    std::vector<int> values = ledger.snapshot();
    std::sort(values.begin(), values.end());
    bbCheck(values.size() == 2000, "stopTask race lost/executed extra tasks");
    for (int i = 0; i < 2000; ++i)
    {
        bbCheck(values[i] == i, "stopTask race duplicate");
    }
    bbCheck(TestTask::live == 0, "race retained task references");
}

TestPoolPolicy::TestPoolPolicy() :
m_target(2),
m_throw(false),
m_initial(0),
m_grows(0)
{

}

CTaskPoolDecision TestPoolPolicy::evaluate(const CTaskPoolLoad& load)
{
    m_initial = load.m_initialThreads;
    m_grows = load.m_successfulGrowCount;
    if (m_throw)
    {
        throw std::runtime_error("injected policy exception");
    }
    return CTaskPoolDecision(m_target.load(), 20);
}

TestFaultTask::TestFaultTask(int32_t mode, std::atomic<int32_t>& calls,
    std::atomic<bool>& entered, std::atomic<bool>& release) :
m_mode(mode),
m_calls(calls),
m_entered(&entered),
m_release(&release)
{
    clone([this]() -> CTask*
    {
        if (m_mode == 2)
        {
            throw std::bad_alloc();
        }
        return m_mode == 3 ? this : nullptr;
    });
}

void TestFaultTask::doTask()
{
    m_entered->store(true);
    bbWait([this]()
    {
        return m_release->load();
    }, "fault original release");
    ++m_calls;
    if (m_mode == 1)
    {
        throw std::runtime_error("injected doTask exception");
    }
}

bool TestFaultTask::reExecute()
{
    return true;
}

/** 验证硬容量限制拒绝新任务且不触发误抢占
*/
static void BoundedAdmission()
{
    CTaskThreadConfig config;
    config.m_maximumTasks = 2;
    CTaskThreadManager& manager = CTaskThreadManager::instance();
    const uint32_t id = manager.init(config);
    bbCheck(id != 0, "bounded initialization failed");
    TestWorkers cleanup;
    cleanup.ids.push_back(id);
    std::shared_ptr<CTaskThread> worker = manager.getThreadInterface(id);
    std::atomic<bool> entered(false), release(false);
    std::shared_ptr<TestTask> current = Block(worker, entered, release, 1);
    TaskLedger ledger;
    bbCheck(worker->postTask(Record(ledger, 1), 1).m_committed, "second task rejected before limit");
    const CTaskThreadResult rejected = worker->postTask(Record(ledger, 2), 2);
    bbCheck(rejected.m_status == CTaskThreadCapacity && !rejected.m_committed, "hard task limit exceeded");
    bbCheck(current->stops == 0, "rejected high-priority task stopped current");
    release = true;
    cleanup.drain();
    bbCheck(ledger.snapshot() == std::vector<int>(1, 1), "rejected task executed or accepted task lost");
    bbCheck(worker->statistics().m_rejected == 1, "missing rejection diagnostic");
}

/** 验证执行和重做工厂异常后后续任务仍能运行
*/
static void CallbackFailures()
{
    std::atomic<int32_t> calls(0);
    for (int32_t mode = 1; mode <= 3; ++mode)
    {
        std::atomic<bool> entered(false), release(false);
        TestWorkers workers;
        const std::shared_ptr<CTaskThread> worker = workers.add();
        try
        {
            bbCheck(worker->postTask(std::make_shared<TestFaultTask>(mode, calls, entered, release)).m_committed,
                "fault task not accepted");
            bbWait([&entered]()
            {
                return entered.load();
            }, "fault original entered");
            bbCheck(worker->postTask(std::make_shared<TestTask>(1, [&calls](TestTask&)
            {
                ++calls;
            }),
                              2)
                        .m_committed,
                "fault high task not accepted");
            release = true;
            workers.drain();
            bbCheck(worker->statistics().m_errors == 1 && worker->statistics().m_outstanding == 0,
                "callback failure escaped diagnostics or leaked admission");
        }
        catch (...)
        {
            release = true;
            throw;
        }
    }
    bbCheck(calls == 6, "callback failures stopped later tasks");
}

/** 验证任务回调中的自等待和管理器批量等待被拒绝
*/
static void CallbackSelfWait()
{
    TestWorkers workers;
    std::shared_ptr<CTaskThread> worker = workers.add();
    const uint32_t id = workers.ids[0];
    std::atomic<int> tested(0);
    worker->postTask(std::make_shared<TestTask>(1, [worker, id, &tested](TestTask&)
    {
        bbCheck(worker->waitForEnd().m_status == CTaskThreadSelfWait, "direct self join not rejected");
        bbCheck(CTaskThreadManager::instance().uninit(id).m_status == CTaskThreadSelfWait, "manager self join not rejected");
        bbCheck(CTaskThreadManager::instance().uninitAll().m_status == CTaskThreadSelfWait, "bulk self join not rejected");
        ++tested;
    }));
    workers.drain();
    bbCheck(tested == 1, "self-wait case did not run");
}

/** 验证系统线程优先级设置、非法值和退出后调用
*/
static void NativePriority()
{
    CTaskThreadConfig config;
    config.m_systemPriority = CTaskPriorityNormal;
    TestWorkers workers;
    const uint32_t id = CTaskThreadManager::instance().init(config);
    bbCheck(id != 0, "normal priority startup failed");
    workers.ids.push_back(id);
    std::shared_ptr<CTaskThread> worker = CTaskThreadManager::instance().getThreadInterface(id);
    bbCheck(worker->setSystemPriority(CTaskPriorityNormal).m_committed, "normal priority runtime update failed");
    bbCheck(worker->setSystemPriority(static_cast<CTaskSystemPriority>(999)).m_status == CTaskThreadInvalid, "invalid priority accepted");
    workers.drain();
    bbCheck(worker->setSystemPriority(CTaskPriorityNormal).m_status == CTaskThreadClosed, "priority touched joined native handle");
}

/** 验证动态策略扩缩容边界及分配器投递排空
*/
static void DynamicPool()
{
    CTaskThreadPool pool;
    CTaskThreadPoolConfig config;
    config.m_initialThreads = 2;
    config.m_maximumThreads = 4;
    config.m_checkMilliseconds = 5;
    std::shared_ptr<TestPoolPolicy> policy(new TestPoolPolicy);
    config.m_policy = policy;
    bbCheck(pool.initialize(config).m_committed, "pool initialize failed");
    CTaskThreadAllocator allocator(pool);
    std::set<uint32_t> selected;
    for (int i = 0; i < 8; ++i)
    {
        uint32_t id = 0;
        bbCheck(allocator.select(id).m_committed, "allocator failed on idle pool");
        selected.insert(id);
    }
    bbCheck(selected.size() == 2, "equal-load allocation did not rotate");
    policy->m_target = 1000;
    bbWait([&pool]()
    {
        return pool.statistics().m_threads == 4;
    }, "policy growth and internal maximum clamp");
    bbWait([policy]()
    {
        return policy->m_grows == 2;
    }, "policy growth count");
    bbCheck(policy->m_initial == 2, "policy missing initial capacity");
    policy->m_throw = true;
    bbWait([&pool]()
    {
        return pool.statistics().m_errors != 0;
    }, "policy exception diagnosis");
    policy->m_throw = false;
    policy->m_target = 0;
    bbWait([&pool]()
    {
        return pool.statistics().m_threads == 2;
    }, "policy shrink and initial floor");
    std::atomic<int> completed(0);
    for (int i = 0; i < 1000; ++i)
    {
        std::shared_ptr<TestTask> task(new TestTask(1, [&completed](TestTask&)
        {
            ++completed;
        }));
        bbWait([&allocator, task]()
        {
            uint32_t id = 0;
            std::shared_ptr<CTaskThread> thread;
            return allocator.select(id, thread).m_committed && thread->postTask(task).m_committed;
        }, "allocator selection/post");
    }
    bbCheck(pool.shutdown().m_status == CTaskThreadOk && completed == 1000, "pool drain lost accepted tasks");
    bbCheck(pool.statistics().m_threads == 0 && !pool.statistics().m_accepting, "pool did not release members");
    uint32_t missing = 999;
    bbCheck(allocator.select(missing).m_status == CTaskThreadClosed && missing == 0, "closed pool selected target");
    bbCheck(pool.shutdown().m_status == CTaskThreadOk, "pool shutdown not idempotent");
}

std::vector<BlackBoxCase> taskCases()
{
    std::vector<BlackBoxCase> cases;
    const std::function<void(const char*, const char*, bool, std::function<void()>)> add =
        [&cases](const char* name, const char* expected, bool stress, std::function<void()> run)
    {
        BlackBoxCase c = {static_cast<int>(cases.size() + 1), name, expected, stress, stress ? 60 : 20, [run]()
        {
            run();
            bbCheck(TestTask::errors == 0 && TestTask::live == 0, "task fixture error or retained ownership after case cleanup");
            bbCheck(CTaskThreadManager::instance().count() == 0, "case left managed threads registered");
        }};
        cases.push_back(c);
    };
    add("lifecycle", "unique IDs; missing ID safe; counts return to zero", false, Lifecycle);
    add("invalid-post", "null/nonpositive priority rejected; INT_MAX accepted", false, InvalidPost);
    add("priority-fifo-snapshots", "3>2>1; FIFO within priority; exact snapshots", false, PriorityFifo);
    add("redo-front", "redo at original priority before same-level backlog", false, []()
    {
        Preempt(true, true, false);
    });
    add("redo-once", "multiple higher posts create only one redo", false, []()
    {
        Preempt(true, true, true);
    });
    add("no-redo", "reExecute=false never replays", false, []()
    {
        Preempt(false, true, false);
    });
    add("null-clone", "null clone does not lose incoming task", false, []()
    {
        Preempt(true, false, false);
    });
    add("equal-lower-no-preempt", "equal/lower priority does not request stop", false, EqualDoesNotStop);
    add("stop-property-level", "selected property ID/level; duplicate ptr notified once", false, []()
    {
        StopFiltered(false);
    });
    add("stop-property-all-levels", "property ID spans levels; other property ID unchanged", false, []()
    {
        StopFiltered(true);
    });
    add("stop-current", "running task receives cooperative stop", false, StopCurrent);
    add("stop-all", "all current/queued tasks notified; queue not erased", false, StopAll);
    add("stop-callback-reentry", "stopTask callback can query/post without deadlock", false, StopReentry);
    add("uninit-and-stale-handle", "stop/join; discard pending; stale post rejected", false, UninitDiscards);
    add("multi-producer-fifo", "every task once; per-producer FIFO", false, []()
    {
        ManyProducers(false, 100);
    });
    add("try-post-contention", "retry TryPost; every accepted task once", false, []()
    {
        ManyProducers(true, 100);
    });
    add("legacy-20-thread-dispatch", "10 legacy types dispatch 200 bounded children", false, RecursiveDispatch);
#ifdef CTTM_TEST_NO_LEGACY_SEND_TASK
    add("post-and-explicit-drain", "async post plus explicit drain preserves completion", false, SynchronousLegacy);
#else
    add("legacy-send-task", "synchronous task completes before return", false, SynchronousLegacy);
#endif
    add("bulk-uninit-all", "20 running threads stopped and joined", false, BulkExit);
    add("idle-wakeup", "100 empty transitions have no lost wakeup", false, IdleWake);
    add("repeated-lifecycle-ownership", "100 lifecycles; no retained TestTask objects", true, RepeatLifecycle);
    add("producer-stress", "8x5000 tasks; no loss/duplicate/order inversion", true, []()
    {
        ManyProducers(false, 5000);
    });
    add("try-post-stress", "8x2000 accepted tasks exactly once", true, []()
    {
        ManyProducers(true, 2000);
    });
    add("redo-repeat", "50 preemption cycles retain ordering", true, []()
    {
        for (int i = 0; i < 50; ++i)
        {
            Preempt(true, true, true);
        }
    });
    add("seeded-priority-model", "seed0x5a17; 2048 tasks match stable priority model", true, PriorityModel);
    add("independent-workers", "blocked worker does not prevent second worker progress", false, IndependentWorkers);
    add("stop-completion-race", "4 producers race stop/query with completion; 2000 tasks exactly once", true, StopWhileCompleting);
    add("models-gates-native-priority", "configuration, concurrent admission and actual OS priority", false, taskThreadModelsTest);
    add("bounded-task-admission", "in-flight plus running obey hard cap; rejection cannot preempt", false, BoundedAdmission);
    add("callback-failure-isolation", "doTask/clone failure and returning this cannot kill worker", false, CallbackFailures);
    add("callback-self-wait", "direct/manager/bulk self joins return SelfWait", false, CallbackSelfWait);
    add("runtime-system-priority", "startup/runtime priority errors and closed-handle rejection", false, NativePriority);
    add("dynamic-pool-allocator", "virtual policy; internal min/max; allocator; shrink/grow; drain", false, DynamicPool);
    const std::vector<BlackBoxCase> white = taskThreadWhiteBoxCases(static_cast<int32_t>(cases.size()) + 1);
    cases.insert(cases.end(), white.begin(), white.end());
    const std::vector<BlackBoxCase> pressure = taskThreadStressCases(static_cast<int32_t>(cases.size()) + 1);
    cases.insert(cases.end(), pressure.begin(), pressure.end());
    const std::vector<BlackBoxCase> regression = taskThreadRegressionCases(static_cast<int32_t>(cases.size()) + 1);
    cases.insert(cases.end(), regression.begin(), regression.end());
#ifndef CTTM_TEST_DLL
    add("v3-read-domain", "read leases, delayed reclamation, concurrent close", false, taskV3ReadDomainTest);
    add("v3-task-storage", "bounded slots, reclamation, growth, shrink, concurrent allocation", false, taskV3StorageTest);
    add("v3-storage-failure", "allocation/policy failure rollback and delayed shrink floor", false, taskV3StorageFailureTest);
    add("v3-queue-ready", "FIFO across segments and explicit reclamation after empty checks", false, taskV3QueueReadyTest);
    add("v3-ready-index", "highest-priority publication and consumer lowering race", false, taskV3ReadyIndexTest);
    add("v3-queue-counters", "bounded concurrent producers, exact count, FIFO and counter reset", true, taskV3QueueCounterTest);
    add("v3-queue-batch", "bounded prefix, cross-segment FIFO and exact exception commit count", false, taskV3QueueBatchTest);
#else
    const char* moduleNames[] = {"v3-read-domain", "v3-task-storage", "v3-storage-failure", "v3-queue-ready",
        "v3-ready-index", "v3-queue-counters", "v3-queue-batch"};
    for (size_t i = 0; i < 7; ++i)
    {
        add(moduleNames[i], "internal module coverage belongs to static build", i == 5, std::function<void()>());
        cases.back().unavailableReason = "private module helpers tested in static build; DLL has public API tests";
    }
#endif
    add("prefetched-priority-redo-stop", "local prefix retains priority, redo front, property stop and FIFO", false, taskPrefetchedPriorityTest);
    add("priority-wake-without-heartbeat", "200 high-priority idle/burst transitions finish without 60s heartbeat", false, taskPriorityWakeTest);
#ifndef CTTM_TEST_DLL
    add("external-queue-entry", "borrowed lifetime, mixed FIFO, exception prefix, capacity and concurrent exact delivery", true, taskV4QueueEntryTest);
#else
    add("external-queue-entry", "external entry module coverage belongs to static build", true, std::function<void()>());
    cases.back().unavailableReason = "module coverage tested in static build";
#endif
#ifndef CTTM_TEST_DLL
    add("map-buffer-maintenance", "concurrent maintenance preserves keys and releases reusable buffers", false, taskV4MapMaintenanceTest);
    add("prepared-benchmark-validator", "detect balanced duplicates/missing and per-level reordering", false, taskV4BenchmarkValidatorTest);
#else
    add("map-buffer-maintenance", "module coverage belongs to static build", false, std::function<void()>());
    cases.back().unavailableReason = "module coverage tested in static build";
    add("prepared-benchmark-validator", "benchmark validator covered by static test build", false, std::function<void()>());
    cases.back().unavailableReason = "benchmark validator tested in static build";
#endif
#ifdef CTTM_TEST_DLL
    add("external-entry-quota", "entry-only module coverage belongs to static build", true, std::function<void()>());
    cases.back().unavailableReason = "module coverage tested in static build";
#else
    add("external-entry-quota", "bounded entry credits, growth failure, shrink, lifetime and concurrent FIFO", true, taskV4EntryQuotaTest);
#endif
#ifdef CTTM_TEST_DLL
    cases[27].unavailableReason = "private module helpers tested in static build; DLL has public API tests";
    for (size_t i = 38; i <= 40; ++i)
    {
        cases[i].unavailableReason = "executable operator-new injection does not cover DLL allocation";
    }
#endif
    add("lazy-redo-factory-order", "no eager clone; deferred snapshot parameters and original-priority redo front", false, taskRedoFactoryOrderTest);
    add("lazy-redo-factory-concurrency", "concurrent preemption creates once; inherited factory supports repeated redo", false, taskRedoFactoryConcurrentTest);
    add("lazy-redo-factory-failure", "throw, null, self return and shutdown do not leak or stop later tasks", false, taskRedoFactoryFailureTest);
    add("lazy-redo-factory-resources", "configuration rollback, failed inheritance cleanup and bounded record failure", false, taskRedoFactoryResourceTest);
    add("lazy-redo-factory-allocation", "failed shared ownership allocation deletes raw clone and preserves later tasks", false, taskRedoFactoryAllocationTest);
#ifdef CTTM_TEST_DLL
    cases.back().unavailableReason = "executable operator-new injection does not cover DLL allocation";
#endif
    add("lazy-redo-factory-lifecycle", "child factory overrides inheritance; in-flight factory shutdown cannot commit redo", false, taskRedoFactoryLifecycleTest);
    add("v5-pending-redo-priority", "higher task precedes a prepared low redo and equal-level FIFO", false, taskV5PendingRedoPriorityTest);
    add("v5-nested-preemption", "two interrupted levels each redo once at their original priority", false, taskV5NestedPreemptionTest);
    add("v5-queued-higher-priority", "already queued higher work cannot be bypassed by redo", false, taskV5QueuedHigherPriorityTest);
    add("v5-factory-publication-race", "higher post and discard during factory obey priority and ownership", false, taskV5FactoryPublicationRaceTest);
    add("v5-repeated-redo", "64 inherited generations preserve original priority and lazy construction", false, taskV5RepeatedRedoTest);
    add("v5-redo-performance", "one million ordinary tasks and 100 cooperative interruptions, lazy factory counts and paired timings", false, taskV5RedoPerformanceTest);
    add("v5-first-priority-race", "first higher priority publication racing with task handoff cannot lose preemption", false, taskV5FirstPriorityRaceTest);
    add("v5-shutdown-handoff", "discard shutdown racing with highest-priority task handoff cannot lose stopTask", false, taskV5ShutdownHandoffTest);
    add("v5-reclaim-batches", "full reclaim mailboxes and a paused helper cannot block other batches or duplicate release", false, taskV5ReclaimBatchesTest);
#ifdef CTTM_TEST_DLL
    cases.back().unavailableReason = "private reclaim model tested in static build; DLL has public lifecycle coverage";
#endif
    add("v5-current-property-stop", "current property match ignores queued-level filter; queued matches are deduplicated and kept", false, taskV5CurrentPropertyStopTest);
    add("v5-concurrent-removal", "16 concurrent joins on different IDs leave no successful-but-registered worker", true, taskV5ConcurrentRemovalTest);
#ifndef CTTM_TEST_DLL
    add("reentrant-protection-exhaustion", "callback holding last read protection returns SelfWait; later recovery remains valid", false, taskReentrantProtectionTest);
#else
    add("reentrant-protection-exhaustion", "static build covers private protection exhaustion", false, std::function<void()>());
    cases.back().unavailableReason = "private protection access requires the static build";
#endif
    add("post-result-contract", "logical/physical capacity, invalid/closed and try detail remain distinct", false, taskPostResultContractTest);
    add("post-contention-contract", "32 producers use ordinary post with no outward Busy/Retry and exact delivery", true, taskPostContentionContractTest);
    // 默认小初始队列反复从空闲进入八生产者突发，核对动态归还与扩容交错中的接纳结果。
    add("dynamic-credit-return-race", "100 default-pool bursts of 8x5000 tasks; ordinary post succeeds; exact count and per-producer FIFO", true, []()
    {
        for (size_t round = 0; round < 100; ++round)
        {
            ManyProducers(false, 5000);
        }
    });
    bbCheck(cases.size() == 92, "existing test IDs must not drift");
    add("native-automation-self-test", "strict CSV/median; inactive owner; late child; timeout; owned termination", false, CTaskTestAutomation::selfTest);
    const char* usageNames[] = {"usage-priority", "usage-redo", "usage-stopping", "usage-resources",
        "usage-lifecycle", "usage-concurrent", "usage-pool"};
    for (size_t i = 0; i < 7; ++i)
    {
        add(usageNames[i], "public API example; independent assertions; manager empty after cleanup", false, [i]()
        {
            bbCheck(runTaskUsageTests(i + 1) == 0, "usage scenario failed");
        });
        cases.back().timeoutSeconds = 120;
    }
    const char* fairnessNames[] = {"fairness-4-1", "fairness-4-10", "fairness-8-1", "fairness-8-10",
        "fairness-16-1", "fairness-16-10", "fairness-32-1", "fairness-32-10",
        "fairness-32-1-cpu4", "fairness-32-10-cpu4", "fairness-64-1-cpu4", "fairness-64-10-cpu4",
        "fairness-8-1-cpu1", "fairness-8-10-cpu1", "fairness-single-record-8-10",
        "fairness-single-record-32-1", "fairness-single-record-32-10"};
    for (size_t i = 0; i < 17; ++i)
    {
        add(fairnessNames[i], "10s; every producer >=1000; exact FIFO; producer/consumer gap<2000ms; no outward contention", true, [i]()
        {
            CTaskTestAutomation::fairness(i);
        });
        cases.back().timeoutSeconds = 150;
    }
    const char* raceNames[] = {"queue-empty-scan-return-value", "queue-empty-scan-return-entry", "queue-partial-return-publication"};
    for (size_t i = 0; i < 3; ++i)
    {
        add(raceNames[i], "private-copy hook fires; usable resource; exact FIFO and empty final state", false, [i]()
        {
            const char* program = std::getenv("CTTM_QUEUE_RACE_PROGRAM");
            bbCheck(program != nullptr && *program != '\0', "prepare Queue fixture and set CTTM_QUEUE_RACE_PROGRAM");
            BlackBoxChild child;
            const std::string log = bbTempPath("-queue-resource-race.log");
            child.startProgram(program, {bbNumber(i + 1)}, log);
            const int32_t result = child.wait(30000);
            bbCheck(result == 0, "queue resource race failed: " + bbRead(log));
        });
        cases.back().timeoutSeconds = 45;
        if (std::getenv("CTTM_QUEUE_RACE_PROGRAM") == nullptr)
        {
            cases.back().unavailableReason = "prepare private Queue fixture; set CTTM_QUEUE_RACE_PROGRAM to its executable";
        }
    }
    bbCheck(cases.size() == g_taskCaseDescriptions.size(), "test metadata coverage mismatch");
    for (size_t i = 0; i < cases.size(); ++i)
    {
        const std::map<int32_t, CTaskTestCaseDescription>::const_iterator item = g_taskCaseDescriptions.find(cases[i].id);
        bbCheck(item != g_taskCaseDescriptions.end() && std::string(cases[i].name) == item->second.m_key,
            "stable test identity changed");
        cases[i].name = item->second.m_name;
        cases[i].expectation = item->second.m_expectation;
    }
    return cases;
}

int main(int argc, char** argv)
{
    if (argc > 1)
    {
        const std::string command = argv[1];
        if (command == "--automation-child")
        {
            bbSleep(60000);
            return 0;
        }
        if (command == "--matrix" || command == "--repeat" || command == "--queue-resource-race" || command == "--automation-self-test")
        {
            return CTaskTestAutomation::run(argc - 1, argv + 1);
        }
        if (command == "--intrusive-prototype")
        {
            return taskRunIntrusivePrototype(argc - 1, argv + 1);
        }
    }
    if (argc > 1 && std::string(argv[1]) == "--fairness")
    {
        return runTaskFairnessTests(argc - 1, argv + 1);
    }

    if (argc > 1 && std::string(argv[1]) == "--prepared-benchmark")
    {
        return runPreparedTaskBenchmark(argc - 1, argv + 1);
    }
    if (argc > 1 && std::string(argv[1]) == "--benchmark")
    {
        return runTaskThreadBenchmark(argc - 1, argv + 1);
    }
    // VS2013 首次局部静态初始化由启动线程串行完成，再启动并发用例
    CTaskThreadManager::instance();
    try
    {
        return bbRun(argc, argv, "CTaskThreadManagerTest", taskCases());
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << "\n";
        return 1;
    }
    catch (...)
    {
        std::cerr << "unknown setup exception\n";
        return 1;
    }
}
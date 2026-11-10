#include "LockFreeQueue.h"

// 模板实现由消费方实例化；本编译单元仅核对库工程头文件，不导出伪造的模板 ABI。

// 使用示例：以下 main 串行演示公开接口，不创建线程，不作为性能基准。
// 实际接入时可有多个生产者，但 pop/clear/maintain 必须由唯一消费者串行执行。
// 使用前配置，停止并收敛全部使用者后才能 init/destroy；Producer 必须先于队列释放。
// 使用方法：复制下方 BEGIN/END 之间的内容，去掉每行最前面的 //，作为独立控制台程序编译。
// 单独复制时还需 #include "LockFreeQueueAPI.h"，并配置本库头文件包含目录。
// 已验证：VS2013 Win32/x64 的 Debug/Release，四次编译运行通过；不是并发或性能测试。
// BEGIN LOCKFREEQUEUE USAGE DEMO

// #include <iostream>
// #include <memory>
//
// int main()
// {
//     // 输入接口名和返回状态；失败只报告，返回 false 交给 main 正常退出。
//     const auto succeeded = [](const char* operation, LockFreeQueueResult result) -> bool
//     {
//         if (result != LockFreeQueueSuccess)
//         {
//             const LockFreeQueueError error(result);
//             std::cerr << operation << ": status=" << error.status()
//                 << ", " << error.what() << std::endl;
//             return false;
//         }
//         return true;
//     };
//
//     try
//     {
//         // 1. 默认构造即可使用；push 已返回状态，不能把失败当作已经接纳。
//         {
//             LockFreeQueue<int32_t> queue;
//             if (!succeeded("push", queue.push(10)) ||
//                 !succeeded("tryPush", queue.tryPush(20)) ||
//                 !succeeded("tryPushAndFence", queue.tryPushAndFence(30)))
//             {
//                 return 1;
//             }
//             // try 接口遇到竞争可以返回 Busy；本例无并发，不应遇到竞争。
//             // 真实业务由原生产者重试或回压，容量不足等错误不可当作 Busy 无限重试。
//             // AndFence 不发送通知；如需通知，必须在成功接纳后按上层协议执行。
//             std::cout << "ordinary size=" << queue.size()
//                 << ", atomicsLockFree=" << queue.atomicsLockFree() << std::endl;
//             int32_t value = 0;
//             if (queue.empty() || !queue.possiblyReady() || queue.size() != 3 ||
//                 !queue.pop(&value) || value != 10 ||
//                 !queue.popReady(&value) || value != 20 ||
//                 !queue.pop(nullptr) || !queue.empty())
//             {
//                 std::cerr << "ordinary FIFO/discard failed" << std::endl;
//                 return 1;
//             }
//             // possiblyReady 只是消费方提示，size/empty 是弱快照，不可当接纳或销毁门禁。
//             if (!succeeded("push before clear", queue.push(40)))
//             {
//                 return 1;
//             }
//             queue.clear(); // 丢弃已排队值；不是停止生产者的接口。
//             if (!queue.empty() || !succeeded("push before init", queue.push(50)))
//             {
//                 return 1;
//             }
//             queue.init(); // 静止下清空并重建；失败抛 LockFreeQueueError。
//             if (!queue.empty())
//             {
//                 return 1;
//             }
//             queue.destroy();
//             queue.destroy(); // 静止下允许重复销毁。
//             if (queue.tryPush(60) != LockFreeQueueNotInitialized ||
//                 !succeeded("tryInit", queue.tryInit()) ||
//                 !succeeded("push after tryInit", queue.push(60)) ||
//                 !queue.pop(&value) || value != 60)
//             {
//                 return 1;
//             }
//         }
//
//         // 2. 延迟初始化、动态池、长寿命 Producer，以及两种前缀消费接口。
//         {
//             // Entry 地址必须稳定；先于 queue 声明，使失败退出时 queue 先结束借用。
//             LockFreeQueueEntry<int32_t> entry4(4);
//             LockFreeQueueEntry<int32_t> entry5(5);
//             LockFreeQueueEntry<int32_t> entry6(6);
//             LockFreeQueueEntry<int32_t> entry7(7);
//             LockFreeQueue<int32_t> queue(false);
//             LockFreeQueuePoolConfig config;
//             config.initialCapacity = 16;
//             config.growThreshold = 1;
//             config.maximumCapacity = 512; // -1 为无配置上限，仍可能分配失败。
//             config.maximumProducers = 4; // 覆盖同时登记的普通调用和绑定上下文。
//             if (!succeeded("configurePool", queue.configurePool(config)) ||
//                 !succeeded("configureProducerCounters", queue.configureProducerCounters(true)) ||
//                 !succeeded("configureCancelableReservations", queue.configureCancelableReservations(false)) ||
//                 !succeeded("configureEntryOnly", queue.configureEntryOnly(false)))
//             {
//                 return 1;
//             }
//             // 增长策略返回新增节点数；上下文是只读快照，回调不重入队列。
//             if (!succeeded("setGrowPolicy", queue.setGrowPolicy(
//                 [](const LockFreeQueueGrowContext&) -> size_t
//                 {
//                     return 64;
//                 })))
//             {
//                 return 1;
//             }
//             // 缩容仅释放完全空闲且无保护的整块；1000ms 是持续空闲要求。
//             if (!succeeded("setShrinkPolicy", queue.setShrinkPolicy(
//                 [](const LockFreeQueueShrinkContext& context) -> LockFreeQueueShrinkDecision
//                 {
//                     LockFreeQueueShrinkDecision decision = {0, 1000};
//                     if (context.currentCapacity > context.initialCapacity)
//                     {
//                         decision.shrinkCount = context.currentCapacity - context.initialCapacity;
//                     }
//                     return decision;
//                 })) || !succeeded("configured tryInit", queue.tryInit()))
//             {
//                 return 1;
//             }
//             // std::function 实参构造也可能抛异常，由 main 最外层处理。
//             // Producer 仅供一个执行流使用，循环外绑定一次，不能多线程共享。
//             LockFreeQueueProducer<int32_t> producer;
//             if (!succeeded("tryAcquireProducer", queue.tryAcquireProducer(producer)) ||
//                 !producer.attached() ||
//                 !succeeded("Producer::push", producer.push(1)) ||
//                 !succeeded("Producer::tryPush", producer.tryPush(2)) ||
//                 !succeeded("Producer::tryPushAndFence", producer.tryPushAndFence(3)) ||
//                 !succeeded("tryPushEntry", queue.tryPushEntry(entry4)) ||
//                 !succeeded("tryPushEntryAndFence", queue.tryPushEntryAndFence(entry5)) ||
//                 !succeeded("Producer::tryPushEntry", producer.tryPushEntry(entry6)) ||
//                 !succeeded("Producer::tryPushEntryAndFence", producer.tryPushEntryAndFence(entry7)))
//             {
//                 return 1;
//             }
//             // 入队成功到消费返回前，不修改/移动/销毁 Entry；不访问 m_cookie/m_owner。
//             int32_t output[64] = {};
//             size_t consumed = 0;
//             if (!queue.popReadyBatch(output, 3, consumed) || consumed != 3 ||
//                 output[0] != 1 || output[1] != 2 || output[2] != 3)
//             {
//                 std::cerr << "popReadyBatch FIFO failed" << std::endl;
//                 return 1;
//             }
//             if (!queue.popReadyPrefix(output, 64, consumed) || consumed != 4)
//             {
//                 std::cerr << "popReadyPrefix count failed" << std::endl;
//                 return 1;
//             }
//             for (size_t index = 0; index < consumed; ++index)
//             {
//                 if (output[index] != static_cast<int32_t>(index + 4))
//                 {
//                     std::cerr << "Entry value FIFO failed" << std::endl;
//                     return 1;
//                 }
//             }
//             // 外部 Entry 仍归调用方，此时可以复用；默认池仍为它们领取节点额度。
//             for (int32_t value = 100; value < 180; ++value)
//             {
//                 if (!succeeded("Producer::push loop", producer.push(value)))
//                 {
//                     return 1; // 前面成功的项已接纳，禁止把整批重发。
//                 }
//             }
//             const LockFreeQueuePoolStatistics peak = queue.poolStatistics();
//             std::cout << "pool capacity=" << peak.currentCapacity
//                 << ", grows=" << peak.successfulGrowCount
//                 << ", allocations=" << peak.allocationCalls
//                 << ", retainedBytes=" << peak.retainedBytes << std::endl;
//             producer.release(); // 先释放上下文，再做队列的静止生命周期操作。
//             producer.release();
//             if (producer.attached())
//             {
//                 return 1;
//             }
//             int32_t expected = 100;
//             while (queue.popReadyPrefix(output, 64, consumed))
//             {
//                 // consumed 是实际交付数量，不保证凑满；逐项处理业务并检查停止请求。
//                 for (size_t index = 0; index < consumed; ++index)
//                 {
//                     if (output[index] != expected++)
//                     {
//                         std::cerr << "prefix FIFO failed" << std::endl;
//                         return 1;
//                     }
//                 }
//                 const LockFreeQueueMaintenanceResult maintenance = queue.maintain(64);
//                 if (!succeeded("maintain", maintenance.result))
//                 {
//                     return 1;
//                 }
//             }
//             if (expected != 180 || !queue.empty())
//             {
//                 std::cerr << "prefix drain failed" << std::endl;
//                 return 1;
//             }
//             const LockFreeQueueMaintenanceResult maintenance = queue.maintain();
//             if (!succeeded("idle maintain", maintenance.result))
//             {
//                 return 1;
//             }
//             std::cout << "maintenance reclaimedNodes=" << maintenance.reclaimedNodes
//                 << ", releasedCapacity=" << maintenance.releasedCapacity << std::endl;
//             // 不等待1000ms，故不要求本次缩容；真正空闲时由消费方周期维护。
//             // 前缀输出赋值若抛异常，consumed 前缀已经消费，异常项仍在队首。
//             // 本例使用 int32_t，不会发生赋值异常；其他 T 需处理已交付前缀。
//             // 当前没有并发使用者，空函数对象分别恢复默认增长、关闭新缩容。
//             if (!succeeded("reset grow policy", queue.setGrowPolicy(LockFreeQueueGrowPolicy())) ||
//                 !succeeded("disable shrink policy", queue.setShrinkPolicy(LockFreeQueueShrinkPolicy())))
//             {
//                 return 1;
//             }
//         }
//
//         // 3. EntryOnly 不分配内置载荷节点；业务负责稳定地址和 T 的生命期。
//         {
//             LockFreeQueueEntry<int32_t> first(200);
//             LockFreeQueueEntry<int32_t> second(201);
//             LockFreeQueue<int32_t> queue(false);
//             LockFreeQueuePoolConfig config;
//             config.initialCapacity = 16;
//             config.maximumCapacity = 16;
//             config.maximumProducers = 2;
//             config.growThreshold = 0;
//             if (!succeeded("EntryOnly configurePool", queue.configurePool(config)) ||
//                 !succeeded("EntryOnly enable", queue.configureEntryOnly(true)) ||
//                 !succeeded("reservations enable", queue.configureCancelableReservations(true)) ||
//                 !succeeded("EntryOnly tryInit", queue.tryInit()))
//             {
//                 return 1;
//             }
//             // 预约与 EntryOnly/Producer 独立；此处仅展示如何开启。
//             // 并发重叠投递按最终有效预约位置排序，默认关闭时按成功发布 CAS 排序。
//             // 两种模式均保持单生产者顺序；不能用函数返回先后推断重叠调用的出队顺序。
//             LockFreeQueueProducer<int32_t> producer;
//             if (!succeeded("EntryOnly acquire", queue.tryAcquireProducer(producer)) ||
//                 !succeeded("EntryOnly first", queue.tryPushEntry(first)) ||
//                 !succeeded("EntryOnly second", producer.tryPushEntryAndFence(second)))
//             {
//                 return 1;
//             }
//             if (queue.push(999) != LockFreeQueueInvalidConfiguration)
//             {
//                 std::cerr << "EntryOnly must reject ordinary value push" << std::endl;
//                 return 1;
//             }
//             LockFreeQueueEntry<int32_t>* ready[2] = {};
//             size_t consumed = 0;
//             if (!queue.popReadyEntryPrefix(ready, 2, consumed) || consumed != 2 ||
//                 ready[0] != &first || ready[1] != &second ||
//                 ready[0]->m_value != 200 || ready[1]->m_value != 201)
//             {
//                 std::cerr << "EntryOnly address FIFO failed" << std::endl;
//                 return 1;
//             }
//             // 返回后借用结束，才允许修改/复用；队列不析构 Entry 中的 T。
//             first.m_value = 202;
//             if (!succeeded("reuse Entry", queue.tryPushEntryAndFence(first)))
//             {
//                 return 1;
//             }
//             queue.clear(); // 结束本次借用；不会 delete 外部 Entry。
//             producer.release();
//             if (!queue.empty() || first.m_value != 202)
//             {
//                 return 1;
//             }
//             queue.destroy();
//             std::cout << "EntryOnly reuse complete" << std::endl;
//         }
//
//         // 4. 智能指针按值复制入队；节点池不消除 shared_ptr 的引用计数成本。
//         {
//             const std::shared_ptr<int32_t> task = std::make_shared<int32_t>(300);
//             LockFreeQueue<std::shared_ptr<int32_t>> queue;
//             if (!succeeded("shared_ptr push", queue.push(task)))
//             {
//                 return 1;
//             }
//             std::shared_ptr<int32_t> received;
//             if (!queue.pop(&received) || received != task || *received != 300)
//             {
//                 return 1;
//             }
//             received.reset(); // 业务处理后释放输出引用；task 仍持有原始引用。
//         }
//         std::cout << "LockFreeQueue usage demo passed" << std::endl;
//         return 0;
//     }
//     catch (const LockFreeQueueError& error)
//     {
//         std::cerr << "queue initialization: status=" << error.status()
//             << ", " << error.what() << std::endl;
//         return 2;
//     }
//     catch (const std::exception& error)
//     {
//         std::cerr << "caller allocation/copy: " << error.what() << std::endl;
//         return 3;
//     }
//     catch (...)
//     {
//         std::cerr << "unexpected caller exception" << std::endl;
//         return 4;
//     }
// }

// END LOCKFREEQUEUE USAGE DEMO
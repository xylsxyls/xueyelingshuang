// 调用示例属于SemaphoreTest，以下全部注释；可在测试工程单独编译运行，不参与正式库编译
// 学习时可复制此main和所需include到独立调用工程；正式测试仍位于SemaphoreTest工程
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
//             Semaphore tasks;
//             // 可选：并发启动前、计数为空时预分配计数等待记录；不调用也会按需建立默认池
//             SemaphorePoolConfig countConfig;
//             countConfig.initialCapacity = 16;
//             countConfig.maxCapacity = 64;
//             showResult("configure optional count pool", tasks.configureCountPool(countConfig));
//             const SemaphorePoolStats countStats = tasks.countPoolStatistics();
//             expect(countStats.capacity == 16, "count pool prepared");
//             tasks.signal();
//             tasks.wait();
//             tasks.signal();
//             expect(tasks.wait(1000), "count timed wait");
//             showResult("count publish", tasks.trySignal());
//             showResult("count infinite wait", tasks.waitResult());
//             showResult("count publish again", tasks.trySignal());
//             showResult("count timed result", tasks.waitResult(1000));
//             // 持续阻塞会按需进入优先交付链，避免后来者夺走已交付许可；无需改变调用方式
//             // wait(0)只探测，不适合循环抢占；停止时先提交退出状态，再发退出许可并join
//
//             // reset只用于生产者、消费者均已停止的静止阶段，不作为并发取消接口
//             tasks.signal();
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
//                             changed.event();
//                         }
//                         else if (current % 3 == 2)
//                         {
//                             // 有多个订阅者时，eventAll提示本次已登记的全部等待者
//                             changed.eventAll();
//                         }
//                         else
//                         {
//                             const SemaphoreResult result = changed.tryEvent();
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
//                 changed.eventWait();
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
//             showResult("prepare jobs", jobs.prepareSignalAndWait());
//             showResult("prepare free slots", freeSlots.prepareSignalAndWait());
//             freeSlots.signal();
//             showCombined("publish job and acquire next slot", jobs.signalAndWaitResult(freeSlots));
//             jobs.wait();
//             freeSlots.signal(); // 消费者处理上一工作后归还槽位
//             showCombined("publish job and acquire slot with budget", jobs.signalAndWaitResult(freeSlots, 1000));
//             jobs.wait();
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
//
//             // 旧void包装便于兼容；新调用方优先Result接口，以便观察失败原因
//             jobsOwner.createProcessSemaphore(jobsName, 4);
//             showResult("create named slots", slotsOwner.createProcessResult(slotsName, 4));
//             expect(probe.tryOpenProcessSemaphore(jobsName), "probe existing named jobs");
//             jobsReader.openProcessSemaphore(jobsName);
//             showResult("open named slots", slotsReader.openProcessResult(slotsName));
//
//             // 旧void发送不报告失败，所以用有限等待展示；业务应优先使用Result发送
//             jobsOwner.processSignal();
//             expect(jobsReader.processWait(1000), "consume legacy named job with budget");
//             // 此处没有其他消费者，确认发布成功后才演示旧的无限等待
//             expect(jobsOwner.tryProcessSignal() == 0, "publish named job");
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
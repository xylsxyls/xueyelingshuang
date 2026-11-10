#pragma once
#include "BlackBoxTestSupport.h"
#include "CTaskThreadTestConfig.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <mutex>
#include <map>

/**
CTaskThreadManagerTest 测试文档

一、范围与独立判定
    本工程覆盖公开接口、私有模块、并发公平性、延迟重做、故障回滚及性能测量。
    本文件是用例说明入口；数字 ID 永久稳定，新增只追加，不复用和重排。
    main 中的 const map 与本头文件的显式枚举一一对应，启动时核对英文键、ID 和覆盖数。
    --list 输出中文名称、瞬时/压力和预期；未具备前提的用例保留 ID，明确 NOT_RUN。
    图形界面、网络和磁盘持久化不是本库接口，不构造这些无关用例。

    除单独注明外，前置为启动线程预热 singleton 后创建全新的任务、线程和计数器；
    每项单独子进程，不依赖上一项残留。任务门控控制先后，业务账本、独立优先级模型、
    每生产者序号和精确数量提供预期，不把产品内部返回值单独当作执行成功。
    收尾放行门控、停止生产者、join 工作线程并注销；适用夹具检查任务存活、错误和注册数归零。
    测试账本中的 mutex 仅用于独立观察，不进入产品实现。测试异常只负责报告断言失败。
    外层等待有时限；失败或超时由子进程所有者终止自己的进程组并回收句柄，不杀用户进程。
    禁止事件包括已接纳项丢失/重复、同生产者同级倒序、错级抢占、提前释放和关闭后误接纳。
    严格任务优先级仍成立；公平性要求线程有进展，不要求低优先级任务越过高级任务。

二、构建、入口与前提
    Windows 使用现有 .sln/.vcxproj，VS2013 v120、C++11，Release 参与验证。
    常规 SDK 来自仓库构建流程；测试产物不能混用架构、CRT、配置或头文件/库版本。
    可用以下属性指向独立 SDK，不改生产库，也不覆盖共享 SDK：
      MSBuild CTaskThreadManagerTest.vcxproj /t:Build /p:Configuration=Release /p:Platform=x64
        /p:BlackBoxStatic=true /p:BlackBoxIncludeDir=<SDK/include/>
        /p:BlackBoxLibraryDir=<SDK/lib/> /p:OutDir=<temp/bin/> /p:IntDir=<temp/obj/>
    上述命令按一行执行；实际 MSBuild 路径取当前安装。x86 使用 Win32 和匹配的 SDK。
    非 Windows 沿用本工程 CMakeLists.txt；编译成功不代表已经通过相同平台压力验证。
    内部白盒和 EXE 分配故障用静态组合；DLL 组合保留原 ID，以 NOT_RUN 说明不适用原因。
    不向 CTaskThreadManager 放测试头、友元测试接口或编译宏；私有访问只存在于本 Test。

    四个执行入口共用用例、隔离、断言和 TSV：
      CTaskThreadManagerTest --tests 1,3-8,93 --report <temp/result.tsv>
      CTaskThreadManagerTest --instant --report <temp/instant.tsv>
      CTaskThreadManagerTest --stress --report <temp/stress.tsv>
      CTaskThreadManagerTest --all --report <temp/all.tsv>
    全量先瞬时后压力；指定 ID 升序去重。非法、零、倒序、未登记 ID 和空选项报错。
    单项 --case ID 为父进程内部入口，不提供超时隔离，日常使用 --tests。
    TSV 记录 ID、类型、PASS/FAIL/NOT_RUN、耗时、独立预期和子日志。
    汇总区分实际执行的瞬时/压力数、通过、失败及未执行数；全未运行返回 77，失败返回非零。
    部分 NOT_RUN 不会计为通过，调用者验收完整范围必须同时核对 not_run=0。

    118 至 120 需要真实基础库源码的私有测试副本。在 VS 开发者命令提示符中执行：
      CTaskThreadManagerTest --queue-resource-race <仓库根> <不存在的夹具目录> <cl.exe完整路径>
    父目录应已存在且 SDK 环境与编译器匹配；工具复制 Queue 头文件，在两个唯一位置插入
    测试同步点，检查移除插入文本后与原文一致，再编译并执行三个用例；不修改基础库源码。
    设置 CTTM_QUEUE_RACE_PROGRAM=<夹具目录>/queue-race.exe 后可单独选或随 --all 运行。
    环境缺失标 NOT_RUN，路径无效、源码上下文改变、编译失败或同步点未触发均不算通过。

三、用例目录
    每行字段为 ID | 中文名 | 类型 | 输入/步骤与独立预期。
    1 至 92 维持原有登记顺序及功能断言；名称中的模块代号不代表另存版本代码。
    93 为自动化设施自检，94 至 100 为可独立运行的公开用法示例。
    101 至 117 共用公平性断言：同系统优先级运行 10 秒，最后四分之一生产者延迟 500ms
    加入；每生产者至少成功 1000 项，已接纳项精确消费、同生产者同级 FIFO，无外露竞争
    重试；生产者及消费者最大进展间隔小于 2000ms。不是操作系统绝对公平或实时期限证明。
    --fairness producers levels seconds cpus report.csv [queueProducers] 可自定义规模；
    cpus=0 不限制亲和性，1/4 为指定核数。每生产者均有独立数据行，不以总吞吐掩盖饿死。

    1 | 生命周期 | 瞬时，超时 20 秒 | 创建、查询及注销线程；ID 不重复，无效 ID 安全，最终注册数为零。
    2 | 非法投递边界 | 瞬时，超时 20 秒 | 空任务和非正优先级被拒绝，INT_MAX 优先级可执行，失败不得接纳。
    3 | 优先级与同级先进先出 | 瞬时，超时 20 秒 | 门控积压 3、2、1 级任务；按 3>2>1 执行，同级 FIFO，快照数量准确。
    4 | 重做插在原级队首 | 瞬时，超时 20 秒 | 低级任务被高优先级打断；高任务先执行，原级重做先于同级积压。
    5 | 合并并发重做意图 | 瞬时，超时 20 秒 | 连续高优先级投递打断同一代；只生成一份重做任务。
    6 | 禁止重做的任务 | 瞬时，超时 20 秒 | reExecute 返回 false；收到通知后不重放，后续任务正常。
    7 | 空重做工厂结果 | 瞬时，超时 20 秒 | 工厂返回空指针；不生成重做，也不丢失新投递任务。
    8 | 同级及低级不抢占 | 瞬时，超时 20 秒 | 当前任务在门控中，投递同级和更低级；不得产生抢占通知。
    9 | 按属性及级别停止 | 瞬时，超时 20 秒 | 队列含重复指针和不同属性；只通知匹配项，同一指针只通知一次。
    10 | 跨级按属性停止 | 瞬时，超时 20 秒 | 同一属性分布在多个级别；全部匹配项收到通知，其他属性不受影响。
    11 | 停止当前任务 | 瞬时，超时 20 秒 | 执行体仍在运行时请求停止；收到合作停止通知而非强行终止线程。
    12 | 停止全部任务 | 瞬时，超时 20 秒 | 运行及排队任务都收到通知；排队项不因停止通知被直接删除。
    13 | 停止回调重入 | 瞬时，超时 20 秒 | stopTask 回调再次查询、投递；不得死锁，后续任务完成。
    14 | 注销与过期句柄 | 瞬时，超时 20 秒 | 运行中关闭并丢弃积压；线程退出，旧句柄投递被拒绝，不泄漏任务。
    15 | 多生产者先进先出 | 瞬时，超时 20 秒 | 并发普通投递；全部已接纳项恰执行一次，每个生产者同级 FIFO。
    16 | 尝试投递竞争 | 瞬时，超时 20 秒 | try 投递只重试未接纳项；无重复、遗漏，已提交项不得重发。
    17 | 多线程派生分发 | 瞬时，超时 20 秒 | 20 条工作线程、10 类任务有界派生 200 个子任务；全部完成并退出。
    18 | 异步投递后显式排空 | 瞬时，超时 20 秒 | 投递后调用排空接口；返回时已接纳任务全部完成。
    19 | 批量注销 | 瞬时，超时 20 秒 | 20 条运行线程批量停止并 join；管理器最终为空。
    20 | 空闲唤醒 | 瞬时，超时 20 秒 | 100 次空闲转工作；每轮都能及时执行，无丢失唤醒。
    21 | 反复生命周期与所有权 | 压力，超时 60 秒 | 100 次创建、投递和注销；任务存活数及注册数回到零。
    22 | 普通投递压力 | 压力，超时 60 秒 | 8 个生产者各 5000 项；精确数量，无重复、遗漏和同级倒序。
    23 | 尝试投递压力 | 压力，超时 60 秒 | 8 个生产者各 2000 个成功项；未提交才重试，精确交付。
    24 | 反复抢占重做 | 压力，超时 60 秒 | 50 轮受控抢占；每轮原级重做及同级队列顺序正确。
    25 | 固定种子优先级模型 | 压力，超时 60 秒 | 种子 0x5a17、2048 项；执行序列与独立稳定优先级排序相同。
    26 | 独立工作线程进展 | 瞬时，超时 20 秒 | 阻塞一条工作线程；另一条仍能完成自己的任务。
    27 | 停止查询与完成竞争 | 压力，超时 60 秒 | 4 个生产者共 2000 项与停止、查询并发；完整交付且无崩溃。
    28 | 配置门控与系统优先级 | 瞬时，超时 20 秒 | 配置验证、并发接纳及实际 OS 线程优先级；非法参数不破坏状态。
    29 | 有界任务接纳 | 瞬时，超时 20 秒 | 运行项及在途项共同占用硬上限；拒绝的新项不得触发抢占。
    30 | 业务异常隔离 | 瞬时，超时 20 秒 | 执行及工厂抛异常、工厂返回自身；工作线程继续，不重复释放。
    31 | 回调内自等待 | 瞬时，超时 20 秒 | 回调直接、经管理器或批量 join 自己；明确 SelfWait，不死锁。
    32 | 运行期系统优先级 | 瞬时，超时 20 秒 | 创建及运行期设置，失败反馈和关闭句柄拒绝；不伪报设置成功。
    33 | 动态线程池与分配器 | 瞬时，超时 20 秒 | 虚策略返回规模，内部裁剪最小最大值；扩缩容、分配和排空正确。
    34 | 任务序号耗尽 | 瞬时，超时 120 秒 | 白盒安排 UINT64_MAX 边界，最后合法序号成功，后续 SequenceExhausted；不回绕或泄漏额度。
    35 | 管理器序号耗尽 | 瞬时，超时 120 秒 | 白盒安排 UINT32_MAX 边界，最后 ID 可创建，耗尽及注销后均不复用 ID。
    36 | 帮助关闭优先级桶 | 瞬时，超时 120 秒 | 桶关闭与访问交错；可继续回收，不保留坏死桶或丢任务。
    37 | 迟到抢占通知隔离 | 瞬时，超时 120 秒 | 已完成投递的抢占通知迟到；不得停止后来无关的低优先级任务。
    38 | 读保护耗尽及恢复 | 瞬时，超时 120 秒 | 保护记录耗尽；错误语义正确，归还后恢复正常操作。
    39 | 初始化分配故障扫点 | 压力，超时 120 秒 | 逐个真实分配位置注入失败；部分初始化回滚，清理后可重建。
    40 | 热投递分配故障扫点 | 压力，超时 120 秒 | 热级别投递逐点故障；提交结果、所有权和后续执行一致。
    41 | 冷投递分配故障扫点 | 压力，超时 120 秒 | 首次级别创建逐点故障；未发布项不得残留或触发抢占。
    42 | 未发布级别不能抢占 | 瞬时，超时 120 秒 | 新高优先级创建失败；运行任务不被未接纳请求打断。
    43 | 有限级别容量复用 | 瞬时，超时 120 秒 | 有限容量反复切换 20 个优先级；已释放容量可复用，桶不无限残留。
    44 | 投递与排空竞争 | 压力，超时 60 秒 | 生产者仍在投递时关闭排空；每个成功接纳项恰执行一次。
    45 | 并发等待同一线程 | 压力，超时 30 秒 | 两个调用者同时 join；完成时均成功，不对外返回 Busy 或 Retry。
    46 | 并发线程池关闭 | 压力，超时 30 秒 | 排空和丢弃关闭交错；请求能完成，无自旋死等或残留工作线程。
    47 | 流量下线程池扩缩容 | 压力，超时 120 秒 | 10 波共 10000 项、规模 2 到 6；缩容不丢任务，不低于初始值。
    48 | 脱离心跳的唤醒 | 压力，超时 30 秒 | 心跳设为 60 秒，100 次空闲突发仍在短等待窗口内完成。
    49 | 持续有界压力 | 压力，超时 660 秒 | 默认 60 秒、8 生产者、4096 容量；已接纳项精确交付，退出后资源回落。
    50 | 管理器遍历保护耗尽 | 瞬时，超时 30 秒 | 遍历保护占满时执行接口；反馈正确，释放保护后可恢复。
    51 | 嵌套停止回调自等待 | 瞬时，超时 30 秒 | 受控 A 到 B 再到 A 的回调；识别自等待，不形成闭环死锁。
    52 | 线程池部分初始化回滚 | 瞬时，超时 30 秒 | 中途资源不足；已建线程全部收回，失败池不残留注册项。
    53 | 单例预热后并发访问 | 瞬时，超时 30 秒 | 启动线程串行预热，再并发访问；单例身份一致且无重复构造。
    54 | 属性 ID 边界 | 瞬时，超时 30 秒 | 默认、重复、零、负数及极值；按属性停止符合非唯一 ID 语义。
    55 | 并发重做顺序 | 瞬时，超时 30 秒 | 8 个线程请求重做、20 轮；代次不重复，高级先于原级重做。
    56 | 优先级缓存回退 | 瞬时，超时 30 秒 | 17 级及 INT_MAX 等超出快路径的级别；正确查询、抢占和执行。
    57 | 分离计数器回绕 | 瞬时，超时 30 秒 | UINT64 边界的生产消费计数；净数量及收尾状态正确。
    58 | 有界并发统计 | 瞬时，超时 30 秒 | 4 个生产者、容量 3、20000 项；逻辑上限、提交和完成计数一致。
    59 | 读域寿命及延迟回收 | 瞬时，超时 20 秒 | 读租约持有时关闭或回收；不能提前释放，租约结束后可回收。
    60 | 稳定任务槽位池 | 瞬时，超时 20 秒 | 容量、持有引用、扩缩容及并发分配；槽位不重用在途对象。
    61 | 槽位失败与缩容边界 | 瞬时，超时 20 秒 | 分配失败、策略拒绝或异常、旧读者暂停；回滚并守住初始容量。
    62 | 队列跨段就绪 | 瞬时，超时 20 秒 | 跨段 FIFO、空检查及显式回收；在途上下文不被释放，静止数量归零。
    63 | 最高优先级发布竞争 | 瞬时，超时 20 秒 | 8 生产者、16 级、160000 次发布和消费下调交错；最高级提示不漏项。
    64 | 队列分布计数压力 | 压力，超时 60 秒 | 独立生产者上下文、有界池与同步消费；精确计数、同线程 FIFO、收尾归零。
    65 | 有界前缀与异常提交 | 瞬时，超时 20 秒 | 0 预算、最大 16 项、跨段和第二次赋值异常；只提交成功前缀。
    66 | 预取后抢占重做停止 | 瞬时，超时 20 秒 | 已预取同级任务再来高级任务；维持优先级、重做队首及属性停止覆盖。
    67 | 高级任务即时唤醒 | 瞬时，超时 20 秒 | 200 次高级空闲突发、60 秒心跳；不能依赖心跳才能执行。
    68 | 外部队列条目 | 压力，超时 60 秒 | 借用寿命、混合 FIFO、异常前缀和并发交付；无提前释放或重复消费。
    69 | Map 缓冲维护 | 瞬时，超时 20 秒 | 并发维护、查询和释放复用缓冲；键值不丢失，安全回收。
    70 | 性能校验器自检 | 瞬时，超时 20 秒 | 注入数量相等的重复及缺失、级内乱序；必须拒绝伪正确样本。
    71 | 外部条目额度压力 | 压力，超时 60 秒 | 额度、扩缩容、失败、寿命和并发 FIFO；前缀预算生效，资源可归还。
    72 | 延迟重做工厂顺序 | 瞬时，超时 20 秒 | 普通执行不建副本，仅真正重做才取参数创建；原级重做先于同级积压。
    73 | 延迟工厂并发合并 | 瞬时，超时 20 秒 | 8 个请求同时抢占同一代；只创建一次，继承工厂可再次重做。
    74 | 延迟工厂失败 | 瞬时，超时 20 秒 | 普通及分配异常、空或自身返回、关闭和禁重做；后续任务可执行且不泄漏。
    75 | 工厂配置与记录资源 | 瞬时，超时 20 秒 | 配置复制失败保留旧值，继承失败释放新任务，记录耗尽反馈正确。
    76 | 工厂控制块分配失败 | 瞬时，超时 20 秒 | 接管返回裸指针时失败；对象必须释放，旧配置及后续任务不受破坏。
    77 | 工厂与关闭生命周期 | 瞬时，超时 20 秒 | 子任务自有工厂优先；在途工厂返回时已关闭，不得再提交或泄漏。
    78 | 待重做项与高级任务 | 瞬时，超时 20 秒 | 原级重做已准备但尚未执行时来高级任务；高级先，原级同级 FIFO。
    79 | 嵌套抢占 | 瞬时，超时 20 秒 | 两个优先级先后被打断；各在原级重做一次，最高级优先。
    80 | 已排队的更高级任务 | 瞬时，超时 20 秒 | 高级项已在队列后续位置；重做不能越过它。
    81 | 工厂发布竞争 | 瞬时，超时 20 秒 | 工厂执行过程中再投递高级或丢弃关闭；顺序和任务所有权正确。
    82 | 多代连续重做 | 瞬时，超时 20 秒 | 64 个继承工厂代次；延迟构造、原级和执行顺序正确。
    83 | 重做成本基准 | 瞬时，超时 20 秒 | 百万普通执行与 100 次合作打断成对计时；工厂次数准确，同时输出开销。
    84 | 首次高级发布与交接 | 瞬时，超时 20 秒 | 第一个高级桶发布恰遇当前任务交接；不能丢抢占或执行项。
    85 | 关闭与最高级交接 | 瞬时，超时 20 秒 | 丢弃关闭恰遇高级任务交接；运行任务不能漏 stopTask。
    86 | 回收批次帮助 | 瞬时，超时 20 秒 | 回收邮箱满且帮助者暂停；其他批次仍能推进，无重复释放。
    87 | 当前任务属性停止 | 瞬时，超时 20 秒 | 当前项属性匹配不受队列级别过滤影响；排队匹配通知去重且不删除。
    88 | 多线程并发注销 | 压力，超时 60 秒 | 16 个不同 ID 同时 join；成功返回的线程不再残留注册表。
    89 | 重入读保护耗尽 | 瞬时，超时 20 秒 | 回调占最后保护记录时重入；返回 SelfWait，退出回调后恢复。
    90 | 投递结果契约 | 瞬时，超时 20 秒 | 逻辑及物理容量、非法或关闭、try 竞争分别判定；普通 post 不返回竞争错误。
    91 | 普通投递竞争契约 | 压力，超时 60 秒 | 32 生产者正常投递；内部消化竞争，不外露 Busy 或 Retry，交付精确。
    92 | 动态额度归还竞争 | 压力，超时 60 秒 | 100 轮默认小池突发，每轮 8 乘 5000；可归还或扩容，FIFO 和总数准确。
    93 | 原生测试调度器自检 | 瞬时，超时 20 秒 | 合法及损坏 CSV、中位数、比较比例、非法 ID、超时和自有子进程清理；拒绝坏报告。
    94 | 公开用法之优先级 | 瞬时，超时 120 秒 | 使用例演示投递、查询和级别排序；独立账本核对并正常退出。
    95 | 公开用法之重做 | 瞬时，超时 120 秒 | 注册延迟工厂并受控打断；高任务和原级重做顺序准确。
    96 | 公开用法之停止 | 瞬时，超时 120 秒 | 当前、按属性和全部停止；合作停止及不删除队列语义成立。
    97 | 公开用法之资源 | 瞬时，超时 120 秒 | 设置 Map、Queue 和任务容量；失败反馈、快照及资源恢复正确。
    98 | 公开用法之生命周期 | 瞬时，超时 120 秒 | 创建、查询、排空、关闭和旧句柄；无悬挂或已关闭误接纳。
    99 | 公开用法之并发 | 瞬时，超时 120 秒 | 多线程公开接口投递；执行数量和每生产者顺序正确。
    100 | 公开用法之线程池 | 瞬时，超时 120 秒 | 公开策略、线程池和分配器；扩缩容下限、选择线程及关闭成立。
    101 | 公平性四生产者一级 | 压力，超时 150 秒 | 4 生产者、1 级、不限制 CPU，队列生产者记录 64。 判定采用上述全部公平性条件。
    102 | 公平性四生产者十级 | 压力，超时 150 秒 | 4 生产者、10 级、不限制 CPU，队列生产者记录 64。 判定采用上述全部公平性条件。
    103 | 公平性八生产者一级 | 压力，超时 150 秒 | 8 生产者、1 级、不限制 CPU，队列生产者记录 64。 判定采用上述全部公平性条件。
    104 | 公平性八生产者十级 | 压力，超时 150 秒 | 8 生产者、10 级、不限制 CPU，队列生产者记录 64。 判定采用上述全部公平性条件。
    105 | 公平性十六生产者一级 | 压力，超时 150 秒 | 16 生产者、1 级、不限制 CPU，队列生产者记录 64。 判定采用上述全部公平性条件。
    106 | 公平性十六生产者十级 | 压力，超时 150 秒 | 16 生产者、10 级、不限制 CPU，队列生产者记录 64。 判定采用上述全部公平性条件。
    107 | 公平性三十二生产者一级 | 压力，超时 150 秒 | 32 生产者、1 级、不限制 CPU，队列生产者记录 64。 判定采用上述全部公平性条件。
    108 | 公平性三十二生产者十级 | 压力，超时 150 秒 | 32 生产者、10 级、不限制 CPU，队列生产者记录 64。 判定采用上述全部公平性条件。
    109 | 公平性四核三十二生产者一级 | 压力，超时 150 秒 | 32 生产者、1 级、4 CPU，队列生产者记录 64。 判定采用上述全部公平性条件。
    110 | 公平性四核三十二生产者十级 | 压力，超时 150 秒 | 32 生产者、10 级、4 CPU，队列生产者记录 64。 判定采用上述全部公平性条件。
    111 | 公平性四核六十四生产者一级 | 压力，超时 150 秒 | 64 生产者、1 级、4 CPU，队列生产者记录 64。 判定采用上述全部公平性条件。
    112 | 公平性四核六十四生产者十级 | 压力，超时 150 秒 | 64 生产者、10 级、4 CPU，队列生产者记录 64。 判定采用上述全部公平性条件。
    113 | 公平性单核八生产者一级 | 压力，超时 150 秒 | 8 生产者、1 级、1 CPU，队列生产者记录 64。 判定采用上述全部公平性条件。
    114 | 公平性单核八生产者十级 | 压力，超时 150 秒 | 8 生产者、10 级、1 CPU，队列生产者记录 64。 判定采用上述全部公平性条件。
    115 | 单记录公平性八生产者十级 | 压力，超时 150 秒 | 8 生产者、10 级、4 CPU，仅 1 个队列生产者记录。 判定采用上述全部公平性条件。
    116 | 单记录公平性三十二生产者一级 | 压力，超时 150 秒 | 32 生产者、1 级、4 CPU，仅 1 个队列生产者记录。 判定采用上述全部公平性条件。
    117 | 单记录公平性三十二生产者十级 | 压力，超时 150 秒 | 32 生产者、10 级、4 CPU，仅 1 个队列生产者记录。 判定采用上述全部公平性条件。
    118 | 值队列空扫后归还 | 瞬时，超时 45 秒 | 私有源码夹具暂停空扫描，另一线程归还可用块；正确接纳并保持 FIFO。
    119 | 条目队列空扫后归还 | 瞬时，超时 45 秒 | 外部条目路径在空扫描后归还额度；已可用资源不能误判耗尽。
    120 | 部分归还发布竞争 | 瞬时，超时 45 秒 | 归还数量与可见发布交错；必须看到真实归还，最终 FIFO 及空状态正确。

四、超时、故障与可重复运行
    主注册项默认瞬时 20 秒、压力 60 秒；白盒默认 120 秒，独立压力/回归模块可覆盖。
    使用示例 120 秒；公平性外层 150 秒、其子测量 120 秒；私有 Queue 夹具外层 45 秒、
    子程序 30 秒。持续压力 ID49 外层 660 秒，默认运行 60000ms，可通过 CTTM_STRESS_MS
    在 1000 到 600000ms 内调整。精确超时来自各项 timeoutSeconds，不靠延长等待判通过。
    故障注入只在测试进程内截取分配；静态组合才覆盖被测分配，DLL 不伪装已注入。
    序号耗尽/计数回绕、寿命和回收白盒直接安排合法边界，随后通过真实入口和独立账本判定。
    不支持的系统优先级或权限问题按接口返回验证，不能默认为 OS 已提升优先级。
    重复用例入口：--repeat 55,78-82,91-92 3 <不存在的批次目录>。
    每轮复用上述真实测试入口，各有 TSV 与日志；子批超时 900 秒，失败立即停止重复。
    重做成本可用 --repeat 83 5 <新目录>；长压可单选 49，不能把短跑当数小时稳定性证明。

五、性能矩阵、原始数据及统计
    原生 C++ 入口取代脚本调度、CSV 解析、重复执行、交错比较和中位数统计：
      --matrix throughput 3 10000000 <新目录> [同测试接口的基线程序完整路径]
      --matrix latency 3 3200000 <新目录> [基线程序]
      --matrix prepared 3 10000000 <新目录> [基线程序]
      --matrix auxiliary 3 1000000 <新目录> [基线程序]
    轮数 1 到 10，任务数 1024 到 40000000 且能被 32 整除，目录父级已存在；不覆盖旧批次。
    throughput/latency 分别选择 throughput-reused/latency-window；prepared 覆盖全部八模式。
    每模式有 4/8/16/32 生产者乘 1/10 优先级；辅助 lifecycle/roundtrip 只用单生产者。auxiliary 总任务数最多 10000000，
    由调度器换算为每生产者 count；lifecycle/roundtrip 固定 1000 次。
    两个版本逐场景交错顺序，串行运行，不同时压测。只读取明确传入的基线程序，不从 Git
    猜测线上版本，不操作暂存区。比较程序须同 CLI、同数据格式、同输入/配置和相应 SDK。
    executables.tsv 记录可执行路径、FNV-1a-64 内容指纹及长度；指纹用于标识，不是安全签名。
    summary.csv 记录各指标、样本数、中位数、基线中位数和 (当前/基线-1)*100；基线为零记 NA。
    吞吐正值为提升，延迟/耗时/内存正值为增加。原始 CSV、日志和延迟分组保留在指定 temp。
    缺行、失败、非有限数、已接纳与完成不等、缺少延迟分组或不等样本数拒绝汇总为成功。
    统计不硬编码性能通过门槛；测试期间的后台负载、功耗/CPU/编译器要在正式验收记录中说明。

    单场景入口：--prepared-benchmark mode producers total levels report.csv。
    throughput：提前构造独立任务，生产和消费同时运行；创建及预热不计入测量。
    throughput-reused：提前建 2000 个任务对象循环投递；并发引用的对象没有可变业务状态。
    throughput-counted：增加精确完成计数，用于拆分统计开销；不代替默认完整语义结果。
    throughput-separated、throughput-reused-separated：分别测生产和排空，用于诊断，不能
    把分阶段峰值称为同时生产消费性能。其余普通吞吐场景保留真实投递与消费者并发。
    latency、latency-reused：记录提交及提交到执行响应，包含相应在途排队。
    latency-window：限制在途量测低积压响应；不能用饱和积压尾延迟代替接口调用成本。
    分别观察提交率、生产期间同窗口消费下界、含排空完成率、P50/P99、内存和错误。
    report.csv.timeline.csv 为进度，延迟场景另有 .samples.csv 与 .latency.csv。
    很小样本可能不足以覆盖每个级别的延迟分组，汇总会拒绝，正式测量用百万/千万级规模。

    辅助入口：--benchmark throughput|lookup|lifecycle|roundtrip producers count levels work report.csv。
    throughput 含调用方任务准备成本的辅助场景，lookup 测线程查找，lifecycle 测反复创建注销，
    roundtrip 测单任务往返；work 控制任务内计算。保留 CPU、延迟、内存、句柄和完整性统计。
    原生矩阵的 auxiliary 默认 work=0；需计算任务、单生产者等其他输入时使用此单场景入口。
    正式独立基准可用对应源文件的 CTTM_PREPARED_MAIN / CTTM_BENCHMARK_MAIN
    编译入口，不链接 CTaskFaultAllocation.cpp，避免测试分配替身影响结果；完整测试程序的
    内置测量适合回归，不把分配替身下的数据与不同构建口径直接对比。

六、保留的辅助覆盖与边界
    CTaskUsageTests 的七个公开接口用法场景已登记为 94 至 100，随本测试工程统一运行。
    基础 Queue 全套用例仍由 LockFreeQueueTest 的原生 C++ TestTool 构建、选择和执行；
    本工程保留实际用到的 Queue 模块及三项受控交错，不复制第二份基础库测试实现。
    线程栈诊断源码 CTaskHangSampler.cpp 可定义 CTTM_HANG_SAMPLER_MAIN 并链接 Dbghelp.lib 单独构建，
    参数为自己启动的同位宽测试子进程 PID；不得采集其他应用；
    CTaskPreparedProfiler 保留 CTTM_BENCH_SAMPLER/CTTM_BENCH_CPU_TRACE 开关；采样会干扰时序，
    不与无采样的正式成绩混算。--intrusive-prototype intrusive|segmented producers total levels report.csv
    只测队列原型，不是完整任务语义验收。
    工程不依赖 Python 测试脚本；历史版本打包、临时实验和报告整理不属于独立产品用例。
    测试产物放 src/temp/CTaskThreadManagerTest 或显式临时目录，不能把结果重新塞回源码树。
    Windows 字符路径由现有窄字符接口处理；测试根/SDK/编译工具含非本机代码页字符的组合
    尚无运行保证，优先使用可表示的路径。Linux/macOS、其他 ABI 及动态链接要分别验证。
    当前有限压力、存活计数和故障扫点不替代 ASan/TSan、完整堆检查及真实业务长时间运行。
    存在尚未完成根因闭环的访问违规诊断风险；一次回归未复现不能据此宣布该风险已消除。
*/

/** 稳定用例 ID，只能追加，不复用已分配编号
*/
enum CTaskTestId
{
    CTaskTestLifecycle = 1,
    CTaskTestInvalidPost = 2,
    CTaskTestPriorityFifoSnapshots = 3,
    CTaskTestRedoFront = 4,
    CTaskTestRedoOnce = 5,
    CTaskTestNoRedo = 6,
    CTaskTestNullClone = 7,
    CTaskTestEqualLowerNoPreempt = 8,
    CTaskTestStopPropertyLevel = 9,
    CTaskTestStopPropertyAllLevels = 10,
    CTaskTestStopCurrent = 11,
    CTaskTestStopAll = 12,
    CTaskTestStopCallbackReentry = 13,
    CTaskTestUninitAndStaleHandle = 14,
    CTaskTestMultiProducerFifo = 15,
    CTaskTestTryPostContention = 16,
    CTaskTestLegacy20ThreadDispatch = 17,
    CTaskTestPostAndExplicitDrain = 18,
    CTaskTestBulkUninitAll = 19,
    CTaskTestIdleWakeup = 20,
    CTaskTestRepeatedLifecycleOwnership = 21,
    CTaskTestProducerStress = 22,
    CTaskTestTryPostStress = 23,
    CTaskTestRedoRepeat = 24,
    CTaskTestSeededPriorityModel = 25,
    CTaskTestIndependentWorkers = 26,
    CTaskTestStopCompletionRace = 27,
    CTaskTestModelsGatesNativePriority = 28,
    CTaskTestBoundedTaskAdmission = 29,
    CTaskTestCallbackFailureIsolation = 30,
    CTaskTestCallbackSelfWait = 31,
    CTaskTestRuntimeSystemPriority = 32,
    CTaskTestDynamicPoolAllocator = 33,
    CTaskTestWhiteTaskSequence = 34,
    CTaskTestWhiteManagerSequence = 35,
    CTaskTestWhiteHelpClosedBucket = 36,
    CTaskTestWhiteLatePreemption = 37,
    CTaskTestWhiteProtectionExhaustion = 38,
    CTaskTestFaultInitAllocationSweep = 39,
    CTaskTestFaultHotPostAllocationSweep = 40,
    CTaskTestFaultColdPostAllocationSweep = 41,
    CTaskTestWhiteUnpublishedPriority = 42,
    CTaskTestWhiteBoundedPriorityReuse = 43,
    CTaskTestPostDrainRace = 44,
    CTaskTestConcurrentJoin = 45,
    CTaskTestConcurrentPoolShutdown = 46,
    CTaskTestPoolResizeTraffic = 47,
    CTaskTestWakeWithoutHeartbeat = 48,
    CTaskTestSustainedPressure = 49,
    CTaskTestManagerTraversalExhaustion = 50,
    CTaskTestNestedStopSelfWait = 51,
    CTaskTestPartialPoolInitRollback = 52,
    CTaskTestSingletonAfterPrewarm = 53,
    CTaskTestPropertyIdentifiers = 54,
    CTaskTestConcurrentRedoOrdering = 55,
    CTaskTestCachedPriorityFallback = 56,
    CTaskTestSplitCountersWrap = 57,
    CTaskTestConcurrentBoundedStatistics = 58,
    CTaskTestV3ReadDomain = 59,
    CTaskTestV3TaskStorage = 60,
    CTaskTestV3StorageFailure = 61,
    CTaskTestV3QueueReady = 62,
    CTaskTestV3ReadyIndex = 63,
    CTaskTestV3QueueCounters = 64,
    CTaskTestV3QueueBatch = 65,
    CTaskTestPrefetchedPriorityRedoStop = 66,
    CTaskTestPriorityWakeWithoutHeartbeat = 67,
    CTaskTestExternalQueueEntry = 68,
    CTaskTestMapBufferMaintenance = 69,
    CTaskTestPreparedBenchmarkValidator = 70,
    CTaskTestExternalEntryQuota = 71,
    CTaskTestLazyRedoFactoryOrder = 72,
    CTaskTestLazyRedoFactoryConcurrency = 73,
    CTaskTestLazyRedoFactoryFailure = 74,
    CTaskTestLazyRedoFactoryResources = 75,
    CTaskTestLazyRedoFactoryAllocation = 76,
    CTaskTestLazyRedoFactoryLifecycle = 77,
    CTaskTestV5PendingRedoPriority = 78,
    CTaskTestV5NestedPreemption = 79,
    CTaskTestV5QueuedHigherPriority = 80,
    CTaskTestV5FactoryPublicationRace = 81,
    CTaskTestV5RepeatedRedo = 82,
    CTaskTestV5RedoPerformance = 83,
    CTaskTestV5FirstPriorityRace = 84,
    CTaskTestV5ShutdownHandoff = 85,
    CTaskTestV5ReclaimBatches = 86,
    CTaskTestV5CurrentPropertyStop = 87,
    CTaskTestV5ConcurrentRemoval = 88,
    CTaskTestReentrantProtectionExhaustion = 89,
    CTaskTestPostResultContract = 90,
    CTaskTestPostContentionContract = 91,
    CTaskTestDynamicCreditReturnRace = 92,
    CTaskTestNativeAutomationSelfTest = 93,
    CTaskTestUsagePriority = 94,
    CTaskTestUsageRedo = 95,
    CTaskTestUsageStopping = 96,
    CTaskTestUsageResources = 97,
    CTaskTestUsageLifecycle = 98,
    CTaskTestUsageConcurrent = 99,
    CTaskTestUsagePool = 100,
    CTaskTestFairness41 = 101,
    CTaskTestFairness410 = 102,
    CTaskTestFairness81 = 103,
    CTaskTestFairness810 = 104,
    CTaskTestFairness161 = 105,
    CTaskTestFairness1610 = 106,
    CTaskTestFairness321 = 107,
    CTaskTestFairness3210 = 108,
    CTaskTestFairness321Cpu4 = 109,
    CTaskTestFairness3210Cpu4 = 110,
    CTaskTestFairness641Cpu4 = 111,
    CTaskTestFairness6410Cpu4 = 112,
    CTaskTestFairness81Cpu1 = 113,
    CTaskTestFairness810Cpu1 = 114,
    CTaskTestFairnessSingleRecord810 = 115,
    CTaskTestFairnessSingleRecord321 = 116,
    CTaskTestFairnessSingleRecord3210 = 117,
    CTaskTestQueueEmptyScanReturnValue = 118,
    CTaskTestQueueEmptyScanReturnEntry = 119,
    CTaskTestQueuePartialReturnPublication = 120,
};

/** 测试注册表的中文场景与独立预期，字符串为静态寿命
*/
struct CTaskTestCaseDescription
{
    // 不变的内部英文键，用于识别登记漂移
    const char* m_key;
    // 中文用例名
    const char* m_name;
    // 输入及独立预期
    const char* m_expectation;
};

/** 持有测试回调并记录任务存活数，回调借用对象须覆盖执行和停止通知
*/
class TestTask : public CTask
{
public:
    /** 建立可观察的测试任务
    @param [in] propertyId 用于按属性停止的非唯一编号
    @param [in] body 执行回调，异常转换成测试错误计数
    */
    TestTask(int propertyId, const std::function<void(TestTask&)>& body);

    /** 减少当前进程的测试任务存活计数
    */
    virtual ~TestTask();

    /** 执行测试回调并记录未处理异常
    */
    virtual void doTask();

    /** 发布停止标志并调用可选停止回调，可与执行体并发
    */
    virtual void stopTask();

public:
    // 是否已经收到合作停止通知
    std::atomic<bool> stopped;
    // 收到的停止通知次数
    std::atomic<int> stops;
    // 测试初始化时配置的停止回调，投递后冻结
    std::function<void()> onStop;
    // 当前子进程尚未析构的测试任务数量
    static std::atomic<int> live;
    // 执行、通知和清理记录的失败总数
    static std::atomic<int> errors;

private:
    // 构造时保存的测试执行体
    std::function<void(TestTask&)> m_body;
};

/** 拥有用例创建的工作线程编号，退出用例时负责清理
*/
class TestWorkers
{
public:
    /** 清理尚未显式排空的工作线程并记录清理异常
    */
    ~TestWorkers();

    /** 创建并登记一条默认工作线程
    @return 线程接口强引用，创建失败抛测试异常
    */
    std::shared_ptr<CTaskThread> add();

    /** 排空全部自有线程并从管理器注销，失败抛测试异常
    */
    void drain();

public:
    // 本夹具拥有的工作线程编号，仅由测试协调线程修改
    std::vector<uint32_t> ids;
};

/** 测试侧的执行顺序账本，不属于产品库的无锁实现
*/
struct TaskLedger
{
public:
    // 保护测试账本的互斥量
    std::mutex mutex;
    // 按实际记录顺序保存的任务标记
    std::vector<int> values;

public:
    /** 追加一次执行记录
    @param [in] value 用于验证执行顺序的任务标记
    */
    void add(int value);

    /** 复制当前执行记录
    @return 一次互斥保护下得到的完整账本
    */
    std::vector<int> snapshot();

};

/** 原任务与重做任务共享的门控及执行记录
*/
struct RedoState
{
public:
    // 原任务已经进入执行体
    std::atomic<bool> entered;
    // 允许原任务离开执行体
    std::atomic<bool> release;
    // 全部代次收到的停止通知次数
    std::atomic<int> stops;
    // 各代次的执行顺序
    TaskLedger ledger;

public:
    /** 建立未进入、未放行且通知次数为零的门控
    */
    RedoState();

};

/** 按代次记录重做顺序，工厂只在确认重做时生成下一代
*/
class RedoTask : public CTask
{
public:
    /** 创建一代测试任务并登记延迟工厂
    @param [in] state 非空共享门控和执行记录
    @param [in] generation 当前代次，0 为原任务
    @param [in] redo 是否响应重做请求
    @param [in] clone 是否允许工厂生成下一代
    */
    RedoTask(const std::shared_ptr<RedoState>& state, int generation, bool redo, bool clone);

    /** 记录代次，原任务在门控放行前保持执行
    */
    virtual void doTask();

    /** 累加共享停止通知次数
    */
    virtual void stopTask();

    /** 查询构造时指定的重做意图
    @return 是否要求重新执行
    */
    virtual bool reExecute();

private:
    // 持有全部代次共用的测试状态
    std::shared_ptr<RedoState> m_state;
    // 当前执行代次
    int m_generation;
    // 是否响应重做请求
    bool m_redo;
    // 是否允许工厂返回下一代任务
    bool m_clone;
};

/** 汇总当前工程的黑盒、白盒和压力用例
@return 可供独立子进程执行的用例注册表
*/
std::vector<BlackBoxCase> taskCases();

/** 测试用动态策略，原子修改算法输出，不改初始化配置。
*/
class TestPoolPolicy : public CTaskPoolPolicy
{
public:
    /** 初始化期望规模。
    */
    TestPoolPolicy();

    /** 保存输入并返回测试决策。
    @param [in] load 本轮负载
    @return 指定目标规模
    */
    virtual CTaskPoolDecision evaluate(const CTaskPoolLoad& load);

public:
    // 期望规模
    std::atomic<size_t> m_target;
    // 是否注入策略异常
    std::atomic<bool> m_throw;
    // 收到的初始线程数
    std::atomic<size_t> m_initial;
    // 收到的累计增长次数
    std::atomic<uint64_t> m_grows;
};

/** 故障任务直接把异常交给被测库处理。
*/
class TestFaultTask : public CTask
{
public:
    /** 保存行为及执行计数。
    @param [in] mode 1 执行异常，2 克隆异常，3 错误返回自身
    @param [in,out] calls 执行次数
    @param [in,out] entered 原任务已进入，覆盖本次测试线程生命周期
    @param [in,out] release 允许原任务返回，覆盖本次测试线程生命周期
    */
    TestFaultTask(int32_t mode, std::atomic<int32_t>& calls,
        std::atomic<bool>& entered, std::atomic<bool>& release);

    /** 执行计数并按模式抛异常。
    */
    virtual void doTask();

    /** 请求重做以实际触发已登记的故障工厂
    @return true
    */
    virtual bool reExecute();

private:
    // 故障类型
    int32_t m_mode;
    // 借用测试计数器
    std::atomic<int32_t>& m_calls;
    // 借用本用例进入通知，工作线程退出前有效
    std::atomic<bool>* m_entered;
    // 借用本用例放行标记，工作线程退出前有效
    std::atomic<bool>* m_release;
};
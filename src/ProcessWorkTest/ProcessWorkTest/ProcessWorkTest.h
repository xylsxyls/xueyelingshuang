#pragma once
/** ProcessWorkTest 测试说明

范围与入口
本工程只通过真实ProcessWork入口、公开Channel入口或测试进程自己打开的共享协议进行验证
白盒故障注入、崩溃子进程、账本、性能计时均在Test内；生产库不得含测试宏、friend或强制测试头
命令行：--list列出稳定ID；--instant、--stress、--all选择范围；--tests 1,3-5按ID选择；--report指定报告
可启动子进程的平台每例在独立进程运行，采用独立PID/名称/目录。iOS由设备宿主逐项启动。超时不计通过，析构仅清理该例自己的子进程和映射
默认串行执行各例；压力例内部才并发。功能通过不代表所有平台和所有时序均无缺陷

共同判定与收尾
普通消息：独立序号账本核对PID、正文、数量、线程内FIFO；不要求不同发送线程全局排序
失败路径：不得产生额外回调、遗失预算或留下永久占位；恢复后使用同一入口重新发送并核对
资源：回调仍执行时不提前归还计费；清理后任务数和在途计费为零，句柄/私有内存检查预热后平台
饥饿：32条同优先级活跃流在六个约一秒观察窗口各自前进；暂停流恢复后重新纳入检查
该观察不能证明任意系统调度下的绝对公平，也不允许把活跃流漏出账本以制造通过
崩溃：仅强杀本例创建的子进程；分别覆盖正文预留、段预留、发布后和尾指针未更新时机
普通send满载等待，默认trySend满载返回CAPACITY；普通及后台发送等待资源时，接收者死亡应令其退出
同名新代重新收发，旧映射不得指向新代。用例有独立超时和收尾

测试用例目录（精确输入、断言及超时以同ID的注册函数为准）
1 空生命周期；2 非法初始化及修正重试；3 名称长度/NUL边界；4 重复初始化不破坏现有接收器
5 PID同步发送；6 名称同步发送；7 PID异步副本；8 名称异步副本；9 混合寻址异步FIFO
10 trySend参数和大小边界；11 非法普通调用不交付；12 零正文容量；13 回调顺序与异常隔离
14 回调重入投递；15 缓存清理后继续通信；16 接收代次重启；17 异步排空再启动三轮
18 发布前不可见及单接收所有权；19 单通道满载CAPACITY不提交；20 零阈值按需扩容再缩容
21 禁止缩容；22 正阈值扩容；23 三进程PID发送；24 三进程名称发送；25 三进程PID投递
26 三进程名称投递；27 三进程混合投递；28 六字节实体123456；29 对端不存在及关闭
30 三十轮生命周期压力；31 三进程四线程同步压力；32 三进程四线程异步压力
33 三进程trySend背压；34 四发送者与缓存清理竞争；35 同名冲突仍支持PID；36 1024块边界
37 满通道关闭唤醒；38 回调自身关闭的借用寿命；39 64位发布与四线程跨32位边界加一
40 非法预算/溢出/保留内存；41 并发页安装及256/65536页边界；42 FIFO发布中断及陈旧代次
43 八线程各五万条段复用；44 -1无配置上限从1024扩到1152再1280；45 单轮扩容字节预算
46 死亡发送者正文预留；47 死亡发送者段预留；48 已提交发送者死亡；49 链接后尾提示前死亡
50 暂停者占位保留、其他流前进、确认死亡再回收；51 固定本地计费/超大单条拒绝/失败回滚
52 八线程各五千条三包预算通知；53 try投递的容量失败分类与未启动消费者自等待保护；54 慢回调背压
55 损坏共享下标或内联/独立正文包头时关闭通道；56 预热20轮再100轮资源平台；57 三进程16线程96万交付
58 两进程32线程96万交付；59 有限本地预算异步背压；60 32线程普通post共16万次无调用方竞争重试
61-65 对应46-50的真实强杀进程版本；66 接收进程崩溃、八发送者退出及同名重启四千次交付
67 接收批次复用及逐包计费；68 两万条0/6/64/65536字节正文并发；69 单槽36/37字节内联边界切换，并逐个验证0..36字节非对齐正文复制
70 六次发送进程强杀及96000条存活流消息；71 借用缓存无shared_ptr复制且重入删除安全
72 发送者死亡与慢回调背压交错；73 单槽32发送流公平进展；74 大小包、晚加入、暂停恢复公平进展
75 普通结果接口不返回竞争Busy、区分容量与资源；76 新流跨旧轮次边界首次轮询必须找到消息
77 系统正文映射失败返回错误并归还占位；恢复同一映射后仅交付一次37字节正文
78 登记未批准返回RESOURCE；单槽占满后try发送（含仅重试竞争）返回CAPACITY而非BUSY，恢复后只交付成功消息
80 通道快缓存借用期间并发清理，结束借用后释放；旧代次不得命中、重入安全回退、八线程哈希碰撞回收全部引用
81 空闲接收批次预分配与消息积压额度独立；16批次循环20轮检查完整正文、顺序和计费归还，并检查超标准大包能回收备用块
82 分别配置64/128字节Post槽：利用增长通知验证60/124字节刚好命中槽，超一字节及满池走堆分配，真实增长被截断为两槽；0/63/65/127/129无效槽大小配置不替换原池。7槽固定上限各循环40轮，覆盖52/53和116/117字节边界、真实池满回退、未释放槽不复用、批量归还池内/堆分配/空指针、二次归还无操作、在途配置及固定额度拒绝；前缀与逐条混用保序，先排空两条短批次再连续取96条，检查短前缀恢复后不丢失、不重复。两种槽大小各由8生产者并发40000次，验证16槽复用、FIFO和计费归还
83 已有正文等待者时仍可取得空闲队列段；归还正文按两个等待者的顺序直接授予，新请求不能从FREE抢走已授予资源，全部领取归还后释放正文
84 四生产者混合PID/名称post与持续清缓存并发，核对1200条不同大小正文、FIFO及前缀通道借用期间的寿命
85 同一实际线程交替使用两个Channel，跨正文页、队列段、槽位复用及通道重新打开后，核对28000条消息完整有序且无重复
86 同一接收Channel依次以513/257/3槽重新打开，每代四批发送，跨元数据页和槽位复用核对3092条消息完整FIFO，禁止借用上一代已释放的页
87 以不可访问内存页保护源记录两端，分别把0..129字节正文置于有效页的开头/结尾，验证260条记录复制不越界、完整内容、独立所有权、NUL及预算归还
79 四种重试开关、普通send/post容量等待、取消/关闭、自等待保护；4096次快速重试后连续八次各等待至少1毫秒，释放单槽后精确交付，无重复/遗漏

跨位数与平台
设置PROCESS_WORK_TEST_PEER为另一位数同版ProcessWorkTest.exe的绝对路径，mesh例交替创建两种节点
推荐23-28、31-33、57-59；分别核对两侧实际程序哈希。不要用协议不一致的旧版当作跨位数对端
Windows x86/x64以及DLL分别记录，不用一个通过结果冒充全部组合；其余原生端当前只有代码审阅
ProcessWork不支持Wasm跨进程；CTaskThreadManager的独立Wasm pthread消费测试在CTaskThreadManagerTest工程内

88 dynamic-local-budget（保留稳定ID及命令键）：固定592字节的精确满载、超额单项终止、额度不触发增长回调；-1申请超过128MiB不计费；未发布及回调中的对象阻止重新配置；接收282/283字节边界；0/-2拒绝；普通post及两个重试开关均true也不会等待永远不可能满足的单项额度。接收额度300、最大包64配置成功，正文24立即拒绝、23精确接纳；额度1配置仍成功，空正文发送立即拒绝
89 component-pool-configuration：MB设置的128MiB精确换算、-1、0/-2拒绝、最大有效MB及溢出保留原值；默认-1及Post/接收生命周期，真实存储增长，有限满载分类，真实接收池增长的拒绝及异常不接纳消息、不发成功通知，恢复后仍可发送；配置确实进入内部Map及任务线程；零任务容量启动失败回滚，修正后重新初始化成功
90 native-growth-notification：Queue、Map节点池、Hash记录池真实增长；初始化不通知，次数和池统计一致；观察回调抛异常仍完整提交，Queue逐项FIFO
91 packet-pool-dynamic-growth（压力）：2个预留槽增长到9槽，超大建议内部裁剪为7槽，之后走预算内分配；32线程各128轮、每轮16包，独立标记校验无重叠、全部完成、计费归零
92 shared-growth-notification：真实Channel从4块增长到有限6块；满载请求未提交，等待维护窗口后扩容，成功通知只包含实际2块；此前4条消息完整且有序
93 reservation-window-capacity：1/63/64/65/129/255/256/257/511/512/513块依次填满再排空，各4轮，核对10504条FIFO；扫描提示不得隐藏窗口外空位、突破容量或把满载误报Busy
88至92均不向生产代码注入测试宏或状态；跨架构使用同样的公开及模块入口。线程全部完成是该次测试的进展证据，不是任意系统调度的形式化无饥饿证明

99 configuration-copy-failure：让配置中的函数对象在复制时抛出异常，Channel必须返回false且不发布接收器；关闭故障后同一实例重新配置，真实发送123456并逐字节核对，释放后无在途消息。标准用例超时75秒，退出由局部对象清理映射和批次
100 prepare-send-mappings：6/64/65536字节正文、257块跨页边界；拒绝非法PID、零/非法前缀及超过初始容量；仅元数据、含正文和重复准备均成功且不扩容、不交付消息；随后send/post各一条逐字节核验。41同时验证扩容正文退役后地址清空、重新创建不复用旧地址、发送侧不缓存可缩容正文
101 granted-body-page（压力）：分别使用513块/64字节正文及64块/65536字节正文，八生产流各发送2000条不同正文；先暂停消费填满共享池，再恢复消费，各场景逐流核对16000条正文、PID和序号，覆盖跨页授予及大包满载恢复
102 prepared-sender-crash：子进程prepareSend后使用公开send发送100条64/65536字节正文，父进程暂停pump，等所有send返回成功后强杀子进程，再消费并逐条核对100条正文、PID及FIFO；发送进程退出不影响已提交正文
103 segment-locality：单流分别测试内联和非内联消息，每种使用1024段池，连续30000次入队/消费应仅复用第一页队列段；再暂停消费积压16000项，验证自动使用后续页并完整按FIFO收齐，共核对92000项，局部复用不限制全池容量
104 idle-grant-notification：64块、最大正文65536字节，真实发送并排空一条消息后保留已合并的通知标记；通过共享队列request模拟发送者满载扫描之后迟到的容量登记，不额外唤醒。接收器waitForData必须在等待前服务已有空闲资源的请求，takeGrant只能成功一次，再用授予槽发布一条64KiB正文并核对完整交付且无重复；不以耗时阈值判断成功。普通用例超时75秒，退出由本例Channel和映射所有者回收资源
105 sender-token-reuse：同一接收/发送Channel对象重开三次，每代预置不同的批准号，核对24次正文所有权均属于当代批准号，随后逐项验证完整头部和正文；不能以每代批准号都为1掩盖旧缓存
106 native-failure-diagnostic【Windows瞬时，测试设施】：启动独立子进程，用含0xE0420001的合成SEH记录调用诊断处理函数，然后以相同错误码退出；父进程必须观察到失败退出并读到原始异常码，不把崩溃当作成功。十秒超时，日志仅归该用例。性能准备阶段的提前退出另记录原始Windows退出码及子日志路径；诊断只在失败路径运行，不参与吞吐热路径
107 post-limited-cpu【Windows压力】：独立用例进程临时限制到一个可用CPU，八生产者与主线程消费者共用4096字节固定积压额度及512个正文/队列槽。持续运行3.1秒，在三个逐秒窗口逐流检查前进；停止后核对各流成功提交数、接收数、精确FIFO和零在途所有权。正常和异常退出均恢复原亲和性；压力用例超时150秒
notification-failure为独立故障构建：--tool build ROOT OUTPUT x64 notification-failure，仅在OUTPUT私有Semaphore副本中令initializationResult报告NO_MEMORY，随后自动运行--notification-failure。验证两种本地队列配置及公开configurePostMemory/initPostThread/initReceive失败且不遗留线程或接收器；未注入的普通构建执行此入口必须失败，不计入常规用例
普通初始化和发送的成功用例同时检查bool返回及最终账本；非法普通send/post检查返回false。对端包长不符但本地合法的post应先返回true，后台丢弃该不可发送项，再用同流屏障核验无错误交付，不能把异步接纳误当远端验证成功

性能入口与有效性
同目录ProcessWorkBenchmark.cpp提供原生C++性能入口：进程数 每进程线程数 每线程包数 正文字节 共享块数 模式
模式0为PID send，1为名称send，2为post，3仅为Channel诊断。模式3不得冒充公开ProcessWork性能
正文6字节时必须严格为123456；任务、正文、线程、初始共享容量及预热在计时前准备
多进程生产和父进程回调消费同时发生；第一条发送开始到最后一次发送返回算生产时间，到最后回调算消费时间
Post生产量只表示本地接纳，必须同时给端到端消费量；失败的未提交资源请求可重试，成功不得重复投递
普通send/post满载及CAS竞争均在库内重试，性能测试不额外替库循环；比较输入使用相同的业务交付语义。消息体预先创建，生产和消费同时运行，退避时间计入真实端到端吞吐
每包仍验证流内序号/PID/正文；任何错误、超时、缺包不输出有效吞吐，首错明细独立保存
正式六字节比较使用5000万包和至少三轮交错顺序，报告每轮原值、中位数与相对基准增长率，清楚注明基准是固定Git HEAD还是本轮优化前快照
预分配共享块、接收批次和Post小包/节点池在计时前完成；单独的诊断副本记录增长、正文新申请、映射和容量等待，诊断吞吐不冒充正式数据。不能仅凭最终容量不变就宣称全程无扩容或无背压
线上基准取固定Git HEAD，不用任意旧备份；无法直接构建时，私有接口适配必须逐项记录
原生C++的--tool build只在指定新目录组合源码/头/库，不发布共享SDK；source-inputs及compiled-inputs记录实际编译输入指纹
设置PROCESS_WORK_BENCH_PREPARE_SEND时，在生产进程计时前调用prepareSend准备全部初始映射；mapping_prepare_total_ms和mapping_prepare_max_ms单独报告各进程准备耗时之和及最大值。未设置时仍按需映射；两种模式分别报告，准备成本不得隐去
正式基准与诊断副本分开构建；诊断计数会改变成本，不能将其吞吐作为正式性能数据
后台编译、模拟器或索引的影响写进报告；未得到空闲环境的测试只能标为当前负载测量，不能宣布峰值达标

十、用例执行契约与新增入口

五端共用1至109的稳定ID。106为Windows原生诊断，107为Windows单CPU亲和性；其他系统将这两项标为未执行。
108为逐流账本反例。109为依赖参数的实际传递、错误隔离和恢复。Android和iOS设备宿主按 --case ID 分次启动。
iOS签名应用无法自行启动独立对端，这类用例返回77，由CTest标注跳过。Windows或模拟器结果不计作其他系统实机通过。
每个用例的中文目的见本注释目录，命令键、独立预期、压力标志由processCases登记；--list列出实际清单
瞬时用例75秒、压力用例150秒超时，跨位数工具各屏障120秒、收尾15秒，性能每样本240秒
不同工具重复同一用例的次数不得算成新的唯一用例数；未执行、平台不可运行和失败不计通过

stream-ledger-negative-control【瞬时，测试设施】：直接喂入合法对端、本进程误路由、未知来源、重包及乱序五组输入
预期网状通信账本只接受合法对端的完整有序消息；显式自发自收配置仍允许本进程来源。该自检不创建真实进程，也不替代实际多进程网状通信测试

通用前置：匹配当前头/库的Release程序，系统允许创建共享映射与子进程；每例在独立进程启动，计费/回调/缓存为空
通用触发：通过公共ProcessWork或明确的模块入口发消息；故障例通过测试自己打开的共享协议或自己的子进程构造时序
独立预期：按发送进程、生产线程、预先确定的序号与全部正文核对，不从待测队列自身计数推算应收到多少条
禁止事件：重复/遗漏/同流乱序、失败请求产生回调、仍在借用时回收、关闭后旧代次复活、普通CAS竞争向外报告Busy
观察：功能/压力逐条账本；公平例按每条活跃流分别检查；性能逐流序号和正文核对，错误样本没有有效吞吐
清理：停止自有生产者，按用例意图排空或取消，停止工作线程后释放回调；manager线程数须归零。子进程由BlackBoxChild作业对象/进程组限定归属
证据：--report逐项写ID、名称、结果和用时，失败保留对应日志；报告写入失败为真实失败。临时输入使用独立PID/前缀，不复用旧完成文件

94 automation-self-test【瞬时，测试设施】：输入有效/截断/重复键/非有限值JSON、奇偶样本和正常/错误退出子进程
预期23项性能工作负载未减少，严格拒绝坏报告，正确求中位数，错误退出不能通过。仅清理自产日志
95 payload-capacity-restart【瞬时，接口容量】：依次建立65536/1/0正文容量接收器，在同PID保留旧Channel
触发最大包发送、关闭重开、超限发送、空包；预期三条正文/类型精确，旧Channel容量归零且不能访问新代，失败不占槽
96 message-type-domain【瞬时，类型契约】：五条路径各11种非负边界类型，正文127字节；INT32_MIN/-2/-1覆盖五个拒绝入口
预期55条全部字节及类型准确、发送后修改输入不改变交付、负类型不交付；检查MessageType有符号32位及公开枚举数值
97 guarded-source-input【瞬时，输入借用】：五条路径×28种0..65536长度×首尾保护页，共280条
触发发送返回后把源区域设为不可访问；Post在撤销访问后才启动消费者。预期无越界/延迟借用，全部正文、零包nullptr、PID及FIFO一致
Windows用VirtualProtect，原生POSIX用mprotect；释放本次完整映射。进程崩溃由父控制器记录失败，不能跳过危险输入
98 overload-rejection-fifo【压力，所有重载】：send/post两模式×0/1/6/65536正文容量，1023字节名称、四生产者各512次
每16条夹入超限、空指针非法长度、负长度、缺失目标，合法调用轮换四种重载并立即改写源正文
预期16384条合法消息完整且每流FIFO，无拒绝消息回调。Post远端超限/不存在可本地接纳，后台拒绝不算本地接纳失败；非法本地参数必须失败

109 dependency-options【瞬时，参数及生命周期】：拒绝非法进程目录；配置本地通知的事件池、计数池和等待切片并直接检查
实际池容量和切片。分别向就绪/备用队列注入非法及合法的调度参数，核对拒绝后原值；Post快速重试预算取0和2时
核对实际尝试计数。向共享收、发通道的四个通知分别注入无效池上限，只有对应端的打开失败；
发送端登记池不合法仍能启动接收器。恢复配置后收齐一条正文并退出，受管线程数归零。

当前版本功能验证（2026-11-22）
Windows Release 的 x64 静态、x86 静态和 x64 DLL 各自运行全部 109 项，均通过 87 项瞬时和 22 项压力用例，零失败、零跳过。
混合 x64/x86 三节点四生产线程验证五种发送模式，共核对 312000 条交付。独立运行注释示例，收到预期的 14 条消息。
Linux、macOS、Android、iOS 本轮仅检查源码及构建入口，没有实机通过数据。

当前性能数据
本版缺少与当前源码、依赖及构建指纹匹配的吞吐量、P99延迟和映射准备耗时。本轮只执行功能、压力及故障验证。
配置、查询和停止不单列包吞吐，未将用例总耗时换算成吞吐。基准输出生产和消费的原始轮次、中位数、最小/最大值
及（最大值－最小值）÷中位数的百分比；少于三轮不输出版本变化百分比。分布明显重叠或离散过大时不能据此判断小幅优化。

十一、原生C++调度与构建

默认VS2013/v120 Win32/x64工程把功能、压力、性能和工具编译进同一ProcessWorkTest.exe
仓库标准SDK构建后可直接构建该工程；测试用私有SDK可通过BlackBoxIncludeDir、BlackBoxLibraryDir、BlackBoxStatic=true选择，避免混用全局SDK
原生CMake使用SDK_ROOT指向匹配目标架构的SDK；PlatformTest提供设备宿主，LIBRARIESRELY可补充系统或库依赖。移动端使用宿主传入的SHARED_MEMORY_TEST_DIR准备身份记录、映射和通知。private VS build仅Windows可用，其他平台明确报不支持，不伪造通过
性能独立CMake目标ProcessWorkBenchmark仍保留；与主程序调用同一个RunProcessWorkBenchmark，避免两份计时实现

命令示例（OUTPUT父目录已存在、OUTPUT本身必须不存在；路径可加引号）：
  ProcessWorkTest.exe --all --report D:\runs\functional.csv
  ProcessWorkTest.exe --tests 95-98 --report D:\runs\compatibility.csv
  ProcessWorkTest.exe --benchmark 1 32 1562500 6 262144 0
  ProcessWorkTest.exe --tool workloads
  ProcessWorkTest.exe --tool matrix D:\runs\six 3 - D:\build\ProcessWorkTest.exe --cases pid-1x1-6,pid-1x4-6,pid-1x8-6,pid-1x16-6,pid-1x32-6,pid-2x4-6,pid-4x4-6,name-2x4-6,post-1x4-6,post-1x1-6 --messages 50000000 --blocks 262144
  ProcessWorkTest.exe --tool matrix D:\runs\compare 3 D:\baseline\ProcessWorkBenchmark.exe D:\current\ProcessWorkTest.exe --cases pid-1x4-64
  ProcessWorkTest.exe --tool matrix D:\runs\direct 3 - D:\build\ProcessWorkTest.exe --custom 1,4,6,1024,3 --messages 10000000
  ProcessWorkTest.exe --tool cross D:\runs\mixed D:\x64\ProcessWorkTest.exe D:\x86\ProcessWorkTest.exe
  ProcessWorkTest.exe --tool build D:\xueyelingshuang D:\runs\native x64 tests
  ProcessWorkTest.exe --tool build D:\xueyelingshuang D:\runs\dll x64 dll
  ProcessWorkTest.exe --tool build D:\xueyelingshuang D:\runs\demo x64 demo
  ProcessWorkTest.exe --tool build D:\xueyelingshuang D:\runs\audit x64 memory
  ProcessWorkTest.exe --tool build D:\xueyelingshuang D:\runs\profile x64 profile
  ProcessWorkTest.exe --tool build D:\xueyelingshuang D:\runs\maps x64 map

build支持tests/dll/benchmark/memory/profile/demo/map/notification-failure，架构x64或x86。demo提取生产ProcessWork.cpp的注释示例原文，编译并要求14条全部完成
map重建LockFreeMapTest既有完整用例并执行--all，不把Map模型写进ProcessWork生产代码
tests/dll/benchmark生成集成功能和性能的ProcessWorkTest.exe；需要跑哪些用例由后续命令明确选择，不把仅编译当成测试通过
DLL组合还应执行76、104：由调用方直接登记内部生产端，再经模块内入队和接收完成完整账本，检查MSVC导入边界的局部TLS访问；仅静态链接通过不能替代该组合
所有源及头复制到新建隔离目录，静态依赖与ProcessWork一起构建；dll只改变私有副本的链接宏。记录原始/实际编译输入、可执行文件FNV-1a-64指纹与长度，不将其称为安全签名
诊断只改私有副本，锚点不唯一或已经变化必须失败并重新审阅，不能忽略。所有生产文件不增加测试宏、测试friend或测试头

cross固定三节点按第一种/第二种/第一种程序启动，五组mode0/1/4/5/6分别覆盖PID、名称、混合Post、trySend、tryPost
每节点四生产线程，mode4每流1000条，其余3000条，合计312000条跨节点交付；以ready/pids/armed/go/done/finish屏障核对，提前退出或超时失败
也可设置PROCESS_WORK_TEST_PEER为另一架构程序运行原mesh用例，编译器/协议必须匹配

matrix标准23项完整保留：6字节的PID 1x1/4/8/16/32、2x4、4x4，名称2x4、Post1x4/1x1；
64字节同七种PID组合、名称2x4、Post1x4；4096/65536字节2x4；空包1x4；64字节1x4的一千万包长测
--messages覆盖总包数，必须能整除生产进程×线程数；--blocks覆盖初始共享容量；--custom可独立设置进程/线程/正文/块数/路径
PROCESS_WORK_BENCH_UNLIMITED_LOCAL环境变量存在时使用-1本地额度，未设置时使用基准的有限额度，identity中记录此条件
PROCESS_WORK_BENCH_POST_SLOT_BYTES可设置64/128；未设置时使用各自库默认值，identity及每样本post_slot_bytes记录实际值。Post正文池按相同1GiB字节预算预留：64字节槽16777216个，128字节槽8388608个；Queue节点初始16777216。报告同时注明步长、槽数和字节预算，不能称作相同槽数；显式调优和默认配置分列
每轮交错基准与当前程序执行顺序，原始log和samples.csv逐样本写入；summary.csv含每组中位数、最小/最大、实际通过/失败轮数及相对基准的生产/消费变化百分比
任意一轮失败的组不输出有效中位数/比较，不把失败样本删除后只统计漂亮结果。失败后独立场景继续，整批最终返回非零
无基准填-，只能报告当前测量，不能声称性能增长。按二进制和配置指纹保留原始样本与汇总

memory十类计数：正文预留失败、容量重试、接收新批次、Post堆分配、Queue增长尝试、Map运行期增长尝试、元数据页安装、共享容量不足、Post等待、接收额度不足
profile九类线程局部计数：发送次数、正文扫描、正文预留失败、段扫描、段预留失败、接收流访问、发布次数、正文CAS失败、空间等待检查
基准逐包检查完整正文，每个实际发送流在业务计时结束后经相同路径发送结束屏障；全部屏障收齐、回调关闭后核对最终账本
94包含校验器的中段损坏、缺包、重包、结束后迟到及重复屏障反例，以及数字参数符号/尾随字符/溢出和custom参数缺项、多项、空项、尾随逗号反例
增长尝试不等于成功增长；真实成功次数由基准的策略grown及各池m_onGrowth补充。初始化/预热计时外完成，诊断副本与正式吞吐分别运行

十二、覆盖范围

交叉进程通信、顺序、重试、资源回收和故障恢复按当前接口和共享布局验证。系统身份解析和网关
集成由相应工程验证。移动应用宿主的沙箱、签名和进程启动条件需要在设备上单独验证。
*/
#include "BlackBoxTestSupport.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <mutex>

/** 已复制的接收消息，正文所有权属于测试账本
*/
struct ReceivedPacket
{
    // 实际发送进程号
    int m_pid;
    // 消息类型字段携带的测试序号
    int m_type;
    // 空正文时回调是否收到空指针
    bool m_nullData;
    // 在回调有效期内复制的完整正文
    std::string m_body;
};

/** 捕获回调异常并保存正文副本，snapshot可与回调并发
*/
class PacketLog : public ProcessReceiveCallback
{
public:
    /** 创建空消息账本及错误计数
    */
    PacketLog();

    /** 复制正文并运行额外断言，失败记入m_errors以避免被观察者异常隔离掩盖
    @param [in] pid 实际发送进程
    @param [in] data 本次回调内借用的正文，空正文为nullptr
    @param [in] length 正文字节数
    @param [in] type 发送端附带的测试序号
    */
    virtual void receive(int32_t pid, const char* data, int32_t length, MessageType type);

    /** 记录容量通知，无效容量计为测试错误
    @param [in] count 当前共享块数，须为正
    */
    virtual void bufferFull(int32_t count);

    /** 在互斥保护内复制当前已完成回调的账本
    @return 独立消息副本；不能代替发送结束及接收收尾屏障
    */
    std::vector<ReceivedPacket> snapshot();

public:
    // 跨线程累计的回调校验错误
    std::atomic<int> m_errors;
    // 容量通知次数
    std::atomic<int> m_fullEvents;
    // 启动接收前设置的额外断言，捕获对象必须覆盖接收生命周期
    std::function<void(const ReceivedPacket&)> m_action;

private:
    // 仅测试账本使用的同步锁，保护快照与追加
    std::mutex m_mutex;
    // 按回调顺序保存的消息副本
    std::vector<ReceivedPacket> m_packets;
};

/** 每次接收都故意抛异常，用于验证生产库的观察者隔离
*/
class ThrowingReceiver : public ProcessReceiveCallback
{
public:
    /** 故意抛出测试异常，验证库仍继续调用其他观察者
    @param [in] pid 实际发送进程
    @param [in] data 本次回调内借用的正文，空正文为nullptr
    @param [in] length 正文字节数
    @param [in] type 发送端附带的测试序号
    */
    virtual void receive(int32_t pid, const char* data, int32_t length, MessageType type);
};

/** 独占本测试进程的ProcessWork接收生命周期，不拥有回调对象
*/
class IpcScope
{
public:
    /** 创建接收生命周期守卫，尚不初始化接收器
    */
    IpcScope();

    /** 关闭接收与异步线程，清理失败记录到测试设施
    */
    ~IpcScope();

    /** 注册回调并初始化本用例接收器，失败抛出测试异常
    @param [in] receiver 借用回调，必须活到close完成
    @param [in] name 接收名称
    @param [in] bytes 共享块字节数，包含12字节内部头
    @param [in] blocks 初始共享块数
    @param [in] threshold 剩余块扩容阈值
    @param [in] increment 每次扩容块数
    @param [in] idle 缩容空闲秒数
    @param [in] rounds 扩容轮数上限
    */
    void start(ProcessReceiveCallback* receiver, const std::string& name, int bytes = 65548,
        int blocks = 8, int threshold = 0, int increment = 0, int idle = 0, int rounds = 0);

    /** 等待接收生命周期结束，可重复调用；外部发送任务须先收敛或能因关闭退出
    */
    void close();

private:
    /** 禁用复制，避免重复拥有同一个接收生命周期
    @param [in] other 不可复制的守卫
    */
    IpcScope(const IpcScope& other);

    /** 禁用赋值，避免覆盖接收生命周期所有权
    @param [in] other 不可复制的守卫
    @return 不提供实现
    */
    IpcScope& operator=(const IpcScope& other);
};

/** 捕获工作异常并发布原子结果的测试任务
*/
class IpcJob : public CTask
{
public:
    /** 保存待执行的测试动作
    @param [in] body 动作副本；捕获的外部对象必须活到任务退出
    */
    explicit IpcJob(const std::function<void()>& body);

    /** 运行测试动作，捕获异常并发布失败标记
    */
    virtual void doTask();

    /** 记录意外中止，正常排空不应触发此状态
    */
    virtual void stopTask();

public:
    // 任务动作抛异常时发布的失败状态
    std::atomic<bool> m_failed;
    // 收到取消通知时发布的状态
    std::atomic<bool> m_stopped;

private:
    // 持有测试动作，捕获的借用对象由用例负责保持有效
    std::function<void()> m_body;
};

/** 持有测试任务和已登记线程，析构前必须允许各动作结束
*/
class IpcJobs
{
public:
    /** 收回尚未排空的任务线程，借用对象必须活到此析构结束
    */
    ~IpcJobs();

    /** 创建一条托管生产线程并投递测试动作
    @param [in] body 测试动作，失败转为测试异常
    */
    void add(const std::function<void()>& body);

    /** 等待所有动作完成并注销线程，检查动作异常及意外中止
    */
    void drain();

private:
    // 已创建的托管线程编号，0表示已完成回收
    std::vector<uint32_t> m_ids;
    // 保持任务对象并在收尾时核验结果
    std::vector<std::shared_ptr<IpcJob>> m_tasks;
};

/** 单回调线程验证逐进程、逐线程FIFO并发布完成和错误计数
*/
class StreamReceiver : public ProcessReceiveCallback
{
public:
    /** 建立多进程逐线程序号账本
    @param [in] threads 每进程线程数
    @param [in] packets 每线程应发送的包数
    @param [in] fixed true正文为123456，false按用例长度序列生成
    */
    StreamReceiver(int threads, int packets, bool fixed);

    /** 核对PID、逐流FIFO及完整正文，所有校验错误发布到m_bad
    @param [in] pid 实际发送进程
    @param [in] data 本次回调内借用的正文，空正文为nullptr
    @param [in] length 正文字节数
    @param [in] type 发送端附带的测试序号
    */
    virtual void receive(int32_t pid, const char* data, int32_t length, MessageType type);

    /** 在开始发送前发布发送进程表及空序号账本
    @param [in] pids 进程编号对应的实际PID；调用时无回调并发
    @param [in] excludedPid 禁止接收的来源PID；网状通信填本进程PID，0表示不排除来源
    */
    void configure(const std::vector<int>& pids, int32_t excludedPid = 0);

public:
    // 已执行的回调总数，包括校验失败的消息
    std::atomic<int> m_received;
    // 正文、来源或FIFO校验失败的消息数
    std::atomic<int> m_bad;

private:
    // 每个发送进程的生产线程数
    int m_threads;
    // 每条发送流的预期消息数
    int m_packets;
    // true固定使用123456正文，false使用混合长度
    bool m_fixed;
    // 网状通信排除本进程来源，避免错误回送被计作其他节点的交付
    int32_t m_excludedPid;
    // configure完成后发布只读PID和序号账本
    std::atomic<bool> m_configured;
    // 初始化后只读的发送进程表
    std::vector<int> m_pids;
    // 仅回调线程更新的各流下一预期序号
    std::vector<int> m_next;
};

// 使用Channel公开接口，控制pump时机以确定性制造满载；不接触共享控制区或private成员。
class PublicChannelScope
{
public:
    /** 准备由测试线程手动泵送的64字节正文通道
    @param [in] initial 初始块数
    @param [in] threshold 剩余块扩容阈值
    @param [in] increment 每次扩容块数
    @param [in] idle 缩容空闲秒数
    @param [in] rounds 最大扩容轮数
    */
    PublicChannelScope(int initial, int threshold, int increment, int idle, int rounds);

    /** 静止关闭双方通道并释放未取出的测试消息
    */
    ~PublicChannelScope();

    /** 运行接收维护并验证本进程发出的单字节x正文
    @return 本次通道泵送的消息数
    */
    int pump();

public:
    // 由用例主线程独占维护的接收通道
    ProcessWorkChannel m_receiver;
    // 本进程发送通道
    ProcessWorkChannel m_sender;
    // 已完整验证的消息序号
    std::vector<int> m_delivered;
    // 共享通道容量通知值
    std::atomic<int32_t> m_full;

private:
    // 公共pump输出的消息，正文由夹具释放
    LockFreeQueue<char*> m_queue;
    // 公共pump输出队列的通知信号
    Semaphore m_notification;
};

/** 生成可独立重建的测试正文
@param [in] value 消息序号
@param [in] length 非负正文字节数
@param [in] fixed true生成从字符1起的字节序列，六字节时为123456
@return 正文副本
*/
std::string packetBody(int value, int length, bool fixed = false);

/** 生成带本用例PID的接收名称
@param [in] suffix 区分场景的后缀
@return 接收名称
*/
std::string receiveName(const std::string& suffix);

/** 验证网状通信账本拒绝误回送、错误来源、重包和乱序，同时允许显式的自发送用例
*/
void testProcessWorkStreamLedger();

/** 经指定公开接口发送测试包，失败抛出测试异常
@param [in] mode 0/1为PID/名称send，2/3为PID/名称post，4混合post，5为trySend，6为tryPost
@param [in] pid 目标PID
@param [in] name 目标名称
@param [in] body 调用期间有效的正文
@param [in] type 测试序号；try类仅重试明确未提交结果
*/
void sendPacket(int mode, int pid, const std::string& name, const std::string& body, int type);

/** 等待账本达到精确数量并检查回调错误
@param [in,out] log 本例账本
@param [in] count 本阶段累计期望数；最后还需停止发送并检查收尾账本
*/
void waitPackets(PacketLog& log, size_t count);

/** 运行真实跨进程测试节点并验证全部交付
@param [in] argc 参数数量
@param [in] argv 本进程命令行参数
@return 0完整通过，非0失败
*/
int runIpcNode(int argc, char** argv);

/** 启动隔离节点全互联通信，核验每个节点退出状态
@param [in] mode 发送接口模式
@param [in] fixed true固定六字节正文
@param [in] packets 每线程向每个对端发送的包数
@param [in] nodes 进程节点数
@param [in] threads 每节点生产线程数
@param [in] initial 接收初始共享块数
*/
void meshTest(int mode, bool fixed, int packets, int nodes, int threads, int initial = 8);

/** 构造稳定ID、超时与独立预期的测试注册表
@return 当前平台实际支持的用例
*/
std::vector<BlackBoxCase> processCases();
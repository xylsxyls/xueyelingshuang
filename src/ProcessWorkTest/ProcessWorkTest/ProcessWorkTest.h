#pragma once
/** ProcessWorkTest 测试说明

范围与入口
本工程只通过真实ProcessWork入口、公开Channel入口或测试进程自己打开的共享协议进行验证。
白盒故障注入、崩溃子进程、账本、性能计时均在Test内；生产库不得含测试宏、friend或强制测试头。
命令行：--list列出稳定ID；--instant、--stress、--all选择范围；--tests 1,3-5按ID选择；--report指定报告。
每例由独立子进程运行，采用独立PID/名称/目录；超时不计通过，析构仅清理该例自己的子进程和映射。
默认串行执行各例；压力例内部才并发。功能通过不代表所有平台和所有时序均无缺陷。

共同判定与收尾
普通消息：独立序号账本核对PID、正文、数量、线程内FIFO；不要求不同发送线程全局排序。
失败路径：不得产生额外回调、遗失预算或留下永久占位；恢复后使用同一入口重新发送并核对。
资源：回调仍执行时不提前归还计费；清理后任务数和在途计费为零，句柄/私有内存检查预热后平台。
饥饿：32条同优先级活跃流在六个约一秒观察窗口各自前进；暂停流恢复后重新纳入检查。
该观察不能证明任意系统调度下的绝对公平，也不允许把活跃流漏出账本以制造通过。
崩溃：仅强杀本例创建的子进程；分别覆盖正文预留、段预留、发布后和尾指针未更新时机。
普通send满载等待，默认trySend满载返回CAPACITY；普通及后台发送等待资源时，接收者死亡应令其退出。
同名新代重新收发，旧映射不得指向新代。用例有独立超时和收尾。

测试用例目录（精确输入、断言及超时以同ID的注册函数为准）
1 空生命周期；2 非法初始化及修正重试；3 名称长度/NUL边界；4 重复初始化不破坏现有接收器。
5 PID同步发送；6 名称同步发送；7 PID异步副本；8 名称异步副本；9 混合寻址异步FIFO。
10 trySend参数和大小边界；11 非法普通调用不交付；12 零正文容量；13 回调顺序与异常隔离。
14 回调重入投递；15 缓存清理后继续通信；16 接收代次重启；17 异步排空再启动三轮。
18 发布前不可见及单接收所有权；19 单通道满载CAPACITY不提交；20 零阈值按需扩容再缩容。
21 禁止缩容；22 正阈值扩容；23 三进程PID发送；24 三进程名称发送；25 三进程PID投递。
26 三进程名称投递；27 三进程混合投递；28 六字节实体123456；29 对端不存在及关闭。
30 三十轮生命周期压力；31 三进程四线程同步压力；32 三进程四线程异步压力。
33 三进程trySend背压；34 四发送者与缓存清理竞争；35 同名冲突仍支持PID；36 1024块边界。
37 满通道关闭唤醒；38 回调自身关闭的借用寿命；39 64位发布与四线程跨32位边界加一。
40 非法预算/溢出/保留内存；41 并发页安装及256/65536页边界；42 FIFO发布中断及陈旧代次。
43 八线程各五万条段复用；44 -1无配置上限从1024扩到1152再1280；45 单轮扩容字节预算。
46 死亡发送者正文预留；47 死亡发送者段预留；48 已提交发送者死亡；49 链接后尾提示前死亡。
50 暂停者占位保留、其他流前进、确认死亡再回收；51 固定本地计费/超大单条拒绝/失败回滚。
52 八线程各五千条三包预算通知；53 try投递的容量失败分类与未启动消费者自等待保护；54 慢回调背压。
55 损坏共享下标关闭通道；56 预热20轮再100轮资源平台；57 三进程16线程96万交付。
58 两进程32线程96万交付；59 有限本地预算异步背压；60 32线程普通post共16万次无调用方竞争重试。
61-65 对应46-50的真实强杀进程版本；66 接收进程崩溃、八发送者退出及同名重启四千次交付。
67 接收批次复用及逐包计费；68 两万条0/6/64/65536字节正文并发；69 单槽36/37字节内联边界切换，并逐个验证0..36字节非对齐正文复制。
70 六次发送进程强杀及96000条存活流消息；71 借用缓存无shared_ptr复制且重入删除安全。
72 发送者死亡与慢回调背压交错；73 单槽32发送流公平进展；74 大小包、晚加入、暂停恢复公平进展。
75 普通结果接口不返回竞争Busy、区分容量与资源；76 新流跨旧轮次边界首次轮询必须找到消息。
77 系统正文映射失败返回错误并归还占位；恢复同一映射后仅交付一次37字节正文。
78 登记未批准返回RESOURCE；单槽占满后try发送（含仅重试竞争）返回CAPACITY而非BUSY，恢复后只交付成功消息。
80 通道快缓存借用期间并发清理，结束借用后释放；旧代次不得命中、重入安全回退、八线程哈希碰撞回收全部引用。
81 空闲接收批次预分配与消息积压额度独立；16批次循环20轮检查完整正文、顺序和计费归还，并检查超标准大包能回收备用块。
82 Post小包池7槽循环40轮，验证池满及52/53字节边界回退、未释放槽不复用、批量归还池内/堆分配/空指针、归还后置空且二次归还无操作、在途配置拒绝及固定额度单项拒绝；混合前缀取件与逐条取件核对FIFO；再由8生产者并发40000次验证16槽复用、FIFO和计费归还。
83 已有正文等待者时仍可取得空闲队列段；归还正文按两个等待者的顺序直接授予，新请求不能从FREE抢走已授予资源，全部领取归还后释放正文。
84 四生产者混合PID/名称post与持续清缓存并发，核对1200条不同大小正文、FIFO及前缀通道借用期间的寿命。
85 同一实际线程交替使用两个Channel，跨正文页、队列段、槽位复用及通道重新打开后，核对28000条消息完整有序且无重复。
86 同一接收Channel依次以513/257/3槽重新打开，每代四批发送，跨元数据页和槽位复用核对3092条消息完整FIFO，禁止借用上一代已释放的页。
87 以不可访问内存页保护源记录两端，分别把0..96字节正文置于有效页的开头/结尾，验证194条记录复制不越界、完整内容、独立所有权、NUL及预算归还。
79 四种重试开关、普通send/post容量等待、取消/关闭、自等待保护；4096次快速重试后连续八次各等待至少1毫秒，释放单槽后精确交付，无重复/遗漏。

跨位数与平台
设置PROCESS_WORK_TEST_PEER为另一位数同版ProcessWorkTest.exe的绝对路径，mesh例交替创建两种节点。
推荐23-28、31-33、57-59；分别核对两侧实际程序哈希。不要用协议不一致的旧版当作跨位数对端。
Windows x86/x64以及DLL分别记录，不用一个通过结果冒充全部组合；其余原生端当前只有代码审阅。
ProcessWork不支持Wasm跨进程；CTaskThreadManager的独立Wasm pthread消费测试在其Test/Platform中。

88 dynamic-local-budget（保留稳定ID及命令键）：固定592字节的精确满载、超额单项终止、额度不触发增长回调；-1申请超过128MiB不计费；未发布及回调中的对象阻止重新配置；接收282/283字节边界；0/-2拒绝；普通post及两个重试开关均true也不会等待永远不可能满足的单项额度。接收额度300、最大包64配置成功，正文24立即拒绝、23精确接纳；额度1配置仍成功，空正文发送立即拒绝。
89 component-pool-configuration：MB设置的128MiB精确换算、-1、0/-2拒绝、最大有效MB及溢出保留原值；默认-1及Post/接收生命周期，真实存储增长，有限满载分类，真实接收池增长的拒绝及异常不接纳消息、不发成功通知，恢复后仍可发送；配置确实进入内部Map及任务线程；零任务容量启动失败回滚，修正后重新初始化成功。
90 native-growth-notification：Queue、Map节点池、Hash记录池真实增长；初始化不通知，次数和池统计一致；观察回调抛异常仍完整提交，Queue逐项FIFO。
91 packet-pool-dynamic-growth（压力）：2个预留槽增长到9槽，超大建议内部裁剪为7槽，之后走预算内分配；32线程各128轮、每轮16包，独立标记校验无重叠、全部完成、计费归零。
92 shared-growth-notification：真实Channel从4块增长到有限6块；满载请求未提交，等待维护窗口后扩容，成功通知只包含实际2块；此前4条消息完整且有序。
93 reservation-window-capacity：1/63/64/65/129/255/256/257/511/512/513块依次填满再排空，各4轮，核对10504条FIFO；扫描提示不得隐藏窗口外空位、突破容量或把满载误报Busy。
88至92均不向生产代码注入测试宏或状态；跨架构使用同样的公开及模块入口。线程全部完成是该次测试的进展证据，不是任意系统调度的形式化无饥饿证明。

性能入口与有效性
同目录ProcessWorkBenchmark.cpp提供原生C++性能入口：进程数 每进程线程数 每线程包数 正文字节 共享块数 模式。
模式0为PID send，1为名称send，2为post，3仅为Channel诊断。模式3不得冒充公开ProcessWork性能。
正文6字节时必须严格为123456；任务、正文、线程、初始共享容量及预热在计时前准备。
多进程生产和父进程回调消费同时发生；第一条发送开始到最后一次发送返回算生产时间，到最后回调算消费时间。
Post生产量只表示本地接纳，必须同时给端到端消费量；失败的未提交资源请求可重试，成功不得重复投递。
普通send/post满载及CAS竞争均在库内重试，性能测试不额外替库循环；线上按其原有接口调用。消息体预先创建，生产和消费同时运行，退避时间计入真实端到端吞吐。
每包仍验证流内序号/PID/正文；任何错误、超时、缺包不输出有效吞吐，首错明细独立保存。
正式六字节比较使用5000万包和至少三轮交错顺序，报告每轮原值、中位数与相对基准增长率，清楚注明基准是固定Git HEAD还是本轮优化前快照。
预分配共享块、接收批次和Post小包/节点池在计时前完成；单独的诊断副本记录增长、正文新申请、映射和容量等待，诊断吞吐不冒充正式数据。不能仅凭最终容量不变就宣称全程无扩容或无背压。
线上基准取固定Git HEAD，不用任意旧备份；无法直接构建时，私有接口适配必须逐项记录。
原生C++的--tool build只在指定新目录组合源码/头/库，不发布共享SDK；source-inputs及compiled-inputs记录实际编译输入指纹。
正式基准与诊断副本分开构建；诊断计数会改变成本，不能将其吞吐作为正式性能数据。
后台编译、模拟器或索引的影响写进报告；未得到空闲环境的测试只能标为当前负载测量，不能宣布峰值达标。

十、用例执行契约与新增入口

当前登记98项：1至93保留原稳定ID，94为测试设施自检，95至98为参考测试迁移后新增的活跃用例。
每个用例的中文目的见本注释目录，命令键、独立预期、压力标志由processCases登记；--list列出实际清单。
瞬时用例75秒、压力用例150秒超时，跨位数工具各屏障120秒、收尾15秒，性能每样本240秒。
不同工具重复同一用例的次数不得算成新的唯一用例数；未执行、平台不可运行和失败不计通过。

通用前置：匹配当前头/库的Release程序，系统允许创建共享映射与子进程；每例在独立进程启动，计费/回调/缓存为空。
通用触发：通过公共ProcessWork或明确的模块入口发消息；故障例通过测试自己打开的共享协议或自己的子进程构造时序。
独立预期：按发送进程、生产线程、预先确定的序号与全部正文核对，不从待测队列自身计数推算应收到多少条。
禁止事件：重复/遗漏/同流乱序、失败请求产生回调、仍在借用时回收、关闭后旧代次复活、普通CAS竞争向外报告Busy。
观察：功能/压力逐条账本；公平例按每条活跃流分别检查；性能逐流序号和正文核对，错误样本没有有效吞吐。
清理：停止自有生产者，按用例意图排空或取消，停止工作线程后释放回调；manager线程数须归零。子进程由BlackBoxChild作业对象/进程组限定归属。
证据：--report逐项写ID、名称、结果和用时，失败保留对应日志；报告写入失败为真实失败。临时输入使用独立PID/前缀，不复用旧完成文件。

94 automation-self-test【瞬时，测试设施】：输入有效/截断/重复键/非有限值JSON、奇偶样本和正常/错误退出子进程。
预期23项性能工作负载未减少，严格拒绝坏报告，正确求中位数，错误退出不能通过。仅清理自产日志。
95 payload-capacity-restart【瞬时，接口容量】：依次建立65536/1/0正文容量接收器，在同PID保留旧Channel。
触发最大包发送、关闭重开、超限发送、空包；预期三条正文/类型精确，旧Channel容量归零且不能访问新代，失败不占槽。
96 message-type-domain【瞬时，类型契约】：五条路径各11种非负边界类型，正文127字节；INT32_MIN/-2/-1覆盖五个拒绝入口。
预期55条全部字节及类型准确、发送后修改输入不改变交付、负类型不交付；检查MessageType有符号32位及公开枚举数值。
97 guarded-source-input【瞬时，输入借用】：五条路径×21种0..65536长度×首尾保护页，共210条。
触发发送返回后把源区域设为不可访问；Post在撤销访问后才启动消费者。预期无越界/延迟借用，全部正文、零包nullptr、PID及FIFO一致。
Windows用VirtualProtect，原生POSIX用mprotect；释放本次完整映射。进程崩溃由父控制器记录失败，不能跳过危险输入。
98 overload-rejection-fifo【压力，所有重载】：send/post两模式×0/1/6/65536正文容量，1023字节名称、四生产者各512次。
每16条夹入超限、空指针非法长度、负长度、缺失目标，合法调用轮换四种重载并立即改写源正文。
预期16384条合法消息完整且每流FIFO，无拒绝消息回调。Post远端超限/不存在可本地接纳，后台拒绝不算本地接纳失败；非法本地参数必须失败。

十一、原生C++调度与构建（不需要Python）

默认VS2013/v120 Win32/x64工程把功能、压力、性能和工具编译进同一ProcessWorkTest.exe。
仓库标准SDK构建后可直接构建该工程；测试用私有SDK可通过BlackBoxIncludeDir、BlackBoxLibraryDir、BlackBoxStatic=true选择，避免混用全局SDK。
POSIX继续使用现有CMake目标，库目录和LIBRARIESRELY须匹配平台。private VS build仅Windows可用，其他平台明确报不支持，不伪造通过。
性能独立CMake目标ProcessWorkBenchmark仍保留；与主程序调用同一个RunProcessWorkBenchmark，避免两份计时实现。

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

build支持tests/dll/benchmark/memory/profile/demo/map，架构x64或x86。demo提取生产ProcessWork.cpp的注释示例原文，编译并要求14条全部完成。
map重建LockFreeMapTest既有完整用例并执行--all，不把Map模型写进ProcessWork生产代码。
tests/dll/benchmark生成集成功能和性能的ProcessWorkTest.exe；需要跑哪些用例由后续命令明确选择，不把仅编译当成测试通过。
所有源及头复制到新建隔离目录，静态依赖与ProcessWork一起构建；dll只改变私有副本的链接宏。记录原始/实际编译输入、可执行文件FNV-1a-64指纹与长度，不将其称为安全签名。
诊断只改私有副本，锚点不唯一或已经变化必须失败并重新审阅，不能忽略。所有生产文件不增加测试宏、测试friend或测试头。

cross固定三节点按第一种/第二种/第一种程序启动，五组mode0/1/4/5/6分别覆盖PID、名称、混合Post、trySend、tryPost。
每节点四生产线程，mode4每流1000条，其余3000条，合计312000条跨节点交付；以ready/pids/armed/go/done/finish屏障核对，提前退出或超时失败。
也可设置PROCESS_WORK_TEST_PEER为另一架构程序运行原mesh用例，编译器/协议必须匹配。

matrix标准23项完整保留：6字节的PID 1x1/4/8/16/32、2x4、4x4，名称2x4、Post1x4/1x1；
64字节同七种PID组合、名称2x4、Post1x4；4096/65536字节2x4；空包1x4；64字节1x4的一千万包长测。
--messages覆盖总包数，必须能整除生产进程×线程数；--blocks覆盖初始共享容量；--custom可独立设置进程/线程/正文/块数/路径。
PROCESS_WORK_BENCH_UNLIMITED_LOCAL环境变量存在时使用-1本地额度，未设置时使用基准的有限额度，identity中记录此条件。
每轮交错基准与当前程序执行顺序，原始log和samples.csv逐样本写入；summary.csv含每组中位数、最小/最大、实际通过/失败轮数及相对基准的生产/消费变化百分比。
任意一轮失败的组不输出有效中位数/比较，不把失败样本删除后只统计漂亮结果。失败后独立场景继续，整批最终返回非零。
无基准填-，只能报告当前测量，不能声称性能增长。历史版本专属报告/归档脚本不再维护，通用原始样本和比较汇总代替它们。

memory十类计数：正文预留失败、容量重试、接收新批次、Post堆分配、Queue增长尝试、Map运行期增长尝试、元数据页安装、共享容量不足、Post等待、接收额度不足。
profile九类线程局部计数：发送次数、正文扫描、正文预留失败、段扫描、段预留失败、接收流访问、发布次数、正文CAS失败、空间等待检查。
增长尝试不等于成功增长；真实成功次数由基准的策略grown及各池m_onGrowth补充。初始化/预热计时外完成，诊断副本与正式吞吐分别运行。

十二、参考测试迁移清单

ProcessWorkReferenceCases.cpp/.h完整保存原legacy注释测试，已经纳入Test工程源文件，不含可执行main，不计入98项实际用例。
ProcessWorkCompatibilityTests.cpp/.h是其中四项独有边界在当前API上的活跃实现，对应95至98；保留原参考输入便于核对，没有删除未迁移断言。
已按当前接口覆盖的主题映射：
  PUBLIC_TESTS/multi-process mesh -> 5-9、23-28、31-33、57-59及cross工具；
  post-drain-cycles/callback-reentrant -> 17、14；try-result-ledger -> 10、19、75、78、79；
  restart/dead peer -> 16、29、66；cache-stress -> 34、71、80、84、85；
  fault/commit death/paused writer -> 42、46-50、61-66、70、72（协议已不同，不能称所有旧私有注入点逐一相同）；
  maximum-capacity/growth -> 20-22、36、44、45、92、93；slow-callback -> 54、59、72；resources -> 56；
  malformed shared state -> 55；payload source guard/type/capacity restart/all overloads -> 95-98。
这些是主题覆盖映射，并非未经验证的逐断言等价声明。

仍为参考、不计通过的专用内容：
  executable-name-model、linux-identity-model、linux-stat-model、linux-zombie-leader、darwin-exit-model、darwin-zombie：
    依赖CSystem平台解析替身或原生平台，应接入对应库/平台的真实实现后再运行，不能用Windows模型冒充平台实测。
  darwin-notification-name、linux-shm-quota、network/network-audit：
    涉及系统通知命名、系统配额和NetSender/网关集成；所需平台/宿主应明确后在相应集成层执行。
  task-id-exhaustion、task-post-oom/task-try-oom、queue-count-boundary：
    涉及CTaskThreadManager/Queue私有状态，保留原断言，不能向生产库恢复测试访问器。
  unknown-live-writer、notification-blackout、self-identity-failure、expanded-writer-boundary、local-counter-boundary、
  post-publication-gap、full-callback-exception、close-lease、generation-boundary、token-boundary、oom-post-init/oom-receive-init、
  expert-unit/unit/clock-regression中的专用注入，以及name-binding/peer-capacity/peer-churn的完整旧交错：
    部分现有用例覆盖同类行为，但旧私有布局或系统故障注入尚未逐断言转换，原文完整保留，不算本轮执行。
  mesh-corrupt的负向控制保留：故意改坏正文必须使账本失败；不能作为产品应通过的正向用例。
删除legacy目录仅表示原文转入Test源文件，不表示以上待接入场景已全部执行或通过。文档与实际--list共同区分活跃、参考和平台未执行范围。
*/
#include "BlackBoxTestSupport.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <mutex>

struct ReceivedPacket
{
    int pid;
    int type;
    bool nullData;
    std::string body;
};

class PacketLog : public ProcessReceiveCallback
{
public:
    PacketLog();

    virtual void receive(int32_t pid, const char* data, int32_t length, MessageType type);

    virtual void bufferFull(int32_t count);

    std::vector<ReceivedPacket> snapshot();

public:
    std::atomic<int> errors;
    std::atomic<int> fullEvents;
    std::function<void(const ReceivedPacket&)> action;

private:
    std::mutex m_mutex;
    std::vector<ReceivedPacket> m_packets;
};

class ThrowingReceiver : public ProcessReceiveCallback
{
public:
    virtual void receive(int32_t pid, const char* data, int32_t length, MessageType type);
};

class IpcScope
{
public:
    IpcScope();

    ~IpcScope();

    void start(ProcessReceiveCallback* receiver, const std::string& name, int bytes = 65548,
        int blocks = 8, int threshold = 0, int increment = 0, int idle = 0, int rounds = 0);

    void close();

private:
    IpcScope(const IpcScope& other);

    IpcScope& operator=(const IpcScope& other);
};

class IpcJob : public CTask
{
public:
    explicit IpcJob(const std::function<void()>& body);

    virtual void doTask();

    virtual void stopTask();

public:
    std::atomic<bool> failed;
    std::atomic<bool> stopped;

private:
    std::function<void()> m_body;
};

class IpcJobs
{
public:
    ~IpcJobs();

    void add(const std::function<void()>& body);

    void drain();

private:
    std::vector<uint32_t> m_ids;
    std::vector<std::shared_ptr<IpcJob> > m_tasks;
};

class StreamReceiver : public ProcessReceiveCallback
{
public:
    StreamReceiver(int threads, int packets, bool fixed);

    virtual void receive(int32_t pid, const char* data, int32_t length, MessageType type);

    void configure(const std::vector<int>& pids);

public:
    std::atomic<int> received;
    std::atomic<int> bad;

private:
    int m_threads;
    int m_packets;
    bool m_fixed;
    // configure完成后发布只读PID和序号账本
    std::atomic<bool> m_configured;
    std::vector<int> m_pids;
    std::vector<int> m_next;
};

// 使用Channel公开接口，控制pump时机以确定性制造满载；不接触共享控制区或private成员。
class PublicChannelScope
{
public:
    PublicChannelScope(int initial, int threshold, int increment, int idle, int rounds);

    ~PublicChannelScope();

    int pump();

public:
    ProcessWorkChannel receiver;
    ProcessWorkChannel sender;
    std::vector<int> delivered;
    std::atomic<int32_t> full;

private:
    LockFreeQueue<char*> m_queue;
    Semaphore m_notification;
};

std::string packetBody(int value, int length, bool fixed = false);

std::string receiveName(const std::string& suffix);

void sendPacket(int mode, int pid, const std::string& name, const std::string& body, int type);

void waitPackets(PacketLog& log, size_t count);

int runIpcNode(int argc, char** argv);

void meshTest(int mode, bool fixed, int packets, int nodes, int threads, int initial = 8);

std::vector<BlackBoxCase> processCases();
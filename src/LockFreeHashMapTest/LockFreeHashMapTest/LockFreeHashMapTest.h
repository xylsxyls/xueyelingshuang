#pragma once
#include "LockFreeHashMapTestEntry.h"

/*
LockFreeHashMapTest 测试文档

一、范围与判定依据
确认：本文按用户要求保存在本头文件注释内，是测试目录和复现说明，不记录历次成绩、版本备份或验收流水。
依据：common/Base/LockFreeHashMap需求文档.txt、当前公开接口合同及common/AI文档/工程测试规范.txt、代码开发规范.txt。
范围：验证本库的查找、条件发布、版本替换和删除、拥有快照、节点池、保护域、回收、异常、公平性及性能；不替代上层业务或其他库验收。
规则：C++11；原生用例、负载、独立模型、故障注入和判定均为C++。Wasm用JS编排同一批C++，不向生产库加入测试入口。
规则：本目录保留测试源码、必要工程配置和测试脚本；构建输出、日志、CSV、dump、隔离缓存及基准快照统一生成到src/temp/LockFreeHashMapTest的新子目录。
检查：读取实际退出码及完整结果，失败、取消、超时、未执行分别记录；不能把没有日志、没有崩溃或旧批次通过当成本次通过。

二、公共前置、观察和收尾
前置：确认被测头目录、编译器、位宽、配置、异常支持和原子能力；只读使用正式源码，不依赖共享SDK或临时目录中的旧头文件。
前置：每批使用独占输出前缀；运行前创建目录，禁止覆盖已有证据。不申请、读取、写入或等待“测试环境占用.txt”。
场景：用确定性输入、独立std::map/数组模型或可计算的不变量检验实际返回值、提交事实、数据和资源责任。
判定：检查m_status与m_committed；查询成功可以未提交，clear的极端身份耗尽可能已经提交，不能把所有非Ok统一重试。
判定：真实容量及预留占满按CapacityLimit处理，不能用ReservationBusy掩盖空间不足；有限池压力中的允许拒绝必须同时验证未提交和原数据未变。
判定：快照必须拥有值且不借用已回收节点；旧版本、删除重插或跨实例凭据不能修改新条目；take对同一版本只能交付一次。
判定：普通失败返回状态；下标赋值允许抛LockFreeHashMapAssignmentError，必须核对错误和目标未被本次失败赋值覆盖。
收尾：释放暂停点、通知工作线程结束并join，退出所有回调和维护者，再销毁表；故障注入必须恢复，不依赖线程退出时清理生产TLS指针。
收尾：需要回收的场景清空并维护，核对occupied/retired、活对象、候选引用和分配基线；显式释放还核对capacity归零。
资源口径：全局new跟踪计量申请载荷，不等于RSS或所有系统资源；poolStats也不覆盖值内部独立缓冲区和外部Snapshot。
资源口径：先建立运行库/线程/互斥量自身的稳定基线；不能通过清零统计掩盖本库泄漏。句柄及线程观察与独立control模块配合解释。
异常边界：测试入口捕获异常并返回非零，不以运行期assert、abort或terminate主动崩溃；异常和线程创建失败仍须完成已启动工作者的收尾。
时间边界：runner对单用例或模块子进程采用120秒上限；用例中的受控等待另有期限。直接启动裸模块没有统一进程看门狗，压力测试应使用runner或外部有限期限。
覆盖限制：有限并发历史和压力测试不能证明数学上的线性化、绝对无饥饿、任意运行时长无泄漏或全部操作系统环境可用。

三、主用例目录（LockFreeHashMapTestMode=cases）
入口：CASES_EXE [1..10]；省略编号运行全部。一个进程只支持单个数字或全部；0、非法字符和越界编号无效。
选择：RUNNER cases CASES_EXE PREFIX REPEATS [1,3,8]可顺序重复指定用例；当前不支持1-3范围表达式。
共同通过条件：对应END id=N result=PASS，最终RESULT failed=0，分配净差为0且进程正常退出；校验次数不是用例数量。

1. 池配置与策略
类型：瞬时；依据：需求二的容量、增长、缩容和失败合同。
场景：初始化/重新配置、0及正阈值、有限及-1上限、固定/比例策略、拒绝增长、目录不足、维护和空闲缩容。
判定：单位和配置校验正确；预留与保留合并预算不超限；最后一槽及下一次申请触发点正确；失败不改已有数据或漏退预留。
收尾：静止后显式释放，检查资源计账归零。入口实现：LockFreeHashMapTests::pool。

2. 独立模型与兼容接口
类型：压力；依据：需求五的数据语义、六的参考模型与桶边界。
场景：固定种子0x5eed1234，10000步增删改查、重复插入、同键更新和clear；常量哈希制造全碰撞，以独立std::map核对。
判定：返回、数量、值集合及下标代理符合模型；桶数归一化和极大桶参数正确；不依赖遍历顺序。
收尾：模型和被测实例独立销毁，检查分配基线。入口实现：LockFreeHashMapTests::model。

3. 版本、快照与接入模型
类型：瞬时；依据：需求五的条件操作、安全引用，六的PID/名称缓存与网关场景。
场景：find、insertIfAbsent、compareExchange、eraseIf、take；同值新版本、删除重插、跨实例、clear及外部拥有引用。
判定：值和版本来自同一次观察；旧身份不能淘汰新条目；未发布候选正确释放；表删除或析构后拥有快照仍有效。
接入示例：LockFreeHashMapMigration.h/.cpp覆盖完整初始化后条件发布、按旧版本淘汰、登记表取走而在途引用继续存活。
收尾：释放最后拥有者并核对析构计数。入口实现：LockFreeHashMapTests::versions。

4. 异常与申请失败
类型：瞬时；依据：需求二、五、六的失败原子性和资源回滚。
场景：分配点失败，用户键值构造/复制、哈希、比较、策略和遍历回调抛异常；条件失败及失败后恢复。
判定：错误状态、提交事实和输出保留正确；不丢原值、不产生孤立条目、不泄漏保护记录或预留；下标特例检查异常类型和目标状态。
收尾：关闭故障注入后重新操作并释放资源。入口实现：LockFreeHashMapTests::exceptions。

5. 同键并发竞争
类型：压力；依据：需求六的唯一条件创建、版本更新和唯一摘除。
场景：多个线程竞争insertIfAbsent、按版本累加及take。
判定：同一创建只有一个发布者；其他调用取得胜者；成功修改与最终值一致；同一版本只交付一次，逻辑失败不能当作提交。
收尾：join全部线程，核对候选和已发布对象责任。入口实现：LockFreeHashMapTests::contention。

6. 暂停线程与回收交错
类型：瞬时并发；依据：需求二的推进与生命周期，六的暂停、ABA及clear交错。
场景：通过受控用户对象暂停读取、复制、发布或析构，安排桶头为空/非空、物理摘除、地址复用和clear交错。
判定：保护期内对象不提前析构；旧代不复活、不误用复用地址；其他合法操作可推进或返回合同允许的资源失败。
收尾：异常路径也打开暂停门并join，再维护回收。入口实现：LockFreeHashMapTests::paused。

7. 有界并发历史模型
类型：压力；依据：需求五的提交语义和六的并发正确性。
场景：固定种子0x1234abcd，600轮，每轮12项操作；记录调用/返回区间，以独立串行模型搜索满足实时先后约束的历史。
场景：包括碰撞、接近身份边界、标量缓存开关及不同缓存容量。
判定：每轮至少存在一个与实际结果一致的合法串行历史；失败保留该轮操作记录。入口实现：LockFreeHashMapTests::histories。
收尾：线程退出后销毁本轮实例，不沿用上一轮状态；该有限搜索不等于全局正确性证明。

8. 全碰撞与并发维护
类型：压力；依据：需求六的高冲突、长期替换和资源稳定性。
场景：6个工作线程全碰撞增删改查，同时维护/遍历；必须完成60000次逻辑写入。
判定：每个拥有键值准确，无错误删除或失败后数据变化；允许的有限池拒绝逐次计数并检查，达到重试上限仍未完成即失败。
收尾：等待所有线程，清空维护到逻辑为空及退役占用归零。入口实现：LockFreeHashMapTests::stress。

9. 正确性计时冒烟
类型：瞬时；依据：需求六的基本性能观测。
场景：1024、8、1桶下各10000对set/get，记录延迟和池观测。
判定：每对读写值正确；计时用于冒烟，不据此宣称完整性能验收或固定提速倍数。
收尾：销毁实例，核对资源。入口实现：LockFreeHashMapTests::performance。

10. 极端边界与优化路径
类型：压力与边界组合；依据：需求六的容量、异常、身份和实例化边界。
场景：回调形态/重入、保护记录耗尽、版本与计数边界、缓存完整身份、增长预留后暂停、活动缓存槽占用、并发比例增长。
场景：shared_ptr同键高冲突、clear、条件写和维护交错，共80000次操作；普通整数和自定义比较器路径分别核对。
判定：明确区分空间不足、目录不足、保护记录不足及用户异常；资源预算不越界；快照、弱引用和保护记录均正确释放。
收尾：恢复注入边界，join后清空维护和显式释放。代码位于LockFreeHashMapBoundaries.cpp、LockFreeHashMapOptimizations.cpp及测试成员指针适配文件。

四、保护域、内存池及整表专项模块
目录：下面使用实际模块名与子函数名定位，内部断言或子步骤不冒充独立全局用例ID。
入口：构建对应Mode后运行MODULE_EXE，或RUNNER module MODULE_EXE PREFIX REPEATS。共同结果为最后一行MODULE_RESULT failed=0及退出码0。

epoch（保护域，瞬时＋压力）
前置/依据：固定保护记录，外部静止初始化；需求二的安全保护和暂停线程推进。
场景：admission验证接纳/释放、重入、对齐和身份边界；paused验证保护者暂停；retirementOrder验证晚进入读者和旧扫描截止关系。
场景：contention使用8线程共800000次接纳尝试，写独占提示再核对，拒绝单独计数。
判定/收尾：记录不被两个借用者同时覆盖，成功＋拒绝等于尝试数，释放后minimum归零；无隐藏线程异常。

pool（节点池，瞬时＋压力）
前置/依据：直接测试当前池和保护域；需求二的容量与回收合同。
场景：boundaries覆盖1/2/63/64/65/127/128/129/65535/65536槽、位图边界、配置、目录及租约计数边界；exceptions覆盖构造和分配失败。
场景：policies覆盖增长、截断和动态空闲时间；paused暂停保护者；pausedDestructor暂停析构；overlappingReclaim安排同位图字的新旧退役责任交错。
场景：contention并发申请与归还；shrinking验证关闭块、连续空闲、复用重置时间、初始容量下限和缩容保护。
判定/收尾：预算不超限，安全前不复用，安全后恰好析构一次，不丢并发新退役位；活对象及POOL_ALLOCATION_DELTA归零。

rewrite（整表综合，瞬时＋压力）
前置/依据：当前实现的组合回归，名称是构建模块标识；先运行pool，再运行表的各子场景。
场景：alignment检查栈/堆/数组中原子字段对齐；model、versions、exceptions核对模型、条件身份和失败原子性。
场景：scalarCache、scalarStress覆盖整数副本缓存的代次/身份、并发clear和高冲突；largeTableLifecycle覆盖大桶表生命周期。
场景：paused、contention、collisions覆盖暂停、同键竞争、全碰撞与回收；单线程Wasm跳过依赖真正线程的子场景。
判定/收尾：表和前置pool均成功，REWRITE_ALLOCATION_DELTA归零；不能用前置模块成功覆盖后续失败。
专项入口：REWRITE_EXE alignment只做对齐检查；无参数或参数1运行整个模块。

五、同类接口公平性（Mode=progress，压力）
依据：相同API、相同优先级线程要能在有限观测窗口内实际推进；不把生产与消费次数不同解释成饥饿。
场景：set_hot、insert、erase、take、find、clear、set_collision、get共8类；普通负载6工作线程，加载场景12工作线程并增加竞争负载。
场景：Windows分别使用可用CPU集合和同一个CPU；插入/消费场景另有供给者，同CPU测试为供给安排另一可用CPU，避免无货误判。
场景：统一起点观察1200ms，每条受测线程都须在窗口内真实成功；记录入场、成功、逻辑未命中、资源失败、首次成功、最长调用/间隔及CPU时间。
判定：所有参与者就绪，入场与调用数对应，无隐藏异常，窗口内成功非零，take结果不重复；不要求线程成功次数严格相同。
负例：故意延迟某线程入场或成功到截止后，必须准确识别对应未推进原因；其他设施异常不能冒充负例检测成功。
规模：一次Windows完整模块为32个实际场景和2个设施负例；其他原生平台只有一种CPU模式。跨配置、重复轮次应另列执行次数。
对照入口：PROGRESS_EXE allocator-control或scheduler-control分别观察分配器及纯调度，不作为Map公平性成绩。
收尾：通知停止、释放事件、join，清空维护到occupied/retired归零，再显式释放capacity；控制器提供进程超时。

六、性能与内存矩阵（Mode=performance/perf-memory/perf-runner，压力）
前置：比较双方使用相同C++夹具、参数、目标工具链和可用CPU。基准由运行者提供并记录源码/程序身份，不能拿同一源码冒充不同版本。
前置：LockFreeHashMapSource指定被测头目录；LockFreeHashMapPerfCurrent=true启用条件/快照专项；LockFreeHashMapPerfStdMutex=true选择std::unordered_map加单mutex适配器。
区别：此处std适配器整表互斥，用于并发对比；下一节single-thread的std完全不加容器锁，两者不可混称。
单项入口：PERF_EXE ENCODED [MULTIPLIER]，ENCODED=批次数*1000+ID，批次数1..64；get/set连续循环倍率1..256，只用于代码允许的场景。
内存入口：MEMORY_EXE ENCODED，不接受MULTIPLIER。实际目录由LockFreeHashMapPerfModels::catalogue/memoryCatalogue生成，不以旧CSV当注册表。

吞吐目录：共164项，以下偏移规则列明全部场景，不改变现有编号。
ID 1..44为int键/int值，45..88为int键/shared_ptr值，89..132为string键/shared_ptr值；后两组分别加44、88偏移。
每组相对ID 1..18：线程1/2/4/8/16/32，各按100%/95%/50%读比例组合；19纯写，20为10%读，21未命中，均4线程。
每组22..27：64/4096/65536键，各纯读和95%读；28..30为全hash0的纯读、95%读、未命中；31为单桶95%读。
每组32..44依次为contains命中、contains未命中、新增、重复插入、删除、删除再插入、数量、遍历、键导出、值导出、键值对导出、clear、clear后填充。
ID 133..140为整数，141..148为shared_ptr：各按顺序为8热点键1/4/16线程、同键覆盖10000次后读取、find、条件创建已存在、find后条件替换、find后条件删除再插入。
ID 149..152为宽64位整数，153..156为字符串值，157..160为512字节值：各100%/95%/50%读及未命中。
ID 161/162比较32768键在64/32768桶下读取；163/164为255/257键读取。
内存目录：ID 1..24，六种键值族各含64/4096键、连续更新/删除再插入四组，每组10000次迭代。

判定：数据、调用数、身份、有限正时长、样本数及额外调用计量都正确，才进入性能汇总；分配失败或未提交不能伪装为完成吞吐。
统计：预热/校准后交替执行，保存全部原始轮次及中位数；延迟p50/p95/p99来自采样，不能与整批吞吐直接互相替代。
统计：extra_calls是外部公开API额外调用数，不是内部CAS次数；casRetries未采样时不能把0解释为无竞争。
内存：记录构造、工作结束、clear后和析构后的申请载荷、峰值、申请次数及净差；逻辑clear不等于立即释放池。
收尾：检查被跟踪分配回到运行库基线；资源失败、参数不匹配、日志不完整或超时均失败。
两版控制器：PERF_RUNNER revision A_PERF B_PERF A_MEMORY B_MEMORY B_EXTENDED PREFIX ROUNDS MASK [FIRST LAST [REPEATS [MULTIPLIER]]]。
三方控制器：PERF_RUNNER three A_PERF B_PERF STD_PERF A_MEMORY B_MEMORY STD_MEMORY B_EXTENDED PREFIX ROUNDS MASK [FIRST LAST [REPEATS [MULTIPLIER]]]。
规则：ROUNDS为1..9，MASK=0为默认；revision双方须支持条件专项。指定FIRST/LAST不重复执行内存矩阵；控制器生成catalogue、runs、comparison等输出。
注意：clear返回成本、不同实际桶数及不同所有权责任须单独解释，不能用不等价操作证明提速；无固定倍数自动验收阈值。

七、单线程标准表对照（Mode=single-thread，性能压力）
依据：比较相同单线程需求的实际成本，std不加mutex、共享锁、池、值缓存或版本包装；当前库保留自己的原子与回收协议。
场景：同一主线程、同一CPU交替运行；int32键，int32及shared_ptr<const int32_t>两类值，256/4096/65536键共144种操作与规模组合。
配置：请求桶数等于键数；当前池初始2*键数+128槽，其他默认，包括4096格整数副本缓存及128保护记录；std默认分配器和负载因子，记录实际桶数。
配置：Windows默认逻辑CPU2，参数可另选可用CPU，禁止4/5；不改变优先级，不声称独占机器。
入口：SINGLE_EXE PREFIX [ROUNDS] [MILLISECONDS] [KEYS] [FIRST] [LAST] [CPU]。
参数：默认9轮、25ms校准、KEYS=0选全部规模、FIRST=0、LAST=23、CPU=2；轮数为3..31的奇数，校准1..250ms，指定键数为2..65536的二次幂。
编号说明：0..23是此已有测量程序的操作索引，区别于cases的1..10，不重编号或改变命令解析。
索引0/1：get命中复制/未命中，std对应find并复制；2/3：contains命中/未命中；4：count(key)。
索引5/6：value与已存在键下标读取；缺失键下标语义不同，不纳入同等比较。
索引7/8：set和下标更新已有键；9/10：新增/重复键insert，对应std原生emplace，候选构造成本保留。
索引11/12：erase命中/未命中；13：删除再插入，一个操作单位含两次调用；14：每十次九读一更新。
索引15/16：size及非空表empty；17：整表forEach累计；18/19/20：完整keys/values/toVector导出。
索引21：clear仅接口返回，本库延迟回收而std当场析构；22：clear加维护/资源检查，双方完成节点析构并允许保留空闲池/桶。
索引23：一次构造、插入全部键及析构的完整生命周期，包含池预分配，仍不包含外部候选业务对象的创建。
计时：基础项目排除容器准备和最终析构，操作内自然分配/维护计入；新增、命中删除每批仅一轮全键，clear预建64张表再计64次调用。
计时：其他项目各自校准；双方操作在计时前做模板分派，使用相同编译器屏障。size/empty另经关闭跨单元优化的独立函数边界，避免循环被消除。
计时：导出包含结果构造及替换前次结果的析构，std使用reserve(size())；最后结果的集合检查在计时外。短批次不得伪称达到校准时长。
判定：返回值、校验和及最终状态符合独立预期；混合读写由计时外数组模型重放；导出检查完整键集合、值集合及对应关系，不依赖顺序。
判定：清空维护最多64轮必须使occupied/retired归零；全部shared_ptr候选use_count回到1；写失败不偷偷外部重试。
输出：-runs.csv逐轮保存调用数、时间、校验和、错误和桶数；-summary.csv保存中位数及范围；退出0并有SINGLE_RESULT passed=1才表示完成。
统计：吞吐变化=(std每操作纳秒/当前每操作纳秒-1)*100；正数当前更快。批次均值不是逐调用p50，整表操作不能冒充单元素成本。
限制：版本条件操作和拥有Snapshot无直接标准接口对应，不用借用迭代器代替；小差距结合波动判断，不只选最好一轮。

八、其他保留模块与设施自检
std-checks：瞬时＋压力；独立std::map模型核对加mutex适配器、导出、回调/复制异常后锁释放、共享拥有引用，以及8线程80000次更新。
判定/收尾：模型一致、唯一插入责任、异常后继续使用，最后释放引用；这是对比较夹具本身的检查。
composition：瞬时；同翻译单元包含HashMap、Map、Queue、Semaphore，运行Queue/Map/HashMap的隔离组合和拥有取出；Semaphore只编译包含，不验收其等待协议。
read-costs：诊断压力；1/4/16线程、输出间隔1/32、实际读取与纯原子屏障对照，各3轮，每线程2000000次；核对值及线程失败，只用于定位缓存行和计量成本。
benchmark：基础计量；64键、10000次读写、不同桶数及删除模式，记录吞吐、延迟、申请/保留/峰值和析构净差；不替代全面矩阵。
workloads：压力；可选倍率1..20，纯读、热点覆盖后冷键、95%/50%读、全碰撞及拥有引用十种负载；逐线程核对结果和总完成数。
control：Windows运行库对照；普通线程与standard分配对照，8线程反复创建/退出，观察句柄、线程和分配趋势，不将运行库常驻申请自动归因于Map泄漏。
module-host：以同配置宿主调用cases测试DLL或静态模块，验证测试模块边界；不向纯模板生产库制造导出符号。
runner：RUNNER selftest PREFIX检验成功、非零退出、截断/错误结束记录、参数、超时和进程回收等设施行为；实际选择器仅支持现有逗号列表。
perf-runner：PERF_RUNNER selftest检验目录、结果字段、身份、数量、时间、样本和百分位规则，拒绝损坏记录，不把解析成功等同于业务通过。
crash-runner：仅Windows，调试运行并采集本批子进程dump/候选栈；CRASH_RUNNER EXE 数字参数 PREFIX 轮数，或CRASH_RUNNER dump DUMP [符号目录]。
限制：崩溃采集会改变调度和耗时，不能当性能数据；只终止自己创建并核实归属的超时子进程，不处理无关程序。
白盒隔离：LockFreeHashMapTestState.h/.cpp与LockFreeHashMapTestBinding.h/.inl通过测试成员指针适配注入静止期边界，不改写private，不依赖对象偏移，不向生产加友元/宏。

九、构建入口
原生：LockFreeHashMapTest/LockFreeHashMapTest.vcxproj通过LockFreeHashMapTestMode选择模块，默认cases；支持Win32/x64、Debug/Release，工具链v120。
模式：cases、epoch、pool、rewrite、progress、runner、performance、perf-memory、perf-runner、single-thread、std-checks、composition、read-costs、benchmark、workloads、control、crash-runner、module-host。
输出：必须将OutDir和IntDir放入本测试临时根下独占子目录，Debug可执行文件名末尾带d；禁止在根目录或共享SDK落中间文件。
示例（仓库根PowerShell，$out选不存在的绝对目录并先创建；根据安装位置设置$msbuild）：
& $msbuild src/LockFreeHashMapTest/LockFreeHashMapTest/LockFreeHashMapTest.vcxproj /t:Build /p:Configuration=Release /p:Platform=x64 /p:LockFreeHashMapTestMode=cases "/p:OutDir=$out/" "/p:IntDir=$out/obj/"
& "$out/LockFreeHashMapTest.exe" 2
其他模式替换LockFreeHashMapTestMode；两版比较另设LockFreeHashMapSource，不能覆盖工作区源码。
测试模块：cases配LockFreeHashMapTestModule=dll或static及对应ConfigurationType；module-host配LockFreeHashMapTestModuleLibrary指向匹配lib，DLL与宿主输出目录一致、对象目录分开。
CMake：入口为本源码目录CMakeLists.txt，C++11；LOCKFREEHASHMAP_SOURCE指定生产头；实际目标和add_test登记以该文件为准，不宣称全部VS模式均有同名CMake目标。
single-thread对应CMake目标LockFreeHashMapSingleThread；LockFreeHashMapSingleObservation.cpp必须保持关闭跨翻译单元优化，不能删除这个防测量失真边界。

十、Wasm脚本的用途与复现
保留run_wasm.js：编译/执行cases、epoch、pool或rewrite；Wasm32/64、单线程/pthreads、Debug/Release共8种组合。
命令：node run_wasm.js EMSDK NODE PYTHON OUTPUT [epoch|pool|rewrite]；省略最后参数为cases。
说明：PYTHON仅用于运行Emscripten自带em++.py编译器，不承载本工程测试逻辑；实际断言、故障注入和数据模型仍来自C++。
参数：编译及链接使用-fexceptions，配DISABLE_EXCEPTION_CATCHING=0、ABORTING_MALLOC=0；线程包加-pthread，64位包加MEMORY64=1。
限制：单线程cases仅执行1/2/3/4/9，其余为未执行；pool/epoch/rewrite也按编译开关跳过线程场景，不把跳过计作通过。
判定：编译子进程300秒、执行120秒上限；检查退出、完整END/RESULT或最后MODULE_RESULT及对应资源净差；运行前后源码哈希须一致。
保留run_performance_wasm.js：为两个外部指定版本及当前扩展接口编译Wasm32/64 pthreads Release，选择24个代表负载，各5轮交替测量。
命令：node run_performance_wasm.js EMSDK NODE PYTHON BASE_HEADERS CURRENT_HEADERS CATALOGUE OUTPUT。
前置：CATALOGUE由原生perf-runner按当前C++注册表重新生成，不依赖已清理CSV；BASE_HEADERS由运行者按实际比较目的准备，不能伪造基准身份。
负载：整数/shared_ptr/string键纯读及混合、未命中、4096键、全碰撞、宽64位、512字节值、删除再插入、clear、新增/删除及拥有快照/条件替换。
判定：脚本核对唯一PERF行、项目名、实际调用数、时长、错误、额外调用及百分位顺序，保存全部轮次；子进程上限300秒，不剔除较慢轮次。
路径：两个脚本的OUTPUT必须是src/temp/LockFreeHashMapTest下尚不存在的子目录；隔离编译缓存，禁止写共享SDK缓存。
缓存：run_wasm.js可用LOCKFREEHASHMAP_TEST_CACHE指定种子，LOCKFREEHASHMAP_TEST_REUSE_CACHE=1只允许复用测试临时根内部的既有缓存；清理后先用默认复制方式，不引用已删除路径。
宿主：Node需支持所选Memory64、Worker和共享内存能力；Node通过不等于微信小程序真实宿主通过，微信仍需独立适配和验证。
收尾：脚本等待子进程结束，记录退出、信号、日志及结果；新生成的JS/Wasm、CSV、日志和缓存都是运行产物，不能当正式测试源码保留。

十一、执行选择、缺口与清理
步骤：核对本轮目标与环境 → 选择实际ID/模块及压力规模 → 先设施自检和相关瞬时 → 执行获授权压力 → 检查完整数据及资源 → 按实际结果归类。
检查：不默认每次全量执行；性能、并发、平台、ABI和回收分别判定；断言数、轮次和多配置执行次数不能冒充唯一用例数。
缺口：当前是命令行模块体系，没有GUI四入口或统一跨模块全局ID；本文保留实际编号，不通过改写文档假称已有此能力。
缺口：Linux/macOS/Android/iOS实机、ARM弱序、TSan、微信真实宿主、长时间持续压力及上层生产接入，必须有对应实际批次才能声称通过。
清理：按用户授权删除本工程测试结果、编译产物、dump、隔离缓存及版本快照；只处理核实归属的路径，先确认没有本批进程使用，不删除正式源码或其他工程目录。
清理后：本文保留用例、判定和运行入口，不保留旧成绩。以后重新构建和运行生成新证据，不能再引用已删除的日志或备份作为现有验收依据。
*/
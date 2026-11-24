#pragma once
#include "NetInterface/NetInterfaceAPI.h"
#include <locale>
#include <vector>

/** NetInterfaceTest 专项测试说明

被测入口为已构建 SDK 的 NetInterface/NetInterfaceAPI.h。所有功能用例调用真实 C++ 接口。
两端 TLS 共用测试工程已有的公开测试证书样本，只在当前进程的内存中交换密文，避免网络和端口影响判定。
通过 NetInterfaceTest --all 运行全部用例，--cases 2,8 可按编号选择，--instant 和 --stress 分别运行瞬时与压力用例。
Windows 运行对应配置的 EXE；Wasm 使用对应位宽的 LibraryTestHost.js 和 NetInterfaceTest.js。
Linux、Linux ARM、macOS、Android、iOS 通过项目 CMake 和 PlatformTest 的相应宿主构建或启动同一组用例。
Linux 在 VS Code 中先配置 CMake、选定并构建 NetInterfaceTest，再用同名调试项运行 --all。

编号、输入与独立预期
01 固定帧（瞬时）。手写32字节 NIF2 头和3字节二进制正文；编码逐字节一致，按片解码还原全部字段。
   半帧 EOF 返回 NI_TRUNCATED，reset 恢复；正文预算2时接收3字节声明必须在头边界拒绝。
02 紧凑完整帧（瞬时）。产品128、类型16384、最高位为1的64位请求号；比对手写变长字段及固定编号。
   未协商的解码器返回 NI_PROTOCOL_ERROR；协商后核对种类、编号、长度及含 NUL 的正文。
03 SHA-256（瞬时）。整段和两段 abc 分别写入独立输出，与标准摘要逐字节比较。在两段之间提交空指针及非零长度须报参数错误，空指针及零长度可用，两次均不影响最终摘要。
   31 字节空间须返回 NI_BUFFER_TOO_SMALL，所有预填字节保持原值，随后以 32 字节重试成功。完成后再更新或再结束均须返回 NI_INVALID_STATE，第二次结束的目标缓冲保持原值。擦除中部须保留两端原字节。
04 protobuf 消息（瞬时）。发送句柄写入73和含 NUL 的业务文本。序列化后清空发送句柄，
   其错误码及长度恢复0；独立接收句柄解析，容量查询为3，足够容量时逐字节一致。
05 跨分片持续复用（压力）。对同一解析器执行1024轮，遍历样本每个非空分割位置。
   前半只返回 NEED_MORE，后半 READY，消费长度准确；每轮正文完整，consume 后 EOF 干净。
06 连续帧与借用（瞬时）。固定帧的引用、指针及字段数组入口和紧凑帧入口分别令长度覆盖正文、元数据或输出，期待 NI_INVALID_ARGUMENT 且原字节保持。长度紧邻输出的合法调用应成功。短输出预填与各位置预期编码不同的字节，检验失败时目标和两侧保护字节完整。一次送入两个不同帧，只接纳第一帧；覆盖原输入后已接纳的正文仍正确。
   句柄 feed 的消费长度落入输入时应保持全部字节和解析位置，换用相邻的独立长度后完整收帧。READY 时再次 feed 消费0，consume 后续帧可读。无效构造参数不能通过 reset 变成有效。
07 紧凑增量（瞬时）。对完整 DATA 基线送入单字节增量和跨分片的变长增量，请求号依次为40、41、44。
   类型和产品继承，正文分别核对。完整 DATA 头已接纳时，分别在正文尚未收全、已就绪待消费和已消费时关闭再开启紧凑许可。当前帧仍可完整读取，随后两种增量头都须等到新的完整基线。重复开启已有许可保留基线。
08 RPC（瞬时）。与独立 NRP2 信封字节比较，核对父编号高低字、正文偏移和时限。重叠缓冲或长度输出别名
   拒绝且不写输出，相邻区间可用，超出16位的种类拒绝；PING 原地转为 PONG，PONG 无回复。本地 NOI2 来源前缀另验独立输出、正文别名与长度别名。解析 RPC 时允许复用字段数组，结果覆盖正文时拒绝。NIS2 服务握手核对输入秘密、帧元数据与协商输出的独立性；NOI2 拆包允许原地更新帧描述，输出与正文重叠时保持输入。
09 信用窗口（瞬时）。验证默认字节数、窗口对齐、使用后禁止改配置。发送7字节、接收3字节，
   各自归还后两项恢复；多还1字节触发持续协议错误。
10 压缩（瞬时）。手写 Zstandard RLE 数据块，期望1024个 A。未协商拒绝，1023字节预算拒绝；
   协商且容量足够时整个输出逐字节一致。
11 HELLO（瞬时）。驱动客户端和服务端互换握手记录，协商得到能力3、窗口16、块4096，核对双方身份字段。
   单独的未完成握手在截止前仍等待，到截止时刻返回 NI_TIMEOUT。
12 TLS 服务名（瞬时）。临时设置带千位分组的数值 locale，为产品12345构造真实 TLS ClientHello。
   按记录、握手及扩展长度定位 server_name，只接受域名列表中完整的 service.12345；测试结束恢复原 locale。
13 TLS 队列与背压（瞬时）。两端用独立 TLS 会话握手，每次最多转交97字节密文。控制和普通记录池各设1槽，
   非法配置须保留原设置，启动后配置须拒绝。连续两轮各接纳一条请求与心跳，第二条同类消息拒绝，
   对端每轮各收到一条并校验编号、类型与正文；排空后无迟到重复帧。
14 正文池（瞬时）。五个大小类各设1槽，检查分级容量、满池错误、跨级隔离、swap/reset 和再次分配。另用独立配置创建五级池，第一级从 1 槽按 2 槽增长、无配置上限，同时持有 6 个租约时应已增长 3 次。其余四级上限分别为 4、3、2、1 槽，逐级核对保护记录数、增长通知、满池拒绝、归还后占用归零及各租约的正文。
   零字节不分配，超出最大档拒绝，租约先于池析构。
15 发现表快照（瞬时）。65个互异节点走缓存编码、解码后完整复制；少一槽时保留原数组，最后一项版本无效时前面的项也不得写入。
   同一数值IPv6和同一端口的两种拼写不得重复入表；400个有效节点实际编码超过16 KiB、443个节点仅名称即超预算，两者均整体拒绝。
16 发现缓存（瞬时）。固定UTC毫秒写出NGC3；从实际导出的缓存验证未来5分钟及过去30天的精确边界，
   两侧各超1毫秒返回NI_TIMEOUT，改动摘要返回NI_PROTOCOL_ERROR，失败不得交付句柄。

17 请求路由终态（瞬时）。两个来源使用相同原号各发一条，按目标身份完成第一条。错误目标与重复回复均不得消耗另一条；
   第二条在截止前可查、恰到截止只回收一次。提前取消只影响指定来源的对应原号，所有记录移除后才能移除端点。
18 请求池容量（压力）。容量4097时修改已登记端点的握手能力值，拒绝零值或未知端点；随后由同一来源连续登记4097条请求，最后一槽可用；下一条拒绝且不增加计数。
   释放一个槽后必须用新原号再次接纳；最终清空请求，各端点可以移除。

19 发现入口文本（瞬时）。39字节IPv6地址及100个不同端口按输入顺序交付；追加等价短写时去重。
   空输出查询、缺一个槽或任一条地址超过39字节时核对返回码、项数及原输出保持性。

20 文件合并写与无进度期限（瞬时）。1537字节正文分1024和513字节两次送达，间隔700毫秒；超时设1000毫秒。
   新块推进时钟，重复块不续期。默认接收端收到第一块即可请求写盘。设置两块合并的 C++ 接收实例与不透明句柄均在第二块到达后交付完整正文，写入成功后才能发累计 ACK，提交成功后才发最终 ACK。发送端、零块、超协商窗口及已有待写数据时的重设分别核对错误码。

21 空文件提交与最终确认（瞬时）。使用空串的已知 SHA-256，两端直接处理 END；接收端只提交一次，确认在消费前可重复读取。
   Windows Debug 另外拒绝一次最终确认的实际内存分配，期待 NI_NO_MEMORY、空输出、文件保持已完成，下次重试成功。

22 动态消息字段（瞬时）。相同字段键分属两个组，分别保存含 NUL 的 513 字节 BLOB 和空 STRING。编码、读字段与读状态的输出区重叠时应返回 NI_INVALID_ARGUMENT 并保持原值，紧邻的状态文本和长度可以正常写入。查询容量、短缓冲保持、精确拷贝、非法类型和缺键分别判断；完整 protobuf 往返后核对两个字段，清除后允许解析空消息。
   Windows Debug 在只查长度时拦截字段副本分配，预期无需分配；替换时拒绝一次新正文分配，预期返回 NI_NO_MEMORY，旧字段和其他组均可继续读取；状态文本分配失败后也须保留状态码、文本和字段。预算恰好写满 1 MiB 后下一次空字段写入返回 NI_LIMIT_EXCEEDED。

23 OPEN 窗口协商（瞬时）。在指定组和缺字段的组分别读窗口。合法两端边界通过，越界和未对齐的 INT32 返回 NI_PROTOCOL_ERROR。
   给已设为 128 KiB 的窗口提交类型不符的消息，预期错误且保留额度；缺字段时可采用 64 KiB，已使用后拒绝再次配置。
   Win32 Debug 在检查长 BLOB 类型期间拒绝一次相同大小的分配，仍须判为类型错误，且不应触发该分配。

24 句柄工厂的池配置（瞬时）。请求、TLS 连接和链路分别使用自定义池。无效配置必须清空输出句柄，单槽端点池满后释放并可复用。
25 提交队列与索引（瞬时）。验证一次登记的唤醒回调在换池后仍能随真实提交触发、重复登记被拒绝，以及两个索引独立配置、队列策略和参数失败回滚，启动后禁止重配，实际提交、领取和归还数据。检查队列容量等待参数的默认值、零次重试、无效零毫秒，以及使用有限池时提交仍返回背压。用65字节 DATA 填满16槽入口，按1批×1项、2批×4项及默认8批×32项分别推进一次，再实测生产者可重新占用的槽数应为1、8及全部原有可用槽。2批×3项时允许基础队列对短前缀改为逐项领取，实际归还须在2至6项之间。持有第一帧时重复 beginPoll 和 next 须返回 NI_INVALID_STATE，队列仍满。最终逐条核对编号和全部65个正文字节，归还全部预算。把同步 DATA 交付期限设为 1 毫秒，用 owner 的单调时间样本推进后须收到 NI_TIMEOUT。
   十个记录池和索引分别限制为一个槽，每次只限制其中一项。两次真实提交应依次得到接纳和背压，独立的短 DATA 仍可接纳。断线后逐个归还请求和票据，字节计数回到零。
   执行者每次仅导入一项，提前 complete 队尾的第三个请求应返回 false。断线后尚有请求时，用原代号或新代号握手都应被拒绝；按原序取完三个请求，各一次。普通借用中断线时，旧代积压帧全部丢弃，同步票据可立即读取失败终态，队尾请求可在 finish 前结算，借用的正文仍有效。结束借用并重新握手只交付新代帧。另在 drainData 的第一帧回调中断线，核对成功消费1帧1字节、后续帧被丢弃、队尾请求可结算及最终空闲。

26 路由与会话配置（瞬时）。五个物理池分别验证错误配置。一个端点槽耗尽后关闭并复用，已启动的路由器拒绝换池。会话验证请求池的参数、压缩类型、发现配置及外部通道缺失时的错误。

27 TLS 密文字节预算（瞬时）。填满 7 字节输入后零接纳，ClientHello 输出上限精确为 19 字节。另一实例一次接纳 40000 字节。管理链路独立限制为 11 字节，握手恰到 10000 毫秒超时。TLS 及连接句柄另验字节数与输入或输出重叠时内容保持，改用独立计数后仍可接纳数据。另外验证压缩协商开关、非法标志和缺失的类型数组。

28 服务通道协商（瞬时）。运输回调独立解析 OPEN 的目标、通道与窗口。默认申请 1 MiB、应答省略窗口，以及申请并获得 128 KiB 时，应分别收到 13 和 29 字节的 TLS 首段。申请 64 KiB 而节点应答 80 KiB 时，应收到 NI_PROTOCOL_ERROR 关闭通知，业务 TLS 输出计为零。启动后重设应报状态错误。

29 解码正文缓存（瞬时）。5001 字节帧在默认设置下消费后释放。设置 -1 时在单帧 6000 字节限额内复用。半帧及 READY 期间拒绝修改；空闲时缩小预算立即释放。核对 0、非二次幂、非法负值、reset 后的保留策略及不透明句柄入口。用例13还在真实 TLS 接收端核对消费后的容量。

30 消息预算（瞬时）。34 字节预算恰好允许写入 2 字节错误文本，拒绝下一次修改。验证两个参数独立、非法设置保留前值、解析按输入大小计账、clear 沿用设置、零预算拒绝收费写入。取消累计限额后反复改写同一字段成本超过 1 MiB，读取最终业务码和长度。

31 执行宿主（瞬时）。先绑定由宿主独立创建的循环，再嵌套进入两个不同执行者。每层回调应收到自己的实例和 begin 时观察到的代次。wake 后本轮仍交付旧代次，下次 begin 才取新值。到期或已推进时等待为零。内层退出恢复外层，最外层退出恢复原宿主循环。两个实例各释放一次。缺少 begin 的函数表须被拒绝，未登记宿主时 wait 返回 NI_NEED_MORE。

32 直接监听的握手配置（瞬时）。独立客户端分别与默认路由器和自定义路由器交换完整 HELLO。默认应协商能力7、请求32、块65536，宿主等待预算最长5000毫秒。自定义应协商3、9、2048，预算最长24000毫秒。分别拒绝能力位、零请求、块大小和期限的无效值。重新配置物理池后仍沿用握手设置，监听开始后及附加客户端模式均拒绝重配。握手完成后，默认与自定义心跳的下一次等待预算分别为30000和12345毫秒，零间隔须被拒绝。

33 路由缓存与文件回收（瞬时）。每种配置转发8192字节真实RPC并完成响应。默认4096字节和零保留时暂存容量应降为零，-1时仍能容纳原信封。随后建立期限不同的三个文件流，按非创建顺序取消。下一次可等待时间随最早在册期限变化。每一条重复取消均只向服务发出一次终止通知，服务看到自己的流号，关闭后释放暂存容量。

34 发送端压缩配置（瞬时）。独立 TLS 对端解密后读取 NIF2 字节。4096 字节样本在门槛 4096 时压缩、4097 时原样发送。等级 1 和 9 的压缩字节与直接调用 Compress 的结果比对。验证等级 0、零门槛、启动前重配及无效等级，均检查失败后先前配置的实际效果。未列入白名单、文件控制、事件、无收益短正文和不可压缩正文应逐字节原样交付。协商紧凑格式后，与白名单 RPC 同类型的连续 DATA 应分别产生完整头及单字节增量头；重新协商后恢复完整头。

35 业务会话缓存与心跳（瞬时）。真实 HELLO 应答后，等待宿主分别观察到默认30000及自定义11000毫秒空闲期限。依次接纳40、8192字节和零字节 RPC，另行解析并逐字节核对，末次收到空正文。默认预算只保留小缓冲，0在每次交付后释放，-1在会话内复用；关闭后容量均为零。非法参数保留前值，连接后重配返回 NI_INVALID_STATE。三个请求同时超时时分别检查自定义每轮一个回调及默认一次结算全部。另让传输实际接纳一次 PING 但不回应，分别核对总无输入和 PONG 两个期限先到的等待值。

36 服务恢复与启动期限（瞬时）。发现器始终无候选时，宿主须分别得到约250、1400、700毫秒的预算，第三种由较短的总期限决定。另一组在首次建立运输后应立即继续推进，观察到 OPEN 后，在没有应答时等待默认30000或自定义2100毫秒。非法和迟到配置均须拒绝。读写轮次另验默认32/32、自定义1/3和4/2，检查帧观察者的实际调用数、发送工作步数、耗尽后的立即推进与空闲后的等待。连续拒绝两次物理连接并固定抖动样本时，初始基数32毫秒、最大40毫秒的两次等待应为16及20毫秒。时间预算以测试期间实际流逝的毫秒数校正。白盒部分调用通道实际使用的解密后帧交付策略，分别测试默认5000和自定义750毫秒。第一次拒收可发生在零时刻，重复拒收不能续期，截止时不再进入观察者，到期前接纳应解除期限。

37 隧道与借用输出（瞬时）。使用手写 NTW3 记录，分别让长度与正文、元数据和目标重叠。两个编码入口都应返回 NI_INVALID_ARGUMENT 并保留原字节，长度紧邻目标时应逐字节匹配手写记录。consumed 与输入重叠时不得消费，随后在同一解码器上完整重试。相交的读取输出应保留调用前的值，改用独立输出仍可读取并消费同一帧。

38 TLS 队列调度（瞬时）。为三对真实 TLS 会话分别使用默认设置、较小的轮转与推进预算、不限控制帧正文长度。两类队列持续有数据时按独立的编号序列逐帧核对解密结果，包括事件的全部正文。验证默认连续优先32个控制帧、可改为2个、0和-1两种控制正文门槛、推进1轮及最多交付2帧的边界。无效配置须保留原配置。预算让出后宿主收到零等待，全部排空后可重新等待。

39 分派预算（瞬时）。逐条提交5条双字节 DATA，在运输回调核对每条的编号、正文和先后顺序。命令／字节预算分别为1／100、10／3、10／1时，第一轮各须交付1、2、1条。空队列恢复5000毫秒可等待状态，非法和启动后重配分别验证。

40 同时到期（压力）。让三条真实文件流和三个已路由请求同时到期，每轮最多清理一个文件、两个请求。三轮后两端分别收到正确流号的三条通知，在册请求数依次为1、0、0。另让两个端点同时关闭，回收预算为一步，每轮只归还一个可供新服务使用的物理槽。

41 服务事件信封（瞬时）。用固定大端目标号和四字节二进制正文手写 NVE2 样本。查询长度、短缓冲和精确容量分别判定，核对每个输出字节和借用正文起点。长度落在正文或目标内应拒绝并保持二者，紧邻目标的长度可用。另验定向空事件、16字节撤销组、错误的撤销类型、全零组和定向目标为零。解析输出覆盖输入时保留原内容。

另有带隔离身份的功能入口 --link-timers <identity.dat>。用真实节点和客户端完成管理TLS及角色认证。给业务输出配置2个记录槽和1个小正文槽。小正文池和记录池分别耗尽时须拒绝，归还后按原顺序再次交付，错误重配保留原容量，启动后不能重配。从同一模拟时刻推进：空闲500毫秒后剩14500毫秒，PING发出500毫秒后剩9500毫秒。对端不回PONG，在满10000毫秒时超时。另在服务端保留一条业务记录后结束输入，排空1000毫秒后剩4000毫秒，满5000毫秒超时。另建一对独立实例，把单轮内部控制预算设为1，在此设置下再次通过认证。将真实 PING 和后续业务事件的密文合并交付，默认预算须在一次推进后能读到事件，预算为1须在第二次推进后才能读到相同内容。再把空闲、PONG、排空期限改为9000、7000、3000毫秒，分别核对剩余预算与精确到期时刻。样本由本机隔离测试网关生成，使用NGI3叶身份文件；测试宿主须能读取指定路径。

42 宿主随机输出（瞬时）。零字节与非法参数均不触发回调；8192 字节逐字节校验，8193 字节拒绝且保留原值。缺少回调或回调只写一字节后失败，均须擦除整个请求区间，且保留首尾保护字节。
43 密码派生（瞬时）。密码和盐各含 NUL，PBKDF2-HMAC-SHA256 迭代两次，与独立 .NET 实现生成的 32 字节向量比较。31 字节输出、零次及超过上限的迭代须返回对应错误，原输出与保护字节均保持。

功能验证结果（北京时间 2026-11-25 01:44）
--all 含43项，其中40项瞬时、3项压力。Windows Win32 Debug、x64 Release 分别用正式工程构建静态库和 DLL，四种组合各通过43项，并核对调用方的 DLL 导入表。Wasm32、Wasm64 的 Node 和 Chrome 四种组合也各通过43项。合计344项次。
Windows 正式测试工程另验 --instant、--stress、重复与范围选择、逐个编号运行，以及无效编号44的退出码2。Wasm 正式 CMake 测试宿主在 Node 上完整通过。--link-timers 使用隔离网关的叶身份，在 Windows 两种配置与 Node 两种位宽上通过。NetInterface.cpp 中的 main 示例在上述两种 Windows 配置和两种 Wasm 位宽运行成功。Wasm32 另启用未定义行为检查，43项及示例通过。编号6、8、14、20、22、25、27、35、36、37、38、39、41、42、43在 Win32 Debug、x64 Release、Node Wasm32 和 Node Wasm64 上分别连续运行8轮，共480项次通过。上述附加验证均单独统计。
Windows x64 Release 另验用例的判错能力。在隔离副本中错配正文池容量、改变随机回调输出、改错派生向量首字节，编号14、42、43分别返回失败。同轮使用正确输入时三项均通过。
Linux、Linux ARM、macOS、Android和iOS完成源码与构建配置静态检查，缺少实机运行结果。Android及Apple目标还需准备匹配ABI的第三方归档。

性能入口与已有数据
--benchmark 单独运行，--all 不启动计时。interface.encode1024 只测一个固定 NIF2 帧，正文1024字节，
复用1056字节输出。计时前独立比较全部头和正文。计时内逐次调用编码、检查交付长度、轮转读取正文形成校验值。每批结束按 256 字节周期的独立预期核对总和。
当前固定编码包含长度参数的重叠检查，缺少这一实现的性能测试数据，本次仅执行功能回归。紧凑编码、RPC、NVE2 事件、NOI2 本地服务信封、压缩等级和门槛、TLS、文件和路由等场景也缺少可比的专项耗时。随机回调、密码派生和不同大小级正文池缺少当前版本的独立计时数据。配置、创建、销毁和状态查询按功能验收，无需单列吞吐。
*/

// 稳定用例编号，供命令行及多平台测试宿主选择。
enum NetInterfaceTestCase
{
    // 固定帧
    CaseFrame = 1,
    // 紧凑完整帧
    CaseCompact = 2,
    // 流式摘要
    CaseDigest = 3,
    // 消息句柄
    CaseMessage = 4,
    // 分片复用
    CaseRepeatedOperations = 5,
    // 连续帧边界
    CaseFrameBoundaries = 6,
    // 增量编号
    CaseCompactSequence = 7,
    // RPC信封与心跳
    CaseRpc = 8,
    // 双向额度
    CaseCredit = 9,
    // 解压与协商
    CaseCompression = 10,
    // 双端握手与超时
    CaseHandshake = 11,
    // TLS 目标名
    CaseServiceName = 12,
    // TLS 双端与队列配置
    CaseTlsPools = 13,
    // 正文池物理槽与租约
    CaseBufferClasses = 14,
    // 发现表快照、预算及重复节点
    CaseDirectoryNodes = 15,
    // 缓存时间边界与内容校验
    CaseDirectoryCache = 16,
    // 来源隔离、截止边界和唯一终态
    CaseRequestLifecycle = 17,
    // 显式物理槽上限与释放后的编号
    CaseRequestCapacity = 18,
    // 数值地址的固定宽度及列表容量查询
    CaseDiscoverySeeds = 19,
    // 合并写的接收进度、超时及提交确认
    CaseFileBatchProgress = 20,
    // 空文件及最终确认缓冲重试
    CaseEmptyFileCommit = 21,
    // 长字段的读取成本和替换失败后的所有权
    CaseMessageFields = 22,
    // OPEN 可选窗口的分组、范围和配置失败后的额度
    CaseCreditNegotiation = 23,
    // 不透明句柄工厂的各级池
    CaseHandlePools = 24,
    // 无锁提交队列及索引的独立配置
    CaseSubmissionConfiguration = 25,
    // 路由器和会话的配置传递
    CaseOwnerPools = 26,
    // 每条 TLS 的双向密文预算和握手期限
    CaseCiphertextConfiguration = 27,
    // 服务通道创建内部业务 TLS 时的配置传递
    CaseServiceCiphertextConfiguration = 28,
    // 解码正文的实际占用和消费后保留策略
    CaseDecoderRetention = 29,
    // 消息句柄的累计修改与错误文本上限
    CaseMessageLimits = 30,
    // 执行宿主函数表和作用域回调
    CaseExecutionConfiguration = 31,
    // 直连路由器的应用握手协商和宿主等待期限
    CaseRouterHandshake = 32,
    // 路由转发缓存和文件流回收
    CaseRouterStorage = 33,
    // 实际发送端的压缩阈值、等级和按类型回退
    CaseOutboundCompression = 34,
    // 会话RPC暂存与业务心跳等待预算
    CaseSessionOptions = 35,
    // 服务发现重试和恢复总限时的事件唤醒
    CaseRecoveryWait = 36,
    // NTW3 记录的缓冲、长度与消费游标
    CaseTunnelBuffers = 37,
    // 连接控制帧轮转、分类、推进与空闲等待
    CaseTlsScheduling = 38,
    // 分派器每次poll的命令数、字节数与空闲等待
    CaseOwnerBudgets = 39,
    // 多个文件与请求同时到期时的独立回收预算
    CaseRouterPollBudgets = 40,
    // NVE2定向及文件撤销事件的线格式和借用边界
    CaseEventEnvelope = 41,
    // 随机回调交付长度及失败时的输出擦除范围
    CaseEntropyOutput = 42,
    // 二进制密码和盐的 PBKDF2 派生及失败边界
    CasePasswordDerivation = 43
};

/** 模拟业务进程启用数字千位分组的环境
*/
class NetInterfaceTestGroupedDigits : public std::numpunct<char>
{
protected:
    /** 每三位分组
    @return 本用例使用的分组宽度
    */
    std::string do_grouping() const override;
};

/** 端点池测试使用的空闲服务宿主，整个用例不会向服务交付请求
*/
class PoolTestSink : public NetEndpointServiceSink
{
public:
    /** 没有业务发送能力，意外交付时拒绝
    @return NI_NOT_SUPPORTED
    */
    int32_t send(uint64_t, const NetInterfaceFrame&, const NetEndpointOrigin&) override;

    /** 端点释放由路由器负责
    */
    void close(uint64_t) override;
};

/** 接纳真实路由帧，记录两端分别看得见的请求号、流号及终止通知
*/
class RouterFileSink : public NetEndpointServiceSink
{
public:
    /** 建立空的投递记录 */
    RouterFileSink();

    /** 从收到的已编码应答中取回实际公开流号 */
    int32_t send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& origin) override;

    /** 路由器拥有端点生命期 */
    void close(uint64_t endpoint) override;

public:
    // 最近实际交付的请求号
    uint64_t m_request;
    // 客户端在授权应答中读到的最近流号
    uint64_t m_stream;
    // 服务端按顺序收到的本地流终止编号
    std::vector<uint64_t> m_notices;
};

/** 用公开运输回调把独立客户端的HELLO交给直接监听的路由器
*/
class RouterHandshakeTransport : public NetServerChannel
{
public:
    /** 借用客户端至关闭
    @param [in,out] client 用于验证服务端实际发送的应答
    */
    explicit RouterHandshakeTransport(NetInterfaceHandshake& client);

    /** 此运输只提供监听，拒绝主动连接 */
    int32_t connect(const char*, uint32_t, const NetInterfaceTlsOptions&, NetTransportListener*, uint64_t& peer) override;

    /** 保存监听者，回调交给后续poll */
    int32_t listen(const char*, uint32_t, const NetInterfaceTlsOptions&, NetTransportListener* listener, uint32_t) override;

    /** 模拟TLS就绪及随后的独立HELLO到达 */
    bool poll() override;

    /** 把路由器交付的应答送到客户端握手状态机 */
    int32_t send(uint64_t peer, const NetInterfaceFrame& frame) override;

    /** 保存关闭原因以便区分正常握手与提前关闭 */
    void close(uint64_t, int32_t reason) override;

    /** 结束监听者借用 */
    void shutdown() override;

    /** 返回本用例的监听端口 */
    uint32_t localPort() const override;

    /** 返回当前物理连接数 */
    uint32_t peerCount() const override;

private:
    // 用例所有的独立客户端
    NetInterfaceHandshake& m_client;
    // 监听开始到停止期间借用的路由入口
    NetTransportListener* m_listener;
    // 已交付的运输事件数
    unsigned m_events;

public:
    // 客户端接收应答的结果或提前关闭原因
    int32_t m_result;
};

class CiphertextProbeHost;

/** 在网关运输边界记录真实业务 TLS 交付的首段密文，只模拟管理节点的 OPEN 响应
*/
class CiphertextProbeTransport : public NetGatewayTransport
{
public:
    /** 将首段长度交给拥有工厂的用例
    @param [in,out] host 保存 OPEN 应答窗口及观测值，寿命覆盖运输
    */
    explicit CiphertextProbeTransport(CiphertextProbeHost& host);

    /** 保存回调，下一次 poll 才报告管理节点就绪
    */
    int32_t connect(const char*, uint32_t, const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener,
        uint64_t& peer) override;

    /** 交付已接纳的管理通知，保持 sendControl 与回调分离
    */
    bool poll() override;

    /** 为产品7的业务通道生成有效的管理 OPEN 回应
    */
    int32_t sendControl(uint64_t peer, const NetInterfaceFrame& frame) override;

    /** 读取真实 TLS ClientHello 的记录类型及第一段长度，然后模拟运输背压
    */
    int32_t sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record) override;

    /** 结束监听借用
    */
    void shutdown() override;

private:
    // 测试宿主提供应答窗口并保存实际线路观测值
    CiphertextProbeHost& m_host;
    // 本次连接的管理回调
    NetLinkTransportListener* m_listener;
    // 是否已交付管理身份
    bool m_ready;
    // 等到下次推进才交付的 OPEN 响应正文
    std::string m_response;
};

/** 为会话的实际服务发现接入路径提供运输和业务回调
*/
class CiphertextProbeHost : public NetGatewayTransportFactory, public NetSessionListener
{
public:
    /** 初始状态尚未收到业务密文
    */
    CiphertextProbeHost();

    /** 会话取得并拥有独立运输
    */
    NetGatewayTransport* create() override;

    /** 当前用例只推进到 ClientHello
    */
    void onSessionReady(uint64_t, uint32_t, uint32_t) override;

    /** 当前用例没有业务请求
    */
    void onSessionResult(uint64_t, uint64_t, uint32_t, uint32_t, uint32_t, const uint8_t*, uint32_t) override;

    /** 当前用例不接纳业务帧
    */
    bool onSessionFrame(uint64_t, const NetInterfaceFrame&) override;

    /** 保存 OPEN 失败的具体原因供用例核对
    */
    void onSessionClosed(uint64_t, int32_t, int32_t) override;

public:
    // 在实际运输回调处观测到的首份密文长度
    uint32_t m_first;
    // 收到的有效 OPEN 申请窗口
    uint32_t m_offered;
    // 模拟节点选定的窗口，0表示省略可选字段
    uint32_t m_reply;
    // 是否在每次连接时报告协议失败以触发真实恢复流程
    bool m_refuseConnect = false;
    // 已收到的终态通知数
    uint32_t m_closes = 0;
    // 已实际交付的请求超时通知数
    uint32_t m_timeouts = 0;
    // 最近的会话关闭回调原因
    int32_t m_closed;
};

// 用真实服务端握手驱动业务会话，记录被通道接纳的RPC正文
class SessionProbeChannel : public NetClientChannel
{
public:
    /** 借用同一用例构造的独立服务端握手
    @param [in,out] server 服务端状态，存活到通道停止
    */
    explicit SessionProbeChannel(NetInterfaceHandshake& server);

    /** 启动唯一连接，保存监听者直到shutdown
    @param [in] address 会话请求的地址
    @param [in] port 会话请求的端口
    @param [in] options 连接配置
    @param [in] listener 本连接的回调
    @param [out] peer 模拟通道标识
    @return NI_OK
    */
    int32_t connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
        NetTransportListener* listener, uint64_t& peer) override;

    /** 在poll中交付就绪和独立生成的握手应答 */
    bool poll() override;

    /** 在调用期复制RPC正文或推进对端握手 */
    int32_t send(uint64_t peer, const NetInterfaceFrame& frame) override;

    /** 记录关闭意图，终态由会话shutdown收敛 */
    void close(uint64_t peer, int32_t reason) override;

    /** 结束监听借用 */
    void shutdown() override;

    /** 已接纳的输出当场交给测试接收端
    @param [in] peer 连接标识
    @return 有效连接为NI_OK
    */
    int32_t outputState(uint64_t peer) const override;

    // 运输实际接纳的DATA编号，按交付顺序记录
    std::vector<uint64_t> m_dataIds;
    // 运输实际接纳的心跳数
    uint32_t m_pings = 0;
    // 接纳的业务正文，与调用方的原始内容独立比较
    std::vector<uint8_t> m_body;

private:
    // 借用的服务端握手
    NetInterfaceHandshake& m_server;
    // 当前会话的运输监听者
    NetTransportListener* m_listener;
    // 就绪通知只交付一次
    bool m_announced;
};

// 接纳与拒收由场景选择，记录交付次数并验证借用的帧仍为原数据。
class FrameDeliveryProbe : public NetTransportListener
{
public:
    /** 此观察者仅验证应用帧，忽略就绪通知
    @param [in] peer 用例逻辑端点
    */
    void onReady(uint64_t peer) override;

    /** 检查逻辑端点、消息头及二进制正文
    @param [in] peer 逻辑端点
    @param [in] frame 调用期借用的应用帧
    @return 用例设置的接纳结果
    */
    bool onFrame(uint64_t peer, const NetInterfaceFrame& frame) override;

    /** 此观察者的生命周期覆盖整个场景，忽略关闭通知
    @param [in] peer 逻辑端点
    @param [in] reason 关闭原因
    @param [in] detail 底层诊断
    */
    void onClosed(uint64_t peer, int32_t reason, int32_t detail) override;

    // 是否接纳当前帧
    bool m_accept = false;
    // 每次交付均核对内容
    bool m_valid = true;
    // 回调累计次数
    uint32_t m_calls = 0;
};

// 没有就绪候选的发现器，用于验证无网络事件时的重试与总期限
class PendingDiscovery : public NetClientDiscovery
{
public:
    /** 开始等待候选 */
    int32_t start(const std::vector<NetGatewayAddress>& seeds, const std::string& cachePath, const NetInterfaceTlsOptions& tls) override;

    /** 保持等待，不产生网络事件 */
    bool poll() override;

    /** 立即结束该发现代次 */
    void shutdown() override;

    /** 查询资源是否已收敛 */
    bool stopped() const override;

    /** 记录一次选择，始终没有就绪的候选 */
    bool choose(NetClientDiscoveryPoint& endpoint) override;

    /** 没有候选被尝试 */
    void failed(const NetClientDiscoveryPoint& endpoint) override;

    // choose的实际调用次数
    uint32_t m_attempts = 0;
    // 是否仍在等待
    bool m_running = false;
};
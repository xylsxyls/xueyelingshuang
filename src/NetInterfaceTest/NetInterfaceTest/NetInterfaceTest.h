#pragma once
#include "NetInterface/NetInterfaceAPI.h"
#include "NetInterface/NetObjectPool.h"
#include <locale>
#include <vector>

/** NetInterfaceTest 专项测试说明

被测入口为已构建 SDK 的 NetInterface/NetInterfaceAPI.h。所有功能用例调用真实 C++ 接口。
两端 TLS 共用测试工程已有的公开测试证书样本，只在当前进程的内存中交换密文，避免网络和端口影响判定。
不带参数或使用 --all 都先执行全部瞬时用例，再执行压力用例。--cases 2,8 可按编号选择，--instant 和 --stress 分别运行瞬时与压力用例。
Windows 运行对应配置的 EXE；Wasm 使用对应位宽的 LibraryTestHost.js 和 NetInterfaceTest.js。
Linux、Linux ARM、macOS、Android、iOS 通过项目 CMake 和 PlatformTest 的相应宿主构建或启动同一组用例。
Linux 和 Linux ARM 的 SDK 静态归档沿用生产库声明的 rt 与 atomic 系统链接项。Linux 在 VS Code 中先配置 CMake、选定并构建 NetInterfaceTest，再用同名调试项运行 --all。

编号、输入与独立预期
01 固定帧（瞬时）。手写32字节 NIF2 头和3字节二进制正文。分别通过帧引用、帧指针和六字段数组编码，成功长度和全部字节均须匹配手写样本。按片解码还原全部字段。
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
07 紧凑增量（瞬时）。链接静态 NetInterface SDK 的测试构建另以旧编号字段作为编码输出，41 到 44 的跳号和单字节正文 77 须得到 E4 01 03 4D。对完整 DATA 基线送入单字节增量和跨分片的变长增量，请求号依次为40、41、44。
   类型和产品继承，正文分别核对。完整 DATA 头已接纳时，分别在正文尚未收全、已就绪待消费和已消费时关闭再开启紧凑许可。当前帧仍可完整读取，随后两种增量头都须等到新的完整基线。重复开启已有许可保留基线。
08 RPC（瞬时）。与独立 NRP2 信封字节比较，核对父编号高低字、正文偏移和时限。重叠缓冲或长度输出别名
   拒绝且不写输出，相邻区间可用，超出16位的种类拒绝；PING 原地转为 PONG，PONG 无回复。本地 NOI2 来源前缀另验独立输出、正文别名与长度别名。解析 RPC 时允许复用字段数组，结果覆盖正文时拒绝。NIS2 服务握手核对输入秘密、帧元数据与协商输出的独立性；NOI2 拆包允许原地更新帧描述，输出与正文重叠时保持输入。
09 信用窗口（瞬时）。验证默认字节数、窗口对齐、使用后禁止改配置。发送7字节、接收3字节，
   各自归还后两项恢复；多还1字节触发持续协议错误。
10 压缩（瞬时）。消费 1024 字节解压明文后复用缓冲区解码 3 字节短帧，原明文剩余 1021 字节应全为零。手写 Zstandard RLE 数据块，期望1024个 A。未协商拒绝，1023字节预算拒绝；
   协商且容量足够时整个输出逐字节一致。分别将 REQUEST 和 RESPONSE 的头分成 31 字节和最后 1 字节。压缩的 60001 至 60006 号文件消息应在头结束时返回 NI_PROTOCOL_ERROR，消费到头末尾，后续输入消费 0。reset 后相同种类和类型的未压缩帧原样可读。相邻的 60000 和 60007 号 RPC 应解压得到 1024 个 A。
11 HELLO（瞬时）。身份字节覆盖句柄输出时返回 NI_INVALID_ARGUMENT，仍能用独立句柄正常握手。驱动客户端和服务端互换握手记录，协商得到能力位 3、请求能力声明 16 项、最大块 4096 字节，核对双方身份字段。句柄入口的能力字段和身份区重叠时返回 NI_INVALID_ARGUMENT，整段输出保持原值。
   单独的未完成握手在截止前仍等待，到截止时刻返回 NI_TIMEOUT。
12 TLS 服务名（瞬时）。临时设置带千位分组的数值 locale，为产品12345构造真实 TLS ClientHello。 同时核对产品号 0 和 UINT32_MAX 的完整十进制名称。
   按记录、握手及扩展长度定位 server_name，只接受域名列表中完整的 service.12345；测试结束恢复原 locale。
13 TLS 队列与背压（瞬时）。两端用独立 TLS 会话握手，每次最多转交97字节密文。控制和普通记录池各设1槽，
   非法配置须保留原设置，启动后配置须拒绝。连续两轮各接纳一条请求与心跳，第二条同类消息拒绝，
   对端每轮各收到一条并校验编号、类型与正文；排空后无迟到重复帧。
14 正文池（瞬时）。五个大小类各设1槽，检查分级容量、满池错误、跨级隔离、swap/reset 和再次分配。另用独立配置创建五级池，第一级从 1 槽按 2 槽增长、无配置上限，同时持有 6 个租约时应已增长 3 次。其余四级上限分别为 4、3、2、1 槽，逐级核对保护记录数、增长通知、满池拒绝、归还后占用归零及各租约的正文。
   零字节不分配，超出最大档拒绝，租约先于池析构。
15 发现表快照（瞬时）。65个互异节点走缓存编码、解码后完整复制；少一槽时保留原数组，最后一项版本无效时前面的项也不得写入。
   直接读取目录文本，校验长度恰好达到上限、允许的标点，以及字段缺失、类型错误、空串、超长、内嵌零或非法字符时清空已有输出。Win32 Debug 下在合法节点名称的实际结果缓冲分配处注入故障，直接读取返回 false 并清空文本，整表解析返回 NI_NO_MEMORY 并清空节点，解除注入后同一输入可重试成功。
   另外从公开节点创建并导出缓存，解包后逐字节检查 COUNT、PORT、CAPACITY 和 BOOT 的类型与小端正文。端口 0x1234 应为 34 12 00 00，容量 0x89abcdef 采用八字节 UINT64，代次 0xf1e2d3c4b5a69788 的第一个字节为 88。用独立构造的同一组字段创建目录并读回完整节点，容量改为 INT32 字节 01 00 00 80 时须拒绝整份目录。
   同一数值IPv6和同一端口的两种拼写不得重复入表；400个有效节点实际编码超过16 KiB、443个节点仅名称即超预算，两者均整体拒绝。
16 发现缓存（瞬时）。固定UTC毫秒写出NGC3。当前时间为零须返回 NI_INVALID_ARGUMENT 并清空输出句柄。从实际导出的缓存验证未来5分钟及过去30天的精确边界，
   两侧各超1毫秒返回NI_TIMEOUT，改动摘要返回NI_PROTOCOL_ERROR，失败不得交付句柄。

17 请求路由终态（瞬时）。两个来源使用相同原号各发一条，按目标身份完成第一条。错误目标与重复回复均不得消耗另一条；
   第二条在截止前可查、恰到截止只回收一次。提前取消只影响指定来源的对应原号，所有记录移除后才能移除端点。
   自动原号从低字全一进位，截止时间也跨过低字边界。两个低字相同的来源持有同一原号，用十项参数完成指定来源的父请求，再按父路由号取走其子请求，按端点号取走另一来源的独立请求。逐项比较十六项输出，核对错误高字及不匹配目标不消耗请求，每个终态只能取走一次。
18 请求池容量（压力）。容量4097时修改已登记端点的握手能力值，拒绝零值或未知端点；随后由同一来源连续登记4097条请求，最后一槽可用；下一条拒绝且不增加计数。
   释放一个槽后必须用新原号再次接纳；最终清空请求，各端点可以移除。

19 发现入口文本（瞬时）。39字节IPv6地址及100个不同端口按输入顺序交付；追加等价短写时去重。
   空输出查询、缺一个槽或任一条地址超过39字节时核对返回码、项数及原输出保持性。本地 vector 重载应按输入顺序返回 IPv4 及去重后的 IPv6，遇到端口 65536 应返回 NI_INVALID_ARGUMENT 并清空既有结果。

20 文件合并写与无进度期限（瞬时）。1537字节正文分1024和513字节两次送达，间隔700毫秒；超时设1000毫秒。
   新块推进时钟，重复块不续期。默认接收端收到第一块即可请求写盘。设置两块合并的 C++ 接收实例与不透明句柄均在第二块到达后交付完整正文，写入成功后才能发累计 ACK，提交成功后才发最终 ACK。发送端、零块、超协商窗口及已有待写数据时的重设分别核对错误码。

21 空文件提交与最终确认（瞬时）。使用空串的已知 SHA-256，两端直接处理 END；接收端只提交一次，确认在消费前可重复读取。
   Windows Debug 另外拒绝一次最终确认的实际内存分配，期待 NI_NO_MEMORY、空输出、文件保持已完成，下次重试成功。

22 动态消息字段（瞬时）。相同字段键分属两个组，分别保存含 NUL 的 513 字节 BLOB 和空 STRING。编码、读字段与读状态的输出区重叠时应返回 NI_INVALID_ARGUMENT 并保持原值，紧邻的状态文本和长度可以正常写入。查询容量、短缓冲保持、精确拷贝、非法类型和缺键分别判断；完整 protobuf 往返后核对两个字段，清除后允许解析空消息。
   Windows Debug 在只查长度时拦截字段副本分配，预期无需分配；替换时拒绝一次新正文分配，预期返回 NI_NO_MEMORY，旧字段和其他组均可继续读取；状态文本分配失败后也须保留状态码、文本和字段。预算恰好写满 1 MiB 后下一次空字段写入返回 NI_LIMIT_EXCEEDED。

23 OPEN 窗口协商（瞬时）。在指定组和缺字段的组分别读窗口。合法两端边界通过，越界和未对齐的 INT32 返回 NI_PROTOCOL_ERROR。
   测试对端直接提供四字节小端字段，包括 64 KiB 的 00 00 01 00、1 MiB 的 00 00 10 00 和负一的 FF FF FF FF。
   给已设为 128 KiB 的窗口提交类型不符的消息，预期错误且保留额度；缺字段时可采用 64 KiB，已使用后拒绝再次配置。
   Win32 Debug 在检查长 BLOB 类型期间拒绝一次相同大小的分配，仍须判为类型错误，且不应触发该分配。

24 句柄工厂的池配置（瞬时）。请求、TLS 连接和链路分别使用自定义池。句柄落在请求池配置或链路声明内时拒绝创建并保留原存储。无效配置必须清空输出句柄，单槽端点池满后释放并可复用。
25 提交队列与索引（瞬时）。首个 BEGIN 尚未导入时连续两次 complete 均须返回 false 并清空命令。beginPoll 导入后仅正确代次能取得原元数据，且只能完成一次。令同步请求的票据输出与正文重叠，令同步 DATA 的票据输出分别与帧请求号或正文重叠。三次均须返回 NI_INVALID_ARGUMENT，保留输入，不接纳请求或帧。换用独立票据后，原请求和原 DATA 仍可提交。生产者连续竞争64次让步，原生回收和同步发送的轮询休眠各默认1毫秒。两个实例分别读回各自配置，输入对象改动、非法重配和换池均应保留已设置值，启动后设置须被拒绝。验证一次登记的唤醒回调在换池后仍能随真实提交触发、重复登记被拒绝，以及两个索引独立配置、队列策略和参数失败回滚，启动后禁止重配，实际提交、领取和归还数据。检查队列容量等待参数的默认值、零次重试、无效零毫秒，以及使用有限池时提交仍返回背压。用65字节 DATA 填满16槽入口，按1批×1项、2批×4项及默认8批×32项分别推进一次，再实测生产者可重新占用的槽数应为1、8及全部原有可用槽。2批×3项时允许基础队列对短前缀改为逐项领取，实际归还须在2至6项之间。持有第一帧时重复 beginPoll 和 next 须返回 NI_INVALID_STATE，队列仍满。最终逐条核对编号和全部65个正文字节，归还全部预算。把同步 DATA 交付期限设为 1 毫秒，用 owner 的单调时间样本推进后须收到 NI_TIMEOUT。同步回执还须交付与原请求期限一致的剩余毫秒，结束或消费后查询写0。
   十个记录池和索引分别限制为一个槽，每次只限制其中一项。两次真实提交应依次得到接纳和背压，独立的短 DATA 仍可接纳。断线后逐个归还请求和票据，字节计数回到零。
   执行者每次仅导入一项，提前 complete 队尾的第三个请求应返回 false。断线后尚有请求时，用原代号或新代号握手都应被拒绝；按原序取完三个请求，各一次。普通借用中断线时，旧代积压帧全部丢弃，同步票据可立即读取失败终态，队尾请求可在 finish 前结算，借用的正文仍有效。结束借用并重新握手只交付新代帧。另在 drainData 的第一帧回调中断线，核对成功消费1帧1字节、后续帧被丢弃、队尾请求可结算及最终空闲。
   回执索引只配一条保护记录，在第二张票据触发扩容时查询已结束的第一张。预期消费查询返回 NI_READY 和配置的 7 毫秒重试间隔，零号和超出分配范围的编号返回 NI_NOT_FOUND，登记观察者返回 NI_LIMIT_EXCEEDED。扩容结束后重新登记，预期通知一次，两张票据各可消费一次 NI_OK，期间没有关闭请求。

26 路由与会话配置（瞬时）。主动关闭第一个端点时，由其 close 回调请求整体停止；回调返回时只关闭了这一个端点，下一次 poll 关闭第二个。第二个回调再次请求停止后，重复推进不重复通知，运输仍可正常解绑。五个物理池分别验证错误配置。一个端点槽耗尽后关闭并复用，已启动的路由器拒绝换池。会话验证请求池的参数、压缩类型、发现配置及外部通道缺失时的错误。 请求表的端点池只给保留槽时，首次服务或客户端登记必须返回 NI_LIMIT_EXCEEDED 并清零输出。扩大到两槽且重新设置租约后，同一端点可以登记。

27 TLS 密文字节预算（瞬时）。创建时让诊断字段与角色、物理池、CA、证书、密钥、主机名或句柄输出共用存储，预期参数错误且原值保持。相邻的主机名和诊断字段可正常创建。填满 7 字节输入后零接纳，ClientHello 输出上限精确为 19 字节。另一实例一次接纳 40000 字节。管理链路创建后尚未借出 HELLO 时 outputState 就应返回 NI_READY，借出后保持同一状态，消费 HELLO 后暂时返回 NI_OK。推进生成密文后恢复 NI_READY，首段保持独立的 11 字节上限。握手恰到 10000 毫秒超时，输出状态返回同一错误。TLS 及连接句柄另验字节数与输入或输出重叠时内容保持，改用独立计数后仍可接纳数据。另外验证压缩协商开关、非法标志和缺失的类型数组。

28 服务通道协商（瞬时）。改变调用方白名单数组并提交包含零类型的非法重配后，协商开启压缩的通道须收到最初的两个类型及其顺序，未协商压缩的通道不应收到配置。OPEN 接纳、业务连接已创建后，正确 peer 应可启停紧凑帧和压缩，错误 peer 应收到 NI_INVALID_STATE。压缩类型计数非零时传空数组应收到 NI_INVALID_ARGUMENT。OPEN 完成后分别给 CHANNEL_CLOSE 提供负值、零、缺少原因、错误类型和正值。会话应分别报告原始负值、截断或后三者对应的协议错误，每代只通知一次。用 1 字节 TLS 密文输入队列接收一次送达的 4 字节记录头前缀。运输暂停供给后，执行者仍应得到三次零等待提示，完成本地暂存前三次搬运。前缀全部交给 TLS 后应恢复正等待预算；总启动期为 350 毫秒、业务握手为 250 毫秒时，等待不超过剩余握手期限，连续静默跨过两项期限应报告先到的 NET_INTERFACE_TLS_TIMEOUT。另一实例总启动期为 150 毫秒、业务握手为 10000 毫秒时应先报告 NI_TIMEOUT，整段信用只归还一次。管理链路单独配置相同的密文输入大小和每轮一步预算，同样应在没有新密文时继续推进，前缀全部接纳后恢复正等待。另将管理 TLS 输出也设为 1 字节且暂停取走 ClientHello。首次接纳后再次推进，双向没有移动时必须恢复正等待。直接构造网关通道时，空地址应被拒绝并清零输出代次，可在原对象上改用有效地址。服务模式已终止而未绑定直连通道时，调用直连应返回 NI_NOT_SUPPORTED，代次和通知次数均保持。为三个独立会话预设不同窗口与 TLS 输出限额，配置并接纳全部三个连接后才开始推进。修改调用方的限额对象，各会话仍应采用自己的原值。运输回调独立解析 OPEN 的目标、通道与窗口。默认申请 1 MiB、应答省略窗口，以及申请并获得 128 KiB 时，应分别收到 13 和 29 字节的 TLS 首段。申请 64 KiB 而节点应答 80 KiB 时，应收到 NI_PROTOCOL_ERROR 关闭通知，业务 TLS 输出计为零。启动后重设应报状态错误。管理运输在 poll 时静默结束，期望 NI_TRUNCATED；当次先给出错误再结束，保留原错误。分配异常应报告 NI_NO_MEMORY，其他运输异常应报告 NI_INVALID_STATE。每次只能收到一次关闭通知。运输在 connect 中抛出普通异常或分配异常时，返回相应错误，输出连接号归零。
   捕获业务通道的管理消息，核对 OPEN 的 UINT64 通道和目标以及 INT32 窗口。目标 0x01020304 的八字节正文须为 04 03 02 01 00 00 00 00。提供 258 字节的 TLS 记录前缀，通道归还 CREDIT 时金额须为 02 01 00 00。往 CHANNEL_CLOSE 填入 FC FC FD FE 后，唯一关闭通知须交付 -0x01020304。
   逐轮检查外层 HELLO、CONTROL 接纳即提示零等待，暂存控制记录时再投递则保持正等待。管理 TLS 的 1 字节输入队列连续接纳 4 个字节时，4 轮均须零等待，第 5 轮恢复正等待。原始 TLS 分别检查 1 字节前缀成功、满队列零接纳、空输入和非法输入；首次 EOF 唤醒一次，重复 EOF 保留正等待，推进后验证截断。静态接收状态另将首段 17 字节填满 TLS 队列，第二段整段进入网关暂存时须提示进展，暂存仍被占用时原样重试保持原等待预算。
   同一执行者在两轮通道 poll 之间直接投递管理就绪和 OPEN 应答，均须零等待；下一轮应分别发出 OPEN 和真实业务 ClientHello。首次非法记录提示零等待收尾，再投递同一错误维持正等待，最终报告首次协议错误。静态 SDK 和 Wasm 用真实业务 TLS 发送 64 个不同的 1024 字节请求，实际占满 64 KiB 外层窗口。零额度下有积压时首次授额提示零等待，已有正额度时再授额维持正等待；逐段恢复窗口后，对端解密全部请求，额度为零且已经排空时再授额也保持正等待。
   令运输拒收同一份 4 字节 CREDIT 两次，下层无积压时通道仍应报告 NI_READY；重试期间再接纳 1 字节后，第三次只归还原有 4 字节并触发零等待。下一轮归还新增的 1 字节，累计 5 字节且通道恢复 NI_OK。令同一轮接纳密文前缀后交付关闭事件、报告带错误的运输结束或静默返回 false，核对自动推进和最终原因。静态 SDK 和 Wasm 另用测试 CA 握手，在网关内部接收状态中将两帧正文及认证关闭一起送入 17 字节 TLS 输入队列，紧接着投递外层关闭。验证完整帧先交付，内层正常、截断、半帧与显式失败各自结算；首帧暂时拒收后可接纳，第二帧持续被拒不能推迟从外层关闭起算的总期限。

   通过公开 connectService 传入目标 0 和 7，分别提交产品 7、9 和 0，核对 beginRequest、beginRequestAt、sendData 对非法产品及尚未就绪的返回值、失败输出和等待表状态。测试节点按普通身份拒绝 OPEN 0，目标 7 推进到业务 TLS ClientHello。另在显式通道上经过与服务连接共用的 HELLO 准备流程，与独立服务端握手机器协商七组能力声明，检查服务端结果和通道实际启用的压缩、紧凑帧位。完整受认证服务间子调用属于网关集成用例。

29 解码正文缓存（瞬时）。缓存复用时，新短帧的三个字节必须正确，旧帧的有效尾部应全为零。5001 字节帧在默认设置下消费后释放。设置 -1 时在单帧 6000 字节限额内复用。半帧及 READY 期间拒绝修改；空闲时缩小预算立即释放。核对 0、非二次幂、非法负值、reset 后的保留策略及不透明句柄入口。用例13还在真实 TLS 接收端核对消费后的容量。

30 消息预算（瞬时）。34 字节预算恰好允许写入 2 字节错误文本，拒绝下一次修改。验证两个参数独立、非法设置保留前值、解析按输入大小计账、clear 沿用设置、零预算拒绝收费写入。取消累计限额后反复改写同一字段成本超过 1 MiB，读取最终业务码和长度。

31 执行宿主（瞬时）。先绑定由宿主独立创建的循环，再嵌套进入两个不同执行者。每层回调应收到自己的实例和 begin 时观察到的代次。wake 后本轮仍交付旧代次，下次 begin 才取新值。到期或已推进时等待为零。内层退出恢复外层，最外层退出恢复原宿主循环。直接调用 exchange 保存当前协议与宿主归属，再通过 bind 恢复，两个归属和被替换的宿主输出均应正确。两个实例各释放一次。缺少 begin 的函数表须被拒绝，未登记宿主时 wait 返回 NI_NEED_MORE。

32 直接监听的握手配置（瞬时）。业务 PONG 截止时交付匹配应答，或总无输入到期后交付 PING，都应以 NI_TIMEOUT 关闭。应用握手期间的合法停顿只使用握手期限。另核对两个心跳期限前一毫秒、恰好相等和 UINT64_MAX。握手后交付 PING，运输返回 NI_TRUNCATED、抛普通异常和抛分配异常时，关闭原因应分别是 NI_TRUNCATED、NI_INVALID_STATE 和 NI_NO_MEMORY。绑定运输后、监听失败回滚后分别调用路由器 poll，两次均应返回 true，运输的 poll 调用数保持零。成功重试后连续两次 poll，运输恰好推进两次并完成应用握手。空地址、错误 TLS 角色和非法握手时限应在调用运输前被拒绝。运输接纳和失败回滚期间禁止重绑与改参数。如果宿主在回滚中请求停止，返回后同样拒绝重配，下一次 poll 应完成停止。分配异常返回 NI_NO_MEMORY 并停止部分启动的资源，随后可在同一实例重试。独立客户端分别与默认路由器和自定义路由器交换完整 HELLO。默认应协商能力7、请求32、块65536，宿主等待预算最长5000毫秒。自定义应协商3、9、2048，预算最长24000毫秒。分别拒绝能力位、零请求、块大小和期限的无效值。重新配置物理池后仍沿用握手设置，监听开始后及附加客户端模式均拒绝重配。握手完成后，默认与自定义心跳的下一次等待预算分别为30000和12345毫秒，零间隔须被拒绝。运输持续拒收 HELLO_REPLY 超过总无输入期限后，在握手期限内恢复接纳，仍须进入就绪并获得完整的首个心跳间隔。自定义握手应答交付当轮即登记 12345 毫秒的心跳预算，期间收到新的合法入站 PING 后须回送对应 PONG，并从这次接收重新计空闲间隔。直接调用 peerInfo 时，分别核对三个输出区域任意两组相交或缺少输出地址，应返回 NI_INVALID_ARGUMENT 且保留原数据。随后用独立输出读取，核对能力和设备标识。
   回滚期间的延迟停止会使本轮等待为零，下一轮完成停止后可恢复完整等待。

33 路由缓存与文件回收（瞬时）。每种配置转发8192字节真实RPC并完成响应。默认4096字节和零保留时暂存容量应降为零，-1时仍能容纳原信封。随后建立期限不同的三个文件流，按非创建顺序取消。下一次可等待时间随最早在册期限变化。每一条重复取消均只向服务发出一次终止通知，服务看到自己的流号。已退役流的文件 DATA 和 ACK 返回 NI_OK；同一编号的其他类型 DATA 和 ACK 应返回 NI_NOT_SUPPORTED，从已关闭的端点再次提交得到 NI_NOT_FOUND。关闭后释放暂存容量。
   路由轮询后交付新的父请求和一个更短的内部查询，等待分别受各自期限约束；再交付较长的内部查询后仍等待较短期限。无目标与服务背压维持原预算，取消后下一轮重新计算。另验证外部首次关闭、重复关闭和交付回调延迟停止。Windows Debug 拒绝下一条请求的转发缓冲分配，检查该请求回报 NI_NO_MEMORY 并关闭来源，服务仅收到此前已成功交付请求的取消。
   将文件租约上限设为45秒，以到期秒 INT64_MAX 验证路由上限，以30秒授权验证服务先到期；重复 OPEN 将原10秒授权续长并保留相同公开流号。用应答前后的 UTC 与单调时钟样本限定客户端到期秒对应的毫秒区间，逐个取消流时核对宿主等待。静态 SDK 与 Wasm 另用固定的875和999毫秒样本检查两种上限、最后一毫秒、恰好到期、时钟无效和单调期限饱和。无效设置和重配物理池均保留上限，登记端点后设置应失败。
   续长同一流后再缩短至5秒，比较新建与缩短租约当轮实际交给宿主的等待时间。

34 发送端压缩配置（瞬时）。独立 TLS 对端解密后读取 NIF2 字节。4096 字节样本在门槛 4096 时压缩、4097 时原样发送。等级 1 和 9 的压缩字节与直接调用 Compress 的结果比对。验证等级 0、零门槛、启动前重配及无效等级，均检查失败后先前配置的实际效果。未列入白名单、文件控制、事件、无收益短正文和不可压缩正文应逐字节原样交付。协商紧凑格式后，独立 TLS 对端逐字节比对九帧 DATA。编号增加 1、3、127、128、UINT32_MAX 时核对相应的紧凑头。增加 UINT32_MAX+1、编号回退和重新协商时均应使用完整头。

35 直连生命周期、业务缓存与心跳（瞬时）。让总无输入和 PONG 期限分别先到，即便到期后的首次轮询恰有匹配 PONG，观察者也须收到唯一一次 NI_TIMEOUT 关闭。底层拒绝、异常或返回零连接号时，首次连接的代次应保持零，可以继续设置池、心跳、超时预算及正文缓存。每次失败只停止一次通道。重试接纳后代次应为1，后续失败应保留最近一次接纳的代次。显式 shutdown 后重新连接需要绑定通道。持续抛异常的 poll 应当场报告唯一关闭和在途请求终态，并保留先前主动关闭的原因。运输 close 抛异常后，下一次 poll 或 shutdown 须停止适配器一次并保留首次关闭原因；在帧回调中触发此故障时，须等运输交付返回后才停止。收尾前已绑定通道和各项初始配置仍受保护，收尾后可替换压缩白名单，首次连接限定的配置仍拒绝。真实 HELLO 应答后，等待宿主分别观察到默认30000及自定义11000毫秒空闲期限。依次接纳40、8192字节和零字节 RPC，另行解析并逐字节核对，末次收到空正文。默认预算只保留小缓冲，0在每次交付后释放，-1在会话内复用；关闭后容量均为零。非法参数保留前值，连接后重配返回 NI_INVALID_STATE。三个请求同时超时时分别检查自定义每轮一个回调及默认一次结算全部。PONG 发送背压或业务事件监听者拒收时，同一帧反复交付仍应按最后一次接纳的时间发起空闲 PING。另让传输实际接纳一次 PING 但不回应，分别核对总无输入和 PONG 两个期限先到的等待值。短等待须按协议使用的单调时钟核对已经经过的时间；系统 sleep 提前返回时补足剩余时间，再触发边界操作，不能把请求睡眠时长直接当作实际经过时长。
   已设置池时，非法地址指针、端口、监听者、TLS 时限或 HELLO 参数均应在通道接到配置前被拒绝。随机源失败也应保留零代次和零配置调用。改正输入后可继续使用同一会话。默认通道关闭紧凑帧返回 NI_OK，尝试启用返回 NI_NOT_SUPPORTED。在空闲 poll 后提交非法预留，等待预算仍沿用心跳；预留 25 毫秒请求后，本轮 wait 应立即采用其剩余期限。直接调用 NetSession::connectWithPools 并提供零控制轮转预算，预期同样拒绝且不进入通道。混合期限场景中从堆内取消两项，余下超时通知按原号104、101、103、105抵达，仍未到期的100可正常取消。
   直连适配器在 connect 中抛出普通异常或分配异常时，分别返回 NI_INVALID_STATE 或 NI_NO_MEMORY，未接纳连接没有关闭通知，原会话可重试。接纳后先请求以 NI_TIMEOUT 关闭、再请求取消时，运输只收到一次关闭请求。排队的就绪事件不得重新启动 HELLO，已送达 HELLO 后才开始关闭时，迟到的真实握手应答也不得让会话重新就绪，shutdown 在运输没有回报时仍应交付最初的 NI_TIMEOUT。
   已有待办请求时，运输在 poll 中不再持有资源且没有关闭回调，应在同一轮交付 NI_RPC_UNAVAILABLE 和 NI_TRUNCATED。本地主动取消则交付 NI_RPC_CANCELLED 和 NI_CANCELLED。再次推进和停止均不应重发终态。
   绑定通道后，空地址指针或空字符串应返回 NI_INVALID_ARGUMENT，随机源与池配置回调的调用数均为零。
   直连前设置数据记录池上限257，再将调用者原值改为513，外部通道每次均应读到257。依次模拟容量拒绝、分配异常和其他异常，核对错误码、零代次、没有关闭回调和原会话重试成功。配置回调内重绑、重配应被拒绝。默认配置不调用通道配置入口。握手期限1、99、60001及最大32位无符号数应在使用随机源前拒绝，0、100和60000应成功。底层会话、直连门面及36号服务连接的握手随机回调均应拒绝重新设置启动参数。初次连接及关闭后重试的随机回调均不属于关闭通知，closedRequestStatus 必须返回 false。

36 服务恢复与启动期限（瞬时）。实际分派器驱动连续两代握手前失败和退避期总期限到期。业务监听者须各收到当代的一次错误、正确代号及 NI_TIMEOUT，旧代、重复和未建立代次的通知均不能增加回调。提交填满地址字段且没有终止符的服务地址，应返回 NI_INVALID_ARGUMENT、代次仍为零，随后合法连接可恢复。在首次服务连接前设置请求池应成功，发现器启动且未给出候选时关闭会话，代次须保留，再次设置请求池应返回 NI_INVALID_STATE。发现器始终无候选时，宿主须分别得到约250、1400、700毫秒的预算，第三种由较短的总期限决定。另一组在首次建立运输后应立即继续推进，观察到 OPEN 后，在没有应答时等待默认30000或自定义2100毫秒。非法和迟到配置均须拒绝。读写轮次另验默认32/32、自定义1/3和4/2，检查帧观察者的实际调用数、发送工作步数、耗尽后的立即推进与空闲后的等待。连续拒绝两次物理连接并固定抖动样本时，初始基数32毫秒、最大40毫秒的两次等待应为16及20毫秒。时间预算以测试期间实际流逝的毫秒数校正。白盒部分调用通道实际使用的解密后帧交付策略，分别测试默认5000和自定义750毫秒。第一次拒收可发生在零时刻，重复拒收不能续期，截止时不再进入观察者，到期前接纳应解除期限。背压时限取 UINT32_MAX 且时钟距离 UINT64_MAX 仅剩三毫秒时，期限须饱和至 UINT64_MAX，恰好到期应返回 NI_TIMEOUT 并保持交付次数。

   配置两个去重后的数值入口及 4096 字节的缓存路径，改写调用方字符串，再提交非法种子、超长路径和含 NUL 的路径。发现器启动应仍收到原来的两个入口、原顺序和完整路径。清空配置后，启动应改用 connectService 的数值地址和端口、空路径。未配置种子时传入主机名或通配地址，预期返回 NI_INVALID_ARGUMENT，代次和发现器启动次数均不变。显式指定第二个 IPC 工厂后，推进会话应把本地地址和端口仅交给该工厂建立的运输。只有发现器而没有工厂时绑定失败。仅绑定工厂且配置了种子或缓存时，或解绑后不提供工厂时，连接应返回 NI_NOT_SUPPORTED。异步发现器停止前应拒绝修改种子及压缩设置，停止完成后可重配。
   发现器第一次返回 false 并清空输出，重试时仍应收到正确的 v1 结构大小和版本，随后选择合法候选并送出 OPEN。发现器在启动、推进、选择或记录失败时抛出普通异常或 std::bad_alloc，分别应报告 NI_INVALID_STATE 或 NI_NO_MEMORY。未接纳的启动可重试，已接纳的代次只交付一次关闭原因。发现器反复抛异常且异步停止未完成时，底层会话仍须推进并交付关闭原因，之后才能解除发现器借用。
37 隧道与借用输出（瞬时）。使用手写 NTW3 记录，分别让长度与正文、元数据和目标重叠。两个编码入口都应返回 NI_INVALID_ARGUMENT 并保留原字节，长度紧邻目标时应逐字节匹配手写记录。consumed 与输入重叠时不得消费，随后在同一解码器上完整重试。相交的读取输出应保留调用前的值，改用独立输出仍可读取并消费同一帧。

38 TLS 队列调度（瞬时）。为三对真实 TLS 会话分别使用默认设置、较小的轮转与推进预算、不限控制帧正文长度。两类队列持续有数据时按独立的编号序列逐帧核对解密结果，包括事件的全部正文。验证默认连续优先32个控制帧、可改为2个、0和-1两种控制正文门槛、推进1轮及最多交付2帧的边界。无效配置须保留原配置。预算让出后宿主收到零等待，全部排空后可重新等待。
   把待发密文交给真实 TLS 帧连接后，先独立断言本次 feed 使等待变为零，再消费发送端输出、推进接收端并校验解密帧。空输入保持正等待。首个 EOF 提示零等待，重复 EOF 维持正等待；随后验证截断及持续错误。
   业务会话在本轮通道推进返回后才把 PING 交给已握手的 TLS 连接，密文尚未生成时，宿主必须收到零等待。随后由真实对端解密并回送对应编号的 PONG；会话连续两次恢复空闲心跳计时。另逐个提交编号 9、10、14 的短 DATA，验证紧凑完整头和两种增量头每次成功均给出零等待，接收端按序读到全部正文。无效短 DATA 和控制记录池已满时的 PING 均保留原等待预算。

39 分派预算（瞬时）。同代一个请求已导入协议、另一个仍在提交队列，远端以 0 或 NI_CANCELLED 关闭时两者均应报告 NI_RPC_UNAVAILABLE。本地主动以这两个原因关闭时两者均为 NI_RPC_CANCELLED，本地或远端错误关闭均为 NI_RPC_UNAVAILABLE。分别通过运输、会话、提交端口和分派器关闭，核对两个编号各自唯一终态、唯一关闭通知和最终收敛。同一客户端跨多代重连；远端关闭交付第一个请求终态时，回调再请求本地正常关闭，不能改写其他请求的远端关闭终态，也不能覆盖原始关闭原因。心跳发送返回 NI_CANCELLED 导致协议层主动关闭时，已经发送和仍待发送的请求也必须一致结算为 NI_RPC_CANCELLED，不得继续向已关闭协议导入请求。只预留不发送的请求，宿主等待不得超过其剩余期限，届满后应报告一次 NI_RPC_TIMEOUT 并释放本地编号。未登记业务监听者、尚未接纳连接、代次不匹配或同代重复 started 或就绪回调重入时，均返回 NI_INVALID_STATE，提交端口原代次及就绪状态保持不变。若宿主仍交付就绪，分派器记录 NI_INVALID_STATE，提交端口保持未就绪。随后登记监听者，可正常接纳新连接并交付数据。逐条提交5条双字节 DATA，在运输回调核对每条的编号、正文和先后顺序。命令／字节预算分别为1／100、10／3、10／1时，第一轮各须交付1、2、1条。空队列恢复5000毫秒可等待状态，非法和启动后重配分别验证。

40 同时到期（压力）。父请求带两个内部子请求，子请求期限夹到父期限。每轮预算为 1，三次推进的在册数应为 2、1、0，客户端与服务端均按原请求号接收唯一的超时响应。另对尚未到期的父子请求主动取消父请求，接纳取消后两条记录均应已清理。让三条真实文件流和三个已路由请求同时到期，每轮最多清理一个文件、两个请求。原生构建等待真实单调期限，Wasm 测试通过链接器为本线程的 CLOCK_MONOTONIC 加上等待量，文件授权的 UTC 仍取真实值。三轮后两端分别收到正确流号的三条通知，在册请求数依次为1、0、0。另让两个端点同时关闭，回收预算为一步，每轮只归还一个可供新服务使用的物理槽。
   端点关闭预算耗尽后等待为零，最后一个端点回收后恢复完整等待预算。

41 服务事件信封（瞬时）。用固定大端目标号和四字节二进制正文手写 NVE2 样本。查询长度、短缓冲和精确容量分别判定，核对每个输出字节和借用正文起点。长度落在正文或目标内应拒绝并保持二者，紧邻目标的长度可用。另验定向空事件、16字节撤销组、错误的撤销类型、全零组和定向目标为零。解析输出覆盖输入时保留原内容。

另有带隔离身份的功能入口 --link-timers <identity.dat>。用真实节点和客户端完成管理TLS及角色认证。给业务输出配置2个记录槽和1个小正文槽。小正文池和记录池分别耗尽时须拒绝，归还后按原顺序再次交付，错误重配保留原容量，启动后不能重配。从同一模拟时刻推进：空闲500毫秒后剩14500毫秒，PING发出500毫秒后剩9500毫秒。对端不回PONG，在满10000毫秒时超时。另在服务端保留一条业务记录后结束输入，排空1000毫秒后剩4000毫秒，满5000毫秒超时。另建一对独立实例，把单轮内部控制预算设为1，在此设置下再次通过认证。将真实 PING 和后续业务事件的密文合并交付，默认预算须在一次推进后能读到事件，预算为1须在第二次推进后才能读到相同内容。再把空闲、PONG、排空期限改为9000、7000、3000毫秒，分别核对剩余预算与精确到期时刻。样本由本机隔离测试网关生成，使用NGI3叶身份文件；测试宿主须能读取指定路径。认证后向接收方分别交付 BYTES 和 CLOSE，接纳当轮必须零等待，同一接收槽占用期间再次投递必须保持正等待，取出的记录类型、通道和正文与原输入一致。

42 宿主随机输出（瞬时）。零字节与非法参数均不触发回调；8192 字节逐字节校验，8193 字节拒绝且保留原值。缺少回调、只写一字节后返回错误、只写一字节后抛出异常，均须返回 NI_INVALID_STATE、擦除整个请求区间并保留首尾保护字节。有回调时恰好调用一次。
43 密码派生（瞬时）。密码和盐各含 NUL，PBKDF2-HMAC-SHA256 迭代两次，与独立 .NET 实现生成的 32 字节向量比较。31 字节输出、零次及超过上限的迭代须返回对应错误，原输出与保护字节均保持。

44 执行宿主异常（瞬时）。create 交付实例后抛异常或返回错误码，均应拒绝绑定且只回收一次，成员 bind 返回 false 并清空原宿主输出，不调用宿主切换。分别使进入、退出、begin、wait、wake 和 destroy 抛出，核对错误状态、回调次数与 current() 的归属。
45 文件协议门面（瞬时）。用手写 NFA2 样本分别核对收发：32 位负值 -0x01020304 在线上为 FE FD FC FC，64 位偏移 0x0102030405060708 为 01 至 08。缓冲区距整型对齐起点偏移一字节，前后保护字节保持不变。检查短元数据、字段、控制包查询与编码、1024 字节已持久前缀和剩余两字节读请求。输错偏移、传入空前缀或重复提交已哈希的前缀时应保留进度，按正确偏移补齐后仍可读取后续两字节。对端确认数据后，END 记录内的 SHA-256 必须与独立计算的 1026 字节文件摘要逐字节相同。输错产出长度应被拒绝，取消后快照保留唯一终态。
46 TLS 与连接句柄（瞬时）。两对独立 TLS 客户端和服务端实际握手，一对传完整 NIF2 帧，一对传原始明文。检查无效配置、未就绪发送、消费后清空、保护字节及输入结束。
   另以 1024 字节密文队列传递一条 16384 字节的原始记录，再接收认证关闭。分别实测 write、append、两种 appendExact、consumeOutput、read 和 close 的等待预算，核对拒收时旧记录首次变得可取、无进展背压、重复关闭和查询；对端按字节比较收到的完整明文。
   静默服务端分别使用原始 TLS 的 100 毫秒期限和帧连接的默认 10000 毫秒期限，首轮推进后才绑定事件执行者。核对每轮剩余预算、恰到期限返回超时、接近 UINT64_MAX 时按已过时间计算、时钟倒退报错，以及已握手或关闭后的完整等待预算。
47 解码句柄（瞬时）。对独立样本检查截断后的复位、字段数组的高低字顺序、两种视图的正文借用、消费及空句柄错误。
48 目录与证书（瞬时）。二进制目录查询包含长度不足、越界和合法索引。分别以写满且无终止符的目录名称和地址调用创建入口，预期返回 NI_INVALID_ARGUMENT 并保持空句柄，更正后可创建。另检查固定候选的版本、端口和地址终止符、规范地址键。证书申请的名称、私钥区和申请区两两重叠时，两个长度输出、输入和输出缓冲均须保持原值，也不能调用随机源。合法名称和熵源应产生含终止符的 PEM。独立解析私钥与 CSR，核对唯一的 CN 等于输入名称，并校验 CSR 公钥与本次私钥配对。缺少熵源时两个长度清零。同模块字符串接口覆盖名称与私钥、名称与 CSR、两个输出三种对象重合，均须拒绝并保留原值。
49 请求路由与通道（瞬时）。检查来源号与路由号的双向查询、错误产品和已完成请求，通用处理器重复注册，连接池非法配置、初始会话和默认运输接口的返回值。
50 并发提交（压力）。静态库占用唯一的请求索引保护记录，此时新索引查询应报 NI_LIMIT_EXCEEDED，执行线程仍应能导入已接纳的完整发送正文并识别原请求。反复启动代号 1 和 2，只对代号 1 报告握手完成。并发读者取得的快照中代号 2 应始终未就绪；单线程宿主逐个检查两次发布的状态。四个生产者各预留64个请求，核对256个不同编号和关闭后返回值及清零的输出。单线程 Wasm 按相同输入串行执行。多线程构建由四个执行线程同时开始。静态库另直接调用生产代码的竞争让步函数，默认 64 次为一周期。逐次检查第 1 至 63 次的计数、第 64 次归零及下一次重新计 1。
51 对象池与引用（瞬时）。两槽池满时创建第三个对象应被拒绝。分别将根引用复制或移动赋值为它持有的子引用，预期父对象恰好析构一次、子对象仍存活。逐次检查构造失败后槽复用、空引用、自赋值、裸所有权归还和最终析构次数。
52 单调时钟（瞬时）。连续256组先后采样平台参考、NetInterface::monotonicMilliseconds、NetSenderSubmission::now及平台参考；两次公开读数须落在前后参考之间，且不得倒退。VS2013及更早MSVC参考直接使用GetTickCount64，Wasm使用clock_gettime(CLOCK_MONOTONIC)并沿用测试已有的模拟推进，其他工具链参考其标准steady_clock。无需校准或修改系统时间，也不添加固定等待。v120误回退到墙钟时将不在GetTickCount64区间内。协议内部取时已统一；调用方显式传入now的接口仍要求同一单调时钟，不能混用UTC。
--link-timers 在已认证的管理链路上连续排队两个通道的业务记录，分别验收成功后 wait 为零、正文池或记录池拒收后仍可等待 60000 毫秒，并逐条核对交付顺序和正文。
静态库的 --link-timers 还使用同一向量存放挑战和签名。验证端以独立保存的原始挑战验签，改动一个字节后必须拒绝。
--link-timers 还检查服务角色和本地节点的必需回调。缺少服务证明回调或允许服务角色却未提供校验回调时，创建应返回 NI_INVALID_ARGUMENT。两端配置齐全时创建成功；服务宿主在实际握手中拒绝提供证明，服务侧应报告 NI_PROTOCOL_ERROR，仅调用一次证明回调，节点不得进入就绪或发出授权。
--link-timers 先用97个不含终止符的字节作节点名称，构造结果应为 NI_INVALID_ARGUMENT。声明长度非零而指针为空、或长度为4097字节时，创建须返回 NI_INVALID_ARGUMENT 并清空输出句柄。传入4096字节含 NUL 的声明后改写原缓冲，对端仍应逐字节读到最初的声明，反向无声明的长度和指针均为零。随后通过不透明句柄建立独立认证链路，核对对端名称、证书、完整高低位编号、两个方向的控制帧及隧道正文，最后验证 EOF 后的持续错误。

功能验证结果（2026-11-28）
六种配置均通过 --all。每组52个不同编号（48瞬时、4压力），共312次编号执行，通过312、失败0、取消0、未执行编号0、删除跳过0。各配置另用隔离NGI3身份运行 --link-timers，6次专项全部通过。DLL 检查导出接口，依赖未导出符号的白盒分支由静态库检查。50号在 Windows 使用实际生产者线程，单线程 Wasm 核对串行提交语义。
以下为功能批次耗时，不是接口性能数据，--all 六组合计31.313秒。路径相对 src/temp/NetPeer-audit-1402/，结果归位后删除临时产物。
Windows Win32 Debug 静态库，52/52通过，9.303秒。debug/sdk/lib/NetInterfaceTestd.exe SHA256 E7FBBEFC069DC9B88862014C832B50BC4A2BE73B295D16F8ADB1051E6D5E5631。
Windows Win32 Debug 共享 DLL，52/52通过，7.666秒。debugdll/sdk/lib/NetInterfaceTestd.exe SHA256 434CECBA56D9164B477483E28F19A7609157DF80CEBFDEB1778EBCC0A66642E7。
    NetInterfaced.dll SHA256 B1DCF50DE458F3C8D90339043291FDC518DDF025FF5186C4E28B4DBABF412830。
Windows x64 Release 静态库，52/52通过，7.309秒。release/sdk/lib/NetInterfaceTest.exe SHA256 8762F40BB8CFC22942EC5E2FA03A65A76B68D6798FEF5996E84D967EF9601C9E。
Windows x64 Release 共享 DLL，52/52通过，5.948秒。releasedll/sdk/lib/NetInterfaceTest.exe SHA256 D5F142F3AEFE41D7D82A56A2D3B388586E8E0D09F32053A341F7166D086819A2。
    NetInterface.dll SHA256 B8FF2C0BE3B492C2E2F15AC15BDE3EB258FF6E44D4F55830213494BF784B6C03。
Node Wasm32 Release，52/52通过，0.491秒。wasm32/ni-test/NetInterfaceTest.wasm SHA256 233AC36E112B0ED27E9B6B7D8A8337D2B5C6BD69CB1B0CF69161262DF60F300B。
    NetInterfaceTest.js SHA256 B5F0FB1FA7B08FF3D796A40F2876DD0AA0175E978B7B2C910AA2FAA6B43458C3。
Node Wasm64 Release，52/52通过，0.596秒。wasm64/ni-test/NetInterfaceTest.wasm SHA256 6475E8F91671A3AB6CFE7E147E6BEF5778CE174F93BEF051849CC655C24D9987。
    NetInterfaceTest.js SHA256 DD0A55208911A05339443E8325EF4F835CF7ACDF470D06D1A69C20346CFF0B14。
52号以真实平台参考夹逼公开时钟，35号短等待以实际协议时钟达到期限为前提；不修改系统时间。Wasm 测试中的 clock_gettime 包装仅用于受控推进协议时间，Windows 使用真实时钟。当前批次未运行未定义行为检测，也未重复逐编号独立进程矩阵，不能沿用其他批次的结论。
Linux、Linux ARM、macOS、Android、iOS 仅核对源码及平台选择，缺少实机运行结果。Wasm运行于Node，浏览器、微信和大端ARM实机未测。本轮未运行性能基准。

性能入口与数据状态
--benchmark 单独运行，--all 不启动计时。interface.encode1024 只测一个固定 NIF2 帧，正文1024字节，
复用1056字节输出。计时前独立比较全部头和正文。计时内逐次调用编码、检查交付长度、轮转读取正文形成校验值。每批结束按 256 字节周期的独立预期核对总和。PlatformTest 预热并校准批量后采集 9 个样本，报告中位数及上下四分位数；跨进程波动本轮未测。
固定编码会检查长度参数重叠。当前版本缺少性能测试数据，本次仅执行功能回归。紧凑编码、RPC、NVE2 事件、NOI2 本地服务信封、压缩等级和门槛、TLS、文件和路由等场景也缺少可比的专项耗时。随机回调、密码派生和不同大小级正文池缺少当前版本的独立计时数据。配置、创建、销毁和状态查询按功能验收，无需单列吞吐。

函数与用例索引
下表按类列出 787 个非模板成员函数名及相应的用例入口。同名重载可对应多个编号，本次未采集分支覆盖率。
纯虚运输和发现回调没有基类函数体。32 号通过具体运输观察 listen 收到的默认 64 与显式 1，36 号通过宿主发现器的 choose 交付候选。每个已索引的函数列出至少一个实际进入它的用例。同名重载可能对应多个编号。输入、预期输出和状态变化见相应的编号说明。模板另按实际对象行为核对。25 号和 50 号分别覆盖 NetSenderSubmissionState::lookupIndex 的回执索引与请求索引实例，包括保护资源暂时不足时的返回和重试。14号通过五级缓冲分配、归还和池统计验证 NetBufferPoolBlock 的实际构造及 NetBufferPool::acquire。51号覆盖 NetObjectPoolItem 构造、NetObjectPool 的创建/引用计数/状态转换/统计，以及 NetObjectReference 的默认构造、复制、移动、两种赋值、解引用、布尔查询、获取与移交所有权、清空和析构。
NetBufferLease
  14 号　NetBufferLease、 adopt、 capacity、 data、 reset、 swap、 ~NetBufferLease
NetBufferPool
  14 号　NetBufferPool、 allocate、 release、 statistics、 status、 ~NetBufferPool
NetBufferPoolConfig
  14 号　NetBufferPoolConfig、 status、 ~NetBufferPoolConfig
  26 号　NetBufferPoolConfig
NetClientChannel
  32 号　NetClientChannel、 ~NetClientChannel
  35 号　configureCompact
  49 号　configureCompression、 configureConnectionPools、 outputState
NetClientDiscovery
  19 号　parseSeeds
  36 号　NetClientDiscovery、 ~NetClientDiscovery
  48 号　addressKey、 validPoint
NetClientSession
  26 号　NetClientSession、 close、 configureCompression、 configureConnectionPools、 configureGateways
      configureRequestPool、 connect、 discoveryStopped、 executing、 quiescent、 releaseAttempt、 shutdown
      stopDiscovery、 ~NetClientSession
  28 号　configureServicePollBudgets、 configureServiceTlsQueueLimits、 configureServiceWindow、 failAttempt
  35 号　beginRequest、 beginRequestAt、 bindChannel、 cancel、 configureExpiryBudget、 configureHeartbeat
      configurePayloadRetention、 onSessionReady、 onSessionResult、 payloadCapacity、 ready、 sendRequest
  36 号　attempt、 bindGateway、 configureRecovery、 configureServiceTimeouts、 connectService、 expireRecovery
      onSessionClosed、 poll、 reserveGeneration
  39 号　onSessionFrame、 outputState、 requestCloseStatus、 sendData
  49 号　beginRequestAt、 boundGatewayFactory、 detachBindingsForDestruction、 generation、 hasBoundChannel
      serviceMode
NetClientSessionState
  26 号　NetClientSessionState、 active、 ~NetClientSessionState
  28 号　acceptsProduct
  36 号　expired、 recovering
NetConnectionPoolConfig
  12 号　NetConnectionPoolConfig、 validCodecSettings、 ~NetConnectionPoolConfig
  26 号　NetConnectionPoolConfig
NetConnectionQueue
  12 号　NetConnectionQueue、 clear、 complete、 status、 ~NetConnectionQueue
  13 号　active、 allocate、 append、 bufferClass
  34 号　empty
NetConnectionWrite
  13 号　NetConnectionWrite、 ~NetConnectionWrite
NetEndpointFileStream
  33 号　leaseDeadline
  40 号　NetEndpointFileStream、 ~NetEndpointFileStream
NetEndpointPeer
  32 号　NetEndpointPeer、 ~NetEndpointPeer
NetEndpointPools
  32 号　NetEndpointPools、 ~NetEndpointPools
NetEndpointRouter
  33 号　configureFileLeaseLimit
  26 号　addPort、 addService
  32 号　NetEndpointRouter、 advance、 bindTransport、 closeEndpoint、 closePeer、 collectFiles
      configureClientHandshake、 configurePools、 deliver、 listen、 onClosed、 onFrame、 onReady、 pendingCount
      poll、 shutdown、 ~NetEndpointRouter
  33 号　routeFile
  40 号　addClient、 bindFileReply、 dispatch、 dropFile、 origin、 payloadCapacity、 receiveClient、 receiveService
      notifyTermination、 reject、 routeEvent、 routeFrame、 terminate
  49 号　addHandler、 localPort
NetEndpointRouterState
  32 号　NetEndpointRouterState、 canConfigure、 clearPayload、 enqueueClosing、 now、 takeClosing、 ~NetEndpointRouterState
  33 号　exchangeFile
  40 号　attachFile、 renewFile、 riseFile、 sinkFile、 takeFile
NetEndpointServiceSink
  32 号　NetEndpointServiceSink、 ~NetEndpointServiceSink
NetExecution
  31 号　NetExecution、 begin、 bind、 configure、 current、 deadline、 progress、 status、 wait、 wake、 ~NetExecution
  44 号　exchange
NetExecutionScope
  31 号　NetExecutionScope、 ~NetExecutionScope
NetFileProtocol
  20 号　fileFlowAccept、 fileFlowCommitted、 fileFlowConsume、 fileFlowCreate、 fileFlowPeek
      fileFlowSetWriteBatchChunks、 fileFlowSnapshot、 fileFlowTakeCommit、 fileFlowWrite、 fileFlowWritten
  45 号　fileControlEncode、 fileFlowCancel、 fileFlowCreateWire、 fileFlowDestroy、 fileFlowPrime
      fileFlowProduced、 fileFlowProgress、 fileFlowRead、 fileFlowSnapshotWire、 fileInfoFields、 fileParseInfo
NetGatewayAddress
  19 号　NetGatewayAddress、 ~NetGatewayAddress
NetGatewayClientChannel
  28 号　onLinkClosed、 onLinkControl、 onLinkReady、 onLinkRecord、 outputState、 sendCredit
  36 号　NetGatewayClientChannel、 close、 configureServicePollBudgets、 configureServiceTimeouts
      configureServiceWindow、 configureTlsQueueLimits、 connect、 finish、 onLinkReady、 poll、 sendOpen
      shutdown、 ~NetGatewayClientChannel
  49 号　configureCompact、 configureCompression、 configureConnectionPools、 outputState、 send
NetGatewayClientChannelState
  28 号　acceptClose、 acceptCredit、 acceptRecord、 advanceInput、 endInput、 fail、 sendOutput
  36 号　NetGatewayClientChannelState、 fail、 ~NetGatewayClientChannelState
NetGatewayClientTiming
  26 号　NetGatewayClientTiming
  36 号　configure、 configurePollBudgets、 deliver
NetGatewayTransport
  36 号　NetGatewayTransport、 ~NetGatewayTransport
  49 号　outputState
NetGatewayTransportFactory
  36 号　NetGatewayTransportFactory、 ~NetGatewayTransportFactory
NetHeartbeatConfig
  32 号　NetHeartbeatConfig、 expired、 valid
NetInterface
  1 号　encode、 encodeFields、 payloadLimit、 status、 writeNumber
  2 号　encodeCompact
  3 号　hashCreate、 hashDestroy、 hashFinish、 hashUpdate、 sha256
  4 号　messageClear、 messageCreate、 messageDestroy、 messageEncode、 messageGetCode、 messageParse
      messageSetCode
  5 号　NetInterface、 clear、 consume、 feed、 finish、 peek、 prepareHeader、 readNumber、 ~NetInterface
  6 号　encode、 encodeFields
  7 号　allowCompact、 reset
  8 号　rpcControl、 rpcEncode、 rpcParse
  9 号　creditConsume、 creditGrant、 creditReceived
  10 号　allowCompression、 fail、 peek
  11 号　handshakeAccept、 handshakeConsumeOutput、 handshakeCreate、 handshakeDestroy、 handshakePeekOutput
      handshakePeerInfo、 handshakeProgress
  12 号　connectionCreate、 connectionCreateServiceClient、 connectionDestroy、 connectionPeekOutput、 connectionProgress
      trustAuthority
  15 号　directoryCache、 directoryCopyNodes、 directoryCreateCache、 directoryCreateNodes、 directoryDestroy
  16 号　directoryCount
  17 号　requestsAddEndpoint、 requestsBegin、 requestsCancel、 requestsComplete、 requestsCount、 requestsCreate
      requestsDestroy、 requestsRemoveEndpoint、 requestsTake
  20 号　fileFlowAccept、 fileFlowCommitted、 fileFlowConsume、 fileFlowPeek、 fileFlowSetWriteBatchChunks
      fileFlowSnapshot、 fileFlowTakeCommit、 fileFlowWrite、 fileFlowWritten
  22 号　messageGetValue、 messageSetValue
  23 号　creditConfigure、 creditConfigureMessage、 creditCreate、 creditDestroy、 creditGet、 creditSent
  24 号　linkCreate、 linkDestroy、 requestsCreate
  27 号　connectionConfigureCompression、 connectionFeed、 tlsCreate、 tlsCreateClient、 tlsDestroy、 tlsFeed、 tlsRead、 tlsWrite
  29 号　configureDecoderRetention、 decoderBufferCapacity
  30 号　messageConfigureLimits
  37 号　connectionPeekOutput、 linkPeerInfo、 tunnelConsume、 tunnelCreate、 tunnelDestroy、 tunnelEncode
      tunnelFeed、 tunnelFinish、 tunnelPeek
  42 号　entropyBytes、 wipe
  43 号　pbkdf2Sha256
  45 号　fileControlEncode、 fileFlowCancel、 fileFlowCreate、 fileFlowCreateWire、 fileFlowDestroy、 fileFlowPrime
      fileFlowProduced、 fileFlowProgress、 fileFlowRead、 fileFlowSnapshotWire、 fileInfoFields、 fileParseInfo
  46 号　connectionConsume、 connectionConsumeOutput、 connectionCreate、 connectionFinishInput
      connectionGetStatus、 connectionPeek、 connectionSend、 tlsClose、 tlsConsumeOutput、 tlsCreate
      tlsCreateClient、 tlsFinishInput、 tlsGetStatus、 tlsPeekOutput、 tlsProgress
  47 号　consume、 create、 destroy、 feed、 finish、 peek、 peekFields、 reset
  48 号　certificateCreateRequest、 directoryCreate、 directoryNode
  52 号　monotonicMilliseconds
  认证时限专项　linkAccept、 linkConsumeControl、 linkConsumeOutput、 linkConsumeRecord、 linkCreate、 linkFinishInput、 linkPeerInfo
      linkPeekControl、 linkPeekOutput、 linkPeekRecord、 linkProgress、 linkSendControl、 linkSendRecord
      linkStatus
NetInterfaceCertificate
  48 号　createRequest、 validName
  认证时限专项　signProof、 verify、 verifyPair、 verifyProof
NetInterfaceCompact
  2 号　encode、 write
  7 号　encode、 header、 read
NetInterfaceCompression
  10 号　decode
  13 号　supports
  34 号　encode
NetInterfaceConnection
  12 号　NetInterfaceConnection、 clearWrites、 detail、 peekOutput、 progress、 status、 wipe
      ~NetInterfaceConnection
  13 号　NetInterfaceConnection、 completeActive、 configurePools、 consume、 consumeOutput、 decodePending
      decoderBufferCapacity、 peek、 send
  27 号　configureCompression、 feed
  34 号　configureCompact
  38 号　decodeBuffered、 feed、 finishInput、 send
  46 号　finishInput、 peek
  认证时限专项　fail、 outputState
NetInterfaceCredit
  9 号　consume、 grant、 received
  23 号　NetInterfaceCredit、 available、 configure、 pending、 sent、 status、 window
  36 号　capacity
NetInterfaceDirectory
  15 号　encode、 encodeCache、 number、 parse、 parseCache、 text
  48 号　address、 valid
NetInterfaceDirectoryAddress
  19 号　parse
NetInterfaceDirectoryPoint
  15 号　NetInterfaceDirectoryPoint、 ~NetInterfaceDirectoryPoint
NetInterfaceEvent
  41 号　encode、 parse
NetInterfaceFile
  21 号　encodeAck、 encodeData、 encodeInfo、 equal、 parseAck、 parseData、 read32、 read64、 validInfo、 write32
      write64、 zero
  40 号　encodeNotice、 parseNotice
  45 号　encodeControl、 isControl、 parseControl、 parseInfo
NetInterfaceFileFlow
  20 号　setWriteBatchChunks、 writeRequest、 written
  21 号　NetInterfaceFileFlow、 accept、 acceptAck、 acceptData、 committed、 consumeOutput、 fail、 peekOutput
      snapshot、 takeCommit、 ~NetInterfaceFileFlow
  45 号　cancel、 prime、 produced、 progress、 readRequest
NetInterfaceFileFlowOpaque
  45 号　NetInterfaceFileFlowOpaque、 ~NetInterfaceFileFlowOpaque
NetInterfaceFileFlowState
  21 号　NetInterfaceFileFlowState、 clearData、 clearOutput、 prepareOutput、 status、 ~NetInterfaceFileFlowState
NetInterfaceFileGuard
  2 号　overlapsFrameLength
  5 号　prepareCount
  7 号　copyFrameBody
  21 号　NetInterfaceFileGuard、 ~NetInterfaceFileGuard
  41 号　overlap、 overlapsFrame
NetInterfaceHandshake
  11 号　NetInterfaceHandshake、 accept、 consumeOutput、 fail、 makeOutput、 peekOutput、 peerInfo、 progress
      validLimits
NetInterfaceLink
  24 号　NetInterfaceLink、 status、 ~NetInterfaceLink
  28 号　accept
  27 号　consumeOutput、 outputState、 peekOutput、 progress
  认证时限专项　NetInterfaceLink、 accept、 configureOutputPools、 consumeControl、 consumeRecord、 drainInput
      finishInput、 outputState、 peekControl、 peekRecord、 peerInfo、 sendControl、 sendRecord、 serviceGrant
NetInterfaceLinkPools
  24 号　NetInterfaceLinkPools、 ~NetInterfaceLinkPools
NetInterfaceLinkState
  24 号　NetInterfaceLinkState、 configure、 fail、 status、 ~NetInterfaceLinkState
  27 号　progress
  认证时限专项　consumeQueuedRecord、 handshakeFrame、 isHeartbeat、 validControl、 prove、 queueRecord、 verifyAcceptance、 verifyProof
NetInterfaceLinkWrite
  认证时限专项　NetInterfaceLinkWrite、 ~NetInterfaceLinkWrite
NetInterfaceMessage
  4 号　NetInterfaceMessage、 canCharge、 clear、 ~NetInterfaceMessage
  15 号　decodeInt32、 decodeUint64、 encodeInteger
  28 号　decodeInt32、 decodeUint64、 encodeInteger
NetInterfaceRequestChildren
  49 号　NetInterfaceRequestChildren
NetInterfaceRequestEndpoint
  18 号　NetInterfaceRequestEndpoint、 ~NetInterfaceRequestEndpoint
NetInterfaceRequestEntry
  18 号　NetInterfaceRequestEntry
NetInterfaceRequests
  17 号　complete、 take
  18 号　NetInterfaceRequests、 addEndpoint、 advance、 begin、 beginNext、 cancel、 count、 removeEndpoint
      setEndpointLimit、 status、 takeExpired、 ~NetInterfaceRequests
  24 号　NetInterfaceRequests
  32 号　takeEndpoint
  40 号　takeChild
  49 号　NetInterfaceRequests、 completeOriginal、 parentContext
NetInterfaceRequestsState
  18 号　NetInterfaceRequestsState、 earlier、 insert、 rise、 sink、 take、 ~NetInterfaceRequestsState
NetInterfaceRpc
  8 号　control、 encode、 parse
NetInterfaceService
  8 号　authenticate、 hello、 unwrap、 wrap
NetInterfaceTls
  12 号　NetInterfaceTls、 detail、 peekOutput、 progress、 status、 ~NetInterfaceTls
  28 号　feed、 finishInput
  13 号　consumeOutput
  27 号　NetInterfaceTls、 append、 feed、 read、 write
  34 号　appendExact
  46 号　close、 finishInput
  认证时限专项　outputState
NetInterfaceTlsOptions
  24 号　effectiveHandshakeTimeout
  27 号　setHandshakeTimeout
NetInterfaceTunnel
  37 号　NetInterfaceTunnel、 consume、 encode、 feed、 finish、 peek、 ~NetInterfaceTunnel
NetLinkTransportListener
  36 号　NetLinkTransportListener、 ~NetLinkTransportListener
NetRequestTablePools
  18 号　NetRequestTablePools、 ~NetRequestTablePools
NetSenderDispatch
  39 号　NetSenderDispatch、 advanceOutput、 bindSubmission、 configurePollBudgets、 error、 execute、 executing
      fail、 listener、 onSessionClosed、 onSessionFrame、 onSessionReady、 onSessionResult、 poll、 prepare
      quiescent、 report、 sendData、 sendRequest、 shutdown、 started、 submission、 waitSynchronous、 wireId
      ~NetSenderDispatch
NetSenderDispatchMap
  39 号　NetSenderDispatchMap、 clear、 erase、 insert、 local、 size、 wire、 ~NetSenderDispatchMap
NetSenderSubmission
  25 号　advanceSynchronous、 beginPoll、 complete、 configureGate、 configurePools、 configureProducerWait、 producerWait、 data、 dataSynchronous
      disconnected、 drainData、 finish、 next、 nextData、 observeSynchronous、 queuedBytes、 sendSynchronous
      submitData、 submitRequest、 synchronousResult、 takeClosed
  39 号　advanceSynchronous、 cancel、 closeRequested、 send、 state、 synchronousPending
  50 号　NetSenderSubmission、 begin、 close、 idle、 now、 ready、 start、 ~NetSenderSubmission
NetSenderSubmissionAccess
  50 号　NetSenderSubmissionAccess、 accepted、 ~NetSenderSubmissionAccess
NetSenderSubmissionEntry
  50 号　NetSenderSubmissionEntry、 clear、 ~NetSenderSubmissionEntry
NetSenderSubmissionFront
  50 号　NetSenderSubmissionFront
NetSenderSubmissionInput
  50 号　NetSenderSubmissionInput、 ~NetSenderSubmissionInput
NetSenderSubmissionItem
  25 号　NetSenderSubmissionItem
  50 号　NetSenderSubmissionItem、 operator=、 ~NetSenderSubmissionItem
NetSenderSubmissionOptions
  50 号　NetSenderSubmissionOptions、 ~NetSenderSubmissionOptions
NetSenderSubmissionPayload
  25 号　NetSenderSubmissionPayload、 ~NetSenderSubmissionPayload
NetSenderSubmissionPools
  50 号　NetSenderSubmissionPools、 ~NetSenderSubmissionPools
NetSenderSubmissionReceipt
  25 号　NetSenderSubmissionReceipt、 ~NetSenderSubmissionReceipt
NetSenderSubmissionSignal
  25 号　NetSenderSubmissionSignal、 ~NetSenderSubmissionSignal
NetSenderSubmissionWait
  25 号　NetSenderSubmissionWait
NetSenderSubmissionState
  25 号　addReceipt、 append、 drainDisconnected、 expireFrames、 find、 lookupReceipt、 reserveReceipt、 rollbackReceipt、 settle
  50 号　NetSenderSubmissionState、 clear、 dropDirect、 dropFrames、 importOwned、 indexResult、 lookupFront、 materializeDirect
      nextId、 pauseContention、 pendingReceipts、 pump、 queueResult、 quiesce、 remove、 requestClose、 retire
      take、 wake、 ~NetSenderSubmissionState
NetServerChannel
  32 号　NetServerChannel、 ~NetServerChannel
NetSession
  26 号　NetSession、 bindChannel、 configureCompression、 configureRequestPool、 generation、 shutdown
      ~NetSession
  35 号　beginRequest、 cancel、 configureExpiryBudget、 configureHeartbeat、 configurePayloadRetention、 connectWithPools、 stopChannel
      notifyReady、 notifyResult、 onFrame、 onReady、 payloadCapacity、 reserveRequest、 sendCancel、 sendFrame
      sendRequest
  36 号　close、 connect、 notifyClosed、 onClosed、 poll、 progressHandshake、 ready
  39 号　closedRequestStatus、 notifyFrame、 outputState、 sendData
  49 号　beginRequestAt
NetSessionCallbackScope
  35 号　NetSessionCallbackScope、 ~NetSessionCallbackScope
NetSessionListener
  26 号　NetSessionListener、 ~NetSessionListener
NetSessionState
  26 号　NetSessionState、 clearPayload、 resetRequests、 ~NetSessionState
  35 号　earlier、 exchange、 expired、 prepareHandshake、 reserve、 rise、 sink、 take
NetTransportListener
  32 号　NetTransportListener、 ~NetTransportListener
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
    // 服务目标与业务产品筛选、普通身份打开内部目标的拒绝，以及连接配置传递
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
    CasePasswordDerivation = 43,
    // 执行宿主六个回调异常与嵌套归属恢复。
    CaseExecutionExceptions = 44,
    // 文件协议门面核对线格式、恢复前缀、宿主读长度和取消终态
    CaseFileProtocolFacade = 45,
    // 通过不透明句柄完成双向 TLS 握手、输入输出交接和普通帧解码
    CaseTlsHandles = 46,
    // 核对解码句柄的完整视图、短输入、消费、复位和终态
    CaseDecoderHandleFields = 47,
    // 校验目录编码句柄、候选规范化、精确缓冲容量及证书申请的输出边界
    CaseDirectoryHandles = 48,
    // 核对请求双向索引、端点注册及通道初始化前的边界
    CaseRoutingBoundaries = 49,
    // 核对多生产者同时预留的请求号、唯一性和关闭后的输出
    CaseSubmissionProducers = 50,
    // 检查池槽实际归还和父子引用间的复制、转交。
    CaseObjectReferences = 51,
    // 比对平台单调参考与公开计时，连续调用不得倒退
    CaseMonotonicClock = 52
};

// 借用指针和字段共享同一起点，用于检查输出区域相交时的返回值。
struct TestBorrowedView
{
    // 当前借用的正文地址
    const uint8_t* m_pointer;
    // 提供完整字段数组的空间，并保留用于核对的预填值
    uint32_t m_padding[6];
};

// 身份输入覆盖句柄存储，尾部提供足够的身份数据。
struct TestHandshakeStorage
{
    // 与身份输入共用起始地址的输出句柄
    NetInterfaceHandshakeHandle m_handle;
    // 两个身份字段的额外存储
    uint8_t m_suffix[32];
};

// 在同一地址分别提供正确对齐的 TLS 句柄和诊断输出。
union TestTlsOutputs
{
    // 初始为空的活动成员
    NetInterfaceTlsHandle m_handle;
    // 只取地址，检测与句柄重叠的诊断输出
    int32_t m_detail;
};

// 记录同一同步提交收到的通知次数。
class TestSubmissionObserver : public NetSenderSubmissionSignal
{
public:
    /** 以零次通知开始
    */
    TestSubmissionObserver();

    /** 记录一次已送达的通知
    */
    void notify() override;

public:
    // 已收到的通知数
    unsigned m_count;
};

// 检查直连绑定，并在测试对象析构时结束对外部宿主的借用。
class TestAttachedSession : public NetClientSession
{
public:
    using NetClientSession::hasBoundChannel;
    using NetClientSession::boundGatewayFactory;

    /** 在派生对象销毁前解除绑定
    */
    ~TestAttachedSession();
};

// 记录实际析构次数；父对象可以持有同一池中的另一个对象。
struct TestReferenceNode
{
public:
    // 外部维护的析构计数。
    unsigned& m_destroyed;
    // 持有子节点所有权。
    NetObjectReference<TestReferenceNode> m_child;

public:
    /** 建立计数节点；按输入模拟对象构造失败
    @param [in,out] destroyed 外部计数，须覆盖节点的生存期
    @param [in] reject 为 true 时构造函数抛出异常
    */
    TestReferenceNode(unsigned& destroyed, bool reject);

    /** 对已构造的节点记录一次析构
    */
    ~TestReferenceNode();
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
    /** 建立空的观测记录和默认测试行为
    */
    PoolTestSink();

    /** 没有业务发送能力，意外交付时拒绝
    @return NI_NOT_SUPPORTED
    @param [in] endpoint 逻辑端点编号，本用例同步读取
    @param [in] frame 本次调用借用的帧和正文，不转移所有权
    @param [in] origin 本次调用借用的请求来源
    */
    int32_t send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& origin) override;

    /** 记录关闭，在配置了路由器时从关闭回调中请求整体停止
    @param [in] endpoint 本次关闭的逻辑端点
    */
    void close(uint64_t endpoint) override;

public:
    // 当前用例借用的路由器，普通池测试留空。
    NetEndpointRouter* m_router;
    // 关闭通知总次数。
    uint32_t m_closed;
    // 最后收到关闭通知的端点编号。
    uint64_t m_lastClosed;
};

/** 接纳真实路由帧，记录两端分别看得见的请求号、流号及终止通知
*/
class RouterFileSink : public NetEndpointServiceSink
{
public:
    /** 建立空的投递记录
    */
    RouterFileSink();

    /** 从收到的已编码应答中取回实际公开流号
    @param [in] endpoint 逻辑端点编号，本用例同步读取
    @param [in] frame 本次调用借用的帧和正文，不转移所有权
    @param [in] origin 本次调用借用的请求来源
    @return 接纳为 NI_OK，格式错误为 NI_PROTOCOL_ERROR，请求按用例返回预设结果
    */
    int32_t send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& origin) override;

    /** 路由器拥有端点生命期
    @param [in] endpoint 逻辑端点编号，本用例同步读取
    */
    void close(uint64_t endpoint) override;

public:
    // 最近实际交付的请求号
    uint64_t m_request;
    // 模拟本次请求的宿主交付结果。
    int32_t m_requestResult;
    // 服务实际收到的取消数和最近一条取消的路由号。
    uint32_t m_cancels;
    // 服务最近收到的取消路由号。
    uint64_t m_lastCancel;
    // 需要从请求交付回调发起停止的路由器。
    NetEndpointRouter* m_stopOnRequest;
    // 客户端在授权应答中读到的最近流号
    uint64_t m_stream;
    // 客户端实际收到的最迟授权 UTC 秒。
    uint64_t m_expiresUtc;
    // 普通 RPC 超时响应中实际收到的原请求号。
    std::vector<uint64_t> m_timeoutIds;
    // 收到并核对的定向业务事件数。
    unsigned m_events;
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

    /** 此运输只提供监听，拒绝主动连接
    @param [in] address 调用方提供的地址字符串，仅在本次调用读取
    @param [in] port 调用方提供的端口
    @param [in] options 本次调用借用的连接选项
    @param [in] listener 接纳成功后借用至 shutdown 的运输观察者
    @param [out] peer 成功时写入模拟连接标识，失败时为零
    @return 始终返回 NI_NOT_SUPPORTED
    */
    int32_t connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options, NetTransportListener* listener, uint64_t& peer) override;

    /** 保存监听者，回调交给后续poll
    @param [in] address 调用方提供的地址字符串，仅在本次调用读取
    @param [in] port 调用方提供的端口
    @param [in] options 本次调用借用的连接选项
    @param [in] listener 接纳成功后借用至 shutdown 的运输观察者
    @param [in] peers 监听接纳的连接数预算
    @return NI_OK，或用例指定的 NI_NO_MEMORY
    */
    int32_t listen(const char* address, uint32_t port, const NetInterfaceTlsOptions& options, NetTransportListener* listener, uint32_t peers) override;

    /** 模拟TLS就绪及随后的独立HELLO到达
    @return 仍需推进时为 true，未监听或协议检查失败时为 false
    */
    bool poll() override;

    /** 把路由器交付的应答送到客户端握手状态机
    @param [in] peer 连接标识
    @param [in] frame 本次调用借用的帧和正文，不转移所有权
    @return 握手或心跳的处理结果；测试也可模拟背压、错误和异常
    */
    int32_t send(uint64_t peer, const NetInterfaceFrame& frame) override;

    /** 保存关闭原因以便区分正常握手与提前关闭
    @param [in] peer 连接标识
    @param [in] reason 当前连接的关闭原因
    */
    void close(uint64_t peer, int32_t reason) override;

    /** 结束监听者借用
    */
    void shutdown() override;

    /** 返回本用例的监听端口
    @return 正在监听为 8081，否则为零
    */
    uint32_t localPort() const override;

    /** 返回当前物理连接数
    @return 已交付就绪且仍监听为 1，否则为零
    */
    uint32_t peerCount() const override;

private:
    // 用例所有的独立客户端
    NetInterfaceHandshake& m_client;
    // 监听开始到停止期间借用的路由入口
    NetTransportListener* m_listener;
    // 已交付的运输事件数
    unsigned m_events;
public:
    // 监听及失败回滚期间检查路由器对重入的处理。
    NetEndpointRouter* m_router;
    // 监听与失败回滚时重绑或改参数的操作均须被拒绝。
    bool m_reentryRejected;
    // 模拟部分启动后分配失败。
    bool m_failListen;
    // 在监听失败的回滚调用中请求路由器整体停止。
    bool m_stopDuringRollback;
    // 运输接到的轮询次数，监听成功前应始终为零。
    uint32_t m_pollCalls;
    // PING 交付故障，1 返回断线，2 抛普通异常，3 抛分配异常。
    unsigned m_pingFailure;
    // 最近一次运输实际接纳的出站PING编号，用于构造对应应答。
    uint64_t m_pingId;
    // 下轮交付匹配的PONG。
    bool m_pongDue;
    // 推迟交付HELLO以区分应用握手与就绪后心跳的期限。
    bool m_deferHello;
    // 模拟运输暂时不能接纳已经生成的 HELLO_REPLY。
    bool m_deferReply;
    // 传输接到的启动次数。
    uint32_t m_listenCalls;
    // 最近一次交给运输的预留连接数。
    uint32_t m_listenPeers;
    // 传输回收部分启动资源的次数。
    uint32_t m_stops;
    // 安排下一次推进交付一条正常入站 PING，核对其 PONG 及空闲时钟。
    bool m_activityInput;
    // 用例安排在下一次推进中发生的宿主关闭。
    bool m_endInput;

public:
    // 客户端接收应答的结果或提前关闭原因
    int32_t m_result;
};

class CiphertextProbeHost;

/** 模拟管理节点的 OPEN 响应，观测业务 TLS 首段输出，并可交付一次固定密文前缀
*/
class CiphertextProbeTransport : public NetGatewayTransport
{
public:
    /** 将首段长度交给拥有工厂的用例
    @param [in,out] host 保存 OPEN 应答窗口及观测值，寿命覆盖运输
    */
    explicit CiphertextProbeTransport(CiphertextProbeHost& host);

    /** 保存回调，下一次 poll 才报告管理节点就绪
    @param [in] address 调用方提供的地址字符串，仅在本次调用读取
    @param [in] port 调用方提供的端口
    @param [in] options 本次调用借用的连接选项
    @param [in] listener 接纳成功后借用至 shutdown 的运输观察者
    @param [out] peer 成功时写入模拟连接标识，失败时为零
    @return NI_OK、NI_INVALID_ARGUMENT 或 NI_PROTOCOL_ERROR；测试指定时抛异常
    */
    int32_t connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener,
        uint64_t& peer) override;

    /** 交付已接纳的管理通知，保持 sendControl 与回调分离
    @return 运输仍需推进时为 true
    */
    bool poll() override;

    /** 为产品7的业务通道生成有效的管理 OPEN 回应
    @param [in] peer 连接标识
    @param [in] frame 本次调用借用的帧和正文，不转移所有权
    @return NI_OK、背压、格式错误或分配失败
    */
    int32_t sendControl(uint64_t peer, const NetInterfaceFrame& frame) override;

    /** 读取真实 TLS ClientHello 的类型和长度，按用例设置模拟接纳或背压
    @param [in] peer 连接标识
    @param [in] record 本次调用借用的隧道记录及密文
    @return NI_OK、NI_LIMIT_EXCEEDED 或 NI_PROTOCOL_ERROR
    */
    int32_t sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record) override;

    /** 该运输同步接纳全部已成功提交的输出
    @param [in] peer 本次管理连接号
    @return 当前连接排空时为 NI_OK
    */
    int32_t outputState(uint64_t peer) const override;

    /** 在拥有者线程上同步投递下一个管理回调，可在通道的两次 poll 之间调用
    @return 管理运输是否仍有资源
    */
    bool dispatchNext();

    /** 在已认证通道上投递指定的 CREDIT 事件
    @param [in] amount 对端归还的密文字节数
    @return 是否成功构造并交付事件
    */
    bool dispatchCredit(int32_t amount);

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
    @return 所有权交给会话的新运输；指定失败时为空，分配失败可抛异常
    */
    NetGatewayTransport* create() override;

    /** 按用例检查就绪回调内的重复登记
    @param [in] generation 本次回调对应的会话代次
    @param [in] maxRequests 协商后的对端请求数量能力
    @param [in] maxChunk 协商后的单块字节上限
    */
    void onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk) override;

    /** 记录业务终态的原号、状态、单字节应答和交付顺序
    @param [in] generation 本次回调对应的会话代次
    @param [in] requestId 请求方最初提交的完整编号
    @param [in] product 请求所属产品
    @param [in] type 请求业务类型
    @param [in] status 请求的唯一终态码
    @param [in] data 回调期间借用的结果正文，size 为零时可为空
    @param [in] size 结果正文的字节数
    */
    void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status,
        const uint8_t* data, uint32_t size) override;

    /** 核对业务帧内容并模拟背压或观察者异常
    @param [in] generation 本次回调对应的会话代次
    @param [in] frame 本次调用借用的帧和正文，不转移所有权
    @return 正常返回 false 形成背压；测试指定时抛异常
    */
    bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame) override;

    /** 记录会话关闭原因、代次和已交付结果数量
    @param [in] generation 本次回调对应的会话代次
    @param [in] reason 当前连接的关闭原因
    @param [in] detail 底层诊断值，本用例不解释
    */
    void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail) override;

public:
    // 在 OPEN 成功且看到业务密文后交付一次的通道关闭事件正文。
    std::string m_closeEvent;
    // 检查该会话实际传给管理运输的握手毫秒数。
    uint32_t m_expectedHandshakeMs;
    // 在实际运输回调处观测到的首份密文长度
    uint32_t m_first;
    // 是否在初始握手输出交付后只提供一次 TLS 记录头前缀
    bool m_offerPrefix;
    // 运输接纳连接时记录的地址和端口，用于区分显式工厂与默认工厂。
    std::string m_connectedAddress;
    // 最近一次接纳连接的端口，未接纳时为零。
    uint32_t m_connectedPort;
    // 提供一个 258 字节的未完 TLS 记录，让 CREDIT 金额跨过单字节范围。
    bool m_widePrefix;
    // 运输是否已把该前缀交给被测通道
    bool m_prefixSent;
    // 被测通道实际归还的前缀密文字节数
    uint32_t m_prefixCredit;
    // CREDIT 被拒收时保持相同金额，统计每一次实际提交。
    bool m_refuseCredit;
    // CREDIT 实际提交次数。
    uint32_t m_creditAttempts;
    // 第一笔 CREDIT 背压时送入记录头的第五字节，形成独立的待返金额。
    bool m_offerCreditTail;
    // 第五个 TLS 头字节是否已经投递。
    bool m_creditTailSent;
    // 送出前缀的同一轮结束运输，1 先发送 CHANNEL_CLOSE，2 先报告错误，3 静默结束。
    unsigned m_endAfterPrefix;
    // 回调改由同一执行者显式投递；运输自身的 poll 只查询活动状态。
    bool m_manualCallbacks;
    // 额度用例中，已建立的服务端 TLS 实际接收并解密通道发出的记录。
    NetInterfaceTls* m_cipherReceiver;
    // 服务端 TLS 实际收到的密文字节总数。
    uint32_t m_cipherReceived;
    // 收到的有效 OPEN 申请窗口
    uint32_t m_offered;
    // 依据手写 UINT64 字节样本识别的 OPEN 目标，供调用方独立核对。
    uint32_t m_openTarget;
    // 模拟节点选定的窗口，0表示省略可选字段
    uint32_t m_reply;
    // 是否在每次连接时报告协议失败以触发真实恢复流程
    bool m_refuseConnect;
    // 运输场景依次模拟空工厂、远端关闭、意外记录、静默结束、带原因结束、推进异常和分配失败；8、9 分别在连接时抛普通异常及分配异常。
    unsigned m_fault;
    // 验证业务帧的异常回调及交付次数。
    bool m_throwFrame;
    // 业务帧回调实际进入次数。
    unsigned m_frames;
    // 最近一次业务终态的请求号
    uint64_t m_resultId;
    // 最近一次业务终态的状态码
    uint32_t m_resultStatus;
    // 第一次结果的状态，用于同一关闭事件中不同排队位置的比较。
    uint32_t m_firstResultStatus;
    // 第一次结果对应的原始业务编号。
    uint64_t m_firstResultId;
    // 可选借用会话，验证结果回调中的关闭请求不能改写已经确定的终态。
    NetClientSession* m_closeOnResult;
    // 可选借用分派器，在就绪回调中检查重复登记被拒绝。
    NetSenderDispatch* m_startOnReady;
    // 就绪回调内重复登记的返回值。
    int32_t m_startResult;
    // 单字节应答的值，其他长度记为256。
    uint32_t m_resultByte;
    // 业务终态交付总数
    uint32_t m_results;
    // 最近一次关闭通知到达时，已经交付的请求结果数。
    uint32_t m_resultsAtClose;
    // 已收到的终态通知数
    uint32_t m_closes;
    // 已实际交付的请求超时通知数
    uint32_t m_timeouts;
    // 超时回调报告的原号，保留抵达顺序以检查相同截止时间的排序。
    std::vector<uint64_t> m_timeoutIds;
    // 最近一次关闭回调报告的会话代次，用于检查重试与迟到通知。
    uint64_t m_closedGeneration;
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

    /** 记录由会话复制并提交给直连通道的配置，模拟一次宿主失败
    @param [in] config 此次创建物理连接应采用的参数
    @return NI_OK；测试指定时返回背压或抛出异常
    */
    int32_t configureConnectionPools(const NetConnectionPoolConfig& config) override;

public:
    // 当前配置的会话，回调时尝试重绑以验证配置借用期。
    NetClientSession* m_configuringSession;
    // 所有配置回调都应在受保护的会话内执行。
    bool m_reentryRejected;
    // 配置回调进入次数，用于核对重试及默认配置路径
    uint32_t m_poolCalls;
    // 从外部会话收到的物理池上限
    int64_t m_recordLimit;
    // 连接接纳阶段模拟宿主异常，NI_OK 允许正常接纳
    int32_t m_connectException;
    // 每次推进都产生指定的宿主异常，直到测试复位。
    int32_t m_pollException;
    // 模拟运输关闭入口与最终停止入口异常。
    bool m_throwClose;
    // 完成监听解绑后模拟停止异常。
    bool m_throwStop;
    // 正在交付观察者的借用帧。
    bool m_deliveringFrame;
    // 记录是否在帧回调尚未返回时停止了运输。
    bool m_stoppedInFrame;
    // 下层实际收到的停止次数。
    uint32_t m_stops;
    // 由测试指定的本次配置结果
    int32_t m_poolResult;

public:
    /** 启动唯一连接，保存监听者直到shutdown
    @param [in] address 会话请求的地址
    @param [in] port 会话请求的端口
    @param [in] options 连接配置
    @param [in] listener 本连接的回调
    @param [out] peer 正常接纳时为7；模拟无效标识时为0，背压或异常时保留原值
    @return NI_OK 或模拟背压 NI_LIMIT_EXCEEDED；测试指定时抛普通异常或分配异常
    */
    int32_t connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
        NetTransportListener* listener, uint64_t& peer) override;

    /** 在poll中交付就绪和独立生成的握手应答
    @return 运输仍需推进时为 true；测试指定时可抛异常
    */
    bool poll() override;

    /** 在调用期复制RPC正文或推进对端握手
    @param [in] peer 连接标识
    @param [in] frame 本次调用借用的帧和正文，不转移所有权
    @return NI_OK 或测试指定的背压、关闭及协议处理结果
    */
    int32_t send(uint64_t peer, const NetInterfaceFrame& frame) override;

    /** 记录关闭意图，终态由会话shutdown收敛
    @param [in] peer 连接标识
    @param [in] reason 当前连接的关闭原因
    */
    void close(uint64_t peer, int32_t reason) override;

    /** 结束监听借用
    */
    void shutdown() override;

    /** 已接纳的输出当场交给测试接收端
    @param [in] peer 连接标识
    @return 有效连接为NI_OK
    */
    int32_t outputState(uint64_t peer) const override;

    /** 记录握手完成后交给通道的紧凑帧许可
    @param [in] peer 连接标识
    @param [in] enabled 本次协商是否允许紧凑帧
    @return 有效连接为 NI_OK，否则为 NI_INVALID_ARGUMENT
    */
    int32_t configureCompact(uint64_t peer, bool enabled) override;

    /** 记录握手完成后交给通道的压缩许可
    @param [in] peer 连接标识
    @param [in] types 调用期间借用并复制的类型数组，count 为零时可为空
    @param [in] count 类型数组元素数
    @param [in] negotiated 本次协商是否允许压缩
    @return 有效协商为 NI_OK，否则为 NI_INVALID_ARGUMENT；复制失败可抛异常
    */
    int32_t configureCompression(uint64_t peer, const uint32_t* types, uint32_t count, bool negotiated) override;

public:
    // HELLO 与应答完成后应用到通道的可选编码能力。
    uint32_t m_codecCapabilities;
    // 本次连接协商后通道收到的压缩消息类型，保留传入顺序。
    std::vector<uint32_t> m_codecTypes;
    // 运输实际接纳的DATA编号，按交付顺序记录
    std::vector<uint64_t> m_dataIds;
    // 最近一次被通道接纳的线路请求号和取消号，以及各自的接纳次数。
    uint64_t m_lastRequest;
    // 最近接纳的取消线路编号。
    uint64_t m_lastCancel;
    // 运输接纳的请求总数。
    uint32_t m_acceptedRequests;
    // 运输接纳的取消总数。
    uint32_t m_acceptedCancels;
    // 最后一次接纳请求时，NRP2信封中的剩余毫秒数。
    uint32_t m_requestTimeout;
    // 模拟本轮运输无法接纳请求帧，保留此前的线路高水位。
    bool m_blockRequest;
    // 暂缓交付已就绪的 HELLO 应答，恢复后在下一次 poll 交付
    bool m_deferReply;
    // 模拟异步关闭，在下一轮仍可送达已排队的就绪事件
    bool m_deferClose;
    // 下一次 poll 模拟远端关闭，NI_READY 表示本轮没有关闭事件。
    int32_t m_disconnectReason;
    // 下层收到的关闭请求总数
    uint32_t m_closeCalls;
    // 下层收到的 HELLO 总数
    uint32_t m_hellos;
    // 运输实际接纳的心跳数
    uint32_t m_pings;
    // 未绑定真实TLS时的心跳发送返回值，用于验证下层错误的关闭路径。
    int32_t m_pingResult;
    // 心跳专项中借用已握手的 TLS 连接，按真实先推进运输、再由会话发送的顺序执行。
    NetInterfaceConnection* m_heartbeatTls;
    // 两端共用已完成 TLS 握手的单调时刻；应用心跳由 NetSession 自己计时。
    uint64_t m_heartbeatTlsTime;
    // 用例交由已绑定会话的下一条业务事件，测试期间借用。
    const NetInterfaceFrame* m_event;
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
    /** 建立空的观测记录和默认测试行为
    */
    FrameDeliveryProbe();

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

    /** 记录通道报告的关闭原因
    @param [in] peer 逻辑端点
    @param [in] reason 关闭原因
    @param [in] detail 底层诊断
    */
    void onClosed(uint64_t peer, int32_t reason, int32_t detail) override;

public:
    // 是否接纳当前帧
    bool m_accept;
    // 观察到的最后一次关闭原因，初始为成功。
    int32_t m_reason;
    // 每次交付均核对内容
    bool m_valid;
    // 回调累计次数
    uint32_t m_calls;
};

// 检查从真实 TLS 排空的连续两帧；拒收期间帧号和正文都必须保持原样。
class GatewayDrainListener : public FrameDeliveryProbe
{
public:
    /** 建立空的观测记录和默认测试行为
    */
    GatewayDrainListener();

    /** 核对当前待接纳帧的身份和正文，仅在成功时前移
    @param [in] peer 本次通道号
    @param [in] frame 解密后的业务帧
    @return 当前是否接纳
    */
    bool onFrame(uint64_t peer, const NetInterfaceFrame& frame) override;
public:
    // 已成功交付给应用的帧数
    uint32_t m_received;
};

// 发现器可在指定的一次宿主调用中模拟错误。
enum class DiscoveryFault
{
    // 正常完成各宿主调用
    None,
    // 开始发现时失败
    Start,
    // 推进现有发现任务时失败
    Poll,
    // 选择连接候选时失败
    Choose,
    // 接收候选连接失败通知时失败
    Failed
};

// 可延迟返回候选或在指定阶段抛出异常，供真实会话验证恢复与错误传递
class PendingDiscovery : public NetClientDiscovery
{
public:
    /** 建立空的观测记录和默认测试行为
    */
    PendingDiscovery();

    /** 记录启动配置并开始等待候选
    @param [in] seeds 调用期间借用的数值地址与端口，按原顺序记录
    @param [in] cachePath 宿主收到的缓存路径
    @param [in] tls 发现使用的 TLS 配置
    @return 成功接纳时返回 NI_OK，测试指定时抛出异常
    */
    int32_t start(const std::vector<NetGatewayAddress>& seeds, const std::string& cachePath, const NetInterfaceTlsOptions& tls) override;

    /** 保持等待，不产生网络事件
    @return 仍在运行时为 true，测试指定时可抛异常
    */
    bool poll() override;

    /** 立即结束该发现代次
    */
    void shutdown() override;

    /** 查询资源是否已收敛
    @return 发现任务已经停止时为 true
    */
    bool stopped() const override;

    /** 记录本次输入表头，按用例选择合法候选或清空输出并返回 false
    @param [in,out] endpoint 输入含版本表头的候选，输出合法地址和端口或默认空候选
    @return 交付合法候选时为 true，否则为 false；测试指定时可抛异常
    */
    bool choose(NetClientDiscoveryPoint& endpoint) override;

    /** 记录已选择候选的连接失败
    @param [in] endpoint 本次连接失败的候选，含版本表头、数值地址和端口，调用期间借用
    */
    void failed(const NetClientDiscoveryPoint& endpoint) override;

public:
    // 发现器每次收到 start 调用时递增。
    uint32_t m_startCalls;
    // 最近一次成功启动收到的有序入口列表。
    std::vector<NetGatewayAddress> m_startedSeeds;
    // 最近一次成功启动收到的完整缓存路径。
    std::string m_startedPath;
    // 指定宿主发生异常的入口
    DiscoveryFault m_fault;
    // 选择分配异常或普通异常
    bool m_allocationFailure;
    // 返回合法的 127.0.0.2:8001，否则清空输出并等待。
    bool m_provide;
    // 每次收到的输入表头均应符合 v1 契约。
    bool m_headersValid;
    // 模拟异步停止，宿主随后才允许释放发现资源。
    bool m_holdStop;
    // 已选候选的失败通知数。
    uint32_t m_failures;

public:
    /** 在选定入口模拟底层分配失败或其他运行错误
    @param [in] operation 当前宿主入口
    */
    void checkFault(DiscoveryFault operation) const;

public:
    // choose的实际调用次数
    uint32_t m_attempts;
    // 是否仍在等待
    bool m_running;
};
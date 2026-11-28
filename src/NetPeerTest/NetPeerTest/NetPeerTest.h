#pragma once
#include "NetPeer/NetPeerAPI.h"
// 同一 C++ 翻译单元还要调用 C 接口。独立 C 翻译单元通过统一入口选择这些声明。
#include "NetPeer/NetSenderC.h"
#include <functional>
#include <memory>

/** NetPeerTest 功能测试

运行方法
使用当前 SDK 构建测试工程。原生 version_debug.sh/version_release.sh 在全部依赖构建成功后，单独复制同版本 NetPeerClientState.h 供白盒只读观察；不更换 SDK 的公开头和 NetPeerMacro.h。手工构建时也须先完成依赖发布，再执行这一准备。Wasm 使用其 SDK 发布事务的头文件，现行发布分支不执行原生 after。
无参数或 --all 执行全部用例，--instant 执行瞬时用例，
--stress 执行压力用例，--cases 1,9 只执行指定编号。任一检查失败返回1，原生选择器参数错误返回2，Wasm 的 LibraryTestHost 对非法编号退出1。
CTest 可选注册：同时提供 NET_PEER_TEST_SERVICE_CERTIFICATE 和 NET_PEER_TEST_SERVICE_KEY 注册 NetPeerServiceOptions；提供 NET_PEER_TEST_DISCOVERY_IDENTITY 注册 NetPeerDiscoveryTimers。桌面传测试进程可读路径，Android/iOS 传设备内已部署的路径，Wasm 需 LIBRARY_TEST_HOST=ON 和 Node 模拟器并传 Node 可读路径；素材仅在运行时读取。原生生成器 NetPeerServiceFixture 需要两个尚未存在的不同输出路径。可选 CTest NetPeerServiceFixtureOutputs 检查路径别名、已有文件、第二项失败回滚、成对创建成功和重复运行。
JS 宿主用带非零偏移的单字节视图核对 C 入口收到的长度与正文；即使对象自行定义 length，借用边界仍由原始视图确定。JS 宿主专项核对直连地址 4096 字节、服务地址 39 字节、直连信任根及 DNS 名的上下界，覆盖多字节地址、末尾根点和 getter 中分离的输入视图，超界时应在首次模块堆分配前返回参数错误。在直连和服务入口分别传入显式无效的 PID 或网络选项，预期返回 -1、最近诊断为 -1、代次保持 0，原生拨号和字节宿主拨号均为零次。再通过首选字段和兼容字段各启动一个真实服务连接，记录首选与兼容 getter 的调用次数，核对原生入口调用一次且字节宿主收到正确地址和端口。JS 宿主专项同时核对连接字段和监听方法的单次读取、读取时的生命周期保护、各级分配失败后的返回码与清理、文件写预算快照、微信回调注册与解绑中断，以及迟到的写入拒绝保留先前的关闭原因。
Wasm 正式入口由 LibraryTestHost 加载匹配位宽的模块。测试使用进程内字节宿主和内存文件后端。
静态 NetPeer 测试直接验证兼容种子解析接口；动态 NetPeer 测试使用固定布局的候选数组，在测试模块内构造和释放种子容器。服务控制回复和发现回复写入调用方持有的字节区。
DLL 测试宿主与 NetPeer 必须共享同一 NetInterface 动态模块；两个独立静态副本各自拥有执行者和宿主登记状态，不能将一份副本的 NetExecutionScope 用来观察另一份副本的期限。
4号为压力用例，其他27项为瞬时用例。每项均可单独选择。

Wasm 异步宿主专项还调用 NetSender 的 JS 门面。未准备的客户端配置网关应覆盖之前的参数诊断并返回 -7。压缩类型数组的 getter 抛错时应返回 -1，随后有效配置成功，每项只读取一次。宿主先报告 onError(-91) 再报告关闭时，C++ 监听者只收到一次关闭且 detail 为 -91。覆盖空闲查询、配置、提交及释放，分别传入只读输出和抛错的 requestId setter。回写失败时仅撤销本次预留，正常回写保留完整64位编号。微信适配器以受控宿主核对传递的地址、端口、写切片、回调注销及随机字节。统一入口轮询空句柄应返回 -1，轮询有效但未准备的客户端应返回 0。postData 和 sendData 各读取帧字段一次，getter 抛错返回 -1，并在读取期间拒绝释放客户端。另验证成功网关配置清除先前错误、失败配置记录错误，状态查询、正常 poll、closeGateway 与 uninitGateway 均保留最近诊断。

Wasm 请求正文专项使用已准备但尚未连接的客户端。宿主使用本上下文及另一个 Node vm 上下文的 ArrayBuffer、Uint8Array、带偏移子视图和零字节正文，核对 postRequest 与 sendRequest 交给原生入口的实际字节。浏览器宿主可通过同源 iframe 执行相同用例。两个入口都须在调用原生之前拒绝已分离的缓冲及其视图、其他视图类型和伪造类型标签，并记录 NI_INVALID_ARGUMENT。

微信专项在 getRandomValues 返回后触发 success，分别检查错误类型、已分离缓冲、随机长度不符和属性读取异常。错误消息触发一次 onError(-7)，不会交付数据；有效随机、TCP 收发均只取真实子视图。真实随机池收到长度数值时须拒绝补充，清零本次读取的输出，下一批有效字节可恢复使用。

Wasm 文件流专项在接收端提交并交付最终 ACK 后、下轮 poll 前安排取消，检查两端成功、文件只关闭一次且无额外取消帧。另由真实监听表依次通知就绪、交付失败 ACK 和同会话的另一帧；逐项核对对端错误 -83、-3、-2、-8 只结束本条流。相同错误字段搭配错误令牌时仍触发协议错误。

Wasm 文件输出专项使用同模块的发送和接收文件流。持久化后重复一条 DATA，令其普通 ACK 遇背压，直至接收端提交才允许发送。双方应各成功一次，并交付且仅交付一条最终 ACK。在接收端已提交、最终 ACK 被宿主拒收期间，经同一客户端的 JS 回调入口重投发送端原始 END，再派发另一流的帧。监听器须消费迟到的 END，正确分派另一帧，且不发起连接关闭。恢复可写后两端均成功，文件只提交一次，最终 ACK 只交付一次。另在最终 peek 注入一次或持续内存不足，分别检查重试成功、按配置超时与主动取消。最终 ACK 持续遇网络背压时须超时，网络永久拒绝时须保留宿主原因，各场景均核对文件只提交和关闭一次。

Wasm 回调整数专项通过测试工程导出的 C++ 辅助入口，调用 Sender 实际登记的 ready/result/frame/closed 回调表。
字节宿主专项将宿主编号设到 0x7ffffff0 附近，经原生回调表验证两个连接独立收发、恢复写入及关闭释放，覆盖编号超过当前线性内存长度的情形。
分别传递 1、有符号最大值、仅最高位为1和无符号最大值，逐项核对 uint32 产品/类型以及 uint64 代次/请求号，
JS 必须收到非负 Number/BigInt；关闭原因 -7、详情 -83 仍为负数。C++ 回调后清零原正文，JS 保存的应答和帧正文仍为 0、83、255，且均不借用 Wasm 堆缓冲。此为 ABI 边界测试，未模拟网络握手。

用例与独立预期
12 号在已就绪的独立客户端上观察提交后的等待时间。预留请求、post 请求和 DATA、带交付票据的请求和 DATA、首次取消及首次关闭均须令当前 wait 的上限变为零。测试端核对两个请求和两条 DATA 均实际交付，两张票据均成功。重复提交、重复取消和重复关闭不得再次触发立即推进，首次关闭原因保留。
12 号在空闲 poll 后启动无磁盘缓存的发现、直连和中继，等待宿主没有外部唤醒。三项成功启动后 wait 的上限应立即由 60000 毫秒变为 0；没有连接事件时，下一轮直连和中继分别登记 4096、4095 毫秒的剩余启动期限。原始宿主同步回报主动关闭，或重复报告已建立的连接时，wait 也应立即返回。下一轮只交付一次对应的取消或协议错误，收敛后恢复 60000 毫秒等待。
12 号的原生文件流场景在同一执行者检查首次启动、接纳输入、消费输出、取消及新 I/O 提交后的立即推进。READ 提交槽被占用、READ 结果尚未发布以及取消后的 CLOSE 在途时，宿主 wait 的上限均为 100 毫秒；收敛后恢复调用方的 60000 毫秒上限。
附加入口 --service-options <certificate.pem> <private-key.pem> 读取隔离 service.7 叶身份，完整完成 NetPeer 服务 TLS 和业务 HELLO。初始化读帧/写记录预算分别设为 (1,1) 和 (3,3)：服务端预先交付七条事件，按每次真实服务运输 poll 的边界计数，接收峰值必须分别为 1 和 3，全部编号及三字节正文顺序相同。客户端另提交四条 16384 字节 DATA，服务端逐字节解密核对；每轮密文记录峰值必须分别为 1 和 3，任何一轮不得超限。再将应用拒收期限设为 80 或 60000 毫秒；对同一条真实事件持续拒收，仅短期限实例应在首次拒收所在公开 poll 开始满 80 毫秒后回报一次 NI_TIMEOUT（单调毫秒取整允许 2 毫秒误差），且不得交付该帧。长期限实例观察至少 120 毫秒后恢复接收，须只交付一次原文且保持就绪。两份 PEM 分别最多 16384 字节，退出前擦除读取缓冲；Wasm 使用 NetPeerServiceHostTest.js <module.js> <certificate.pem> <private-key.pem> 在运行时临时装入 /service.crt 与 /service.key，退出时擦除并删除内存文件；构建时不嵌入叶私钥。浏览器或设备宿主可调用该脚本导出的 runServiceTest 并提供相同的叶素材字节。全部判定成功时输出 NetPeerServiceOptions passed。
原生测试可用 NetServerManager 的固定 service.7 素材生成器。配置 CMake 时传入 -DNET_PEER_SERVICE_FIXTURE=ON，再执行 cmake --build <build-dir> --target NetPeerServiceFixture。运行 NetPeerServiceFixture <certificate.pem> <private-key.pem>，两个输出路径都应位于隔离测试目录。服务端编译签发工具，测试客户端与 Wasm 从运行时文件加载这份叶身份。直连 TLS 用例另用 Mbed TLS 的公开测试证书。CMake 和 Visual Studio 工程会把同版本素材头准备到当前测试构建的 test-sdk/include/NetGatewayTest 中。
12 号还使用执行宿主记录 wait 的实际毫秒数。上限填 60000，分别验证直连与中继启动、中继记录拒收、发现重试、目录缓存合并写以及原生文件流无进度期限，并核对静止时没有变成零等待。中继步数设为1，一次给出两条完整记录，在预算耗尽后的各轮应立即推进、按序交付并恢复读取；空闲运输 poll 后在同一轮首次接纳发送时，执行者 wait 应取得 0 毫秒，后续仍有排队记录时也应立即继续，宿主写入背压时等待真实可写通知。目录缓存另验证空闲 poll 后同轮 queue 的两种重载均立即推进，4093毫秒合并期限不因覆盖快照而重置；0间隔时在途正文保持独立，下一次发布最后接纳的快照。提交背压、结果未到达和停止未收敛时交给宿主最多100毫秒补查，重复 shutdown 不产生连续零等待；释放失败后仍能重试，完全空闲及停止后恢复调用方60000毫秒上限，重新 open 再立即推进。
12 的初始化配置实效补充：经 NetPeer 的 m_configure 设置实际会话和派发器，随后用公开句柄连接、提交、推进和关闭；缓存容量仅在 Test 通过实际 SDK 句柄定义白盒只读观察，不保留配置回调引用。请求编码正文为独立构造的 8192 字节；保留量 0 与 -1 分别产生零容量和至少 8208 字节容量。到期预算 1 与 3 在一次公开 poll 内分别结算 2 与 6 个已到期请求，最终 8 个请求均只收到一次超时。心跳分别让 PONG 期限和总无输入期限先到，必须先观察到正确 PING，再仅收到一次 NI_TIMEOUT 关闭。入口固定 16 槽，配置每次领取的批数/项数 (1,1)、(1,4)、(2,4)，在通道持续背压时观察可重新接纳的槽数分别为 1、4、8；解除背压后逐字节核对全部长 DATA 的顺序与正文。服务连接以受控管理运输检查真实 OPEN 的 65536/131072 窗口以及真实 TLS ClientHello 的 31/67 字节输出，TLS 输入预算分别为 1/512 字节；连续交付 258 字节不完整记录和 1 字节后缀，仅小预算通道拒收第二条，返还信用分别为 258/259 字节。启动期限 80/60000 毫秒仅使短期限连接在观察窗口内超时。无发现候选时，80 毫秒恢复总期限应触发一次超时关闭。另将总期限设为 120000 毫秒，分别配置 20/60000 毫秒选择间隔；每次重置宿主等待记录，实际剩余预算不得大于对应间隔，剩余预算加最近 choose 后经过的单调时间须至少达到对应间隔（允许 2 毫秒取整误差），因此默认 250 毫秒不能冒充任一配置。短间隔还须实际选择至少三次，长期限观察 100 毫秒保持一次选择；连续拨号被拒绝时固定抖动为零，恢复初值 100 毫秒、上限 120/400 毫秒必须给宿主产生 50/60/60 与 50/100/200 毫秒的三轮退避预算，核对实际等待预算及已经经过的单调时间。不修改生产布局、不使用测试钩子。
1  生命周期。重复创建不能覆盖已有对象；准备前后均读出代次0及未就绪；未握手时预留请求失败并清零编号，销毁后指针置空。
2  客户端隔离。依次经业务入口设置连接、请求与提交池；零帧预算和最大槽数小于初值必须被拒绝，更正后同一对象恢复成功。NetPeer 核心及 NetSender::backend() 的完整提交配置分别拒绝每批 33 项和零交付期限，更正后同一入口成功。两个独立客户端配置压缩及派发；空表可以清除，非空计数配空表或命令0被拒绝，更正后可继续。无终止符的4097字节种子和缓存路径均被拒绝，恰好4096字节的路径及清空配置可用。Wasm 宿主另用多字节路径核对 UTF-8 上界，并观察越界参数不会分配对应的模块堆块。
3  发现及线程参数。执行者数量0、1、2分别返回参数错误、成功、不支持；回环种子去重，轮选两个端口，端口0被拒绝并清空结果。发现器启动时拒绝非法管理预算或 TLS 握手期限，更正后同一实例可用；0、100 和 60000 毫秒均应接纳。
4  压力。同一发现器启停4轮，每轮逐项核对512次轮选的端口和地址，两个端口各出现256次。停止后地址清空、端口为0。
5  准备事务。用初始化回调更改会话与派发参数，在回调内用两个准备重载及销毁入口操作同一客户端，分别预期 NI_INVALID_STATE。外层返回负值或 NI_READY 时均不得发布核心，正值转换为 NI_INVALID_STATE，更正后同一客户端可重新准备。成功后自定义准备不可覆盖，普通准备仍可重复。
6  直连TLS。使用公开连接接口和测试证书验证异步拨号与默认握手期限；真实TLS产生密文，每轮写预算11，前两轮宿主累计接纳11和22字节。
   原始连接及TLS连接分别从零增长一槽，停止时连接数归零。
7  管理链路。由服务运输工厂和发现器各实际发起一次拨号，按四个独立的增长观察者分别检查管理连接槽及其宿主连接槽。
8  宿主随机。检查零长度、非零长度空指针、有效字节、首尾保护字节，以及宿主部分写入后失败时请求范围被擦除。Wasm 宿主另测余量高于补充阈值却不足以满足请求、同时请求合并、随机源一次拒绝后恢复，以及补充前后字节顺序。
9  输入与公平调度。提交65字节后改写原缓冲，监听者仍收到全部65字节的原文；只增长连接池及选中的正文大小级。
   宿主报告结束时仍有数据、可写、关闭事件，业务再次关闭时宿主不再收到关闭调用，预算为1时分三次按顺序交付，最终NI关闭码和原始宿主诊断码分别正确。Wasm另验JS事件预算的初始化和非法值，输入池满后拒绝，
   输出池独立增长，输入缓冲归还、再次借出时内容已清零。随机余量等于阈值时不补充，低于阈值时发起一次补充。
10 文件接收。文件后端对cancel、consume返回未约定的正数时，门面须返回并记录NI_INVALID_STATE。后端查询 done 期间请求停止时，停止通知在查询返回后送达一次。文件流在后端查询抛异常、查询期间发起停止或停止通知仍待重试时必须拒绝启动，后端重新初始化后同一文件流可启动。模拟后端在 stop 后继续工作，显式 uninit 保留资源；作用域退出时须由后端析构回收，停止只通知一次。停止期间仍可消费原有结果，重复 stop 和消费动作均不能再次通知后端。首次停止抛异常时应保留错误并允许重试；重试成功后再次停止无副作用。两个65536字节分块形成一批，独立后端校验每次写的偏移与长度，并比较完整131072字节正文；
   第二次持久化前不得出现ACK，全部完成后才有ACK。校验批量范围、窗口上限、启动后禁止修改、进度时限和关闭收敛。Wasm另用同一份160字节元数据创建JS文件对象，校验非零 uint32 产品号的边界、批量、流号、超时范围、定时器请求的17毫秒间隔及释放。
11 参数边界。在公开准备入口逐项把直连、服务、发现的正数预算改成0，均须返回参数错误；
   直连、服务及发现三条路径分别拒绝小于-1的保留量或控制正文上限、越界压缩等级和零编解码预算；-1、等级9及预算1合法。正文超协议上限被拒绝。恢复默认并将退避、抖动及缓存合并间隔清零后同一对象可准备成功。
12 公开句柄。用同一个配置创建两个客户端，分别经clientConnect和clientConnectService拨号；
   根据真实增长量识别直连、服务及发现池，核对互不挪用；两次关闭均可继续poll直至释放。另在初始化时绑定帧通道，双方完成业务握手。设置后立即重置调用方的配置对象；通道须仍接到解码保留量-1、压缩起点23、控制突发3、控制正文上限-1、帧预算13、步数19、压缩等级7及白名单17、257。提交入口仅有一槽，第二次预留必须返回背压且清零编号；推进后对端收到原始正文73，会话请求池才增长一槽。将提交队列初始容量设为 2、上限设为 -1，安装单次增加 4 槽的策略。重置调用方策略后连续提交 32 个单字节 DATA，仍须调用原增长策略；运行期间重配被拒绝。分别在初始化时设置每轮的命令数和字节数为 (1,1)、(8,3)、(2,32)。对端每轮新增帧数的峰值应分别为 1、3、2，任何一轮都不能超限，三组都按序收到全部 32 帧。前两组另分别配置 100 与 1000 毫秒交付期限，配置成功后把调用方参数改成 1 毫秒。在相同的 200 毫秒持续背压下，短期限票据须返回 NI_TIMEOUT 且不发送第 33 帧，长期限票据须仍为 NI_READY；解除背压后仅长期限实例交付完整第 33 帧并返回 NI_OK。终态再次查询为 NI_NOT_FOUND，原连接保持就绪。

13 双向直连。真实TLS客户端与服务端完成握手，服务端解密并检查客户端已修改过的提交缓冲原文；预设一个数据记录槽，首帧占用时第二次提交必须背压；
   拒绝无效的后续编解码预算后原来的一槽限制仍生效。直连正文上限设为64字节后，65字节提交须同步背压，随后的64字节帧须完整解密且逐字节相符。反向回送三帧，逐轮核对帧预算为1时的交付数、编号及正文顺序。随后双方启用紧凑编码，客户端须能接收服务端连续的两条 DATA；关闭紧凑编码后客户端发出的下一帧须能由仅接收普通帧的服务端解析。远端 EOF 到达时，监听者暂时拒收最后一帧。2 毫秒排空期限的实例应在保护期限内返回 TLS_TRUNCATED；60000 毫秒实例至少观察 5 毫秒后仍能接纳该帧，再对裸 EOF 回报截断。等待接收期间查询输出、提交新帧及更改紧凑编码或压缩配置均应返回 NI_INVALID_STATE。

14 中继与管理配置。分别观测控制及数据记录和正文的四个池，在每轮预算为1时两次写各送一条；步数、启动时限和背压时限逐项置零时，配置、拨号和监听均拒绝，原有效配置仍生效。管理连接池替换在实际拨号生效，启动后拒绝再次替换。另以两个实例交换 2 和 60000 毫秒的启动/背压期限。在等待建连及持续拒收一条有效中继记录两个场景中，仅对应短期限实例应回报一次 NI_TIMEOUT；长期限实例随后可完成建连或恢复并接纳原正文。另在已有待发记录且接收方暂停时关闭宿主，查询待发和再次发送应返回 NI_INVALID_STATE；恢复接收后仍交付原记录，并回报一次底层关闭错误。

15 候选退避。两个发现器使用相同种子，立即重试的实例不取随机数；设60000毫秒退避并启用抖动的实例取一次随机数，随后轮选排除此候选。另一个实例仍然可以选择该端口。另将候选退避设为零，在相同宿主错误后分别观察 2 和 60000 毫秒的重拨间隔；仅短间隔实例在观察窗口内重新分配管理连接。抖动随机源临时失败后，已知种子应在配置的有限退避后重新可选。另在发现运行期间注入错配的缓存完成编号及一次释放失败，后续 poll 应使缓存后端恰好释放一次，种子仍可选择。
16 原生发送端收到最终 ACK 后，宿主分别回报关闭成功和关闭失败，核对本地终态和协议快照，只接纳一个关闭任务。已发生的超时不被后续关闭错误覆盖。发送端收敛后重收原成功 ACK，应返回 NI_NOT_FOUND，快照仍保留协议完成状态和原本的本地关闭结果，关闭任务只提交一次。恢复文件的一字节前缀未读时，accept 暂时返回 -3。补齐前缀后重投同一合法失败 ACK，若对端原因也是 -3，首次接纳返回 NI_OK；关闭文件前后再投递均返回 NI_NOT_FOUND，snapshot 当即保留原始失败 -3。另在正文 READ 已接纳而结果尚未分派时交付该失败 ACK，并用另一任务令收尾 CLOSE 首次提交遇背压。READ 等待分派、CLOSE 提交背压和已接纳 CLOSE 等待分派时，poll 均返回 NI_NEED_MORE 且 done 为 false；完成 CLOSE 后 done 为 true，poll 返回原始失败 -3。另验 -83 仍直接返回，错令牌和越界偏移报告协议错误，各自只关闭文件一次。
17 目录缓存写入间隔。比较间隔0与 UINT32_MAX 毫秒，检查两次真实发布或仅首次发布；读写完成的文件号或偏移错配时返回协议错误并停止，不能触发发布。另查发布请求被接纳后回报失败、发布或关闭回报文件号错误，均不能计作成功完成。后端首次释放抛异常时缓存仍未停止，拒绝重开；再次推进释放成功后才能重开。完全停止后再调用 shutdown，stopped() 应保持为 true，并能立即 open。
18 目录缓存往返。新构造的实例先调用 shutdown 仍应处于停止状态，可直接首次 open。4059字节目标路径能发布，4060及4096字节路径仍能读；两种 queue 重载均须同步拒绝无法生成临时名称的写入，既有快照仍可读取。用内存文件后端以 65 个唯一节点写入，超出协议计数上界的请求须在转换前被拒绝，原快照仍可发布。重开后先查询容量，再逐项核对节点，一份快照只交付一次。
19 文件写完成长度。工厂首次分配失败时返回 NI_NO_MEMORY，门面释放后同一实例可重试。两个 65536 字节分块合并写，分别让第一段或第二段回报 65535 或 65537 字节成功。四种情况下本地结果均须为 NI_INVALID_STATE；对端收到失败 ACK、错误码相同、累计偏移为零。宿主随后只收到一次关闭，错误发生后没有新写入或发布。第10号覆盖长度相符的成功路径。
20 直连 TLS 队列。两个实例同时连接，各用一个推进步骤和 11 字节宿主写预算。输入/输出容量为 1/3 的实例首轮写出 3 字节；两个方向均设为 -1 的实例首轮写出 11 字节。送入相同的 TLS 记录头前三字节后，只有受限实例请求暂停读取。创建实例后修改配置不影响这两条连接。
21 运输启动重试。直连拒绝越界监听端口和拨号端口0；空池拒绝增长后返回 NI_LIMIT_EXCEEDED。调整连接池后同一实例可监听端口0并从宿主读回实际端口。管理链路拒绝空地址后可更正配置并拨号到65535。中继拒绝零监听容量和越界拨号端口后，可在同一实例监听65535。直连、管理和中继分别拒绝空字符串地址，调用方可在同一实例继续启动。校验拒绝时的连接号、宿主资源与业务通知。
22 文件进度期限。三个接收流共享同一后端及单调时钟，分别取 1000 毫秒、默认 15000 毫秒及 60000 毫秒。期限前一毫秒应正常，恰到期限时仅对应文件进入超时，后端收到该文件的一次关闭；终态保留 NI_TIMEOUT。
22 号另让接收端真实消费 END、完成 FLUSH 和 PUBLISH，验证最终 ACK 在期限内获接纳或持续背压至超时。12、22 号也验证最终 ACK 消费前及消费后连续取消两次：前者保留第一次错误，后者保留成功，后续均无待交付输出。PUBLISH 成功结果还未分派时先取消的情形，协议报告取消，后端的发布成功仍按原结果消费。各情形均只提交一次发布。12 号还核对这段等待的剩余毫秒数，并检查先 poll 再分派已完成的 FLUSH 时，wait 的上限降为零；收到其他任务的结果时保留调用方的等待上限。16 号同时覆盖后端已停止但仍须分派的成功及失败 CLOSE 结果。另在最终 ACK 已接纳后，分别于首次提交 CLOSE 前、共享队列拒收 CLOSE 后停止后端。后端释放句柄后，poll 和 snapshot 应保留本地 NI_CANCELLED 与协议 NI_FILE_COMPLETED，完成队列为空，后续推进不再申请任务号。
23 直连与管理运输在默认预算和单事件预算下核对相同的关闭序列。直连 TLS 启动阶段的关闭。连接就绪前和 TCP 已建立但 TLS 尚未创建时，远端零原因 EOF 均产生一次 TLS_TRUNCATED；宿主错误保留规范化原因和原始诊断，本地主动关闭保留调用方原因。均不得发布就绪或再次通知宿主关闭。管理运输在原始连接通知之前和之后分别检查主动停止、零原因 EOF 和宿主错误，对应回报本地原因、NI_TRUNCATED、NI_INVALID_STATE，并且只回调一次。
13 号另在单事件预算下先派发可写提示，再交付握手或应用输入；连接结束时排队的两条完整帧须按序交付，本地待发帧不能继续写入宿主。14 号在单事件预算下验证中继同轮存在待发记录、被业务暂缓的入站记录、可写提示和远端关闭时仍交付入站记录及一次关闭。
24 直连大帧。单帧上限 131072 字节，输出队列设为 -1，每轮 49152 字节。宿主对写入施加背压并确认至少一次拒收。恢复后每轮写入均在预算内，单次最多 32768 字节；服务端验证 98304 字节正文、种类和编号，消费后队列为空。
25 公开发送门面。经 NetSender::backend().prepareGateway(client, config) 首次准备，再经统一门面重复默认准备。先通过无效产品号设置参数错误，再分别在准备前配置网关、压缩表及提交请求，最近错误都应改为 NI_INVALID_STATE。检查提供方、文件服务、工作者配置，独立创建与销毁，连接前参数拒绝与更正，以及请求、数据、同步票据和空闲状态的返回值与输出清零。低层入口及 C++ 门面对票据与正文部分重叠、与帧字段重叠返回 NI_INVALID_ARGUMENT，保留原值。前后相邻的正文仍允许独立的失败票据清零。
26 C 发送接口。独立 C 翻译单元从所选 SDK 的统一 NetSenderAPI.h 接入，核对指针位宽、静止句柄状态及失败时的请求号清零。原生和 Wasm 归档的 C、C++ 用例都包含 NetSender/NetSenderAPI.h。Wasm 入口位于目标 SDK 的 include/wasm/NetSender 下，与同批 NetPeer/NetSenderAPI.h 字节一致。经真实 TLS 与业务握手取得代次；校验请求应答的编号、类型、产品和二进制正文。EVENT 第一次拒收后再次交付；回调内 poll 和 destroy 应返回 NI_INVALID_STATE。主动关闭只回调一次。连接就绪后，票据与请求正文、数据正文或帧字段重叠时，C 入口须保留输入并返回 NI_INVALID_ARGUMENT。随后可用同一业务编号提交并收到一次应答。
27 运输适配器。宿主抛出拨号异常后连接号和计数归零；另一实例能正常连接并关闭。检查未知管理连接、路由器非法地址与端口、默认会话和独立提交入口的绑定规则。
28 C 文件接口。先用非零哨兵检查元数据、长度、流句柄、借用视图和快照输出的别名拒绝，确认相交时不写入；再用独立输出核对各接口约定的失败值。核对元数据字段和控制包容量，发送方按读取请求产出正文，接收方落盘后才确认。提交只允许领取一次，两端完成快照应报告同一偏移，并清空流号。
附加功能入口 --discovery-timers <identity.dat> 读取隔离测试网关的 NGI3 叶身份。原生传入文件路径，Wasm 测试宿主须先把文件放入模块虚拟文件系统。真实管理连接完成认证后，节点收到编号 1 的发现请求。节点不应答时，100 毫秒应答期限的发现器在保护时限内关闭，60000 毫秒实例继续等待。节点应答后，两种实例均应发布一项候选；10 毫秒刷新间隔的实例须发出编号 2 的查询，60000 毫秒实例在 150 毫秒观察期内只有首条查询。节点给编号 2 返回空目录正文时，宿主须收到 NI_PROTOCOL_ERROR，发现器仍保留之前认证的一项候选。慢应答回归设置刷新100毫秒、应答500毫秒。节点实际收到首条查询后继续驱动双端至少120毫秒，再经 TLS 返回合法目录。以发现器公布候选的那次 poll 开始时刻为下界，节点至少经过100毫秒才应收到编号2。继续120毫秒仍应等待；保持不应答，原连接应以 NI_TIMEOUT 关闭，且关闭时间不得早于刷新下界加500毫秒。两项时间下界允许2毫秒取整误差。再让已认证的管理监听者持续拒收编号 1 的查询。背压期限设为 10 毫秒时应回报一次 NI_TIMEOUT；设为 60000 毫秒时，观察 150 毫秒后恢复接收应只交付一次原查询。另一组在同一管理连接上依次拒收两条控制请求和一条透明记录。前一项恢复接收时，即使已过其原等待期限，下一项第一次拒收也应保留连接。解除各项背压后核对请求顺序和记录正文。
9号另验同次轮询前的首个终态。宿主已报告错误后再收到超容量数据、重复连接和 EOF 时，监听者应只收到首个错误详情。若先收到 EOF，之后的错误和数据不得覆盖原有终态或造成二次关闭。socket.write 同步报告 -91 后抛异常或返回负数，本次写须报告未接纳，原生继续 poll 后只交付一次原错误。另在 JS 事件预算为 1 时排队字节 19、83 和原因 -83 的 EOF。每轮恢复读取，写入持续返回 NI_LIMIT_EXCEEDED，底层 socket 不得收到写或暂停操作；两字节分轮按序到达，关闭一次，详情为 -83。关闭后再写返回 NI_INVALID_STATE。
9号的 Wasm 异步子场景由两个真实字节通道共享32768字节输出池。两端均暂停读取，各自先收到一个字节。同步拒收后同轮两次可写提示合并交付一次；一端的异步写归还后，另一端在数据仍排队时也能收到共享池可写通知并重试。恢复读取后两端各交付原字节一次，并核对可写通知总数。远端EOF与本地主动关闭均只交付一次。
10号还拒绝与构造器所属模块不同的文件流连接适配器；直接传入或通过 connection() 取得时均应在任何堆访问和异步文件操作前抛出 TypeError。
10号的 Wasm 文件流专项还分别令连接的 ready、disposed 属性读取抛错。主动取消以 -72 结束，异步读取拒绝以 -83 结束。四种情形均检查 wait 保留首次原因、文件只关闭一次、取消帧与连接故障通知均为零次，并监听未处理的 Promise 拒绝。
10号的 Wasm 异步子场景通过 Wasm CTest 的 NetPeerWasmHost 运行。协商块长1024，宿主单次最多300字节，1300字节的文件应写为300、300、300、124、276字节。逐一核对偏移和正文；ACK不得超过刷新进度，读端和写端各关闭一次，写端关闭成功后才可提交一次。另验读端拒绝且未提供异常对象、写端关闭失败、读端最终关闭失败。分别预期读端返回 NI_INVALID_STATE、写端保留宿主错误且不提交、读端关闭报错且写端仍完成提交；各端只关闭一次。发送端关闭尚未完成时连续调用 destroy，done 和 wait 均须继续等待，随后关文件失败返回 -9。另在取消帧发送抛异常时分别提供重入且抛异常的故障回调、或省略回调，均须保留首次取消原因 -83，最多通知一次，宿主关闭一次。传入小数原因 -0.5 时以 NI_CANCELLED 结束。发送端完成异步读后，连接适配器的 schedulePump 抛出 -83 时应关闭一次，wait 返回 -83，对端没有收到文件正文。宿主读取或最终关闭拒绝 Promise，且拒绝对象的 code 属性读取也抛异常时，wait 须返回 NI_INVALID_STATE（-7），不产生未处理的 Promise 拒绝。另验直接以 -83、-1 拒绝读取时 wait 返回原值，以 -0.5、0、-2147483649 拒绝时返回 -7，以 -2147483648 拒绝最终关闭时保留该值。读取失败场景只发起一次读取，各端关闭一次，不再重试；最终关闭失败场景须先完整传输 1300 字节、正常完成两次读取，再报告关闭错误，各端仍只关闭一次。

功能结果记录（2026-11-28，北京时间）
本轮重新编译 NetPeer 及仓库依赖。Windows 使用 VS2013 v120，Wasm 使用 Emscripten 3.1.74、Node 24.0.0。六种配置各运行 --all 的28个编号，27个瞬时、1个压力。各配置另行执行发现期限、服务 TLS 两个专项，两个 Wasm 位宽还分别执行 JS 宿主专项。
Windows Win32 Debug 静态库，28/28 编号与 2/2 专项通过，耗时 4.594 秒。
    NetPeerTestd.exe SHA256 02696470AEE84EA6ACC2A73B846F32B4C8B9E6569EFDCF0D2D5D4DC88E344485
Windows Win32 Debug 共享库，28/28 编号与 2/2 专项通过，耗时 5.074 秒。
    NetPeerTestd.exe SHA256 8AC611EFD673666152DFEC5ED26618F54F088ED521D7C2ED0B559135659ABAE1
    NetInterfaced.dll SHA256 59B2273A626F055012ADE8299D98828732C42ABA20F3FA12F81CD4FAE208CC73
    NetPeerd.dll SHA256 4A22825DD9DBE98E185D766416118886C6ED69D64913543CC00424120D1AACB9
Windows x64 Release 静态库，28/28 编号与 2/2 专项通过，耗时 3.509 秒。
    NetPeerTest.exe SHA256 4F6541C93241D91FC30D82F91F962F6BB53DFFCC2E993DE9D8AC54E863E9E312
Windows x64 Release 共享库，28/28 编号与 2/2 专项通过，耗时 3.509 秒。
    NetPeerTest.exe SHA256 2EE405C90E2863E8ACB9992E8557811C66019B827110F1A85E9A3CD5F393C420
    NetInterface.dll SHA256 AC1B34B48844D5D58E37835C6A9DB84ED0C05D9190026EA5373F7EAEE5C3C724
    NetPeer.dll SHA256 20D898F66AB3DC0B44C0B445359E53E44FACFE6CB0AB7D2E652F5A10A1A9845F
Node Wasm32 Release，28/28 编号与 3/3 专项通过，耗时 5.178 秒。
    NetPeerTest.wasm SHA256 A883133BBB5C8396ECCEBBA5FE10B637E1F432DAF898B746F7D4B24E59334C1C
    NetPeerTest.js SHA256 07CB03E7DF4924AC02E7871FAA0919B6BC69B9912934FC441E2931A8055BE914
Node Wasm64 Release，28/28 编号与 3/3 专项通过，耗时 5.322 秒。
    NetPeerTest.wasm SHA256 4AE7D826A92F22293A95992709D33D42261B766AD2A18199994ED07B7A2EA1AC
    NetPeerTest.js SHA256 E4860E90F5B31E3C377607551B43A5EB44FF87DDD6A7E4219071AE03DAD11E5F
执行168次编号（162瞬时、6压力），通过168，失败、取消、未执行和删除跳过均为0。编号4每次4轮，每轮512次候选选择。另通过14次专项。六配置功能运行耗时合计 27.186 秒，含进程启动及专项。
x64 Release 静态库另行执行 NetPeerServiceFixtureOutputs，核对成对生成、保留已有文件及失败回收。服务 TLS 使用本轮生成的测试证书和密钥，发现使用隔离网关的 NGI3 叶身份。用相同 SDK 编译运行 NetPeer.cpp 中的离线 main 示例，退出码0。
x64 静态隔离对照还在有效目录应答复制候选表时单次注入 std::bad_alloc，观测到旧关闭码 -2、修复后 -5（NI_NO_MEMORY），均未发布候选。正式 --discovery-timers 另验证损坏应答返回 NI_PROTOCOL_ERROR 且保留上一份候选。回归辨识力核验使用隔离的 x64 静态库。第12号同步关闭场景在缺少通知的实现上因等待时间未归零而失败。第16号在最终 ACK 后、提交 CLOSE 前停止后端，缺少本地取消处理的实现未返回 NI_CANCELLED，该项失败。第28号另用 x64 隔离副本验证了两个反例：重叠时提前写零和独立输出失败时漏清零，各使对应的非零哨兵检查失败。与正式库相同的实现返回0。上列正式结果来自修复后的产物。临时构建目录为 src/temp/NetPeer-fresh-280805，结果归入本头后清理。
Wasm32 与 Wasm64 各通过 CTest 的4个入口（编号批次、JS 宿主、服务 TLS、发现期限）。另用仓库发布程序从干净根分别打包两种位宽的静态 SDK。纯 C、C++ 共四个调用方通过 NetSender/NetSenderAPI.h 编译、链接并在 Node 返回 0，编译依赖均指向 include/wasm/NetSender。污染静态统一头时发布摘要检查失败。另在两个位宽实际配置测试工程：共享目录存在原生同名头时，Wasm 头缺失及与提供方不一致均配置失败，恢复同批头后配置成功。修复前同样四次编译均因缺头失败。两种位宽均验证了四种回归的辨识力：首错被覆盖、越界直连材料触发堆分配、最终 ACK 背压时迟到 END 干扰同会话其他流，以及非法 PID 进入原生拨号；对应修复前的模块各自失败。另外在错误和 EOF 已登记但尚未上送时分别重试写入，两种旧模块在两条路径上均返回 -1 并被测试拒绝。当前实现没有额外调用宿主写入，预算为 1 时依次交付两个字节 19、83，只通知一次关闭并保留原关闭详情 -83。在 Wasm32、Wasm64 分别独立执行 ready/disposed 访问抛错与主动取消/异步读失败的四种组合。修复前各四组均失败，两个异步场景均观测到未处理的 Promise 拒绝。当前各四组均通过，保留 -72/-83，文件各关闭一次。另行在两个位宽通过公开 Sender 建立直连并确认发出 TLS ClientHello，随后字节宿主报告 -91。每端仅一次上层关闭，报告 TLS 截断及底层 TLS 诊断（本构建实测 reason=-105、detail=-29312）。上层详情的具体数值随 TLS 实现而定；字节监听者保留原始 -91。Win32 Debug 共享库另在独立进程复跑5轮发现期限和第12、16号。上述复跑单列。
Linux、Linux ARM、macOS、Android 和 iOS 仅检查源码及构建分支。另用 CMake 3.31.6 和 NMake 生成 Linux ARM64 分支的链接命令，预置原子操作直接可链接和需要 libatomic 两种探针结果。NetPeer 共享库、NetPeerTest 与 NetPeerServiceFixture 共 6/6 个生成结果符合预期：直接可链接时不带 -latomic，需要时位于归档组之后。此项未编译或运行 ARM 二进制。未执行大端 ARM、浏览器或微信真机测试。未运行性能基准，当前版本缺少性能测试数据。
生产目录 100 个文件 SHA256 529D9313DC5A841FDF6BE26301E68086E8A41933452BE51E86BF7CE5AAFF9551，测试目录 15 个文件 SHA256 F337B49B96FC8065EAD6AD3B1327E95D5D2DCF57722C8D5160AF5E6E1B8A6F0D。按仓库相对路径（/ 分隔）字典序排列，每行路径加一个空格和大写文件 SHA256，行间 LF、末尾无换行，取 UTF-8 汇总 SHA256。测试头的本结果段不纳入摘要。
独立源码验收记录。三个全新上下文 peer_accept_window_t、peer_accept_window_u、peer_accept_window_v 依次审查同一版 100 个生产文件和 15 个测试文件，三份独立结论均通过。三轮期间生产逻辑及测试代码没有变更。三个窗口均已核对最终使用说明，其 SHA256 为 519D39DBD18CE8DCD7C5C0E9AE5C83829AEEE348514760F55AF418B0CE248636。T、V 核对既有功能证据。U 独立重跑 Windows 四种配置的瞬时用例和专项、两种 Wasm 的宿主专项，并验证公开 TLS 关闭回调。

性能口径
--benchmark 单独测量已准备、未连接、同一执行者的 gatewayState，每次核对generation和ready。
使用同一Release编译器、位宽、宿主、CPU和调度条件；先预热一个进程，再启动至少五个独立进程，
分别记录校准后九个批次的中位数，报告进程中位数、最小值、最大值和环境。调度干扰明显时应记录离散程度。
当前版本缺少性能测试数据，未执行 --benchmark，尚无可报告的耗时、吞吐量或跨进程波动范围。建立/关闭/参数校验无需单列吞吐指标；
状态查询、请求提交、原始宿主交付、对端解密、文件落盘各有不同的计时终点。

实际平台和检查器范围以上述本轮功能结果为准。

函数与用例索引
以下是当前源码调用关系与测试场景的索引，不代表本轮逐函数动态覆盖率。下列编号可用 --cases 单独运行；认证时限专项使用上文的独立入口。
每个函数列出相应调用场景。同名重载可能对应多个编号。输入、预期输出和状态变化见相应的编号说明。仅内联且没有独立符号的函数随调用场景检查。
C 导出函数
  Wasm 宿主专项　netPeerWasmConfigureGateways、netPeerWasmFileFlowAccept，分别验证未准备客户端配置和文件帧交付状态。
  26 号　netPeerConfigureHost、 netPeerPointerBytes、 netPeerRandomBytes、 netPeerSenderClientBegin
      netPeerSenderClientCancel、 netPeerSenderClientClose、 netPeerSenderClientConfigureCompression
      netPeerSenderClientConfigureDispatch、 netPeerSenderClientConfigureGateways、 netPeerSenderClientConnect
      netPeerSenderClientConnectService、 netPeerSenderClientCreate、 netPeerSenderClientDestroy
      netPeerSenderClientPoll、 netPeerSenderClientPost、 netPeerSenderClientPostData、 netPeerSenderClientSend
      netPeerSenderClientSendBegin、 netPeerSenderClientSendData、 netPeerSenderClientSendDataBegin
      netPeerSenderClientSendResult、 netPeerSenderClientState、 netPeerSenderConfigureWorkers
  28 号　netPeerFileControlEncode、 netPeerFileFlowAccept、 netPeerFileFlowCancel、 netPeerFileFlowCommitted
      netPeerFileFlowConsume、 netPeerFileFlowCreate、 netPeerFileFlowCreateWire、 netPeerFileFlowDestroy
      netPeerFileFlowPeek、 netPeerFileFlowPrime、 netPeerFileFlowProduced、 netPeerFileFlowProgress
      netPeerFileFlowRead、 netPeerFileFlowSetWriteBatchChunks、 netPeerFileFlowSnapshot
      netPeerFileFlowSnapshotWire、 netPeerFileFlowTakeCommit、 netPeerFileFlowWrite、 netPeerFileFlowWritten
      netPeerFileInfoFields、 netPeerFileParseInfo
NetBufferPool
  9 号　~NetBufferPool
NetBufferPoolConfig
  4 号　NetBufferPoolConfig
  9 号　~NetBufferPoolConfig
NetClientChannel
  20 号　NetClientChannel
NetClientDiscovery
  4 号　NetClientDiscovery
NetConnectionPoolConfig
  4 号　NetConnectionPoolConfig、 ~NetConnectionPoolConfig
NetDirectorySnapshot
  17 号　NetDirectorySnapshot、 cache、 create、 reset、 ~NetDirectorySnapshot
  18 号　copy、 parse
  认证时限专项　values
NetDirectTransportConfig
  11 号　NetDirectTransportConfig
  20 号　NetDirectTransportConfig、 ~NetDirectTransportConfig
NetFileIo
  10 号　cancel
  19 号　released、 status
  22 号　NetFileIo、 accepting、 configureBackend、 consume、 done、 finishCall、 init、 nextRequestId、 peek
      queryCompletion、 stop、 submit、 uninit、 ~NetFileIo
NetFileIoBackend
  22 号　NetFileIoBackend、 ~NetFileIoBackend
NetFileTransfer
  16 号　accept、 consumeOutput、 peekOutput、 snapshot
  19 号　setWriteBatchChunks
  22 号　NetFileTransfer、 cancel、 done、 onIoResult、 poll、 prepare、 setProgressTimeout、 start、 submit
      ~NetFileTransfer
NetFileTransferState
  22 号　NetFileTransferState、 ~NetFileTransferState
NetGatewayAddress
  4 号　NetGatewayAddress、 ~NetGatewayAddress
NetGatewayDirectoryStore
  17 号　NetGatewayDirectoryStore、 complete、 open、 poll、 queue、 shutdown、 stopped、 submit、 utc
      ~NetGatewayDirectoryStore
  18 号　queue、 takeLoaded
  27 号　load
NetGatewayDirectoryStoreState
  17 号　NetGatewayDirectoryStoreState、 ~NetGatewayDirectoryStoreState
NetGatewayDiscovery
  3 号　discovered
  4 号　NetGatewayDiscovery、 choose、 parseSeeds、 shutdown、 start、 stopped、 ~NetGatewayDiscovery
  7 号　onLinkClosed、 poll
  15 号　failed
  认证时限专项　onLinkControl、 onLinkReady、 onLinkRecord
NetGatewayDiscoveryConfig
  4 号　NetGatewayDiscoveryConfig、 valid、 ~NetGatewayDiscoveryConfig
NetGatewayDiscoveryState
  4 号　NetGatewayDiscoveryState、 rebuild、 ~NetGatewayDiscoveryState
NetGatewayHostTransportFactory
  7 号　create
  11 号　NetGatewayHostTransportFactory、 ~NetGatewayHostTransportFactory
NetGatewayNativeTransport
  7 号　NetGatewayNativeTransport、 connect、 shutdown、 ~NetGatewayNativeTransport
  12 号　poll
  27 号　NetGatewayNativeTransport、 outputState、 sendControl、 sendRecord
NetGatewayTransport
  7 号　NetGatewayTransport
NetGatewayTransportFactory
  11 号　NetGatewayTransportFactory
NetHostPoolConfig
  4 号　NetHostPoolConfig
  9 号　NetHostPoolConfig、 ~NetHostPoolConfig
NetHostSocket
  8 号　configureDefault、 randomBytes
  9 号　NetHostSocket、 close、 configureInputPool、 configurePeerPool、 connect、 pauseRead、 poll、 shutdown
      status、 uninit、 ~NetHostSocket
  12 号　defaultSupports
  20 号　write
  21 号　listen、 localPort
  24 号　isBackpressure
  27 号　NetHostSocket、 peerCount
NetHostSocketListener
  9 号　NetHostSocketListener、 ~NetHostSocketListener
NetHostSocketPeer
  9 号　NetHostSocketPeer、 ~NetHostSocketPeer
NetHostSocketState
  8 号　valid
  9 号　NetHostSocketState、 addPeer、 clearInput、 close、 closed、 connected、 data、 dispatch、 poll、 queue、 start
      stop、 validConnect、 writable、 ~NetHostSocketState
  14 号　validListen
  27 号　fail
NetInterfaceLinkPools
  4 号　NetInterfaceLinkPools、 ~NetInterfaceLinkPools
NetInterfaceTlsOptions
  4 号　setHandshakeTimeout
  6 号　effectiveHandshakeTimeout
NetLinkTransport
  14 号　NetLinkTransport、 configurePeerPool、 connect、 shutdown、 ~NetLinkTransport
  23 号　close、 poll
  27 号　localPort、 outputState、 sendControl、 sendRecord
  认证时限专项　listen
NetLinkTransportConfig
  4 号　NetLinkTransportConfig、 ~NetLinkTransportConfig
NetLinkTransportListener
  9 号　NetLinkTransportListener
NetLinkTransportPeer
  14 号　NetLinkTransportPeer、 ~NetLinkTransportPeer
NetLinkTransportState
  14 号　NetLinkTransportState、 close、 collectClosed、 configure、 onRelayClosed、 ~NetLinkTransportState
  23 号　pump
  认证时限专项　onRelayConnected、 onRelayRecord、 backpressureExpired
NetPeer
  25、26 号　TicketOverlaps（NetPeer.cpp 内部函数），检查输出票据与调用方输入的重叠范围。
  1 号　beginRequest、 clientBegin、 clientPoll、 gatewayReady、 gatewayState、 pollGateway
  2 号　clientConfigureCompression、 clientConfigureConnectionPools、 clientConfigureGateways
      clientConfigureRequestPool、 clientConfigureSubmissionPools、 configureCompression
      configureConnectionPools、 configureGateways、 configureRequestPool、 configureSubmissionPools
  3 号　configureWorkers
  5 号　clientState、 configureDispatch、 gatewayGeneration、 lastGatewayError、 prepareGateway
  8 号　backend、 configureHost、 randomBytes
  11 号　clientCreate、 clientDestroy、 createClient、 destroyClient、 prepareGateway
  12 号　clientConnect、 clientConnectService、 clientPost
  25 号　beginSendData、 beginSendRequest、 cancelRequest、 clientCancel、 clientClose、 clientPostData、 clientSend
      clientSendBegin、 clientSendData、 clientSendDataBegin、 clientSendResult、 closeGateway、 connectGateway
      connectServiceGateway、 pointerBytes、 postData、 postRequest、 sendData、 sendRequest、 sendResult
      uninitGateway
  27 号　clientConfigureSubmissionPools、 clientCreate
  28 号　NetPeer、 fileProtocol、 instance、 ~NetPeer
NetPeerBackend
  2 号　configureSubmissionPools
  8 号　configureHost
  25 号　configureGateways、 prepareGateway
NetPeerClientBackend
  27 号　bindSubmission
NetPeerClientOpaque
  11 号　NetPeerClientOpaque、 inactive、 ~NetPeerClientOpaque
  12 号　connect、 onSessionClosed、 onSessionReady
  26 号　onSessionFrame、 onSessionResult
NetPeerClientSession
  11 号　NetPeerClientSession、 ~NetPeerClientSession
  12 号　connect、 connectService
  27 号　NetPeerClientSession、 bindChannel
NetPeerConfig
  11 号　NetPeerConfig、 valid、 ~NetPeerConfig
NetPeerEndpointRouter
  27 号　NetPeerEndpointRouter、 listen、 ~NetPeerEndpointRouter
NetPeerFileIoState
  22 号　NetPeerFileIoState
NetPeerNetSender
  25 号　backend、 beginRequest、 beginSendData、 beginSendRequest、 cancelRequest、 closeGateway
      configureCompression、 configureConnectionPools、 configureDispatch、 configureRequestPool
      configureSubmissionPools、 configureWorkers、 connectGateway、 connectServiceGateway、 createClient
      destroyClient、 gatewayGeneration、 gatewayReady、 gatewayState、 lastGatewayError、 pointerBytes
      pollGateway、 postData、 postRequest、 prepareGateway、 randomBytes、 sendData、 sendRequest、 sendResult
      uninitGateway
  28 号　NetPeerNetSender、 fileProtocol、 instance、 ~NetPeerNetSender
NetPeerSenderClientOpaque
  26 号　NetPeerSenderClientOpaque、 onSessionClosed、 onSessionFrame、 onSessionReady、 onSessionResult
      ~NetPeerSenderClientOpaque
NetPeerSenderState
  11 号　NetPeerSenderState、 callbacks
  25 号　connect、 requirePrepared
  26 号　closed、 frame、 ready、 result
NetRelayPoolConfig
  4 号　NetRelayPoolConfig、 valid、 ~NetRelayPoolConfig
NetRelayTransport
  14 号　NetRelayTransport、 close、 configurePools、 connect、 listen、 outputState、 poll、 send、 shutdown
      ~NetRelayTransport
  21 号　localPort
NetRelayTransportListener
  9 号　NetRelayTransportListener、 ~NetRelayTransportListener
NetRelayTransportPeer
  14 号　NetRelayTransportPeer、 clearInput、 enqueue、 pop、 status、 ~NetRelayTransportPeer
NetRelayTransportState
  14 号　NetRelayTransportState、 close、 now、 onClosed、 onConnected、 onData、 onWritable、 pump
      ~NetRelayTransportState
NetRelayTransportWrite
  14 号　NetRelayTransportWrite、 ~NetRelayTransportWrite
NetSenderSubmissionOptions
  11 号　~NetSenderSubmissionOptions
NetServerChannel
  20 号　NetServerChannel
NetSessionListener
  11 号　NetSessionListener
NetTransport
  6 号　configurePeerPool
  8 号　randomBytes
  13 号　configureCompact、 configureCompression、 outputState
  20 号　NetTransport、 connect、 peerCount、 poll、 shutdown、 ~NetTransport
  21 号　NetTransport、 listen、 localPort
  23 号　close
  24 号　configureConnectionPools、 send
NetTransportPeer
  20 号　NetTransportPeer、 ~NetTransportPeer
NetTransportState
  20 号　NetTransportState、 close、 collectClosed、 configure、 now、 onClosed、 onConnected、 onData、 poll、 pump
      shutdown、 ~NetTransportState
  24 号　onWritable、 writableConnection
*/
/** 由纯C调用方验证统一发送器的调用约定、初始状态和错误输出
@param [in] client 本用例已准备且未连接的客户端，借用至返回
@return 全部检查通过为1
*/
extern "C" int NetPeerTestCEntry(NetSenderClientHandle client);

// 功能用例的稳定选择编号。
enum NetPeerTestId
{
    // 客户端创建、准备及释放
    NetPeerTestLifecycle = 1,
    // 客户端配置、隔离和更正后重试
    NetPeerTestConfiguration = 2,
    // 地址发现与执行者参数
    NetPeerTestDiscovery = 3,
    // 种子轮选压力及启停
    NetPeerTestRepeatedOperations = 4,
    // 准备事务、重入和重试
    NetPeerTestPreparedConfiguration = 5,
    // 宿主与直连 TLS 槽和输出预算
    NetPeerTestDirectTransport = 6,
    // 管理及发现链路池隔离
    NetPeerTestLinkBudgets = 7,
    // 宿主强随机参数与恢复
    NetPeerTestEntropyBounds = 8,
    // 输入缓冲、背压和公平调度
    NetPeerTestHostInput = 9,
    // 文件批量写、停止及宿主错误
    NetPeerTestFileBatch = 10,
    // 配置数值边界及默认值
    NetPeerTestNumericOptions = 11,
    // 客户端路径、提交队列、配置实效
    NetPeerTestClientPaths = 12,
    // 双向 TLS、帧顺序及紧凑编码
    NetPeerTestTlsFrames = 13,
    // 中继数据池和独立控制池
    NetPeerTestRelayPools = 14,
    // 候选选择、抖动及重试
    NetPeerTestDiscoveryBackoff = 15,
    // 最终确认后的发送端文件关闭
    NetPeerTestSenderFileClose = 16,
    // 目录缓存合并间隔和完成身份
    NetPeerTestCacheWrites = 17,
    // 目录缓存序列化、发布完成与重新加载
    NetPeerTestCacheRoundTrip = 18,
    // 写完成长度必须与本次持久化请求一致。
    NetPeerTestWriteLength = 19,
    // 两个直连实例的 TLS 密文输入、输出队列互不影响。
    NetPeerTestTlsQueueLimits = 20,
    // 无效的拨号或监听参数允许同一运输更正后启动。
    NetPeerTestEndpointRetry = 21,
    // 三个文件流按各自配置的单调时钟期限收敛
    NetPeerTestProgressTimeout = 22,
    // TCP 和 TLS 启动阶段收到远端关闭以及本地关闭的原因
    NetPeerTestEarlyEof = 23,
    // 密文积压后按宿主上限切分，完整交付大帧。
    NetPeerTestLargeTlsWrite = 24,
    // 统一发送门面的生命周期、参数拒绝、票据和错误输出
    NetPeerTestSenderFacade = 25,
    // 通过C入口完成真实TLS和业务握手，核对回调、背压、请求完成和关闭
    NetPeerTestSenderC = 26,
    // 检查默认与独立运输适配器、宿主异常、端点计数与提交端口借用
    NetPeerTestTransportAdapters = 27,
    // 通过C文件接口传输包含零字节的文件，逐次核对落盘、确认、提交和最终快照
    NetPeerTestFileC = 28
};

// C 发送入口测试的回调观察值，借用至客户端完全销毁。
struct PeerSenderEvents
{
    // 当前 C 门面句柄。
    NetSenderClientHandle handle;
    // 本次就绪代次。
    uint64_t generation;
    // 已预留的业务请求号。
    uint64_t request;
    // 就绪通知次数。
    unsigned ready;
    // 请求终态次数。
    unsigned results;
    // 数据帧交付次数，包含首次背压。
    unsigned frames;
    // 关闭通知次数。
    unsigned closed;
    // 所有回调内容和重入拒绝是否符合预期。
    bool valid;
};

// 初始化配置实效测试独占的公开客户端；所有借用通道必须比它先构造。
class PeerConfiguredClient
{
public:
    /** 创建客户端，在发布前调用提供的初始化设置
    @param [in] setup 仅构造期间借用，操作实际会话和派发器
    */
    explicit PeerConfiguredClient(const std::function<int32_t(NetClientSession&, NetSenderDispatch&)>& setup);
    /** 关闭并有界推进释放；失败输出诊断
    */
    ~PeerConfiguredClient();
    /** 完全释放测试客户端，清空本地句柄
    @return 成功为 NI_OK，否则为最后一次销毁错误
    */
    int32_t release();

    // 创建结果与拥有的句柄。
    int32_t m_created = NI_INVALID_STATE;
    NetPeerClientHandle m_handle = nullptr;
    // 就绪、关闭及请求超时通知的实际观察值。
    unsigned m_ready = 0, m_closed = 0;
    int32_t m_closeReason = NI_READY;
    // 最近关闭回调的真实单调毫秒，用于核对下一次恢复等待预算。
    uint64_t m_closedAt = 0;
    std::vector<uint64_t> m_timeouts;
    // 每次回调的参数是否符合测试的产品、类型及空超时正文约定。
    bool m_valid = true;
    // 附加服务通路场景的应用帧观察者，可空；捕获对象须覆盖客户端生命周期。
    std::function<bool(const NetInterfaceFrame&)> m_onFrame;

private:
    /** 禁止复制拥有的句柄
    @param [in] other 原对象
    */
    PeerConfiguredClient(const PeerConfiguredClient& other);
    /** 禁止复制拥有的句柄
    @param [in] other 原对象
    @return 不可调用
    */
    PeerConfiguredClient& operator=(const PeerConfiguredClient& other);
};

class PeerServiceProbe;

class PeerServiceFlow;

// 透明管理运输把服务业务密文交给真实 TLS 对端，保留每轮实际发送记录计数。
class PeerServiceFlowTransport : public NetGatewayTransport
{
public:
    /** 借用一条完整服务测试会话
    @param [in,out] flow 须保持到运输销毁
    */
    explicit PeerServiceFlowTransport(PeerServiceFlow& flow);
    /** @copydoc NetGatewayTransport::connect */
    int32_t connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
        NetLinkTransportListener* listener, uint64_t& peer) override;
    /** @copydoc NetGatewayTransport::poll */
    bool poll() override;
    /** @copydoc NetGatewayTransport::sendControl */
    int32_t sendControl(uint64_t peer, const NetInterfaceFrame& frame) override;
    /** @copydoc NetGatewayTransport::sendRecord */
    int32_t sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record) override;
    /** @copydoc NetGatewayTransport::shutdown */
    void shutdown() override;
    /** @copydoc NetGatewayTransport::outputState */
    int32_t outputState(uint64_t peer) const override;
private:
    // 比运输长寿的 TLS 对端。
    PeerServiceFlow& m_flow;
    // 仅在连接存活期间借用的客户端监听者。
    NetLinkTransportListener* m_listener;
    // 管理就绪只交付一次。
    bool m_announced;
    // OPEN 回复由下一轮交付。
    std::string m_openReply;
};

// 完整服务握手场景拥有真实服务 TLS、HELLO、逐字节观察及回收计数。
class PeerServiceFlow : public NetGatewayTransportFactory
{
public:
    /** 创建使用隔离服务叶身份的真实 TLS 对端
    @param [in] tls 证书与私钥借用覆盖本对象，必须为服务端设置
    */
    explicit PeerServiceFlow(const NetInterfaceTlsOptions& tls);
    /** @copydoc NetGatewayTransportFactory::create */
    NetGatewayTransport* create() override;
    /** 推进服务 TLS 并核对客户端完整业务帧
    @return 正文和握手正确且未关闭时返回 true
    */
    bool advance();
    /** 真实应用回调逐项核对事件，允许暂时拒收同一条
    @param [in] frame 借用至返回的解密业务帧
    @return false 保持背压，true 接纳一个完整事件
    */
    bool accept(const NetInterfaceFrame& frame);

public:
    // 独占服务 TLS。
    std::unique_ptr<NetInterfaceConnection> m_server;
    // 服务端的真实业务握手。
    NetInterfaceHandshake m_hello;
    // 各运输 poll 开始后发出的密文记录数、应用成功接纳帧数。
    std::vector<uint32_t> m_recordsPerPoll, m_framesPerPoll;
    // 已完整接纳的事件编号和数据帧数量。
    std::vector<uint64_t> m_events;
    uint32_t m_dataFrames;
    // 所有线格式及正文是否正确、是否已回送业务 HELLO。
    bool m_valid, m_helloSent;
    // 应用拒收开关、实际调用次数及首次拒收单调毫秒。
    bool m_reject;
    uint32_t m_frameCalls;
    uint64_t m_firstRefusalMs;
    // 管理运输创建/停止的真实次数。
    unsigned m_created, m_stopped;
};

// 受控管理运输只提供 OPEN 回复和记录真实业务 TLS 密文，不伪造业务握手成功。
class PeerServiceTransport : public NetGatewayTransport
{
public:
    /** 借用一次测试的独立观察值
    @param [in,out] probe 须存活至运输销毁
    */
    explicit PeerServiceTransport(PeerServiceProbe& probe);
    /** @copydoc NetGatewayTransport::connect */
    int32_t connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
        NetLinkTransportListener* listener, uint64_t& peer) override;
    /** @copydoc NetGatewayTransport::poll */
    bool poll() override;
    /** @copydoc NetGatewayTransport::sendControl */
    int32_t sendControl(uint64_t peer, const NetInterfaceFrame& frame) override;
    /** @copydoc NetGatewayTransport::sendRecord */
    int32_t sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record) override;
    /** @copydoc NetGatewayTransport::shutdown */
    void shutdown() override;
    /** @copydoc NetGatewayTransport::outputState */
    int32_t outputState(uint64_t peer) const override;
private:
    // 工厂观察值及存活期间借用的真实通道监听者。
    PeerServiceProbe& m_probe;
    NetLinkTransportListener* m_listener = nullptr;
    // 本次是否已报告管理连接就绪，及待交付的 OPEN 回复。
    bool m_announced = false;
    std::string m_response;
};

// 每个服务设置场景有自己的工厂和完整线格式观察值。
class PeerServiceProbe : public NetGatewayTransportFactory
{
public:
    /** @copydoc NetGatewayTransportFactory::create */
    NetGatewayTransport* create() override;
    // 实际创建的运输数、OPEN 请求窗口和首个 ClientHello 记录字节数。
    unsigned m_created = 0;
    uint32_t m_window = 0, m_firstCiphertext = 0;
    // 连续两条不完整 TLS 输入记录的接纳结果和实际返还的密文信用。
    bool m_inputSent = false, m_tailAccepted = false;
    uint32_t m_creditBytes = 0;
    // 拒绝拨号用于独立观察恢复退避，默认接纳管理连接。
    bool m_refuseConnect = false;
    // 运输收到的停止次数。
    unsigned m_stopped = 0;
};

// 始终没有候选的发现器，观察真实恢复协调器选择次数。
class PeerPendingDiscovery : public NetClientDiscovery
{
public:
    /** 初始化独立的候选选择次数及真实单调时间观察
    */
    PeerPendingDiscovery();

    /** @copydoc NetClientDiscovery::start */
    int32_t start(const std::vector<NetGatewayAddress>& seeds, const std::string& cachePath,
        const NetInterfaceTlsOptions& tls) override;
    /** @copydoc NetClientDiscovery::poll */
    bool poll() override;
    /** @copydoc NetClientDiscovery::shutdown */
    void shutdown() override;
    /** @copydoc NetClientDiscovery::stopped */
    bool stopped() const override;
    /** @copydoc NetClientDiscovery::choose */
    bool choose(NetClientDiscoveryPoint& endpoint) override;
    /** @copydoc NetClientDiscovery::failed */
    void failed(const NetClientDiscoveryPoint& endpoint) override;

public:
    // 本代是否活跃。
    bool m_active;
    // 实际选择次数。
    unsigned m_choices;
    // 最近一次 choose 返回前的真实单调毫秒，用于校验剩余等待预算。
    uint64_t m_lastChoiceMs;
};

class PeerConfigChannel;

// 准备回调借用的业务通道及本次测试的两个独立派发量子。
struct PeerDispatchProbe
{
    // 本次会话绑定的业务握手对端。
    PeerConfigChannel* m_channel;
    // 每轮最多派发的命令数。
    uint32_t m_commands;
    // 每轮可派发的正文字节数。
    uint32_t m_bytes;
};

// 配置回调操作的目标及实际返回码，生命周期限于一次准备测试。
struct PeerPreparationProbe
{
    // 借用的门面客户端。
    NetPeerSenderState* m_client;
    // 同一客户端上重入操作的返回码。
    int32_t m_nested;
};

// 文件宿主查询期间的外部控制，由测试用例持有至后端释放。
struct PeerFileQueryProbe
{
    // 非空时在下一次查询中发起停止，并立即清除此借用。
    NetFileIo* m_stopOnQuery = nullptr;
    // 回调尚未退栈。
    bool m_active = false;
    // 下一次查询抛出宿主错误，随后恢复正常。
    bool m_rejectNext = false;
};

// 可控宿主只记录原始字节和连接事件；每次成功create都有对应destroy。
struct PeerTestWire
{
    // 宿主创建时借用的回调上下文。
    void* observer = nullptr;
    // 向调用方报告已连接。
    NetHostConnected connected = nullptr;
    // 向调用方交付原始输入。
    NetHostData data = nullptr;
    // 向调用方报告恢复可写。
    NetHostWritable writable = nullptr;
    // 向调用方报告关闭原因。
    NetHostClosed closed = nullptr;
    // 宿主成功监听的端口，尚未监听时为零。
    uint32_t boundPort = 0;
    // 宿主接纳的总字节数。
    uint32_t writes = 0;
    // 测试宿主暂缓写入时的拒收次数和单次接纳的最大字节数。
    uint32_t m_writeRefusals = 0, m_largestWrite = 0;
    // 为 true 时宿主向运输回报发送背压。
    bool m_blockWrites = false;
    // 宿主收到的关闭调用数。
    uint32_t closes = 0;
    // 可选的宿主关闭原因输出，观察者在本宿主释放后仍可核对。
    int32_t* m_closeReason = nullptr;
    // 当前连接是否存活。
    bool live = false;
    // 宿主是否已暂停读入。
    bool paused = false;
    // 是否保留交付的字节供对端使用。
    bool capture = false;
    // 宿主已接纳的原始字节。
    std::vector<uint8_t> output;
};

// 主动连接在 connect 返回前同步回报的两个事件及预期结果。
struct PeerTestDialCase
{
    // 正数报告连接，负数报告相应编号的远端 EOF，零表示没有事件。
    int32_t m_first;
    // 第二个同步事件，编码与第一个相同。
    int32_t m_second;
    // 宿主在 connect 输出参数中返回的连接编号。
    uint64_t m_returned;
    // 对此组事件和编号，NetHostSocket 是否应接纳。
    bool m_accepted;
};

// 记录宿主、TLS 和管理连接的交付顺序及最终原因。
class PeerTestListener : public NetTransportListener, public NetHostSocketListener, public NetLinkTransportListener, public NetRelayTransportListener
{
public:
    // 输入事件次数。
    uint32_t inputs = 0;
    // 连接和中继就绪通知数。
    uint32_t connectedCount = 0;
    // 可写通知数。
    uint32_t writableCount = 0;
    // 最近一次输入的独立副本。
    std::vector<uint8_t> input;
    // 就绪与关闭通知次数。
    uint32_t readyCount = 0, closeCount = 0;
    // 最近关闭的协议原因与宿主原始诊断。
    int32_t closeReason = 0, closeDetail = 0;
    // 为 true 时持续拒绝本条中继记录，直至用例解除背压。
    bool m_rejectRelayRecord = false;
    // 实际收到的中继记录回调次数，包含拒收后的重试。
    uint32_t m_relayOffers = 0;
    // 最近一次接纳的中继字节正文。
    std::vector<uint8_t> m_relayBody;
    // 拒绝当前直连帧，让调用方控制 EOF 后的排空时机。
    bool m_rejectFrame = false;
    // 直连帧被拒绝的回调次数。
    uint32_t m_refusedFrames = 0;
    // 允许接纳的最大管理请求号；较大的号保持背压。
    uint64_t m_acceptControlThrough = UINT64_MAX;
    // 管理控制帧在背压期间实际被拒收的次数。
    uint32_t m_refusedControl = 0;
    // 按接收顺序保存的请求号。
    std::vector<uint64_t> frames;
    // 符合预期正文格式的帧的首字节。
    std::vector<uint8_t> bodies;

public:
    /** @copydoc NetTransportListener::onReady */
    void onReady(uint64_t peer) override;

    /** @copydoc NetTransportListener::onFrame */
    bool onFrame(uint64_t peer, const NetInterfaceFrame& frame) override;

    /** @copydoc NetHostSocketListener::onConnected */
    void onConnected(uint64_t peer) override;

    /** @copydoc NetHostSocketListener::onData */
    void onData(uint64_t peer, const uint8_t* data, uint32_t size) override;

    /** @copydoc NetHostSocketListener::onWritable */
    void onWritable(uint64_t peer) override;

    /** @copydoc NetHostSocketListener::onClosed */
    void onClosed(uint64_t peer, int32_t reason, int32_t detail) override;

    /** @copydoc NetRelayTransportListener::onRelayConnected */
    void onRelayConnected(uint64_t peer) override;

    /** @copydoc NetRelayTransportListener::onRelayRecord */
    bool onRelayRecord(uint64_t peer, const NetInterfaceTunnelRecord& record) override;

    /** @copydoc NetRelayTransportListener::onRelayClosed */
    void onRelayClosed(uint64_t peer, int32_t reason) override;

    /** @copydoc NetLinkTransportListener::onLinkReady */
    void onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity) override;

    /** @copydoc NetLinkTransportListener::onLinkControl */
    bool onLinkControl(uint64_t peer, const NetInterfaceFrame& frame) override;

    /** @copydoc NetLinkTransportListener::onLinkRecord */
    bool onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record) override;

    /** @copydoc NetLinkTransportListener::onLinkClosed */
    void onLinkClosed(uint64_t peer, int32_t reason) override;
};

// 单执行者内存文件后端，只接收当前案例的持久化工作并返回独立完成结果。
class PeerTestFileBackend : public NetFileIoBackend
{
public:
    // 最近分配的请求号。
    uint64_t next = 0;
    // 待消费结果，id 为零时为空。
    NetFileIoResult completion = {};
    // 是否已接纳停止通知。
    bool stopped = false;
    // 启用后返回协议以外的正值。
    bool invalidReply = false;
    // 当前用例拥有的查询探针，可空。
    PeerFileQueryProbe* m_query = nullptr;
    // 是否在查询回调退栈前收到过停止。
    bool stoppedInsideQuery = false;
    // 后端收到的停止次数。
    unsigned stopCalls = 0;
    // 下一次停止抛出异常。
    bool rejectNextStop = false;
    // 可空，记录本后端实际完成的释放次数，由用例持有。
    unsigned* m_releases = nullptr;
    // 模拟停止已接纳、工作线程仍在退出。
    bool m_deferDone = false;
    // 可空，由用例在析构时接收后端已收到的停止次数。
    unsigned* m_stopCallsOnDestruction = nullptr;
    // 下一次释放抛出异常。
    bool rejectNextRelease = false;
    // 启用目录缓存文件模式。
    bool cache = false;
    // 已成功提交的目录快照数。
    unsigned published = 0;
    // 该后端实例模拟的目标文件；消费成功的发布回报后才更新。
    std::vector<uint8_t> saved;
    // 本次接收的已写字节。
    std::vector<uint8_t> written;

public:
    /** 记录生命周期负责人最终销毁了后端 */
    ~PeerTestFileBackend() override;

    /** @copydoc NetFileIoBackend::init */
    int32_t init() override;

    /** @copydoc NetFileIoBackend::nextRequestId */
    int32_t nextRequestId(uint64_t& id) override;

    /** @copydoc NetFileIoBackend::submit */
    int32_t submit(const NetFileIoRequest& request) override;

    /** @copydoc NetFileIoBackend::cancel */
    int32_t cancel(uint64_t id) override;

    /** @copydoc NetFileIoBackend::peek */
    int32_t peek(NetFileIoResult& out) override;

    /** @copydoc NetFileIoBackend::consume */
    int32_t consume(uint64_t id) override;

    /** @copydoc NetFileIoBackend::stop */
    void stop() override;

    /** @copydoc NetFileIoBackend::done */
    bool done() const override;

    /** @copydoc NetFileIoBackend::uninit */
    void uninit() override;
};

// 作为业务握手对端，记录真实通道接到的配置和已发出的业务请求。
class PeerConfigChannel : public NetClientChannel
{
public:
    /** 创建借用服务端握手的测试通道
    @param [in] server 由用例持有，须覆盖通道生命周期
    */
    explicit PeerConfigChannel(NetInterfaceHandshake& server);

    /** @copydoc NetClientChannel::configureConnectionPools */
    int32_t configureConnectionPools(const NetConnectionPoolConfig& pools) override;

    /** @copydoc NetClientChannel::configureCompression */
    int32_t configureCompression(uint64_t peer, const uint32_t* types, uint32_t count, bool negotiated) override;

    /** @copydoc NetClientChannel::connect */
    int32_t connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& tls, NetTransportListener* listener, uint64_t& peer) override;

    /** @copydoc NetClientChannel::poll */
    bool poll() override;

    /** @copydoc NetClientChannel::send */
    int32_t send(uint64_t peer, const NetInterfaceFrame& frame) override;

    /** @copydoc NetClientChannel::outputState */
    int32_t outputState(uint64_t peer) const override;

    /** @copydoc NetClientChannel::close */
    void close(uint64_t peer, int32_t reason) override;

    /** @copydoc NetClientChannel::shutdown */
    void shutdown() override;

public:
    // 对端收到且正文正确的请求数。
    uint32_t m_requests = 0;
    // 对端收到且内容、序号都正确的 DATA 帧数。
    uint32_t m_dataFrames = 0;
    // DATA 交付尝试总数，包含通道背压时被拒绝的尝试。
    uint32_t m_dataOffers = 0;
    // 为 true 时拒收 DATA，直到用例解除背压。
    bool m_blockData = false;
    // 本场景期望的 DATA 长度；长正文的非首字节按固定独立模式校验。
    uint32_t m_dataSize = 1;
    // RPC 期望正文；默认仍为原有配置用例的单字节 73。
    std::vector<uint8_t> m_expectedRpc = std::vector<uint8_t>(1, 73);
    // 实际发送的合法 PING 数量及首次时刻，用真实单调时间检查期限。
    uint32_t m_pings = 0;
    uint64_t m_firstPingMs = 0;
    // 通道实际接到的池参数副本。
    NetConnectionPoolConfig m_received;
    // 协商成功后实际接到的公开命令白名单。
    std::vector<uint32_t> m_types;

private:
    // 用例持有的服务端握手对象。
    NetInterfaceHandshake& m_server;
    // 连接存活时借用的会话监听者。
    NetTransportListener* m_listener = nullptr;
    // 是否已报告本连接就绪。
    bool m_announced = false;
};
#pragma once

/*
NetWorkTest 测试文档

一、判定和运行约定
1. 用例实现、独立预期和结果必须分开。测试程序不能用被测编码器生成唯一的预期值，
   不能只统计总数而遗漏逐生产者顺序、正文、请求ID、代次和终态。
2. 瞬时、压力、指定、全量入口分别为--instant、--stress、--cases和--all；
   --list列当前稳定数字ID。--report=<CSV>写真实结果，PASS/FAIL/CANCELLED/NOT_RUN分开。
   超时只表示失败，未安装工具、缺候选产物、无实机不能算通过。
3. 所有输出必须使用新的独占目录。测试只拥有自己启动的进程，Windows先挂入独立Job
   再恢复主线程，超时后回收本Job的后代，不按进程名结束业务程序。
4. Native进程测试由配套候选SDK运行。正式网关是机器单例；检测到现存客户端、服务端
   或守护租约即拒绝运行，不替换用户已有实例。网络仅使用本机回环。
5. JS/Wasm测试保留JavaScript代码，独立执行入口位于本头文件同目录；开发工具模拟器
   通过不等同于官方微信工具或手机实机通过。未运行的平台只记录代码审查。

二、测试设施的独立验证
入口：NetWorkTest --runner-selfcheck --repository=<仓库绝对路径> --output=<新目录>。
900：完整比较三个NetSender的全部public声明，不以过期固定数量替代契约比较。
901：--list非空、严格递增、不重复；902：63,1,63只能执行1和63一次，输出顺序正确。
910—927：空选择、空项、倒置范围、非正数、非法后缀、未知ID、过大范围、重复选项、
   模式冲突、空报告路径及未知参数均返回2，不能执行业务测试或生成虚假PASS。
940：主动失败必须FAIL；941：超时必须FAIL；942：取消必须CANCELLED；
943：报告路径不可写返回3，不因行为完成就宣称整批成功。
903：CSV判定器用固定正反例验证引号、字段、时长、重复行、TOTAL算术及状态一致性。
960：真实子进程参数包含空串、空格、引号、反斜杠和UTF8，输出逐字节十六进制核对。
961：重复观察退出结果不变，禁止重用已启动对象；962：既有日志绝不能覆盖。
963：不存在的EXE不得留下已拥有PID；964：含NUL参数必须启动前拒绝。
965：等待超时与退出不同，随后只能终止自有进程；966：销毁父观察器须回收其作业内
   仍存活后代，十秒内退出且不能出现正常完成标记，不能仅凭退出码判断强制清理。
967：两个同名映像分别在两个Job；只能观察/终止自己的一个，错误映像或另一个Job
   的PID必须拒绝，外部作业仍正常存活。每个用例结束都回收自有内核句柄。
968：已经全部退出的作业与未启动对象必须区分；969：父进程正常返回0，但后代仍
   活着时不能宣称整棵进程树已退出。批次结束保留清理失败状态，再回收自有后代。
950—955：空选择、空项和未知套件不创建输出；缺Wasm/Luma候选记NOT_RUN，缺原生
   EXE记FAIL，均不得返回成功。--batch的原生报告还须与独立取得的--list逐ID一致，
   每项恰好PASS一次；空报告、少行、重复、非法编号、错误汇总都拒绝。

process_host_test.js：真实Node子进程验证半个PID不提前就绪、跨pipe回调的UTF8字符、
   晚安装统计观察器、分段统计、缺记录、非零退出、未终止行、非法JSON、重复统计、
   启动失败、旧日志拒绝覆盖、证据写入失败；账号适配器导入不依赖共享SDK，并从
   所选Account模块旁读取常量，拒绝缺失或无效元数据；平铺微信素材组装后逐文件核对，
   已有输出不得覆盖。统一十五秒期限，仅回收自建进程。
performance_assertions_test.js：固定合法记录与逐项篡改记录验证漏收、多收、错正文、
   生产者数量和分配错误、非法计时、分位数逆序、伪造吞吐以及非正TCP计数必被拒绝。
deadline_test.js：合法/非法期限、原异常保留、超时、显式取消、重复关闭、关闭后拒绝；
   旧Promise在超时后才完成也不能执行后续步骤。全局超时不能遗留仍会启动进程的任务。
peer_memory_test.js：分别在wasm32和wasm64的真实模块上，对postRequest、sendRequest、
   postData、sendData、连接配置及文件元数据各检查普通JS数组和Wasm堆借用视图。
   必须由公开调用内部的首次malloc触发真实内存增长；保存的输入包含NUL和非ASCII
   字节，要求C边界收到完整内容、原长度和精确uint64编号。底层桩固定返回拒绝，
   不冒充一次TLS成功；释放后无客户端及宿主残留。对应实际网络另由进程链路测试验证。
gateway_identity_process_test.js：Node独立验证证书链、hostname、节点CN及SHA256指纹；
   首次创建、重启不变、删除后新建、损坏拒绝且文件不被覆盖四项。就绪PID必须是
   本测试创建的进程，正常路径经停止文件退出，不能靠固定运行时间结束代替收尾。

三、真实常驻进程场景
入口：NetWorkTest --process-fixture=<场景> --bin=<候选目录> --output=<新目录>。
parser：十进制PID和端口的正反例，拒绝溢出、零、负数、尾随字符、缺值和重复字段。
replacement：实际业务端先完成65537字节请求回应，再正常停止并重新启动客户端网关；
   检查关闭事件、新代次和第二次完整请求回应。原生端再重复服务端网关替换场景。
   业务先正常停止，再停服务端网关；不能把同时停止造成的断链混入正常收尾判定。
singleton：服务端和客户端各自先启动八个复用进程，再等待全部退出。原实例仍就绪；
   客户端无参复用、服务端无参替换、客户端显式替换均核对实际PID。随后结束仅自有
   实例模拟崩溃，服务端重新取得租约，客户端由原守护恢复且没有守护链。停止文件
   必须让网关和守护都退出。默认5203端口占用时拒绝，不触碰已有监听者。
background：正常CreateProcess启动GUI，验证PE子系统并以Console负例检查判定器；
   子进程AttachConsole必须失败且返回ERROR_INVALID_HANDLE，枚举到唯一隐藏生命周期
   窗口，没有可见顶层窗口。网关和守护不同映像；只有网关登记ProcessWork和业务ready。
   八个无参/--background调用复用同一网关；两个业务程序完成实际通信并退出后仍驻留。
   两次崩溃注入均核对原守护、唯一新PID、IPC名称、新连接代次和实际业务回包；第一次
   在退避期立即触发SDK启动，不能覆盖原本连接配置。检查WM_QUERYENDSESSION、取消的
   WM_ENDSESSION以及确认的WM_ENDSESSION，正常结束超过首次退避期也不得重启。
   错误守护角色返回2；中文空格的相邻目录/平铺目录部署成功，同级正式网关优先于
   业务目录内的损坏旧副本；缺网关返回NOT_FOUND且PID为0，缺Guard不能发布ready。
   登录项仅操作HKCU Run中的NetClientManager值，先保存原始类型/字节证据，再检查
   带引号路径和--background、重复卸载、恢复原值；写注册表权限不足整组不得标PASS。
   用例期限包含就绪、关闭和回收；任何步骤失败均保留日志并回收自有Job。

四、多会话并发及性能计量
83：在独立测试进程执行8轮预热和64轮真实NetWork连接，交替正常关闭与回调抛错。
   保持预热后句柄零增长、每轮任务线程归零及72次关闭断言；独立进程避免其他IPC
   用例的后台句柄开闭污染进程总计，不能通过增加句柄容差掩盖泄漏。子进程日志并入
   本批，超时、异常退出、遗留后代、证据读取失败均失败。
244：八个TLS会话共用send的两条工作线程或post的一条工作线程。关闭一个或四个会话，
   剩余会话各提交十六个请求；分别核对每个请求ID只产生一个成功终态、每个客户端
   一次关闭回调及实际112/64个结果。不得以取消终态或别的会话多回包补足计数。
单向性能：逐次public调用提交正文，起点在首次加密前，终点是接收业务进程最后一次
   TLS解密并校验之后，同主机QPC。正文可为0、123456六字节、1024、65536字节。
   分别统计调用数、接收数、每生产者严格递增序号、正文总量、实际TCP上/下行字节；
   预热单独核对并排除，测量区间不能出现业务响应。不能用提交结束时间当接收完成。
RPC性能：发送与响应均逐请求校验，检查每生产者的精确分配、正文、方法、并发度、
   p50/p95/p99和总耗时；服务端回包数量必须等于预热加测量请求数。吞吐从数量和耗时
   重新计算，不信任被测程序单独打印的速率。小次数仅用于验证判定器，不能作为性能结论。
五、独立模块与跨语言用例
同层的独立模块入口由NetWorkTestModules.vcxproj分别编译为EXE，不把多个main或全局分配故障注入并入主EXE。
module-crypto：密码派生、随机、证书链/域名/角色校验和签发边界；核对已知向量、非法
   输入和资源释放。同层NetTestTls文件提供证书测试素材，仅用于本机测试，不能部署为生产身份。
module-proto：protobuf固定格式、非法字段、Variant所有权、深度和容量，分别检查
   完整解析及拒绝后对象状态。编码器不能生成自己的唯一预期答案。
module-interface：独立消费者覆盖线格式和密码/消息层，检查导出、依赖和释放边界。
module-sdk：三个NetSender的C++接口及Peer/Work的纯C翻译单元分别消费对应API入口。
   一组DLL通过不能证明其他位数、CRT或静态组合通过；SDK头和库必须同批。
module-tls：握手、错证书、篡改、重放、EOF/期限、随机源、有界传输、记录合并、
   KDF、紧凑协议和提交池。十二组公开接口检查独立执行，异常由main转换为失败。
module-protobuf：四种消息、类型/边界、截断/变异、失败清理、分配故障和只读并发。
   此EXE替换自身new/delete做故障注入，禁止链接入产品或其他测试可执行文件。
module-wire：四种结构各256条确定性protobuf样本，与另一版本独立生成的基准文件
   逐字节比较；变异、截断、尾随字节或空基准都不得通过。只有输出参数表示生成样本，
   不代表兼容通过；批次没有--wire-baseline则明确NOT_RUN。

history：真实管理TLS连续处理8193个拒绝OPEN，超过历史缓存边界；陈旧CLOSE、信用
   及事件不杀死共享物理连接，未来未分配通道仍须拒绝。逐批核对全部响应，保持同一
   管理连接；推进器须消费TLS已有明文，不能仅因socket输入队列为空就停止推进。
tls/link/service/internal_service/managed_service：分别验证管理/业务TLS、认证角色、
   服务注册、内部产品0和可信根上下文，不让公网角色伪装内部服务。
compression/mesh：只允许协商后的公开消息压缩；跨节点转发不能改变端到端正文。
slow_channel/service_capacity/managed_cancel/channel_capacity/window_rtt：真实慢消费、
   池耗尽/回滚、取消、逐通道隔离和窗口恢复；显式配置的小池只属于该测试场景。
local/sender-facade/legacy-desktop/legacy-file：桌面IPC到客户端网关、服务端网关及
   独立业务进程的完整链路，覆盖共同门面、上传下载、背压、关闭及文件校验。
recovery-entry/recovery-service/recovery-budget/balanced-failover/cluster：入口断开、
   业务服务退出、整个节点消失、静默断网和连续集群变更；检查新TLS代次、迁移分散、
   健康连接保持、原请求唯一终态及不重放写操作。总条数不能替代各客户端状态核对。
directory-cache：认证目录的版本、TTL、非法缓存、候选更新、原子保存及失败后恢复。

peer-contract/peer-memory/peer-exchange：真实NetPeer32/64位模块，验证共同C接口、
   内存增长前后借用输入、三类平台号及真实TCP/IPC往返。uint64编号不得转成Number。
peer-host：真实编译模块配合受控socket宿主，验证部分接纳、旧代回调、重入关闭、
   随机源、固定回调桥及待发送缓冲；桩只能证明宿主契约，不能冒充真实网络性能。
peer-file-host/file-host：文件分块、顺序、重试、取消、短读写、异步回调和销毁；
   文件宿主只提供I/O，摘要、授权令牌及协议推进仍使用实际C++实现。
sender-delivery：事件循环持续推进时检查post接纳与send Promise交付、票据、正文
   副本、超时及取消。NetPeer场景与NetWork的并行send执行模型分别判定。
schedule/recovery-callback：定时推进的重入/停止、错误回调和重连不遗留旧任务。
wx-host：Node模拟微信socket/随机/时钟，运行NetWxSampleApp.js及真实NetPeer模块，
   核对固定用例400/401、TLS及二进制请求；wx是官方微信工具批次，缺工具即NOT_RUN。
account-bridge/luma-account/luma-server：需要独立Account模块、配套业务候选及显式
   本机MySQL测试配置。每轮创建独立测试库；禁止访问生产数据。缺依赖记NOT_RUN，
   不能用替身宣称账号授权、VIP或Qt界面已通过。
   luma-account使用测试专用protocol-module生成密码验证参数，Account常量必须与
   --account-module同目录；account-bridge使用生产NetPeer模块验证双模块调度。
wasm：同一C++可移植用例集分别在wasm32/wasm64执行；先取--list，再核对CSV完整性。
   模块为测试专用NetWorkWasmTest，不等同于已支持NetWork多线程Wasm产品。

六、构建与复现
1. Windows先用version准备匹配依赖，再用VS/MSBuild编译NetWorkTest.vcxproj。独立
   模块使用NetWorkTestModules.vcxproj，/p:TestModule=crypto|proto|interface|sdk|tls|
   protobuf|wire，并指定/p:TestSdk=<匹配SDK根>及Configuration、Platform。
   每个目标独立中间目录，正式网关程序和测试EXE不能混用其他批次的头或基础库。
2. Wasm测试构建由原生NetWorkTest --build-tests完成，参数为：
   --target=protocol或wasm-tests --bits=32或64 --sdk=<对应Wasm SDK>
   --headers=<与归档同批的私有C++头根> --emscripten=<官方工具链目录>
   --python=<官方编译器解释器> --repository=<仓库根> --output=<全新目录>。
   这是C++构建驱动调用官方emcc，不保留Python测试代码；不修改或编译生产依赖。
   SDK发布的JS/JSON不代替私有C++头，尤其不能把新.a与旧基础库头混用。
   980—987验证空参数、未知/重复选项、空路径和缺参数拒绝；真实编译另外检查32/64。
3. 非Windows测试构建入口为本目录CMakeLists.txt；原生传BIT、DEBUGRELEASE、SDK_ROOT
   和LIBRARIESRELY；Wasm使用Emscripten工具链、BIT=wasm32/wasm64、SDK_ROOT及TEST_HEADERS。
   未在对应系统实际构建运行只能记录代码审查，不能把Windows结果当作跨平台结果。
4. 统一批次：NetWorkTest --batch --bin=<候选EXE目录> --module=<NetPeerAPI.js>
   --protocol-module=<NetProtocolTest.js> --node=<Node程序> --repository=<仓库根>
   --output=<全新目录> --only=<逗号分隔套件>。wasm另传--wasm-tests=<NetWorkWasmTest.js>，
   独立模块可传--modules-bin及--wire-baseline。未选择的测试不等于通过。
5. 全部测试源码和必要素材平铺在本目录，按文件名划分模块，不再新建源码子目录。
   NetWxSampleApp/NetWxSamplePage为微信样例；官方wx打包时仅在输出目录组装app/pages，
   生成test-config及复制NetPeer模块，源码目录不保存生成Wasm或运行数据。
6. version_dependencies_test.js参数为仓库根、Bash绝对路径及新输出目录；运行真实
   version脚本、替换构建器记录参数，核对14份脚本的默认/32/64选择、dll/lib和失败
   传播；不编译业务产品，也不把命令验证冒充产品编译成功。
7. 性能JS分别为gateway_oneway_performance_test.js与gateway_performance_test.js。
   必须使用相同编译器、SDK、正文、计数、池、会话/线程数及CPU策略做交替对照，
   报告全部轮次、单位、分位数、上下行实际字节及变化百分比。RPC窗口为显式测试
   负载设置，不能拿受窗口约束的RPC/s与无回复的DATA/s或LibuvTcp裸传输直接比较。
   单向大正文默认最多256条；长测显式传--large-count=4096等实际条数（8至100000），
   同时设置--count不小于它。报告必须记录实际条数，不能把短测当作稳定吞吐。
8. 原生稳定ID全集以--list为准，对应实现和独立断言在同目录各Test/Cases文件；
   本头描述共用前提和风险分组。--batch核对真实目录，不能漏项仍宣称全量通过。
9. service默认使用--client-host隔离客户端租约；mesh显式设每服务8个实际会话池槽，
   核对正反两条路由落到不同服务，不能依赖生产默认容量。真实单例和Guard仍由
   singleton/replacement/client-background独立测试，不以隔离宿主代替。
   输出、旧版本备份、生成JS/Wasm、编译日志、CSV及性能结果全部放调用者的新临时
   目录；测试源码工程只保留代码、构建入口和必要的确定性素材。
*/

/** 由runner执行期限与取消检查，失败抛出测试异常
*/
void netTestCheckpoint();
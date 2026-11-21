#pragma once

/** Windows 线格式用例通过 SDK 中的 ProtoMessage.pb.h 创建和读取协议样本。版本脚本准备 ProtoMessageManager 静态库，生成类型的实现由该库提供。PlatformTest 使用同配置的动态库。
*/

/** 功能用例与独立预期
无参数运行全部功能组；--list 列出可选ID，--case=<ID>只运行该组及其素材准备。未知ID或选项退出2。
containers：检查通用消息的链式字段、二进制赋值、分组、深拷贝、读借用、清空、移动、错误码。对专用Record、Array、Table验证容量和维度边界，失败时保留原字段或表结构。
wire：四种实际类经工厂构造并释放；使用生成的protobuf类型独立创建及解析字节。覆盖数值边界、浮点位型、嵌入NUL、逐字节截断、packed与mixed表示、重复字段、冲突形状、短输出缓冲、表形状及失败恢复。对确定性输入变异10000次，成功必须可重编解码且保持行列，失败必须清空行。
allocation：遍历四类消息及120个分配失效位置，检查编码输出回滚、解析失败关闭以及后续重试。
concurrent：五个原生端使用四个线程只读同一个已发布的32字段记录，各自编码、解析1000次，检查无错误。单线程Wasm构建可选择前三组。
*/

/** ProtoMessageManager测试工程。
平台范围 Windows、Linux、macOS、Android、iOS、Wasm。
运行对应测试程序，返回0表示通过，非0表示失败。
Wasm 的两个位宽分别以 BIT=wasm32 和 BIT=wasm64 构建。启用 LIBRARY_TEST_HOST 后用 ctest 在 Node 执行。
浏览器通过 HTTP 打开构建目录的 LibraryTestHost.html?test=ProtoMessageManagerTest。
微信将 ProtoMessageManagerTest.js、ProtoMessageManagerTest.wasm 和 LibraryTestHost.js 放入分包，调用
await require('./LibraryTestHost')(require('./ProtoMessageManagerTest'), { wasmPath: '包内路径/ProtoMessageManagerTest.wasm' })。
运行 wasm64 的宿主必须支持 Memory64。
*/

/** 性能用例
运行本工程时增加 --benchmark 参数，使用公开接口执行独立批次并验证结果。计量单位为纳秒/操作。
先自动预热并校准批次大小，再输出九轮数据、中位数、第一及第三四分位数。
*/

/** 当前实现已有性能数据（采集于2026-11-22）
每条 Record 有 32 个 int32 字段，编码为 259 字节。编码重复生成同一记录，解码从同一字节串恢复记录并检查每项值。
Windows x64 Release，Intel Core i9-14900HX，VS2013 v120 /MT。两组各先预热一个进程，再顺序运行七个独立进程。每个进程自行预热、校准批量并采九批，用各批操作数归一化。单位为微秒/次，两列是在同一逻辑 CPU 6 上依次执行的独立批次，各列给出七个进程中位数的中位数 [最小值, 最大值]。
  Record 编码    6.771 [6.720, 6.876]    6.771 [6.542, 7.067]
  Record 解码    13.569 [13.322, 13.752]    13.635 [13.444, 14.024]
跨进程范围反映宿主调度与负载对该场景的影响。复测应使用同一输入、构建配置、宿主及 CPU 亲和性。
Wasm Release 使用 Emscripten 3.1.74。Node 24.0.0、Chrome 157 和微信开发者工具基础库 3.17.4 分宿主顺序采样。每格为一次宿主运行内九批的中位数 [Q1, Q3]，单位微秒/次。各列依次为Record 编码、Record 解码。
  Node wasm32    4.386 [4.346, 4.435]    14.365 [14.212, 14.555]
  Node wasm64    7.386 [7.264, 7.662]    38.361 [37.918, 38.866]
  Chrome wasm32    9.863 [9.766, 9.863]    42.383 [41.992, 43.359]
  Chrome wasm64    15.820 [14.648, 15.967]    61.133 [56.836, 61.523]
  微信开发者工具 wasm32    13.818 [13.623, 14.160]    42.969 [42.773, 43.359]
固定逻辑处理器 6，另用五个独立 Node 进程分别预热、校准并采九批。以下各列仍按上述接口顺序，单位微秒/次，报告进程中位数的中位数 [最小值, 最大值]。
  Node wasm32    4.386 [4.291, 4.546]    14.334 [14.265, 14.576]
  Node wasm64    7.095 [7.031, 7.368]    39.036 [38.601, 45.517]
Chrome 和微信的数据缺少独立宿主进程复测。微信开发者工具强制 wasm64 时实例化失败。Linux、macOS、Android、iOS、微信手机及原生 Windows x86 缺少同口径的性能数据。
分发的 C++ 运行库及 protoc 为 3.3.0。before 按运行构建脚本的宿主选择 Windows x86、Linux x64 或 Linux arm64 的 protoc，也可以设置 PROTOC_EXECUTABLE，脚本核对 --version。其他宿主可使用仓库已有的匹配生成文件。
*/

/** 当前功能组的稳定数字编号。 */
enum ProtoMessageManagerTestId
{
    // 容器与数据访问
    ProtoMessageManagerTestContainers = 1,
    // 二进制线格式及变异回读
    ProtoMessageManagerTestWire = 2,
    // 逐点分配失败及回滚
    ProtoMessageManagerTestAllocation = 3,
    // 独立线程并发读取
    ProtoMessageManagerTestConcurrent = 4,
};

/** 指定测试可用 --cases 1 或 --cases 1,2-3。--instant、--stress 和 --all 按当前平台选组。
1 containers 容器与数据访问（瞬时）。
2 wire 二进制线格式及变异回读（压力）。
3 allocation 逐点分配失败及回滚（压力）。
4 concurrent 独立线程并发读取（压力），仅原生端。
没有适用压力项时 --stress 返回 2；沿用 --case=英文名 和 --list。
*/

/** 当前版本功能验证（2026-11-22）
完整功能套件在Windows的Debug和Release分别完成42093、42080条断言，两个Wasm位宽均为42078条；分配失败用例的检查次数随平台和构建配置而异。
Windows x86、x64 的 Debug、Release 四种功能测试均通过。公共 x64 SDK 的 Debug、Release 消费者分别通过。
Node 和 Chrome 的 Wasm32、Wasm64 均通过，Chrome 自动选择 64 位。微信开发者工具（基础库 3.17.4）自动选 32 位，强制 32 位通过；强制 64 位因宿主不支持 Memory64 在实例化时失败。微信手机未实测。
Linux x64 Release 已交叉编译、链接，未在 Linux 运行。macOS、Android、iOS 仅检查平台分支和构建源码。主类示例在 Windows x64 Release 与 Node 两个位宽运行成功。
最终构建链接同批 Variant；原生端执行并发读取，单线程 Wasm 执行容器、线格式和分配失败回滚。
数字选择、重复或非法范围及适用的分组与兼容入口累计 59 次，退出码符合预期。当前构建的测量结果和波动范围见性能章节。
*/
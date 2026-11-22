#pragma once

/** Windows 线格式用例通过 SDK 中的 ProtoMessage.pb.h 创建和读取协议样本。版本脚本准备 ProtoMessageManager 静态库，生成类型的实现由该库提供。PlatformTest 使用同配置的动态库。
*/

/** 功能用例与独立预期
无参数运行全部功能组；--list 列出可选ID，--cases 1 选择数字编号，--case=containers 等英文名只运行对应组及其素材准备。未知ID或选项退出2。
containers：检查通用消息的链式字段、二进制赋值、分组、深拷贝、读借用、清空、移动、错误码。缺少分组时三个复制接口须保留调用方输出。对专用Record、Array、Table验证字段覆盖、自身元素追加、容量和维度边界，失败时保留原字段或表结构。
wire：四种实际类经工厂构造并释放；使用生成的protobuf类型独立创建及解析字节。覆盖数值边界、浮点位型、嵌入NUL、逐字节截断、packed与mixed表示、复用解码临时容器时的INIT值、通用消息的非全局列表与非规则表（空行、嵌入NUL、不同类型）、显式响应码与错误文本、重复字段、冲突形状、空输出指针与零长度正文、短输出缓冲、表形状、表的独立复制及失败恢复。对确定性输入变异10000次，成功必须可重编解码且保持行列、所有单元类型及完整字节，失败必须清空行。
allocation：原生端先验证故障预算只作用于设置它的线程。遍历四类消息及120个分配失效位置，每种结构都必须观察到编码和解析各至少一次真实失败。编码保留旧输出；解析失败清除内容，成功和重试均恢复原字段的类型及完整字节。另对通用列表的显式追加、链式头插和尾插逐点注入：失败必须保留原有列表或分组缺失状态，重试只增加一个元素。
concurrent：五个原生端使用四个线程只读同一个已发布的32字段记录，各自编码、解析1000次，检查字段数及每一个键值。单线程Wasm构建可选择前三组。
*/

/** ProtoMessageManager测试工程。
平台范围 Windows、Linux、macOS、Android、iOS、Wasm。
运行对应测试程序，返回0表示通过，非0表示失败。
Wasm 的两个位宽分别以 BIT=wasm32 和 BIT=wasm64 构建。启用 LIBRARY_TEST_HOST 后用 ctest 在 Node 执行。
浏览器通过 HTTP 打开构建目录的 LibraryTestHost.html?test=ProtoMessageManagerTest。
微信将 ProtoMessageManagerTest.js、ProtoMessageManagerTest.wasm 和 LibraryTestHost.js 放入分包，调用
await require('./LibraryTestHost')(require('./ProtoMessageManagerTest'), { wasmPath: '包内路径/ProtoMessageManagerTest.wasm' })。
运行 wasm64 的宿主必须支持 Memory64。
Linux ARM32 的发布归档由 Arm GNU Toolchain 13.3.Rel1 构建，目标为 armv7 hard-float。ProtoMessageManager 与归档应采用相同的 C++ 参数传递约定。更换工具链后，运行 NetInterfaceTest --cases 4 检验字符串字段的编码及解码。
Windows、Linux x86/x64、wasm32/64 使用 protobuf-lite 3.3.0。Linux ARMv7 hard-float（arm32）和 AArch64（arm64）使用 3.19.3；两种位宽分别提供可链接静态程序及共享库的 PIC 归档。before 在当前 schema 与已记录的 SHA-256 匹配时，将随仓的 3.19.3 生成文件发布到目标 SDK 的 include/ProtoMessageManager/protoc-3.19.3。更新 schema 时，用 PROTOC_EXECUTABLE 指定宿主可执行的 protoc 3.19.3；显式指定的版本不匹配会报错。
Android、macOS、iOS 需指定目标平台及 ABI，提供对应 protobuf 的头文件和静态归档。若 tools 尚无该目标的归档，将匹配的头和 libprotobuf-lite.a 分别放入专用 CLOUD_REBUILD_SDK_ROOT 的 include/protobuf 和 lib。每个平台、位宽及配置使用独立 SDK。
*/

/** 性能用例
运行本工程时增加 --benchmark 参数，使用公开接口执行独立批次并验证结果。计量单位为纳秒/操作。
先在计时区外逐字段核对固定输入，再预热并校准批次大小，输出九轮数据、中位数、第一及第三四分位数。containers 也核对这份基准输入，仅执行正确性检查。
*/

/** 已有 Record 基准（2026-11-22，同一 32 字段负载）
每条 Record 有 32 个 int32 字段，编码为 259 字节。计时区内反复调用 toString 或 from，包含测试进程分配器的开销、返回值检查以及结果字节数或字段数的累计。逐字段校验在计时区外执行。
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
本轮未运行 --benchmark。上述数字来自已有采样；当前重新构建的二进制缺少性能测试数据。相同宿主和位宽下，这组小整数的解码耗时高于编码。General、Array、Table 的编解码、Record 其他字段数及大 BLOB、复制、移动、预留、findValue、at、setBytes、serializedSize 和 serializeTo 均缺少同口径数据，不能按表内数字外推。kind、size、rows、columns、类型投影及 release(nullptr) 只查询状态或处理空指针，确实无需单列微基准。
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

/** 本批功能验证（北京时间 2026-11-22 16:27）
共 4 个稳定编号。Windows 每个配置执行 1 个瞬时组及 3 个压力组；单线程 Wasm 每个配置执行 1 个瞬时组及 2 个压力组。以下为无参数运行的组内检查数。
Windows VS2013 v120：x86、x64 Release 各完成 47450 项检查，Debug 各完成 47462 项，均为 0 项失败。线格式及白盒用例链接生产静态库；故障注入的检查次数随运行库实际分配行为变化。
Emscripten 3.1.74、Node 24.0.0：wasm32、wasm64 的 Debug 和 Release 各完成 47436 项检查，均为 0 项失败。
以上八个配置分别运行默认、--instant、--stress、--all、--cases 1,2-3、--case=wire 及 --list。逐行核对了 --list 的数字编号、组名和分类。未知编号及倒置区间由 C++ 返回 2，Node 宿主报告对应退出码并以非零状态退出。
从 ProtoMessageManager.cpp 注释提取的 main 示例在 Windows 两个位宽的 Debug/Release 静态库及 DLL、Node 两个位宽的 Debug/Release 均运行通过；输出包含四类消息的实际字段、行数或字节数。
Chrome 157.0.8103.80 Release 测试包强制 wasm32、强制 wasm64、自动选择 wasm64 均通过。微信开发者工具基础库配置 3.17.4，页面回执确认强制 wasm32 和自动选择 wasm32 均通过；强制 wasm64 在 WebAssembly.instantiate 阶段报 CompileError（invalid table elements limits flags），C++ 用例未启动。
Linux x86 系列、macOS、Android、iOS 完成源码、依赖、配置及部署入口审查，未在目标平台运行。微信手机未实测。本批仅验证基准素材的正确性，未执行 --benchmark。
本批相对构建路径与执行文件 SHA-256
native32Debug/lib/ProtoMessageManagerTestd.exe 452cd27445881b4f3131d9468ba00a5386776041fc2c38913a6061f4bdb27da5
native32Release/lib/ProtoMessageManagerTest.exe e5609bbebd69a2c86300f2ca07820c7eb6edc670d8d8efc0d76181930dee2741
native64Debug/lib/ProtoMessageManagerTestd.exe 93e2862edc8c79c126e6a365b0bb1d8e498c776d3b7096d908f6a05f5091ac5e
native64Release/lib/ProtoMessageManagerTest.exe 817294111b02b75eece193520f39c9d0ff742c923ae268e0f97e083c2d74d0c2
wasm32Debug/test/ProtoMessageManagerTest.wasm f5452d18f2459d83018ad94f3694151968ce8a538a240f50a711a630224e1939
wasm32Release/test/ProtoMessageManagerTest.wasm 65823752110c9f0a44a900804703bf6d6ae5fbf8ba76e7f965985171c2050b99
wasm64Debug/test/ProtoMessageManagerTest.wasm 73aa7c72eda6c5ab60b9fac6562fb8c312b0bf5d0c4fcdf0618d29f219d94145
wasm64Release/test/ProtoMessageManagerTest.wasm 0735bd9c126969066e61575cde59c8cd8102504b24af01e9ce1b9e3d90300db9
*/

/** Linux ARM 功能验证，2026-11-22
ARMv7 hard-float（arm32）及 AArch64（arm64）分别交叉构建 Debug、Release 静态库和共享库，使用 Clang 19.1.7、Debian 12 sysroot 及对应的 protobuf-lite 3.19.3 PIC 归档。在 QEMU 的 Debian 6.1.0-50 内核中，四个配置的静态库测试程序各执行 4 个功能组、47438 项检查，均为 0 失败。
四个配置均另行运行从 ProtoMessageManager.cpp 提取、动态链接两库的公开示例。Record 返回 user=10001，Array 返回 pages=3，Table 返回两行，General 返回 code=0，全部退出 0。没有测量 ARM 性能，未在物理开发板运行。
本批 ProtoMessageManagerTest ELF 的 SHA-256
    armhf Release aeb3cd4451039b825e8af2846bf39d470f8d519924d9c2ab453a56a00eeaac74
    armhf Debug 9b37468839d73bac238ac38926d8454b7617e51b1db1cc9147d78fbca33718ad
    arm64 Release 4ba107d6aa591b07ad5089887496f5b86e013aeb401d6afeac27ab18e9a83d2b
    arm64 Debug 8901233e0d6a76d27c8f4b6b54ca11bd6adfafbb4e83abf662970662530d3194
*/
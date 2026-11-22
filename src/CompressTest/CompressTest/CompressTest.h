#pragma once

/** 功能用例、输入和独立预期
1 raw-levels（压力）　重复字节和固定种子的伪随机输入各取 0、1、4096、131072 字节，逐个调用 0 至 9 级。检查压缩上界、标准魔数、校验位、逐字节还原和两项实际长度。每级重新填充目标，防止读到上轮结果；零长度和非空输入均检查压缩、解压目标末端哨兵。
2 raw-frames（瞬时）　覆盖上界溢出、非法等级、空指针、过短目标、长度变量别名。参数错误或空间不足时要得到对应错误码且长度归零。独立构造的标准块须解出 hello，尾随字节和拼接帧只消费首帧。截断、破坏魔数、翻转校验和、其他压缩格式和可跳过帧均须返回数据错误。
3 wrapped（瞬时）　检查本机序四字节原长、带头压缩上界、短缓冲区及边界哨兵。按值、独立目标、原地及带限额重载应还原含 NUL 的 5 字节正文。原地压缩的中间结果须可独立解码，长度头须与本机序的原长逐字节相符。覆盖精确限额、少一字节、0、-1 和非法负数，以及声明长度通过预算检查但实际流输出多一字节的情况。损坏校验和、伪造内外长度、截断头、附加帧和尾字节均返回空结果或零长度。独立的无内容长度 RLE 帧须还原为 65537 个 Z；262145 字节已知原长帧在新目标和复用目标上均须完整还原。
输入由 C++ 用例创建并在本组返回时释放。组内失败时打印检查位置并返回 1，无效命令行返回 2。无参数运行全部功能组；--list 列出名称，--case=wrapped 按名称执行，--cases 1,2-3 按数字或闭区间选择；--instant、--stress、--all 分别执行瞬时、压力或全部功能组。全量顺序为 2、3、1，指定数字范围去重后按编号升序。性能另用 --benchmark。
*/

/** 通过 C 入口同步执行三个功能组，Wasm 宿主也可直接调用
@return 全部检查通过为0，功能检查失败为1
*/
extern "C" int runCompressTests();

/** Windows 入口
在 src/CompressTest 下执行 version_debug.sh（Win32 DLL）或 version_release.sh（x64 静态库）准备 Compress 和 PlatformTest，再用 VS2013 构建 CompressTest.sln 的相应配置。测试与依赖应选相同位宽、Debug/Release 和 CRT。从测试程序目录运行 CompressTest.exe 或 CompressTestd.exe 并检查退出码。
*/

/** Wasm 宿主测试入口
用与 SDK 匹配的 Emscripten 工具链配置本工程，指定 -DBIT=wasm32 或 wasm64、
-DSDK_ROOT=<对应 SDK>、-DLIBRARY_TEST_HOST=ON，构建后运行 ctest --output-on-failure 可验证 Node。
浏览器通过 HTTP 打开构建目录的 LibraryTestHost.html?test=CompressTest。
微信把生成的 CompressTest.js、CompressTest.wasm、LibraryTestHost.js 放入同一分包，调用
await require('./LibraryTestHost')(require('./CompressTest'), { wasmPath: '包内路径/CompressTest.wasm' })。
内存位宽由宿主实际支持范围决定，实例化失败会报告失败。
*/

/** Linux 与 macOS 的 CMake 入口
从本工程目录执行 cmake -S . -B build -DSDK_ROOT=<当前系统及架构的SDK> -DBIT=<64或arm64> -DCMAKE_BUILD_TYPE=Release，
执行 cmake --build build，再在 build 内运行 ctest --output-on-failure。交叉编译时需另传目标工具链；
只有与目标 ABI 匹配的系统才能直接运行产物。
*/

/** 原生移动端测试
Android 用对应 ABI 的原生 SDK 与 NDK 工具链构建，adb 连接设备后运行 ctest --output-on-failure。
CTest 在设备的独占私有临时目录执行并读取 C++ 退出码。可用 PLATFORM_TEST_DEVICE 指定序列号；
SDK 外的共享运行库（例如 libc++_shared.so）用 PLATFORM_TEST_RUNTIME_LIBRARIES 指定绝对文件路径。
iOS 用 Xcode 生成器、CMAKE_SYSTEM_NAME=iOS 和 iphoneos 或 iphonesimulator 的同架构 SDK 构建。
模拟器启动后运行 ctest -C Release --output-on-failure。真机配置签名开发团队和 PLATFORM_TEST_DEVICE
为设备 UDID 后运行同一命令（需要 devicectl），也可以在 Xcode 运行并读取屏幕的 PASS/FAIL 和退出码。
每次运行拥有自己的可写临时目录，结束后清理本次文件。
从本测试工程的 CMakeLists.txt 所在目录配置，以下命令以 arm64 为例：
cmake -S . -B build-android -DCMAKE_TOOLCHAIN_FILE=<NDK>/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DBIT=arm64 -DSDK_ROOT=<Android目标SDK> -DCMAKE_BUILD_TYPE=Release
cmake -S . -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator -DCMAKE_OSX_ARCHITECTURES=arm64 -DBIT=arm64 -DSDK_ROOT=<iOS模拟器目标SDK> -DCMAKE_BUILD_TYPE=Release
使用真机时将 iphonesimulator 改为 iphoneos，传入该设备 SDK 与 -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=<团队标识>。
选定构建目录执行 cmake --build . --config Release，再执行 ctest -C Release --output-on-failure。
*/

/** 性能用例与数据
--benchmark 使用 PlatformTest 单调时钟，分别预热并校准每批操作数（目标至少约 20 毫秒），保留九批原始值及中位数、Q1、Q3。程序输出纳秒/次，下表换算为微秒/次。固定输入在计时外准备，两种解压使用独立帧。计时包含接口调用及输出长度累计，逐字节核验在计时外。比较两次运行时固定宿主、CPU 亲和性、构建和负载，保留慢轮与异常，使用多个独立进程的中位数和范围。
已有字符串接口测量（2026-11-22），本次未运行性能测试。以下是相同负载及公开接口的已有记录，本次重建的二进制缺少性能测试数据。
输入为 65,536 字节，字节 i 取 (17×i + i/113 取整 + i/1024 取整) 对 127 的余数。Zstandard 3 级生成 1,845 字节，复用目标字符串容量，解压后逐字节检查。限额解压使用独立的10字节无内容长度RLE帧及4字节外层原长头，解为65537个Z。每次调用传入精确限额，批次验证总长并在计时外逐字节校验。前两项使用同一份原文及其压缩结果，限额解压使用另一份输入。容量估算与参数合法性无须单独计时。
Windows x64 Release，Intel Core i9-14900HX，VS2013 v120 /MT。两组各先预热一个进程，再顺序运行七个独立进程。每个进程自行预热、校准批量并采九批，用各批操作数归一化。单位为微秒/次，两列是在同一逻辑 CPU 6 上依次执行的独立批次，各列给出七个进程中位数的中位数 [最小值, 最大值]。
  compress（Zstandard 3 级）    36.830 [36.750, 38.406]    37.112 [36.292, 43.600]
  uncompress（还原至 64 KiB）    18.996 [18.782, 19.974]    19.152 [18.819, 19.927]
  限额解压（无原长 RLE，65537 字节）    3.721 [3.464, 4.307]    3.620 [3.348, 3.807]
跨进程范围反映宿主调度与负载对该场景的影响。复测应使用同一输入、构建配置、宿主及 CPU 亲和性。
Wasm Release 使用 Emscripten 3.1.74。Node 24.0.0、Chrome 157 和微信开发者工具基础库 3.17.4 分宿主顺序采样。每格为一次宿主运行内九批的中位数 [Q1, Q3]，单位微秒/次。各列依次为compress（Zstandard 3 级）、uncompress（还原至 64 KiB）、限额解压（无原长 RLE，65537 字节）。
  Node wasm32    79.134 [77.796, 84.034]    20.556 [20.238, 21.416]    6.001 [5.889, 6.055]
  Node wasm64    71.791 [71.248, 72.839]    24.004 [23.752, 24.352]    3.335 [3.222, 3.346]
  Chrome wasm32    141.406 [126.172, 154.688]    42.969 [32.227, 46.094]    10.522 [9.351, 10.547]
  Chrome wasm64    164.844 [155.469, 168.359]    54.492 [54.102, 54.883]    5.493 [5.469, 5.542]
  微信开发者工具 wasm32    1,384.375 [851.562, 2,184.375]    122.266 [117.578, 386.719]    13.232 [13.037, 13.428]
固定逻辑处理器 6，另用五个独立 Node 进程分别预热、校准并采九批。以下各列仍按上述接口顺序，单位微秒/次，报告进程中位数的中位数 [最小值, 最大值]。
  Node wasm32    78.741 [76.460, 100.344]    19.832 [19.607, 22.327]    5.620 [5.219, 6.298]
  Node wasm64    74.027 [72.304, 75.000]    23.969 [23.596, 24.388]    3.219 [2.930, 3.311]
Chrome 和微信的数据缺少独立宿主进程复测。微信开发者工具强制 wasm64 时实例化失败。Linux、macOS、Android、iOS、微信手机及原生 Windows x86 缺少同口径的性能数据。
在 Windows x64 的固定 64 KiB 输入上，3 级压缩每次约 37 微秒，解压约 19 微秒。限额解压的 RLE 帧与前两项的编码和内容不同，不能据此推导限额检查带来了加速。微信开发者工具的压缩样本分散，单次宿主结果不足以作为稳定预算。
rawCompress、rawUncompress、char 缓冲区重载、按值返回、字符串原地操作、冷启动及其他等级和数据规模缺少单独的性能测试数据。三个容量接口只做常数规模的计算或读取，无须单列吞吐。
*/

/** 当前功能组的稳定数字编号。 */
enum CompressTestId
{
    // 压缩等级和长度组合
    CompressTestRawLevels = 1,
    // 原始帧边界和损坏输入
    CompressTestRawFrames = 2,
    // 封装帧、容量与别名
    CompressTestWrapped = 3,
};

// 同一登记项供名称查询、PlatformTest 数字选择与 C 入口调度使用
struct CompressTestCase
{
    // PlatformTest 选择器要求的稳定编号
    unsigned id;
    // PlatformTest 选择器要求的压力组标记
    bool stress;
    // --list 和 --case 共用的命令行名称
    const char* m_name;
    // 同步执行该组并累加检查数，返回0表示通过
    int (*m_run)(int& checks);
};

/** 本轮功能实测（2026-11-22，北京时间 08:12 批次）
Windows VS2013 分别使用 x86、x64 Debug DLL（/MDd）和 x86、x64 Release 静态库（/MT）。Node 上使用 Emscripten 3.1.74 构建 wasm32、wasm64 Release 静态库。六个配置均从源码重建。每个配置的 ID 1、2、3 分别通过 328、20、35 项检查，合计 3 组、383 项，通过 3 组、失败 0 组。
每个配置核对 14 种 CLI 调用，包括默认、列名、按名称选择三个单组、全量、瞬时、压力、重复数字范围和五个非法选择。--all 按 2、3、1 执行，--cases 3,1-2,2 按 1、2、3 各执行一次。非法选择在原生 CLI 返回 2；Node 宿主将 C++ 非零退出转换为宿主失败。Wasm32 和 Wasm64 均使用 Windows 反斜杠 SDK 路径成功链接并通过 CTest。
测试设施反例（Windows x64 Release）：在独立的测试副本模拟跳过两次原地操作，原地压缩中间结果检查检出错误。第 3 组返回 1，第 1 组随后正常执行，全量退出码为 1。
直接从 Compress.cpp 末尾逐行注释提取 demo，在 Windows x64 Release、Node wasm32/wasm64 上分别编译、链接和运行。三个消费者均将 13 字节含 NUL 原文压为 30 字节封装帧并完整还原，退出码为 0。
Linux、macOS、Android、iOS 本次仅核查源码、构建和平台测试入口。macOS、Android、iOS 随仓缺少目标 Zstandard 静态库。浏览器和微信宿主未运行。分配器耗尽返回 COMPRESS_NO_MEMORY 的路径未做故障注入；UINT32_MAX 用例验证上界计算和伪造长度，没有申请 4 GiB 业务缓冲区。本轮未执行性能测试。
测试程序或 .wasm 文件的 SHA-256 前 16 位如下。
Windows x86 Debug  F8FD5ED6D9624C3A
Windows x64 Debug  5A93D9CEB510B5B8
Windows x86 Release  63237979D5F9A169
Windows x64 Release  2601D750C80E8897
Node Wasm32 Release  0D0A504A5E10D4C2
Node Wasm64 Release  CC3D865681B8C7AF
*/
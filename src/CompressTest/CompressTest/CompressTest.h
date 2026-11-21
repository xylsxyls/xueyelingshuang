#pragma once

/** wrapped 用例用 5 字节二进制文本验证精确上限、超限、0、-1、非法负数、截断头和原地解压。限额重载包含独立的65537字节无内容长度RLE帧性能用例。 */

/** 功能用例与独立预期
无参数运行全部功能组；--list 列出可选ID，--case=<ID>只运行该组及其素材准备。未知ID或选项退出2。
raw-levels：长度0、1、4096、131072，分别使用重复字节和固定种子的伪随机输入。遍历0至9级，验证容量可容纳结果、解压逐字节一致且只消费完整首帧。
raw-frames：覆盖空指针、非法级别、UINT32_MAX容量、短输出、尾随字节、截断及错误帧。独立构造的Zstandard数据必须解出hello，zlib与可跳过帧应拒绝。
wrapped：检查四字节原长、缓冲区边界哨兵、各字符串重载、嵌入NUL、别名和重复使用。独立的RLE块不带内容长度，应在字符串和调用方缓冲区两种接口都还原为65537个Z；验证原地、业务上限、伪造内外长度、附加帧和校验错误。
*/

/** Compress公开接口回归测试。Windows、Linux、macOS、Android、iOS和Wasm共用同一组输入。
Wasm宿主可主动调用导出的runCompressTests。
@return 全部断言通过为0，首个失败返回1并输出断言位置。
*/
extern "C" int runCompressTests();

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

/** 性能用例
运行本工程时增加 --benchmark 参数，使用公开接口执行独立批次并验证结果。计量单位为纳秒/操作。
先自动预热并校准批次大小，再输出九轮数据、中位数、第一及第三四分位数。
*/

/** 当前实现已有性能数据（采集于2026-11-22）
输入为 65,536 字节，字节 i 取 (17×i + i/113 取整 + i/1024 取整) 对 127 的余数。Zstandard 3 级生成 1,845 字节，复用目标字符串容量，解压后逐字节检查。第三列用独立的10字节无内容长度RLE帧及4字节外层原长头，解为65537个Z。每次调用传入精确限额，批次验证总长并在计时外逐字节校验。三项样本内容和编码均不同。容量估算与参数合法性无须单独计时。
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
压缩和解压优先复用独立目标字符串的容量。原地调用仍隔离输入。解压在确认唯一完整帧及声明原长后直接解码。对不可复用目标、不同数据规模和压缩等级，耗时须用对应输入重测。
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

/** 指定测试可用 --cases 1 或 --cases 1,2-3。--instant、--stress 和 --all 按当前平台选组。
1 raw-levels 压缩等级和长度组合（压力）。
2 raw-frames 原始帧边界和损坏输入（瞬时）。
3 wrapped 封装帧、容量与别名（瞬时）。
没有适用压力项时 --stress 返回 2；沿用 --case=英文名 和 --list。
*/

/** 当前版本功能验证（2026-11-22）
完整功能套件通过289条断言，包含省略内容长度的独立Zstandard帧和伪造内外长度场景。
Windows x86、x64 的 Debug、Release 四种功能测试均通过。公共 x64 SDK 的 Debug、Release 消费者分别通过。
Node 和 Chrome 的 Wasm32、Wasm64 均通过，Chrome 自动选择 64 位。微信开发者工具（基础库 3.17.4）自动选 32 位，强制 32 位通过；强制 64 位因宿主不支持 Memory64 在实例化时失败。微信手机未实测。
Linux x64 Release 已交叉编译、链接，未在 Linux 运行。macOS、Android、iOS 仅检查平台分支和构建源码。主类示例在 Windows x64 Release 与 Node 两个位宽运行成功。
全量用例覆盖 Zstandard 原始帧、封装帧、输出预算、损坏和尾部字节；两种位宽的已发布宿主包均运行成功。
数字选择、重复或非法范围及适用的分组与兼容入口累计 57 次，退出码符合预期。Windows x64 性能已另行完成独立进程复测，结果见性能章节。
*/
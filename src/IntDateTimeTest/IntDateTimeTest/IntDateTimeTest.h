#pragma once

/** 功能用例选择
1〔瞬时〕构造、日期运算、边界、格式、空值与比较，按宿主 time_t 选取可表示的长跨度。仅能表示 1970 到 2038 的 Windows 32 位 time_t 另验证超范围转换失败。
2〔压力〕八个工作线程各执行 5000 次文本解析和独立的时间戳转换。每个线程使用 2024 年 2 月不同一天的当地正午，检查日期和时刻，能发现转换结果串入另一线程。
默认或 --all 运行所有适用场景，--instant 和 --stress 分类运行；--cases 1、--cases 1-2 可选择编号，非法及无适用项返回 2。
*/

/** IntDateTime测试工程。
平台范围 Windows、Linux、macOS、Android、iOS、Wasm。
验证字符串、整数、本地时间戳和空指针构造，按字段核对日期时间、格式化与比较。线程版额外检查并行格式化。
运行对应测试程序，返回0表示通过，非0表示失败。
*/

/** Wasm 宿主测试入口
用与 SDK 匹配的 Emscripten 工具链配置本工程，指定 -DBIT=wasm32 或 wasm64、
-DSDK_ROOT=<对应 SDK>、-DLIBRARY_TEST_HOST=ON，构建后运行 ctest --output-on-failure 可验证 Node。
浏览器通过 HTTP 打开构建目录的 LibraryTestHost.html?test=IntDateTimeTest。
微信把生成的 IntDateTimeTest.js、IntDateTimeTest.wasm、LibraryTestHost.js 放入同一分包，调用
await require('./LibraryTestHost')(require('./IntDateTimeTest'), { wasmPath: '包内路径/IntDateTimeTest.wasm' })。
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

/** 性能实测（2026-11-22）
从 time_t 转本地时间，同时读取一个 HHMMSSmmm 整数。批内时间戳从 1704067200 开始逐秒递增。
时间戳经过一次线程安全的 localtime 转换，直接以整数合成日期、时刻。构造、赋值和 setTime 复用同一实现。
运行 IntDateTimeTest --benchmark。固定逻辑处理器 4，预热一个独立进程，再独立启动七次。每个进程校准后计时九轮，并用独立生成的时刻序列核对校验和。以下为七个进程各自批次中位数的中位数（最小～最大）。
Windows x64 / VS2013 Release / MT，52.069（50.139～55.169） ns/次。
Node 主组通过 --no-liftoff 直接编译优化代码，然后在计时区内调用接口。正常功能验证使用默认 JIT。
Node wasm32 / Emscripten 3.1.74 Release，636.746（444.347～786.179） ns/次。
Node wasm64 / Emscripten 3.1.74 Release，612.186（587.273～1141.321） ns/次。
Chrome 157 与微信开发者工具 3.17.4 通过正式宿主入口顺序新建四个模块实例。首个预热，其余三个各自校准并采九轮，核对校验和。按 Chrome wasm32、Chrome wasm64、微信 wasm32 的顺序，实例中位数及全距分别为 668.335（611.877～747.681） ns/次、796.509（677.490～845.337） ns/次、3845.215（3808.594～4003.906） ns/次。宿主共享线程调度及即时编译开销会影响数值，微信真机缺少性能数据。
本次同一处理器上，Node wasm32 的 IntDateTime，Node wasm64 的 IntDateTime 的最大进程中位数超过最小值的 1.5 倍，这些场景缺少可用的稳定性能基线。上列值仅记录本机观测范围。
Linux、macOS、Android、iOS，以及格式化、负年份解析缺少本次专项性能数据。
*/

/** 功能验证（2026-11-22）
Windows x86/x64 Debug、Release 及 Node wasm32/64 Debug、Release 均运行成功。 Chrome 157 的自动选择使用 wasm64，强制 wasm32、wasm64 也均通过。微信开发者工具（基础库 3.17.4）自动选择与强制 wasm32 均通过；强制 wasm64 时宿主在实例化阶段返回 CompileError。微信真机尚未运行。
*/


/** 现行接口与功能判定
数值用例按已知公历检查 2024 闰日到三月一日的加、减、秒差和比较；同时检查文本、对象及时间戳赋值。值只存两个 int32_t，没有池和容量配置。无参构造取当前本地时间，显式 (0,0) 构造空值。日期或 HHMMSS 缩放越出 int32_t 时保留已有值；文本仍接受既有十进制前缀规则。不可转换的日期加减返回空值，无效日期之间的秒差返回 0；超出 int32_t 的有效秒差饱和到边界。时间戳转换与秒运算使用系统时区。
无参数执行功能，未知参数返回 2。
*/

/** 用例 1 验证 -10203 的年、月、日、显示及文本往返。
负年份采用整个 YYYYMMDD 整数带符号的编码，例如 -10203 表示 -1 年 2 月 3 日。getMonth、getDay 和显示文本中的月日始终为正。秒运算依赖宿主能否表示该年代。比较仍按两个原始整数进行。
*/

/** Windows x64 的 CMake 入口使用匹配位宽和运行库的 SDK。
同一构建目录切换 Debug 与 Release 时，分别链接库名带 d 与原名的版本。
*/
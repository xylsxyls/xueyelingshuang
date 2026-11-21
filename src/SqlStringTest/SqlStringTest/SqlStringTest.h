#pragma once

/** 功能用例选择
1〔瞬时〕增删改查的完整模板、占位符及 NUL 拒绝。
2〔瞬时〕建表、删除及聚合、日期表达式，逐项与 SQL 字面量对照。
默认或 --all 运行所有适用场景，--instant 和 --stress 分类运行；--cases 1、--cases 1-2 可选择编号，非法及无适用项返回 2。
*/

/** SqlString测试工程。
平台范围 Windows、Linux、macOS、Android、iOS、Wasm。
逐字节验证全部SQL构造接口的关键字、反引号、字段顺序、占位符数量和日期格式。测试只构造字符串。
空列列表的两个建表入口都应返回空字符串。
运行对应测试程序，返回0表示通过，非0表示失败。
*/

/** Wasm 宿主测试入口
用与 SDK 匹配的 Emscripten 工具链配置本工程，指定 -DBIT=wasm32 或 wasm64、
-DSDK_ROOT=<对应 SDK>、-DLIBRARY_TEST_HOST=ON，构建后运行 ctest --output-on-failure 可验证 Node。
浏览器通过 HTTP 打开构建目录的 LibraryTestHost.html?test=SqlStringTest。
微信把生成的 SqlStringTest.js、SqlStringTest.wasm、LibraryTestHost.js 放入同一分包，调用
await require('./LibraryTestHost')(require('./SqlStringTest'), { wasmPath: '包内路径/SqlStringTest.wasm' })。
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

/** 性能基准（2026-11-22）
32 列 INSERT 模板。列名 f0、field1 至 field31，表名 items。
直接计数逗号，预留最终 SQL 空间，一次构造列和占位符。
运行 SqlStringTest --benchmark。五次测量统一固定在逻辑处理器 4。先以单独进程预热，再独立启动五次。每个进程校准批量后计时九轮，取每轮单位操作均值的中位数。以下为五个进程中位数的中位数（最小～最大），所有轮次均核对校验和。
Windows x64 / VS2013 Release / MT，676.587（643.594～703.293） ns/次。
Node 主组通过 --no-liftoff 直接编译优化代码，然后在计时区内调用接口。正常功能验证使用默认 JIT。
Node wasm32 / Emscripten 3.1.74 Release，2888.989（2180.365～3126.038） ns/次。
Node wasm64 / Emscripten 3.1.74 Release，2928.259（2851.562～3720.740） ns/次。
Chrome 157 与微信开发者工具 3.17.4 通过正式宿主入口顺序新建四个模块实例。首个预热，其余三个各自校准并采九轮，核对校验和。按 Chrome wasm32、Chrome wasm64、微信 wasm32 的顺序，实例中位数及全距分别为 4150.391（3479.004～4638.672） ns/次、6176.758（5224.609～7226.563） ns/次、5859.375（5615.234～5957.031） ns/次。宿主共享线程调度及即时编译开销会影响数值，微信真机缺少性能数据。
对照组在并行构建期间按同一 Release 配置测量，Windows 固定逻辑处理器 12、Node wasm32 固定处理器 14、wasm64 固定处理器 16。对照中位数及全距为 1646.210（1051.312～3370.197） ns/次、4838.086（3134.375～19363.672） ns/次、6123.340（5535.522～7386.499） ns/次。评估接口成本时采用顺序测量组；对照组记录其他任务争用处理器时的附加开销。
正常输入含 INSERT 的边界检查。无效输入的提前返回、UPDATE、DELETE、SELECT 及 DDL 缺少单独计时。
*/

/** 功能验证（2026-11-22）
Windows x86/x64 Debug、Release 及 Node wasm32/64 Debug、Release 均运行成功。 Chrome 157 的自动选择使用 wasm64，强制 wasm32、wasm64 也均通过。微信开发者工具（基础库 3.17.4）自动选择与强制 wasm32 均通过；强制 wasm64 时宿主在实例化阶段返回 CompileError。微信真机尚未运行。
*/


/** 现行接口与功能判定
本库不缓存语句，无初始化、池或后台线程，生成的 std::string 由调用方持有。INSERT、UPDATE、DELETE、SELECT 的任一输入含 NUL 时返回空串；空的建表列列表也返回空串。表名/表达式应由可信模式产生，参数值由后续绑定提供。单次成本随 SQL 文本长度和列数增长，按需复用调用方已生成的模板。
无参数执行功能，未知参数返回 2。
*/

/** Windows x64 的 CMake 入口使用匹配位宽和运行库的 SDK。
同一构建目录切换 Debug 与 Release 时，分别链接库名带 d 与原名的版本。
*/
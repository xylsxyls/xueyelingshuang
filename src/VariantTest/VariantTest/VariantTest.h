#pragma once

/** 功能用例选择
1〔瞬时〕标量、字节归属、分配失败回滚和数值原始字节往返；原生测试与 Wasm 均执行字节断言。
2〔压力〕大量整数文本对照、数据长度与移动、收缩后擦除。
默认或 --all 运行所有适用场景，--instant 和 --stress 分类运行；--cases 1、--cases 1-2 可选择编号，非法及无适用项返回 2。
*/

/** 默认功能套件额外验证 0～65536 字节边界、两万组有符号与无符号整数、视图和资源所有权，Windows Debug 验证分配失败。
以 --benchmark-operations 单独运行构造、赋值、复制、移动、转换和借用等十二种场景，可再传 0～11 选择其中一项。
@param [in] benchmark 是否执行十二项独立性能场景
@param [in] selected 性能编号，-1 表示全部
@return 全部通过时为 0
*/
int RunVariantScenarios(bool benchmark, int selected);

/** Variant测试工程。
平台范围 Windows、Linux、macOS、Android、iOS、Wasm。
DOUBLE 文本支持标准小数点和当前 C 区域的小数点；Windows 用德语小数点核对格式化往返、负小数和指数。静态链接或 EXE 与 DLL 共用同一份动态 CRT 时，测试程序设置的德语区域也用于库内解析，1,25 读为 1.25。Release DLL 与测试程序分别静态链接 CRT（/MT）时，DLL 保持初始 C 区域，1,25 读为 1。当前 DOUBLE 文本解析路径缺少性能数据，本次不做性能测试。
核对整型极值、浮点位模式、STRING/BLOB嵌入NUL、空值、拷贝和自赋值。Windows调试配置额外覆盖分配失败。
运行对应测试程序，返回0表示通过，非0表示失败。
*/

/** Wasm 宿主测试入口
用与 SDK 匹配的 Emscripten 工具链配置本工程，指定 -DBIT=wasm32 或 wasm64、
-DSDK_ROOT=<对应 SDK>、-DLIBRARY_TEST_HOST=ON，构建后运行 ctest --output-on-failure 可验证 Node。
浏览器通过 HTTP 打开构建目录的 LibraryTestHost.html?test=VariantTest。
微信把生成的 VariantTest.js、VariantTest.wasm、LibraryTestHost.js 放入同一分包，调用
await require('./LibraryTestHost')(require('./VariantTest'), { wasmPath: '包内路径/VariantTest.wasm' })。
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
给同一个 Variant 反复赋 1,024 字节的 std::string，每次读取 size()。
长字节赋值可复用当前已拥有、长度充足的缓冲，memmove 支持从自身子区间取值。增长时先分配并复制，再释放原内容。
运行 VariantTest --benchmark。五次测量统一固定在逻辑处理器 4。先以单独进程预热，再独立启动五次。每个进程校准批量后计时九轮，取每轮单位操作均值的中位数。以下为五个进程中位数的中位数（最小～最大），所有轮次均核对校验和。
Windows x64 / VS2013 Release / MT，25.364（25.084～29.529） ns/次。
Node 主组通过 --no-liftoff 直接编译优化代码，然后在计时区内调用接口。正常功能验证使用默认 JIT。
Node wasm32 / Emscripten 3.1.74 Release，67.039（61.094～67.884） ns/次。
Node wasm64 / Emscripten 3.1.74 Release，73.075（69.480～89.665） ns/次。
Chrome 157 与微信开发者工具 3.17.4 通过正式宿主入口顺序新建四个模块实例。首个预热，其余三个各自校准并采九轮，核对校验和。按 Chrome wasm32、Chrome wasm64、微信 wasm32 的顺序，实例中位数及全距分别为 122.070（90.027～125.504） ns/次、147.629（119.781～248.718） ns/次、124.741（123.596～132.370） ns/次。宿主共享线程调度及即时编译开销会影响数值，微信真机缺少性能数据。
本次同一处理器上，Node wasm32 的 blob16_copy、integer_text、blob4096_copy_read、blob4096_move_chain、raw64_ingest、blob4096_borrow，Node wasm64 的 blob64_assign、blob4096_assign、blob16_copy、blob64_copy、integer_text、blob4096_copy_read、blob4096_move_chain、raw64_ingest、blob4096_borrow 的最大进程中位数超过最小值的 1.5 倍，这些场景缺少可用的稳定性能基线。上列值仅记录本机观测范围。
对照组在并行构建期间按同一 Release 配置测量，Windows 固定逻辑处理器 12、Node wasm32 固定处理器 14、wasm64 固定处理器 16。对照中位数及全距为 30.469（28.783～48.977） ns/次、143.830（81.376～254.069） ns/次、119.463（113.811～126.682） ns/次。评估接口成本时采用顺序测量组；对照组记录其他任务争用处理器时的附加开销。
*/

/** 功能验证（2026-11-22）
Windows x86/x64 Debug、Release 及 Node wasm32/64 Debug、Release 均运行成功。 Chrome 157 的自动选择使用 wasm64，强制 wasm32、wasm64 也均通过。微信开发者工具（基础库 3.17.4）自动选择与强制 wasm32 均通过；强制 wasm64 时宿主在实例化阶段返回 CompileError。微信真机尚未运行。
*/

/** 十二个操作场景实测（2026-11-22）
运行 VariantTest --benchmark 共输出 13 个 [BENCH]，包含上文的 1024 字节复用基准，以及下列十二项。独立命令 --benchmark-operations 只运行下列十二项，末尾加 0～11 的十进制序号可单独运行表中对应行。
单位 ns/次，统计方式同上。每个数值后列出五个进程中位数的最小值和最大值。
Windows 的 16 字节复制，对照组五个进程中位数分布在 9.148～23.936 ns，同一处理器组分布在 5.930～8.445 ns。Node wasm64 的 4 KiB 拷贝读回分布在 110.183～277.930 ns。判断性能变化时可对照各自的波动范围。
场景 | Windows x64 | Node wasm32 | Node wasm64 | Chrome wasm32 | Chrome wasm64 | 微信 wasm32
0 scalar_construct | 3.894（3.193～4.101） | 3.078（2.986～3.327） | 4.496（4.289～5.024） | 6.342（4.458～6.509） | 9.036（7.176～10.204） | 23.556（21.744～25.845）
1 scalar_assign | 3.434（3.039～3.484） | 2.585（2.529～2.854） | 4.087（3.989～5.119） | 7.558（5.984～7.653） | 11.730（8.559～13.161） | 21.362（21.362～27.847）
2 blob16_assign | 18.069（16.445～18.815） | 13.887（13.326～15.314） | 19.581（17.952～21.387） | 35.000（30.231～38.719） | 39.291（32.139～46.921） | 64.850（61.798～80.109）
3 blob64_assign | 7.827（7.172～7.969） | 8.169（7.822～9.872） | 12.885（10.747～17.456） | 21.076（17.881～21.935） | 27.847（26.608～28.229） | 54.359（53.787～67.329）
4 blob4096_assign | 47.349（44.350～49.606） | 61.260（57.372～76.287） | 45.632（44.049～67.207） | 142.670（128.555～163.269） | 105.667（101.852～153.351） | 146.103（140.381～178.528）
5 blob16_copy | 7.279（5.930～8.445） | 6.703（6.457～12.061） | 10.121（9.768～14.986） | 15.974（14.067～19.455） | 22.697（20.981～24.128） | 35.381（34.904～43.678）
6 blob64_copy | 78.396（62.627～82.023） | 25.742（24.728～32.189） | 35.084（33.437～62.383） | 46.539（46.539～56.839） | 73.624（64.087～77.820） | 143.433（126.648～146.866）
7 integer_text | 27.549（25.099～27.907） | 48.499（48.124～73.741） | 129.023（118.831～216.035） | 123.596（108.719～128.937） | 280.762（274.658～305.939） | 196.838（186.920～222.778）
8 blob4096_copy_read | 77.506（72.498～82.120） | 112.479（110.638～169.196） | 112.968（110.183～277.930） | 279.999（263.977～292.969） | 267.029（240.326～312.805） | 309.753（299.072～381.470）
9 blob4096_move_chain | 735.007（719.882～756.775） | 635.507（612.677～985.989） | 844.205（816.055～1894.305） | 1257.324（1208.496～1287.842） | 2105.713（2105.713～2124.023） | 1861.572（1818.848～1873.779）
10 raw64_ingest | 8.185（7.693～9.007） | 20.654（20.484～32.303） | 53.801（51.606～120.484） | 58.174（55.504～62.370） | 124.359（117.493～167.084） | 92.316（90.408～96.130）
11 blob4096_borrow | 1.526（1.474～1.742） | 2.447（2.419～4.906） | 3.109（2.991～7.198） | 6.461（6.127～7.868） | 8.869（8.488～9.632） | 7.844（7.725～7.963）
assign 为给同一对象重新赋值；copy 新建副本；copy_read 把 4096 字节读进新字符串；move_chain 对新建的 4096 字节值执行两次移动；raw64_ingest 用 assignBytes 传入 64 个字节；borrow 仅读内部视图的首字节与长度。
Chrome 和微信开发者工具各列使用上文的独立模块实例测量方法。微信开发者工具强制 wasm64 在实例化阶段报 CompileError，微信真机尚未运行。
*/


/** 现行接口与功能判定
值由对象独占，不设全局内存池。默认构造为 INIT；16 字节是对象内联存储的布局常量。字符串/二进制超过内联空间时按实际字节数申请，最大值受 size_t 与可用内存约束；assignBytes 失败保持原值。复制需要复制有效字节，移动转交所有权，data() 是只读借用视图，变更、移动或析构后需重新取得。
无参数执行功能，未知参数返回 2。文本 INT32 读取有符号十进制前缀，忽略开头的空白，超出范围时饱和到 INT32_MIN 或 INT32_MAX。非法前缀转换为 0，现有性能采样未覆盖这个文本输入分支。 堆内字节缩短复用时，复制完成后立即擦除废弃尾部，clear 或析构时再擦除剩余有效字节。缩短复用及擦除成本取决于删减量，当前缺少性能数据。MSVC Debug 用 CRT 释放钩子检查整个原始分配块在释放前均为零。

*/

/** Windows x64 的 CMake 入口使用匹配位宽和运行库的 SDK。
同一构建目录切换 Debug 与 Release 时，分别链接库名带 d 与原名的版本。
*/
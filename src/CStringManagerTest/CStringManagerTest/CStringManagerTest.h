#pragma once
#include <locale>

/** 验证 6 条编码路径、内嵌零和缓冲边界，4 种 Format 重载的别名、长字符串、多字节宽字符。Windows Debug 逐分配位置注入失败，核对异常、回收和结果完整性。
@return 所有适用的回归断言通过时为 true
*/
bool RunCStringManagerRegressionTests();

/** 构造每三位分组的数值区域设置，验证协议整数不会受宿主显示习惯影响
*/
class CStringManagerTestGrouping : public std::numpunct<char>
{
protected:
    /** 返回每三个数字采用一个分组的规则
    @return 首字节数值为3的分组序列
    */
    std::string do_grouping() const override;
};

/** CStringManager测试工程。
平台范围 Windows、Linux、macOS、Android、iOS、Wasm。
验证截取、查找、替换、拆分、数字和宽窄格式化、大小写、URL和Unicode转换。线程版额外检查并行格式化。
用例1覆盖嵌套括号、引号类相同分隔符的双向及相邻匹配、孤立和越界选择。FindOther 从选中的括号向另一侧检查嵌套深度；相同分隔符按出现顺序两两配对，不解释转义，单个未配对符号及越界索引返回 npos。辅助空间为常数，当前实现缺少独立耗时测量。
运行对应测试程序，返回0表示通过，非0表示失败。
*/

/** Wasm 宿主测试入口
用与 SDK 匹配的 Emscripten 工具链配置本工程，指定 -DBIT=wasm32 或 wasm64、
-DSDK_ROOT=<对应 SDK>、-DLIBRARY_TEST_HOST=ON，构建后运行 ctest --output-on-failure 可验证 Node。
浏览器通过 HTTP 打开构建目录的 LibraryTestHost.html?test=CStringManagerTest。
微信把生成的 CStringManagerTest.js、CStringManagerTest.wasm、LibraryTestHost.js 放入同一分包，调用
await require('./LibraryTestHost')(require('./CStringManagerTest'), { wasmPath: '包内路径/CStringManagerTest.wasm' })。
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

/** 性能测试 P01（--benchmark）
把2 KiB交替字符中的标记全部替换，并对4096字节混合输入进行URL编码和解码。另对含高字节与 NUL 的四千字节输入测量路径编码、十六进制正反向转换，结果与按块给定的固定字节串完整比对。固定输入在批外生成，替换测量包含每个可变输入的工作副本。
预热后自动加倍至至少约20毫秒，九轮分别以纳秒/调用报告原值、中位数及四分位数。
*/

/** 当前功能用例
1 文本编辑和数值转换。核对分隔、替换、格式化及截断边界。输入引用输出元素时仍应得到 alpha、beta、gamma。宽窄替换串引用目标时使用启动时的内容。aaab 中 ab 增长替换为 bbaa 后必须得到 aabbaa。
2 编码与缓冲区。以独立构造的 256 字节百分号表验证 URL。对照包含 BOM、内嵌零和非 BMP 字符的 UTF-16LE 样本，孤立代理项、错误长度必须保留旧输出。另核对全局区域设置启用千位分组后的整数字符串、带空白和尾随字符的解析、容量哨兵。Windows 代码页1252下也应百分号编码高位字节；POSIX及Wasm的截断UTF-8输入不能留下有效前缀。
3 转码和格式化失败处理。核对宽格式化、空输入、内嵌零、容量故障以及区域设置恢复；仅在提供 ANSI 接口的目标执行 ANSI 用例。
4 并发压力。八个线程各做 5000 轮格式化与拆分，独立校验线程号和字段数量。仅在线程可用的目标登记。
5 格式化和整数。StrFormat 应能处理 2048 字节文本；格式化内容遇零时截断，Format 保留零之后的字节。用固定的十进制端点、十六进制位模式、前缀、溢出及内嵌零检查 IntToStr 和 StrToInt。
6 十六进制字节。逐个检查 256 种输入的 Char2Hex、CharToHex 和成对字节的大小写。StrToHex 应产生小写，HexToStr 应接受混合大小写，奇数位末尾补低半字节零，非法字符仅将其所在的半字节置零。
7 路径编码。用独立白名单对照全部 256 个字节，核对 EncodeUrlPath 和 UrlEncode；用汉字、非 BMP 字符、标点和内嵌零检查 EncodeUnicodeUrl 原位输出，预期百分号字节用大写十六进制。
默认或 --all 执行当前目标全部功能用例。--instant 和 --stress 按类型选择。--cases 1 或 --cases 1,3-4 按本目标有效编号选取，未登记编号、无适用用例以及非法参数返回非零。Wasm 配对包支持 node run-node.js <位宽> 后接相同参数；页面参数 args 为 URL 编码的 JSON 字符串数组。
该类不持有实例或全局可调缓冲池。调用时指定源、目标、容量和数值回退值。split 的字符分隔重载可直接读取输出向量的元素。Replace 支持目标同时作为模式或替换串。CopyToBuffer 支持重叠的可写范围，容量计入末尾零。单字符替换只作一次扫描。别名需要保存源内容，增长替换需为新增字符分配内存，多字符替换和别名路径缺少本版性能测试数据。无状态工具无需初始化或析构计时。
Windows 使用 MSVC v120。Linux、macOS、Android、iOS 使用匹配目标的 SDK 和 CMake 工具链。Wasm 分别提供 wasm32 与 wasm64，实际可用位宽由宿主决定。
性能数据及适用范围（2026-11-22）
URL 输入为 4,096 字节的字母、空格、加号和百分号混合数据。替换场景在 2,048 字节 xa 重复文本上替换 1,024 处，含构造工作副本的耗时。路径与十六进制三个场景使用 512 个八字节块，每块依次为 A、/、:、NUL、0xFF、空格、~、?。逐字节检查路径编码产生的 7,168 字节、十六进制编码产生的 8,192 个字符和解码还原的 4,096 字节。计时包含结果字符串的构造和销毁。StrFormat、整数与宽路径转换、三个 split 重载、嵌套分隔符查找与多字符替换缺少单项性能数据。常数时间的单字节数位映射本次无须单独计时。
Windows x64 Release，Intel Core i9-14900HX，VS2013 v120 /MT。两组各先预热一个进程，再顺序运行七个独立进程。每个进程自行预热、校准批量并采九批，用各批操作数归一化。单位为微秒/次，两列是在同一逻辑 CPU 6 上依次执行的独立批次，各列给出七个进程中位数的中位数 [最小值, 最大值]。
  UrlEncode（4 KiB）    6.600 [6.435, 6.740]    6.522 [6.398, 6.941]
  UrlDecode（还原至 4 KiB）    8.224 [7.839, 8.606]    8.226 [8.003, 8.752]
  ReplaceAll（1,024 处）    1.066 [1.036, 1.088]    1.080 [1.054, 1.166]
  EncodeUrlPath（4 KiB）    12.765 [12.400, 13.120]    12.832 [12.302, 13.732]
  StrToHex（4 KiB）    4.014 [3.883, 4.283]    3.976 [3.933, 4.271]
  HexToStr（8 KiB）    9.788 [9.551, 9.969]    9.973 [9.525, 10.281]
跨进程范围反映宿主调度与负载对该场景的影响。复测应使用同一输入、构建配置、宿主及 CPU 亲和性。
Wasm Release 使用 Emscripten 3.1.74。Node 24.0.0、Chrome 157 和微信开发者工具基础库 3.17.4 分宿主顺序采样。每格为一次宿主运行内九批的中位数 [Q1, Q3]，单位微秒/次。各列依次为UrlEncode（4 KiB）、UrlDecode（还原至 4 KiB）、ReplaceAll（1,024 处）、EncodeUrlPath（4 KiB）、StrToHex（4 KiB）、HexToStr（8 KiB）。
  Node wasm32    8.811 [8.217, 9.780]    7.027 [6.918, 7.115]    1.567 [1.541, 1.615]    18.506 [18.447, 19.074]    9.634 [9.524, 9.833]    9.562 [9.194, 10.603]
  Node wasm64    10.632 [10.479, 10.739]    7.932 [7.714, 8.060]    1.746 [1.732, 1.785]    20.018 [19.872, 20.269]    12.467 [12.083, 13.055]    12.151 [12.037, 12.469]
  Chrome wasm32    9.229 [8.887, 9.717]    7.520 [7.471, 7.617]    1.746 [1.733, 1.782]    20.117 [19.922, 20.508]    10.010 [9.912, 10.303]    9.595 [9.546, 9.766]
  Chrome wasm64    11.328 [11.133, 11.377]    9.106 [9.033, 9.351]    1.855 [1.825, 1.886]    22.363 [21.680, 22.559]    12.939 [12.891, 13.037]    12.158 [11.865, 12.256]
  微信开发者工具 wasm32    35.742 [35.254, 36.133]    34.473 [34.180, 34.863]    7.275 [7.202, 7.495]    91.797 [89.844, 93.359]    36.328 [36.133, 36.816]    32.227 [31.641, 32.715]
固定逻辑处理器 6，另用五个独立 Node 进程分别预热、校准并采九批。以下各列仍按上述接口顺序，单位微秒/次，报告进程中位数的中位数 [最小值, 最大值]。
  Node wasm32    8.714 [8.675, 8.886]    6.841 [6.808, 7.115]    1.558 [1.547, 1.645]    19.007 [18.581, 19.527]    9.341 [9.042, 9.658]    9.471 [9.386, 10.959]
  Node wasm64    10.908 [10.598, 11.096]    7.927 [7.773, 8.333]    1.758 [1.624, 1.822]    19.687 [19.160, 20.939]    11.848 [11.366, 12.461]    12.190 [11.682, 12.900]
Chrome 和微信的数据缺少独立宿主进程复测。微信开发者工具强制 wasm64 时实例化失败。Linux、macOS、Android、iOS、微信手机及原生 Windows x86 缺少同口径的性能数据。
配置 CMake 时加入 -DLIBRARY_TEST_BENCHMARKS=ON，构建目录内用 ctest -V -L performance 运行并读取原始九轮、迭代次数与校验值。
双位宽宿主包可执行 node run-node.js 32 --benchmark 或 node run-node.js 64 --benchmark，浏览器及小程序页面参数为 bits=32&benchmark=1 或 bits=64&benchmark=1。
*/

/** 当前版本功能验证（2026-11-22）
基础功能与边界组分别通过56、16条断言。转换与格式化两组在 Windows Release 分别通过33、43条，在 Windows Debug 分别通过148、108条，含 CRT 堆失败注入；Wasm 分别通过6、43条。Windows 另运行原生并发组。5、6、7 三组分别核对格式与整数转换、全部字节十六进制转换、窄宽路径转义；四种 Windows 配置与两个 Wasm 位宽均通过。
Windows x86、x64 的 Debug、Release 四种功能测试均通过。公共 x64 SDK 的 Debug、Release 消费者分别通过。
Node 和 Chrome 的 Wasm32、Wasm64 均通过，Chrome 自动选择 64 位。微信开发者工具（基础库 3.17.4）自动选 32 位，强制 32 位通过；强制 64 位因宿主不支持 Memory64 在实例化时失败。微信手机未实测。
Linux x64 Release 已交叉编译、链接，未在 Linux 运行。macOS、Android、iOS 仅检查平台分支和构建源码。主类示例在 Windows x64 Release 与 Node 两个位宽运行成功。
相同分隔符的首尾双向匹配、相邻的一对、未配对、非分隔字符及越界选择均与预期相符。
Windows 与两种 Wasm 位宽共运行 54 次选择器用例，包括全部编号、组合、重复、未登记和非法范围；退出码均符合预期。当前构建的测量结果和波动范围见性能章节。
*/
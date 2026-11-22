#pragma once
#include <locale>

/** 用于按编号选择或组合执行功能用例的稳定标识
*/
enum CStringManagerCaseId
{
    // 文本切分、编辑、查找与数值转换
    kCaseStrings = 1,
    // 字节编码、宽字符和有界复制
    kCaseEncoding = 2,
    // 转码与格式化，含调试堆分配故障注入
    kCaseConversions = 3,
    // 多线程并行格式化和切分
    kCaseConcurrentFormat = 4,
    // 格式化和整数边界
    kCaseNumberAndFormat = 5,
    // 全字节十六进制映射
    kCaseHexBytes = 6,
    // 窄、宽路径转义
    kCasePathEncoding = 7,
    // 容量、精度与 UTF-16 参数
    kCaseParameterBounds = 8,
    // 同一参数表重复转发
    kCaseForwardedArguments = 9,
    // 相同替换、等长跨界匹配和二进制文本
    kCaseReplaceAll = 10,
    // 独立检查基准输入、预期输出和原位替换的计数口径
    kCaseBenchmarkInputs = 11
};

/** 验证 6 条编码路径、内嵌零和缓冲边界，4 种 Format 重载的别名、长字符串、多字节宽字符。Windows Debug 逐分配位置注入失败，核对异常、回收和结果完整性。
@return 所有适用的回归断言通过时为 true
*/
bool RunCStringManagerRegressionTests();

/** 核对 ReplaceAll 的相同模式、源对象别名及等长跨界匹配
@return 字节内容正确且 Windows Debug 中相同替换未申请普通堆内存时返回零
*/
int TestReplaceAll();

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

/** Windows 构建与运行
准备位宽、配置和链接方式一致的 CStringManager、PlatformTest SDK。include 下使用该 SDK 发布的头文件，lib 下提供对应的两个库。
在 VS2013 开发者命令提示符切到 src/CStringManagerTest，运行
MSBuild CStringManagerTest.sln /p:Configuration=Release /p:Platform=x64 /p:TestSdkRoot="<SDK目录>"
默认输出为仓库 lib/CStringManagerTest.exe。Debug 对应 CStringManagerTestd.exe，32 位选择 Win32。动态链接时将匹配的两个 DLL 与测试 EXE 放在一起。
在 EXE 目录执行 CStringManagerTest.exe --all，或用 --cases 10 单独验证相同内容替换。进程码 0 表示所选用例均通过，1 表示用例失败，2 表示选择无效。
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
从本工程目录执行 cmake -S . -B build -DSDK_ROOT=<当前系统及架构的SDK> -DBIT=<目标架构> -DCMAKE_BUILD_TYPE=Release，
执行 cmake --build build，再在 build 内运行 ctest --output-on-failure。交叉编译时需另传目标工具链；
Linux ARM32 选择 arm32，AArch64 选择 arm64。先为目标分别准备 CStringManager 和 PlatformTest；交叉构建这两个库及本测试程序时均传入同一个 -DCMAKE_TOOLCHAIN_FILE=<Linux工具链文件>。在目标设备上运行 CStringManagerTest --all。macOS 的目标架构为 64 或 arm64。
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
把2 KiB交替字符中的标记全部替换，并对4096字节混合输入进行URL编码和解码。另对含高字节与 NUL 的四千字节输入测量路径编码、十六进制正反向转换，结果与按块给定的固定字节串完整比对。固定输入在批外生成。ReplaceAll 分两项记录：一项包含复制输入，另一项在同一个字符串上往返替换，每迭代两次调用。
预热后自动加倍至至少约20毫秒，九轮分别以纳秒/调用报告原值、中位数及四分位数。
*/

/** 当前功能用例
1 文本编辑和数值转换。核对分隔、替换、格式化及截断边界。输入引用输出元素时仍应得到 alpha、beta、gamma。宽窄替换串引用目标时使用启动时的内容。aaab 中 ab 增长替换为 bbaa 后必须得到 aabbaa。
2 编码与缓冲区。以独立构造的 256 字节百分号表验证 URL。对照包含 BOM、内嵌零和非 BMP 字符的 UTF-16LE 样本，孤立代理项、错误长度必须保留旧输出。另核对全局区域设置启用千位分组后的整数字符串、带空白和尾随字符的解析、容量哨兵。Windows 代码页1252下也应百分号编码高位字节；POSIX及Wasm的截断UTF-8输入不能留下有效前缀。
3 转码和格式化失败处理。核对宽格式化、空输入、内嵌零、容量故障以及区域设置恢复。Windows Debug 每条转码路径单独核对：拒绝分配从 API 调用开始，必须既有完整结果，也有受控失败，堆块和字节数均复原。仅在提供 ANSI 接口的目标执行 ANSI 用例。
4 并发压力。创建八个线程后统一放行，每个线程做 5000 轮格式化与拆分，独立校验线程号和字段数量。仅在线程可用的目标登记。
5 格式化和整数。StrFormat 应能处理 2048 字节文本；格式化内容遇零时截断，Format 保留零之后的字节。用固定的十进制端点、十六进制位模式、前缀、溢出及内嵌零检查 IntToStr 和 StrToInt。
6 十六进制字节。逐个检查 256 种输入的 Char2Hex、CharToHex 和成对字节的大小写。StrToHex 应产生小写，HexToStr 应接受混合大小写，奇数位末尾补低半字节零，非法字符仅将其所在的半字节置零。
7 路径编码。用独立白名单对照全部 256 个字节，核对 EncodeUrlPath 和 UrlEncode；用汉字、非 BMP 字符、标点和内嵌零检查 EncodeUnicodeUrl 原位输出，预期百分号字节用大写十六进制。
8 调用参数边界。Find 的偏移、编辑返回的长度、CopyToBuffer 的零、一字节和恰好够用的容量、定点精度 0/17 与范围外输入、BOM 及损坏的 UTF-16 代理对均按固定预期核对，转换失败保留目标。复制 A、NUL、B 时容量 4 应原样复制三字节并补零，紧随其后的哨兵保持原值。容量 2 复制三字节 UTF-8 字符时只写首字节和零，查询长度始终含终止符。分别检查正文首个 U+FEFF 与流头 BOM，验证代理区两侧、U+10000、U+10FFFF 的确切字节和逆转换；有效前缀后出现非法码元也必须保留旧输出。32 位 wchar_t 另测 U+110000。
9 可变参数转发。窄、宽 formatV 连续读取同一份参数表；可借用输出作为格式或字符串实参，支持内嵌零和空格式。
10 ReplaceAll。相同对象、内容相同的独立参数及内嵌零应保留原字节，Windows Debug 在相同替换调用期间应观察到零次普通堆分配。等长替换 aaab 中的 ab 为 ba 应得到 baaa；增长替换仍只处理原匹配，替换文本引用原目标 aba 时将 a 替换为启动时的 aba，得到 abababa。
11 基准负载的功能核验。使用与 --benchmark 相同的输入、不启用计时。URL 和二进制转换须逐字节匹配已知结果；2048 字节文本的原位替换须匹配完整正向结果，反向恢复原文，两次调用的抽样校验值须与已知字节相等。
默认或 --all 先执行全部瞬时用例，再执行本目标已登记的压力用例。--instant 和 --stress 按类型选择。--cases 1 或 --cases 1,3-4 按本目标有效编号选取，未登记编号、无适用用例以及非法参数返回非零。Wasm 配对包支持 node run-node.js <位宽> 后接相同参数；页面参数 args 为 URL 编码的 JSON 字符串数组。
当前版本性能与使用成本
本次不做性能测试。当前构建缺少与库、测试程序、输入和宿主配置对应的性能测试数据。以下列出可复测场景、单次调用的工作量和采样口径；各接口的耗时排名、吞吐量及两次独立运行的波动均待实测。

接口                 每次计时的工作量
UrlEncode            输入 4096 字节的字母与 !、空格、+、% 的混合串，扫描输入并生成转义文本。
UrlDecode            输入上述预先编码的文本，识别 %HH 和加号，输出 4096 字节。
ReplaceAll           复制 2048 字节的 xa 重复串，单次调用替换 1024 个 x 为 y。结果包含工作副本的构造和销毁成本。
ReplaceAll 原位      在同一个 2048 字节字符串上，先把 1024 个 x 换成 y，再把 1024 个 y 换回 x。输入和模式串在批外准备，每轮计两次调用，报告两种方向的平均每调用耗时。
EncodeUrlPath        输入 4096 字节，包含路径标点、高位字节和 NUL，输出 7168 字节。
StrToHex             输入相同的 4096 字节二进制串，输出 8192 个小写数位。
HexToStr             输入预先准备的 8192 个十六进制数位，输出 4096 字节。

输入与完整预期在计时外构造；新字符串的场景统计结果构造、读取及析构；原位场景保留同一缓冲区，两次调用后分别读取发生替换的字节。用例 11 可以单独核对所有基准负载的预期输出，全程不计时。PlatformTest 使用单调时钟，先预热并校准批量，再报告九批的原值、中位数和四分位数，单位为纳秒/次。比较两次运行时应固定 Release 配置、链接方式、位宽、CPU、宿主版本及输入，顺序运行多个独立进程，记录进程中位数的范围。
单字节替换只需扫描一次。等长或缩短的多字符 ReplaceAll 可能多次移动后缀并重新匹配；整串十六进制编码必写两倍输入长度，UTF-16LE 与宽字符转换还须检查代理项。直接传递已有编码可省去一次转换及结果分配。上述场景与格式化、拆分、查找、数值转换都缺少本版耗时数据。GetLength 和 CharToHex 的工作量为常数，无需单独给出耗时；无初始化和内存池生命周期可计时。
配置 CMake 时加入 -DLIBRARY_TEST_BENCHMARKS=ON，构建目录内用 ctest -V -L performance 运行并读取原始九轮、迭代次数与校验值。
双位宽宿主包可执行 node run-node.js 32 --benchmark 或 node run-node.js 64 --benchmark，浏览器及小程序页面参数为 bits=32&benchmark=1 或 bits=64&benchmark=1。
*/

/** 当前功能测试结果（2026-11-22）
Windows VS2013 v120：Win32 和 x64 各测 Debug/Release × DLL/静态 LIB，每个组合全量 11/11 通过（10 项瞬时，1 项压力）。按类型选择、单选 10 及组合选择 3,8 和 8,10-11 的编号与退出码均符合预期。倒序范围 9-8 返回 2。Debug 下用例 10 的三次相同内容替换合计申请 0 个普通堆块。
Debug 下用例 3 的转码故障注入覆盖 48 个位置（24 次完整结果，24 次受控分配失败），格式化注入覆盖 32 个位置（22 次无分配被拒，10 次触发拒绝）；每次普通堆块数与字节数回到调用前状态。
Wasm：Emscripten 3.1.74 Release，Node 模块化宿主。wasm32、wasm64 分别由 CMake/CTest 执行，全量 10/10 通过，均为瞬时用例。单选 10、组合选择 3,8 和 8,10-11 均通过。单线程构建选择 4、--stress 或倒序范围时均未执行任何用例；C++ 返回 2，LibraryTestHost.js 报告该值并以进程码 1 退出。
从 CStringManager.cpp 单独启用示例，Windows x86/x64、Node wasm32/wasm64 输出 sample01%2Ewav [live]，均返回 0。
测试设施反例（Windows x64 Release 静态链接）：在隔离的测试源码副本中仅改动用例 11 十六进制预期的一位数字，运行 --all。程序报告 StrToHex 输出不符，用例 11 返回 1；其余 10 项通过，包括随后调度的压力用例，进程正常返回 1。
测试设施反例（Windows x64 Debug 静态链接）：在隔离副本中让 Utf8ToUnicode 的 8 个分配位置全部放行，以 --cases 3,8 运行。用例 3 检出该接口未触发故障并返回 1，用例 8 继续执行并通过，进程正常返回 1。
StrToHex 在乘二前检查字符串长度上限。当前环境未构造超过输入字符串 max_size()/2 的实际输入，此分支仅做静态核对。用例 6、11 对实际可分配输入逐字节比对。
Linux ARMv7 hard-float 与 AArch64 已用 Zig 0.13.0 交叉编译 Release 静态库与共享库，并分别用静态库链接 CStringManagerTest，核对了 ELF32/ARM、ELF64/AArch64 及对应的 Linux 动态加载器。以 ARM32 工具链配置 BIT=arm64 时，库的 CMake 按预期拒绝配置。两种 ARM 架构均未运行功能用例。
macOS、Android、iOS 已静态核对源码、公开头和构建入口。本次没有在这三端或浏览器、微信宿主上运行。
*/
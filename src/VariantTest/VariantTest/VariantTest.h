#pragma once
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

/** VariantTest 功能测试（2026-11-22）
使用本次目标 SDK 的 Variant 和 PlatformTest。每个 ID 建立自己的对象和输入，可单独运行。
无参数或 --all 先执行瞬时 1、3、4，再执行压力 2。--instant 和 --stress 分别选择这两类。
--cases 2-3,2 执行 2、3 各一次；倒置区间、越界 ID、未知参数返回 2。功能失败返回 1，全通过返回 0。
Node 宿主把非零 C++ 退出码写入错误信息，并以宿主失败码 1 退出。

1〔瞬时〕标量、文本、复制和异常提交
  INT32/UINT64 极值、负整数、有符号零、无穷与带负载 NaN 的拷贝应保持位模式；异型标量读取应为零。
  1,000 轮标量操作核对深拷贝、自赋值、赋值返回值的显式副本以及 INT32 原始字节。
  INT32 文本覆盖空白、尾随字符、非法前缀和正负溢出，UINT64 覆盖前缀与溢出；空数字文本保留类型且长度为零。
  DOUBLE 覆盖标准小数点、负小数和指数。在 Windows 德语区域另核对标准文本往返和库所在 CRT 的本地小数点。
  普通 STRING 在首个 NUL 截断，原始 STRING 和 BLOB 保留全部字节；相同长度仅末字节不同须不等，正负零也须不等；跨类型的全零字节应相等。长块重叠复制核对完整后缀，同长度重赋核对完整新内容。
  对类型号 -1～7、长度 0～9 穷举字符串校验重载和构造结果。MSVC Debug 检查标量无分配；
  依次拒绝第 1～16 次普通分配，失败保留整数 17，成功得到完整负载，随后可再次赋值，普通堆块数和总字节恢复。

2〔压力〕长度边界、移动、十进制输出和资源释放
  对 0、1、7、8、15、16、17、31、64、4096、65536 字节分别走 STRING/BLOB 输入。
  按独立字节序列核对借用和输出。复制、移动、自赋值、自移动及交换均核对结果类型；移动源须为 INIT、长度为零且视图为空。另查赋值返回引用、自身切片和空指针拒绝。
  对类型号 -1～7、长度 0～33 核对长度重载和 assignBytes 的返回值；合法组合逐项核对类型、长度和正文，非法组合保持旧整数 42。
  固定种子 0x12345678 生成 20,000 组整数，INT32/UINT64 十进制文本分别与 classic 区域标准流对照。
  MSVC Debug 检查 16 字节内联操作与堆块移动无分配；将 128 字节堆块缩至 64 字节，验证释放前原 128 字节全部为零。
  对 4096 字节输入逐次注入分配故障，至少一次被拒绝，失败保留 73，成功保存全部字节；末尾核对普通堆余额。

3〔瞬时〕原始数值和输出参数
  对 -123456、0xfedcba9876543210 和 -0.125，用原始构造和 assignBytes 解码，后者读取偏移一字节的非对齐地址。
  类型和原始字节须与独立输入相同。用当前对象的视图将数值改成 BLOB 再还原，逐次核对类型和所有字节。预填充的输出字符串应被完整替换，十进制文本须匹配常量；clear 后两种输出均清空旧内容。

4〔瞬时〕基准工作负载自检
  逐一执行 13 项工作负载，批量取 1、2、3、127、128，与独立预期核对。不读时钟，也不输出性能数字。
  非法性能编号返回 2，不能在没有执行场景时报告通过。
*/

/** 功能场景的稳定选择编号
*/
enum VariantTestCase
{
    // 标量表示、文本和所有权
    TestVariantOwnership = 1,
    // 大量转换、长度边界和资源故障
    TestVariantScenarios = 2,
    // 原始字节与输出参数
    TestVariantRawOutput = 3,
    // 校验基准工作负载和选择边界，不计时
    TestVariantBenchmarkBatches = 4
};

#if defined(_MSC_VER) && defined(_DEBUG)
/** 在单线程 Debug 用例的作用域内替换 CRT 分配钩子
*/
class VariantTestAllocHook
{
public:
    /** 安装本次观察或注入回调
    @param [in] hook 在本作用域有效的 CRT 回调
    */
    explicit VariantTestAllocHook(_CRT_ALLOC_HOOK hook);

    /** 恢复进入作用域时的回调
    */
    ~VariantTestAllocHook();

    /** 钩子作用域由创建者独占
    @param [in] other 已持有恢复责任的守卫
    */
    VariantTestAllocHook(const VariantTestAllocHook& other) = delete;

    /** 禁止转交恢复责任
    @param [in] other 已持有恢复责任的守卫
    @return 不可调用
    */
    VariantTestAllocHook& operator=(const VariantTestAllocHook& other) = delete;

private:
    // 进入本作用域前登记的回调，离开时恢复
    _CRT_ALLOC_HOOK m_previous;
};
#endif

/** 验证长度边界、十进制输出和所有权
@return 全部通过时为 0
*/
int RunVariantScenarios();

/** 运行独立基准场景，预热与每批观测均核对结果
@param [in] selected -1 运行全部，0～11 运行对应单项
@return 全部通过为 0，失败为 1，编号无效为 2
*/
int RunVariantBenchmarks(int selected);

/** 运行 1024 字节复用基准以及全部十二项操作基准
@return 全部通过为 0，失败为 1
*/
int RunAllVariantBenchmarks();

/** 用固定批量验证全部基准的工作负载、校验值和非法选择
@return 全部符合预期为 0，失败为 1
*/
int CheckVariantBenchmarkBatches();

/** 功能验证（2026-11-22，北京时间 13:20 起）
分别从源码构建被测库、PlatformTest 和本测试程序。Windows VS2013 v120，Win32/x64 × Debug/Release × 静态库/DLL 共 8 种组合。
Node + Emscripten 3.1.74，wasm32/wasm64 各 Debug、Release，共 4 种静态 SDK 组合。12 种组合的四个 ID 均通过。
每种组合运行默认、--all、--instant、--stress、--cases 3-4,1,4 及四个单项，核对退出码和逐项顺序。
倒置区间、未登记 ID、性能序号 12、-1 和含尾随字符的 5x 均返回 C++ 退出码 2，没有启动计时。
ID 4 在每种组合中核对 13 个负载，每项五个固定批量。ID 1/2 的检查计数分别为
Windows Debug 28241/41170、Windows Release 28208/41157、Node wasm 28206/41157；上述计数是检查次数。
Windows x64 静态库 Debug、Release 均经 NMake + CMake/CTest 通过。只给 DEBUGRELEASE=debug 可构建 Debug；
给 x64 编译器指定 arm64 或 32、给 Release 同时指定 DEBUGRELEASE=debug，三项配置均被拒绝。四种 wasm 组合另经正式 CTest 宿主通过。
隔离副本把 ID 3 的第一个预期文本改错一位，另让 ID 4 的副本字节观测值加一。--all 分别报告 3、4 失败，继续运行 2，最终返回 1。
Variant.cpp 的注释 main 在 Windows x64 Release、Node wasm32 和 wasm64 单独编译运行，三端均输出 outbound state sent, recorded bytes 3 并返回 0。
Linux、macOS、Android 和 iOS 已检查 C++11、CMake 链接和目标运行入口，未在这四端编译或运行。
浏览器、微信宿主本轮未运行。CRT 分配及释放钩子仅在 MSVC Debug 执行。
Windows 从 VariantTest.sln 构建；先运行对应 version 准备默认 32 位 DLL 依赖，其他位宽或静态组合须单独准备匹配的 SDK。
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

/** 当前接口的性能场景和数据
本次不做性能测试。当前生产实现和基准工作负载缺少配套的性能测试数据，六端均无本轮 ns/次、吞吐或跨进程波动测量。
--benchmark 运行同一对象反复赋 1024 字节，读取 size()、类型和首字节，随后运行以下十二项。
--benchmark-operations 只运行十二项；再传十进制 0～11 可单独选择对应行。
0 scalar_construct     构造 INT32，读回数值并析构。
1 scalar_assign        给已有对象赋 INT32，读回数值。
2～4 blob16_assign、blob64_assign、blob4096_assign     同一对象反复赋固定长度源，读类型和首字节；64/4096 字节在预热后可复用堆块。
5～6 blob16_copy、blob64_copy     从已建对象构造独立副本，读类型和首字节并析构，覆盖内联与堆分配。
7 integer_text         构造 -1234567 并读取格式化字符串长度。
8 blob4096_copy_read   toBlob() 返回独立的 4096 字节字符串，读长度和首字节并析构。
9 blob4096_move_chain  新建 4096 字节 BLOB 后移动两次，读类型和首字节并析构。包含初始分配、复制和最终擦除。
10 raw64_ingest        从预制的 64 字节源 assignBytes，核对接纳并读类型和首字节。
11 blob4096_borrow     从已建对象借用 data()，读首字节与 size()。

所有场景在单线程内运行。选择、源数据准备放在批循环之外，每批按独立公式核对校验和；
PlatformTest 先预热，再校准到约 20 ms 的批量或达到迭代上限，之后输出九轮单位耗时、四分位数、中位数及校验和。
复测使用相同 Release 产物、处理器、位宽、CRT、负载及宿主版本与 JIT 参数；顺序启动独立进程，记录每个中位数和全距。
分散明显时报告各轮与条件，不能只选最快一次。当前缺少多次测量证据，测量重复性未经实测确认。
计时包括各行注明的结果读取。type、size 及两种长度校验无需为每个重载单列数字。等长缓冲比较的耗时取决于首个不同字节的位置，当前缺少独立性能测试数据。
DOUBLE 解析/格式化、长块收缩与清理、分配失败等未独立计时，缺少性能测试数据。
*/
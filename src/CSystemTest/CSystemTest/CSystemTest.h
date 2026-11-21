#pragma once

#include "CSystem/CSystemAPI.h"
#include <cstdint>
#include <string>

/** 核对目录迁移、纯工具与文件操作，所有断言满足时返回 0。 */
int RunSystemInterfaceTests();

#if CSYSTEM_HAS_NATIVE_API
/** 核对熵源及缓冲区边界，成功返回 0；仅适用于原生目标。 */
int RunSystemEntropyTests();

/** 八线程核对线程号和 UUID，成功返回 0；仅适用于原生目标。 */
int RunSystemConcurrentTests();

#endif

#if CSYSTEM_HAS_DESKTOP_API
/** 核对桌面进程租约及资源清理，成功返回 true。 */
bool SystemLeaseAudit();
#endif

/** 第四个元素构造时抛出，用存活数验证二维数组释放先前各行 */
struct CSystemTestElement
{
public:
    // 当前已成功构造且尚未析构的元素个数
    static int live;

    /** 前三个构造成功，第四个抛出异常 */
    CSystemTestElement();

    /** 释放一个构造完成的元素 */
    ~CSystemTestElement();
};

/** 验证 rectValid 的四个独立坐标 */
struct CSystemTestRect
{
    // 输入矩形的左边界
    int32_t left;
    // 输入矩形的上边界
    int32_t top;
    // 输入矩形的右边界
    int32_t right;
    // 输入矩形的下边界
    int32_t bottom;
};

/** 用 QRect 同样的访问接口验证模板，四个边坐标按左、上、右、下排列。 */
struct CSystemTestQtRect
{
public:
    // 对应左、上、右、下的坐标。
    int32_t m_edges[4];

    /** 读取左边坐标。
    @return 当前坐标
    */
    int32_t left() const;

    /** 读取上边坐标。
    @return 当前坐标
    */
    int32_t top() const;

    /** 读取右边坐标。
    @return 当前坐标
    */
    int32_t right() const;

    /** 读取下边坐标。
    @return 当前坐标
    */
    int32_t bottom() const;

    /** 更新左边坐标。
    @param [in] value 新坐标
    */
    void setLeft(int32_t value);

    /** 更新上边坐标。
    @param [in] value 新坐标
    */
    void setTop(int32_t value);

    /** 更新右边坐标。
    @param [in] value 新坐标
    */
    void setRight(int32_t value);

    /** 更新下边坐标。
    @param [in] value 新坐标
    */
    void setBottom(int32_t value);
};

#if CSYSTEM_HAS_NATIVE_API
/** 持有本次测试创建的路径和句柄，退出时清理 */
struct CSystemTestFile
{
public:
    // 本次排他创建且尚待清理的文件
    std::string m_path;
    // 原始文件句柄，-1 表示无效
    intptr_t m_handle;
    // 从原句柄复制的独立引用
    intptr_t m_duplicate;

public:
    /** 排他创建本次用例文件
    @param [in] path 当前用例生成的唯一 UTF-8 路径
    */
    explicit CSystemTestFile(const std::string& path);

    /** 关闭全部引用并移除本用例创建的文件 */
    ~CSystemTestFile();

    /** 禁止复制拥有句柄的测试上下文
    @param [in] other 已有测试上下文
    */
    CSystemTestFile(const CSystemTestFile& other) = delete;

    /** 禁止复制已有文件的清理责任
    @param [in] other 已有测试上下文
    @return 不可调用
    */
    CSystemTestFile& operator=(const CSystemTestFile& other) = delete;
};
#endif

/** CSystem测试工程。
平台范围 Windows、Linux、macOS、Android、iOS、Wasm。
公共用例检查二维数组、矩形、路径、参数和时间单位；五个原生端额外核对当前进程、线程、处理器、UUID及带哨兵的随机缓冲区。接口逐项平台说明见CSystem.h。
原生另在可写工作目录（可设CSYSTEM_TEST_DIR）排他创建临时文件，验证路径边界、读写、EOF、重复句柄、刷新、截断与清理。
运行对应测试程序，返回0表示通过，非0表示失败。
*/

/** Wasm 宿主测试入口
用与 SDK 匹配的 Emscripten 工具链配置本工程，指定 -DBIT=wasm32 或 wasm64、
-DSDK_ROOT=<对应 SDK>、-DLIBRARY_TEST_HOST=ON，构建后运行 ctest --output-on-failure 可验证 Node。
浏览器通过 HTTP 打开构建目录的 LibraryTestHost.html?test=CSystemTest。
微信把生成的 CSystemTest.js、CSystemTest.wasm、LibraryTestHost.js 放入同一分包，调用
await require('./LibraryTestHost')(require('./CSystemTest'), { wasmPath: '包内路径/CSystemTest.wasm' })。
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

/** 功能用例、独立预期与执行范围
批次 2026-11-22。通过 CSystem/CSystemAPI.h 调用同架构 SDK，稳定 ID 为 1 至 9。除 7 为压力用例外均为瞬时用例。
1 六端纯工具。构造第 4 个元素抛错后存活数归零；两行数组可独立写入；两种矩形逐边钳零。验证四种文件名模式、扩展名、UTC 纪元、2 秒的三种单位换算。参数解析应排除程序名，保留空参数、空格及失败前输出。Windows 按真实进程参数核对。
2 五端原生系统与文件。读取本进程身份两次应一致；核对支持目标上的进程名和 PID、CPU、UUID 格式、熵源哨兵。独占建文件、重复建文件失败、模式 0/4 失败。按偏移读回已知二进制字节，EOF 可少读；超限偏移失败且反馈错误，零容量成功。复制的句柄观察到相同截断长度。带 NUL 的路径不得影响前缀文件。宽路径预算不足须保留原输出。Windows 另与注册表频率、系统调度等级、进程资源及监视状态对照。
3 六端接口组合。六端验证时间格式和数组；原生端另核对可执行文件与模块目录，使用独占临时目录测试创建、并发递归创建、复制、发布、改名、枚举与迁移。预期保留空文件，冲突保留双方字节，合法迁移后源目录消失；枚举全局停止只访问一个匹配项。POSIX 额外核对反斜杠文件名、末尾反斜杠目录、目标别名指向源树、悬空源链接，拒绝重叠时不得创建子目录。
4 六端线程调度。不活动线程返回 Invalid 且清零错误码。五端原生另检验活动线程越界等级；Windows 回读全部五档，POSIX 核对负档 Unsupported 和普通档设置结果。
5 五端短等待。每组预热后请求 20 次 2 ms，累计至少 38 ms。Windows 另通过测试翻译单元调用真实的普通计时器回退。--precise-sleep 额外要求各组累计小于 160 ms，仅作调度诊断。
6 五端熵源。覆盖空指针、0、1、32、256、8192、8193 字节和回调返回值。有效写入不得越界，超上限时整块缓冲保持原字节。
7 五端并发压力。8 个线程各执行 2000 次，发现线程号为零或 UUID 为空即记失败，全部 join 后检查失败数。
8 桌面租约。检查取得、另一个线程竞争返回 0、非空 state 拒绝复用、ready 置位及撤销、重复释放、释放后重新取得。Windows Debug 另在 32 个分配位置注入 CRT 故障，校验无异常逃出、句柄数及堆块恢复。
9 桌面及 Android 命令执行。子进程应返回真实的退出码 7 并交付预期字节；空或含 NUL 的命令应失败。Windows 另有 shell、输入输出别名及并行管道；POSIX 核对包含 NUL 的输出。

默认或 --all 执行本平台全部登记项。--instant、--stress 按类型选取，--cases 1,3-4 按编号选取。Wasm 有 1、3、4；iOS 有 1 至 7；Android 有 1 至 7 和 9；三个桌面端均有 1 至 9。退出码 0 为选中项通过，1 为失败，2 为参数无效或无可运行项。
桌面输入专项另起进程，向 --console-line 或 --console-reader 写入 first、换行、X、换行并关闭管道。前者在库内清行并读回整行，后者由应用先预读、把自己的 getchar 交给库，检查剩余字符和 EOF。两种入口的预期退出码均为 0，换为 Y 均应返回 1。父进程设置超时，防止 EOF 循环掩盖失败。

验证结果
2026-11-22 功能批次以工程源码重建独立 SDK。被测 CSystem.cpp 的 SHA-256 为 496ea3fedce9c212df126cd69f10e5fa0797228d8cf85311bbb4072e223c05e6。
Windows 依次构建 CSystem 和 PlatformTest 的匹配 SDK，再使用该 SDK 的 include/lib 构建 CSystemTest.sln。执行 CSystemTest.exe --all（Debug 使用 CSystemTestd.exe），每个已选 ID 应输出 returned 0，全部成功进程退出 0。
Windows MSVC v120，Win32/x64 × Debug/Release × 静态库/DLL 共 8 种配置；每种 8 个瞬时和 1 个压力用例通过。两项管道专项的正反输入均得到预期退出码，--cases 999 返回 2。八种配置分别通过 --precise-sleep 的上下界检查。
Node + Emscripten 3.1.74 的 wasm32/wasm64 Release 均通过 1、3、4 和编号拒绝检查，项目的 CMake/CTest 宿主入口分别通过。Windows Win32/x64 Release 和两个 Wasm 位宽另行编译运行 CSystem.cpp 中的 main 示例，退出码均为 0。
另用 Windows x64 Debug 静态库验证共享目录身份后端，9 个 ID 通过，租约 ID 8 独立启动 25 次以及整套连续启动 10 次均通过。故障用例先检查本次租约操作前后的堆和进程句柄，再单独启动竞争线程。
桌面 SDK 启用共享身份目录时，测试 CMake 同传 -DPROCESS_WORK_PORTABLE_IDENTITY=ON；MSBuild 为库和测试均定义 PROCESS_WORK_PORTABLE_IDENTITY 并设置 CSYSTEM_TEST_DIR 为本次可写目录。
Linux 通过 x86-64 Debug 分支语法检查；macOS、Android、iOS 进行了预处理分支、声明、实现和构建入口检查。四端均未运行。浏览器、微信、剪贴板交互、设备、登录启动项及实时调度权限未进行实机验证。上述 9 个 ID 的通过范围限于列出的断言。
*/

/** 性能用例 P01
CSystemTest --benchmark 会用两条固定路径依次测 GetName 全名、GetName 扩展名、joinPath。内容检查在计时前；批内循环含字符串构造、长度累加和析构，每批末核对总长度。准备过程与 I/O 不参与计时。CMake 可用 -DLIBRARY_TEST_BENCHMARKS=ON 注册，ctest -V -L performance 输出原值。
性能数据与适用范围
本次仅执行功能验证，未运行 --benchmark。当前交付构建缺少单独采样的性能测试数据。下面保留 GetName、joinPath 相同生产实现与固定输入的已有测量，供容量估算；不能由这些数值推算其他接口、设备或宿主的耗时。
样本条件为 Intel Core i9-14900HX、Release。两条 72 字节路径交替，文件名 session-recording.2026.final.flac 共 33 字节。成本含接口调用、返回字符串构造、长度累加和析构。单位 ns/调用，依次给出各独立进程中位数的中位数、[Q1, Q3] 以及完整范围。
Windows x64 使用 VS2013，绑定逻辑处理器 4，预热两个进程后测九个独立进程。Node 两个位宽绑定逻辑处理器 8，各预热一个进程后测五个独立进程。每进程每项九轮。
Windows x64 / GetName 文件全名　79.313 [78.054, 83.415]，全距 74.055–89.348
Windows x64 / GetName 扩展名　51.177 [48.921, 51.824]，全距 48.779–54.682
Windows x64 / joinPath 拼接　60.319 [57.322, 65.495]，全距 56.231–69.187
Node Wasm32 / GetName 文件全名　70.473 [61.725, 72.010]，全距 57.995–72.124
Node Wasm32 / GetName 扩展名　51.680 [48.653, 54.237]，全距 43.784–54.713
Node Wasm32 / joinPath 拼接　231.841 [218.176, 239.214]，全距 203.718–288.104
Node Wasm64 / GetName 文件全名　89.274 [88.741, 90.793]，全距 87.234–96.098
Node Wasm64 / GetName 扩展名　77.136 [76.529, 77.333]，全距 75.832–89.578
Node Wasm64 / joinPath 拼接　296.111 [293.254, 302.911]，全距 288.771–312.607
在这组输入上，取 4 字节扩展名的中位耗时低于取 33 字节全名。两个 Node 位宽下拼接整条路径的成本都高于取文件名。短文件名、不同分配器、CPU 电源策略、浏览器或移动端需要各自的数据。
运行 P01 时使用 Release、相同的库链接方式、编译器、输入及宿主；固定 CPU 和电源设置，同机串行运行多个独立进程。预热后批量校准到约 20 ms，每批反馈长度校验值，报告九轮原值、中位数和四分位数。比较进程间的中位数和全距；离散较大时保留样本、核对调度与负载，再作判断。本次没有复测进程间离散程度。
相同短等待实现另有 Windows 八种配置的调度记录，每种三轮，预热后连续请求 20 次 2 ms。公开入口累计 48.323 至 58.836 ms，普通计时器回退累计 53.105 至 58.789 ms，含实际睡眠与唤醒调度时间；当前构建只验证等待上下界。
文件读写、刷新与迁移、进程枚举和启动、租约、熵源、同步等待、线程调度、桌面设备，以及 Linux、macOS、Android、iOS 的全部场景，缺少当前构建性能测试数据。短等待的通过界限仅用于判断是否等待到期。无效参数、EOF、关闭和异常回滚按正确性判定，无需单列微基准。
*/
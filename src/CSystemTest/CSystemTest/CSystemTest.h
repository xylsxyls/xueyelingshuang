#pragma once

#include "CSystem/CSystemAPI.h"
#include <cstdint>
#include <functional>
#include <string>

/** 跨平台保持编号的功能用例 */
enum CSystemCaseId : unsigned
{
    // 数组回滚、路径、时间单位和参数符合独立预期
    CSystemCaseCommon = 1,
    // 进程位数、系统资源、文件内容和句柄状态与独立观察一致
    CSystemCaseNative = 2,
    // 目录枚举、改名、复制和迁移保留预期字节及属性
    CSystemCaseInterfaces = 3,
    // 等级及系统回读结果一致，未生效时返回对应状态
    CSystemCasePriority = 4,
    // 预热后的累计等待达到下界
    CSystemCaseSleep = 5,
    // 有效请求成功，越界请求保留完整缓冲区
    CSystemCaseEntropy = 6,
    // 八个线程的身份稳定且独立，全部 UUID 互异
    CSystemCaseConcurrent = 7,
    // 竞争、就绪、释放与重试符合租约状态
    CSystemCaseLease = 8,
    // 输出字节和退出码与子进程约定一致
    CSystemCaseCommand = 9,
    // 启动参数、排他日志、输出字节和监视退出码一致
    CSystemCaseChild = 10
};

/** 一个可执行场景的编号、分类、报告文字和测试入口 */
struct CSystemCase
{
    // 当前平台上可执行的稳定编号
    CSystemCaseId id;
    // 并发压力项为 true，瞬时项为 false
    bool stress;
    // 随执行记录输出的中文名称，借用静态文本
    const char* name;
    // 可独立核对的预期结果，借用静态文本
    const char* expected;
    // 同步执行真实用例，零表示通过
    int (*run)();
};

/** 核对目录迁移、纯工具与文件操作，所有断言满足时返回 0。 */
int RunSystemInterfaceTests();

#if CSYSTEM_HAS_NATIVE_API
/** 将生成测试文件时使用的系统窄字符路径转换为 UTF-8
@param [in] path 不含 NUL 的文件系统路径，Windows 使用活动 ANSI 代码页，POSIX 使用 UTF-8
@return UTF-8 路径；系统转换失败返回空串
*/
std::string TestPathUtf8(const std::string& path);

/** 同步运行一组工作线程，创建失败时释放已创建的线程并等待结束
@param [in] count 本次工作线程数，必须大于零
@param [in] work 单个线程执行的检查，参数为从零开始的下标，返回是否通过
@return 全部启动且各线程均返回 true 时成功
*/
bool RunSystemWorkers(size_t count, const std::function<bool(size_t)>& work);

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
    @param [in] path 当前用例生成的唯一系统窄字符路径
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
Linux 32 位 ARM 取 BIT=arm32，AArch64 取 BIT=arm64，Linux x86/x86-64 取 BIT=32/64；macOS 取 64 或 arm64。
先用相同的目标工具链、配置、链接方式和 SDK_ROOT 分别构建 src/CSystem/scripts 和 src/PlatformTest/scripts。
从本测试工程目录执行 cmake -S . -B build -DSDK_ROOT=<同一SDK绝对路径> -DBIT=<目标架构> -DCMAKE_BUILD_TYPE=Release，
交叉编译时另传 -DCMAKE_TOOLCHAIN_FILE=<同一工具链文件>。执行 cmake --build build。在匹配目标 ABI 的系统上，
原生构建可在 build 内运行 ctest --output-on-failure；交叉构建可将 CSystemTest 和所需共享库部署到目标机后执行 ./CSystemTest --all。若共享库放在测试程序旁，可在该目录执行 LD_LIBRARY_PATH="$PWD" ./CSystemTest --all。
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
批次 2026-11-22。通过 CSystem/CSystemAPI.h 调用同架构 SDK，稳定 ID 为 1 至 10。除 7 为压力用例外均为瞬时用例。
1 六端纯工具。构造第 4 个元素抛错后存活数归零；两行数组可独立写入；两种矩形均以互补输入覆盖四条边的钳零和保留。用混合分隔符与含 UTF-8 字节的多点文件名逐项验证全名、大小写保留的 Gz 后缀、去后缀名和目录；另测 .config、空路径、非法模式、补分隔符与保留末尾反斜杠。核对 UTC 纪元、正负 2 秒的三种单位换算及零区间。将起点设为读取时钟前一秒，毫秒、微秒和纳秒三个“从起点至今”结果均须落在同一时钟给出的对应单位区间内。参数解析应排除程序名，保留空参数、空格及失败前输出。Windows 将 exeParam 与 CRT 窄字符参数逐项对照，commandArguments 另外与系统宽字符命令行转换出的 UTF-8 列表对照。
2 五端原生系统与文件。Windows 对照系统报告的宿主架构，POSIX 对照编译目标指针位数。使用两个已知字节串检验排他安装冲突保留源和目标，替换成功后临时源消失，缺失源的替换失败且目标不变，再排他安装到空缺路径并读回完整字节。本进程、PID 0 和 UINT32_MAX 均不得判为确定退出。读取本进程身份两次应一致；核对支持目标上的进程名和 PID、CPU、两种 UUID 的小写十六进制及连字符位置、熵源哨兵。独占建文件、重复建文件失败、模式 0/4 失败。对既有多块文件以模式 1 写入须失败并保留全部字节；模式 3 打开须保留原长度，按偏移覆盖后逐字节验证前缀、写入段及后缀。超限熵源请求使用足够大的缓冲及两端哨兵，失败后整块保持原值。按偏移读回已知二进制字节，EOF 可少读；超限偏移失败且反馈错误，零容量成功。复制的句柄观察到相同截断长度；关闭原件后，副本仍须读回两个已知字节。createObservedOutputFile 的写端保持打开，使用 readBinaryFile 独立读回前两字节和追加后的全部五字节，再次创建须失败。带 NUL 的路径不得影响前缀文件。宽路径预算不足须保留原输出。fileStampUtf8 遇到空输出、输出别名、目录或内嵌 NUL 路径须失败并清零已提供输出。macOS 的本进程名称须与实际可执行路径最后一个斜杠后的字节一致。私密文件排他打开遇到既有文件或非法路径时须报告错误，原字节保持不变。普通二进制流重新打开一个三字节文件并写入两个含 NUL 的字节，关闭后须恰好读回两字节。私密流另经排他新建、写入、提交、关闭并独立读回全部字节；无效流和空数据请求须返回失败，合法的零字节写入须成功。Windows 另与注册表频率、系统调度等级、进程资源及监视状态对照，分别采样当前进程和有效监视器；分别用 GetModuleFileNameA、GetModuleFileNameW 取得的文件名核对两种进程名称查询，宽字符 PID 列表须含本进程，首 PID 查询须非零；用系统返回的可执行文件名核对模块列表，并与 SM_CYFULLSCREEN 对照客户区高度。主屏矩形及中心点须与 SM_CXSCREEN、SM_CYSCREEN 的坐标和尺寸相符。Windows 独立线程依次进入 MTA、STA 并枚举摄像头。库返回后，同模式再次初始化须返回 S_FALSE；宿主释放后切换模式须返回 S_OK。设备列表每个实例 ID 的首段须等于其分组名。
3 六端接口组合。原生端还通过 UTF-8 接口建两层目录，独立窄路径查询须能看到子目录，再次请求须成功，最后从内向外删除。改名目标已存在时，Windows 须失败且两份原字节仍在；POSIX 须成功，目标含源内容且源名称消失。六端验证时间格式和数组；原生端另核对可执行文件与模块目录，使用独占临时目录测试创建、并发递归创建、复制、发布、改名、枚举与迁移。在测试进程内分别设置带和不带末尾分隔符的仓库根目录，环境查询须立即读到新值，两次均须选中 2.5 而非 1.0，空名称须拒绝。新建空文件和合并同内容目标都要保留源修改标记；POSIX 先独立设置带小数秒的修改时间。源路径或目标路径分别兼作错误输出时，仍须迁移真实文件并清空错误字符串。冲突保留双方字节，合法迁移后源目录消失；枚举全局停止只访问一个匹配项。将同一个未访问目录列表的元素分别用作根路径和精确文件名，须返回指定文件并清空列表。另先取得一次真实的缺失目录诊断，目录可读后直接用该元素重试，须找到新文件且不再报告缺失。Windows 的 ShellCopy 应拒绝空源或空目标，并能在独占目录内按带正斜杠的路径复制完整文件内容。空扩展名和 txt. 分别选择无后缀文件或两类文件。Linux、macOS 在独占目录创建无写端 FIFO，普通文件打开接口须返回 -1 并报告错误；桌面 POSIX 的 CTest 超时为 120 秒。POSIX 额外核对扩展名中的反斜杠、反斜杠文件名、末尾反斜杠目录、目标别名指向源树、悬空源链接，拒绝重叠时不得创建子目录。
4 六端线程调度。不活动线程返回 Invalid 且清零错误码。五端原生另检验活动线程越界等级；Windows 回读全部五档，POSIX 核对负档 Unsupported，并与 pthread 的设置结果对照；成功须回读普通档 SCHED_OTHER 和优先级 0，受限时原生错误码须一致。
5 五端短等待。每组预热后请求 20 次 2 ms，累计至少 38 ms。Windows 另通过测试翻译单元调用真实的普通计时器回退。--precise-sleep 额外要求各组累计小于 160 ms，仅作调度诊断。
6 五端熵源。覆盖空指针、0、1、32、256、8192、8193 字节和回调返回值。有效写入不得越界，超上限时整块缓冲保持原字节。
7 五端并发。八个同时创建的工作线程各循环 2000 次，线程号须非零、在线程内稳定且线程间互异；16000 个 UUID 均非空且无重复。一个工作线程返回失败时须回收两个线程并报告失败。
8 桌面租约。检查取得、另一个线程竞争返回 0、非空 state 拒绝复用、ready 置位及撤销、重复释放、释放后重新取得。Linux、macOS 在独占测试名下放置无写端的 FIFO 就绪记录，查询须返回 false 并正常结束。Windows Debug 另在 32 个分配位置注入 CRT 故障，校验无异常逃出、句柄数及堆块恢复。模块列表在前六个普通堆分配位置均须触发 bad_alloc。按窄、宽进程名查询 PID 列表各覆盖前六个位置，均须观察到实际分配失败；按名查首个 PID、按自身 PID 查名称的窄、宽接口各拒绝前两个分配位置。没有分配即可正常返回。每次调用后都与调用前的系统句柄数对照，仅接受预期分配异常。
9 桌面及 Android 命令执行。子进程应返回真实的退出码 7 并交付预期字节；空或含 NUL 的命令应失败。Windows 另有 shell、输入输出别名及并行管道；POSIX 核对包含 NUL 的输出。
10 Windows 子进程与输出。通过当前测试程序启动独立子进程，逐项核对空串、空格、引号、反斜杠和中文参数。捕获 stdout 和 stderr 后须读回约定的完整字节；另将 Win32 标准输出切换到独立管道，从管道读回 OutputVector 写出的标记，监视器须交付真实退出码 23。在系统句柄已发出退出信号且仍持有句柄时，processDefinitelyExited(pid) 须确认退出；相同日志不得覆盖。分离启动须返回退出码 37；无效参数须失败并清零输出。每个子进程最多等待 10 秒。另创建名字含 U+FFFD 的文件，合法 UTF-8 路径须可用，含孤立 FF 字节的路径须拒绝规范化、打开及属性查询。

用例入口在运行前检查登记编号递增、中文场景与预期非空、回调有效。执行时逐项打印编号、名称、预期和实际返回码。Windows 的用例 2、3 同时验证系统窄路径和 UTF-8 路径接口，CSYSTEM_TEST_DIR 应选当前 ANSI 代码页可以表示的目录。默认或 --all 执行本平台全部登记项。--instant、--stress 按类型选取，--cases 1,3-4 按编号选取。Wasm 有 1、3、4；iOS 有 1 至 7；Android 有 1 至 7 和 9；Linux、macOS 有 1 至 9，Windows 有 1 至 10。退出码 0 为选中项通过，1 为失败，2 为参数无效或无可运行项。
桌面输入专项另起进程，向 --console-line 或 --console-reader 写入 first、换行、X、换行并关闭管道。前者在库内清行并读回整行，后者由应用先预读、把自己的 getchar 交给库，检查剩余字符和 EOF。两种入口的预期退出码均为 0，换为 Y 或 Xsuffix 均应返回 1。父进程设置超时，防止 EOF 循环掩盖失败。

验证结果
2026-11-22 功能批次以工程源码重建独立 SDK。被测 CSystem.cpp 的 SHA-256 为 5416b9e988dea431e7a6f7e1581e074eb09edf008338c929cd9f6f18750936fa。
Windows 依次构建 CSystem 和 PlatformTest 的匹配 SDK，再使用该 SDK 的 include/lib 构建 CSystemTest.sln。执行 CSystemTest.exe --all（Debug 使用 CSystemTestd.exe），每个已选 ID 应输出 returned 0，全部成功进程退出 0。
Windows MSVC v120，Win32/x64 × Debug/Release × 静态库/DLL 共 8 种配置；每种 9 个瞬时和 1 个压力用例通过。两项管道专项的正反输入均得到预期退出码，--cases 999 返回 2。八种配置分别通过 --precise-sleep 的上下界检查。Win32 Debug 静态库单独运行 --instant、--stress、--cases 1,3-4，分别选中 9、1、3 项，编号与登记一致。另将 10 个 ID 分别放入新进程单独执行，均通过且输出各自登记的场景和预期。Win32 Release 静态 SDK 另经 CMake 和 CTest 正常入口通过 10 项功能用例，并核对 performance 标签下只登记 CSystemTestBenchmark。
Node + Emscripten 3.1.74 的 wasm32/wasm64 Release 均通过 1、3、4 和编号拒绝检查，项目的 CMake/CTest 宿主入口分别通过。两个位宽也分别以独立 Node 进程只选 1、3、4 中的一项，六次均通过。另经生产 CMake 入口构建两种位宽的 lib 和 dll，静态归档分别链接并通过三个功能用例；模块在 Node 宿主实例化后完成一次内存分配和释放。Windows Win32/x64 Release 和两个 Wasm 位宽另行编译运行 CSystem.cpp 中的 main 示例，退出码均为 0。示例按不同公开函数名统计直接调用 93/129 个，Windows 路径另演示摄像头名称与设备分组枚举；交付源码的 main 已全部注释。
另用 Windows x64 Debug 静态库验证共享目录身份后端，10 个 ID 通过，首次查询未配置目录应失败，在有效目录重试应成功，并产生可读回的身份记录。故障用例先检查本次租约操作前后的堆和进程句柄，再单独启动竞争线程。
桌面 SDK 启用共享身份目录时，测试 CMake 同传 -DPROCESS_WORK_PORTABLE_IDENTITY=ON；MSBuild 为库和测试均定义 PROCESS_WORK_PORTABLE_IDENTITY 并设置 CSYSTEM_TEST_DIR 为本次可写目录。
Linux ARM 交叉构建验证（Release，Clang 18.1.6，glibc 2.31 目标）：Cortex-A7 硬浮点与 AArch64 各自编译 CSystem、PlatformTest 的静态库和共享库，四种组合均成功链接 CSystemTest。核对 ELF 后，ARM32 产物为 32 位 ARM，已链接产物标记为硬浮点；ARM64 产物为 64 位 AArch64。共享库测试程序引用 libCSystem.so 和 libPlatformTest.so，静态库测试程序不依赖这两个共享库。将 Linux x86-64 编译器误配为 arm64 时，测试工程在配置阶段拒绝。Linux ARM 的用例 1 至 9 尚未在目标系统运行，缺少性能测试数据。
Linux x86-64 普通身份和共享目录身份配置的生产源码及两个测试翻译单元均通过 Debug 语法检查；macOS、Android、iOS 进行了预处理分支、声明、实现和构建入口检查。四端均未运行。浏览器、微信、剪贴板交互、摄像头采集、指定设备型号、登录启动项及实时调度权限未进行实机验证。Windows 实际调用了两种设备枚举接口；允许无摄像头，设备数量依主机而定。Windows Shell 复制取消分支仅作代码检查。macOS 文件名含反斜杠、POSIX 文件时间保留与用户名并发查询未在目标机执行。POSIX 普通文件与租约的 FIFO 回归、带反斜杠的程序名及扩展名、线程调度回读均待目标机运行。Windows 的 Win32 Debug 静态库及 x64 Debug 共享目录身份静态库还在含中文和空格的代码页 936 临时目录各执行 2、3、10，退出码为 0，目录内无遗留文件。Windows ARM64 仿真、原生 32 位操作系统及位数查询失败分支未实机执行。上述 10 个 ID 的通过范围限于列出的断言。
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
在这组输入上，取 4 字节扩展名的中位耗时低于取 33 字节全名。两个 Node 位宽下拼接整条路径的成本都高于取文件名。Wasm32 的 joinPath 跨进程全距为 203.718 至 288.104 ns/调用，最高比最低约高 41%，该组数据只适合估计量级，缺少用于判定小幅回归的稳定数据。短文件名、不同分配器、CPU 电源策略、浏览器或移动端需要各自的数据。
运行 P01 时使用 Release、相同的库链接方式、编译器、输入及宿主；固定 CPU 和电源设置，同机串行运行多个独立进程。预热后批量校准到约 20 ms，每批反馈长度校验值，报告九轮原值、中位数和四分位数。比较进程间的中位数和全距；离散较大时保留样本、核对调度与负载，再作判断。从启用 LIBRARY_TEST_BENCHMARKS 的 Release 构建目录，执行 cmake -DMAX_SPREAD_PERCENT=<容许的整数百分比> -P <源码根>/src/CSystemTest/CSystemTest/CheckBenchmarkSeries.cmake。脚本先读取当前构建目录的 CMake 缓存，确认单配置构建为 Release 或多配置生成器提供 Release。再启动一个预热进程，再串行启动五个独立进程，每项输出五个进程中位数的中位数、最小值和最大值，汇总单位为 ps/调用，1000 ps = 1 ns。每项必须有一条完整输出，含正迭代数、每迭代一次调用、九个大于零的原始样本及完整校验值字段。脚本按数值排序，核对第 3、5、7 项与报告的 Q1、中位数、Q3 一致后才汇总，单个原始样本缺失、为零或与汇总不符均失败。原始行保留在 CTest 输出中。任一项的（最大值－最小值）/最小值超过指定比例，或缺少样本、计时为零，均返回失败。不传容差时只报告离散程度。应按同机基线及需辨别的变化选容差。本次仅用固定输入验证了汇总和拒绝条件，当前构建的跨进程重复性缺少实测数据。
CTest 的 CSystemTestBenchmarkReport 用固定记录经过真实 CTest 输出通道，自动核验完整记录通过、截断记录失败、中位数与九轮样本不一致失败。Windows Win32 Release、Wasm32 Release、Wasm64 Release 三个构建目录均通过该宿主回归。可从构建目录运行 ctest -R CSystemTestBenchmarkReport --output-on-failure。另以 23 组固定输入核对汇总与拒绝条件。单配置 Debug、未指定配置均应在运行基准前失败；含 Release 的多配置生成器应选择 Release。预热离群值被排除，范围恰好等于容差时通过，超过容差时失败；省略容差时仍报告中位数与全距。缺指标、重复指标、八轮或十轮、单轮为零、原始值格式错误、三种统计量分别错误、无效迭代数或每次操作数、校验字段残缺、未登记基准或无效容差均失败。另用乱序、低于 1 ns 的固定数值核对分位数和皮秒换算。固定输入仅用于验证判定逻辑。
相同短等待实现另有 Windows 八种配置的调度记录，每种三轮，预热后连续请求 20 次 2 ms。公开入口累计 48.323 至 58.836 ms，普通计时器回退累计 53.105 至 58.789 ms，含实际睡眠与唤醒调度时间；本次采用功能诊断模式验证等待上下界。
进程环境读取、commonFile 递归版本查找、UTF-8 路径转换、ShellCopy、文件读写、刷新与迁移、目录后缀过滤、子进程捕获输出与参数传递、进程枚举和启动、租约、熵源、同步等待、线程调度、桌面设备，以及 Linux、macOS、Android、iOS 的全部场景，缺少当前构建性能测试数据。短等待的通过界限仅用于判断是否等待到期。无效参数、EOF、关闭和异常回滚按正确性判定，无需单列微基准。
*/
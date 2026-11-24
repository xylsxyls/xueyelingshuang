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
    CSystemCaseChild = 10,
    // 绝对路径、独占创建、持久刷新及同卷发布
    CSystemCaseDurable = 11
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

/** 在隔离目录核对持久文件的字节、描述符边界及排他移动。
@return 本平台适用的文件系统检查全部通过时返回 0
*/
int RunDurableFileTests();

/** 核对目录迁移、纯工具与文件操作
@return 所有适用场景满足预期时返回 0
*/
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

/** 核对原生平台的熵源及缓冲区边界
@return 请求状态和哨兵均符合预期时返回 0
*/
int RunSystemEntropyTests();

/** 在原生平台使用八个线程核对线程号和 UUID
@return 工作线程全部结束且标识满足预期时返回 0
*/
int RunSystemConcurrentTests();

#endif

#if CSYSTEM_HAS_DESKTOP_API
/** 核对桌面进程租约及资源清理
@return 取得、竞争、释放和资源回收均正确时返回 true
*/
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
批次 2026-11-24。通过 CSystem/CSystemAPI.h 调用同架构 SDK，稳定 ID 为 1 至 11。除 7 为压力用例外均为瞬时用例。
1 六端纯工具。构造第 4 个元素抛错后存活数归零；两行数组可独立写入；两种矩形均以互补输入覆盖四条边的钳零和保留。用混合分隔符与含 UTF-8 字节的多点文件名逐项验证全名、大小写保留的 Gz 后缀、去后缀名和目录；另测 .config、空路径、非法模式、补分隔符与保留末尾反斜杠。POSIX 和 Wasm 在 API 调用前后独立读取 CLOCK_MONOTONIC，32 位毫秒结果须处在该区间内，按无符号减法处理回绕。核对 UTC 纪元、正负 2 秒的三种单位换算及零区间。将起点设为读取时钟前一秒，毫秒、微秒和纳秒三个“从起点至今”结果均须落在同一时钟给出的对应单位区间内。参数解析应排除程序名，保留空参数、空格及失败前输出。Windows 将 exeParam 与 CRT 窄字符参数逐项对照，commandArguments 另外与系统宽字符命令行转换出的 UTF-8 列表对照。
2 五端原生系统与文件。默认参数及 1、2、-1 生成的 UUID 应含 32 个小写十六进制字符及四个连字符，分组为 8-4-4-4-12；传 0 应恰为 32 个小写十六进制字符。共享身份配置使用绝对的私有子目录，未设置 CSYSTEM_TEST_DIR 和 TMPDIR 时在当前可写工作目录建测试夹具。内存快照的三个输出分别作两两重叠测试，失败后清零；正常查询总量大于零、可用量不超过总量，Windows 对照 GlobalMemoryStatusEx，POSIX 虚拟余量为 UINT64_MAX。64 位单调毫秒时钟须在等待后推进，差值须落在含时钟粒度余量的独立单调参考时钟给出的区间内。Windows 对照系统报告的宿主架构，POSIX 对照编译目标指针位数。使用两个已知字节串检验排他安装冲突保留源和目标，替换成功后临时源消失，缺失源的替换失败且目标不变，再排他安装到空缺路径并读回完整字节。排他安装和替换分别误传含哨兵文件的源目录时须失败，目录与文件留在原处，目标名称仍不存在。本进程、PID 0 和 UINT32_MAX 均不得判为确定退出。读取本进程身份两次应一致；核对支持目标上的进程名和 PID、CPU、两种 UUID 的小写十六进制及连字符位置、熵源哨兵。独占建文件、重复建文件失败、模式 0/4 失败。对既有多块文件以模式 1 写入须失败并保留全部字节；模式 3 打开须保留原长度，按偏移覆盖后逐字节验证前缀、写入段及后缀。超限熵源请求使用足够大的缓冲及两端哨兵，失败后整块保持原值。字节数与错误码共用一个输出变量时须拒绝读取，该变量清零，目标缓冲哨兵保持原值；随后用独立输出仍能读回原字节。对五字节文件在 4 GiB + 1 处读取一字节，应成功返回零字节，保持缓冲哨兵及文件长度。随后按偏移 1 读回应有字节，EOF 可少读；超限偏移失败且反馈错误，零容量成功。复制的句柄观察到相同截断长度；关闭原件后，副本仍须读回两个已知字节。createObservedOutputFile 的写端保持打开，使用 readBinaryFile 独立读回前两字节和追加后的全部五字节，再次创建须失败。带 NUL 的路径不得影响前缀文件。宽路径预算不足须保留原输出。fileStampUtf8 遇到空输出、输出别名、目录或内嵌 NUL 路径须失败并清零已提供输出。macOS 的本进程名称须与实际可执行路径最后一个斜杠后的字节一致。私密文件排他打开遇到既有文件或非法路径时须报告错误，原字节保持不变。普通二进制流重新打开一个三字节文件并写入两个含 NUL 的字节，关闭后须恰好读回两字节。私密流另经排他新建、写入、提交、关闭并独立读回全部字节；无效流和空数据请求须返回失败，合法的零字节写入须成功。Windows 的进程资源输出各组同类型引用两两重叠时应失败并把非零哨兵清零；Win32 中把有效监视句柄低位附上多余高位，采样及轮询须拒绝，释放该值后原句柄仍可用。Windows 只读本进程视图中 Shell Link 的组件注册值。用系统宽字符接口独立展开环境变量、提取目录，再按活动 ANSI 码页对照 GetRegOcxPath；组件未注册时应返回空串。Windows 另与注册表频率、系统调度等级、进程资源及监视状态对照，分别采样当前进程和有效监视器；分别用 GetModuleFileNameA 配合 GetFullPathNameA 的文件名指针、GetModuleFileNameW 的宽字符文件名核对两种进程名称查询，并核对窄字符可执行目录、全名和去后缀名，宽字符 PID 列表须含本进程，首 PID 查询须非零；用系统返回的可执行文件名核对模块列表，并与 SM_CYFULLSCREEN 对照客户区高度。主屏矩形及中心点须与 SM_CXSCREEN、SM_CYSCREEN 的坐标和尺寸相符。Windows 独立线程依次进入 MTA、STA 并枚举摄像头。库返回后，同模式再次初始化须返回 S_FALSE；宿主释放后切换模式须返回 S_OK。设备列表每个实例 ID 的首段须等于其分组名。
3 六端接口组合。原生端向同一文件依次追加带内嵌 NUL 的向量元素、空行和键值对，独立二进制流须读回包括分隔符在内的全部字节。桌面 POSIX 以无写端的 FIFO 为源或有读端的 FIFO 为目标时，复制应及时返回，已有普通文件保持原内容，管道内无数据。源文件经符号链接复制到空缺目标须得到相同字节；覆盖指向源的硬链接须保留双方内容。Windows 独占打开已存在文件时，第二次打开读取正文应遇分享冲突，窄路径及 UTF-8 规范化仍应解析到同一个现存文件。Windows 中文代码页 936 另以尾字节为 ASCII A 和反斜杠的双字节文件名建目录，规范化保留这些字节，路径包含、筛选和迁移须与宽字符文件系统观察一致。另仅映射名字含双字节字符的程序副本，模块列表须含副本的完整文件名。原生端还通过 UTF-8 接口建两层目录，独立窄路径查询须能看到子目录，再次请求须成功，最后从内向外删除。改名目标已存在时，Windows 须失败且两份原字节仍在；POSIX 须成功，目标含源内容且源名称消失。六端验证时间格式和数组；原生端以独立单调时钟观察 Sleep(40) 的实际等待，并对照 32 位计时器的无符号差值；原生端另核对可执行文件与模块目录，使用独占临时目录测试创建、并发递归创建、复制、发布、改名、枚举与迁移。在测试进程内分别设置带和不带末尾分隔符的仓库根目录，环境查询须立即读到新值，两次均须选中 2.5 而非 1.0，空名称须拒绝。新建空文件和合并同内容目标都要保留源修改标记；POSIX 先独立设置带小数秒的修改时间。源路径或目标路径分别兼作错误输出时，仍须迁移真实文件并清空错误字符串。冲突保留双方字节，合法迁移后源目录消失；枚举全局停止只访问一个匹配项。将同一个未访问目录列表的元素分别用作根路径和精确文件名，须返回指定文件并清空列表。另先取得一次真实的缺失目录诊断，目录可读后直接用该元素重试，须找到新文件且不再报告缺失。Windows 的 ShellCopy 应拒绝空源或空目标，并能在独占目录内按带正斜杠的路径复制完整文件内容。空扩展名和 txt. 分别选择无后缀文件或两类文件。Linux、macOS 在独占目录创建无写端 FIFO，普通文件打开接口须返回 -1 并报告错误；桌面 POSIX 的 CTest 超时为 120 秒。POSIX 额外核对扩展名中的反斜杠、反斜杠文件名、末尾反斜杠目录、目标别名指向源树、悬空源链接，拒绝重叠时不得创建子目录。
4 六端线程调度。不活动线程返回 Invalid 且清零错误码。五端原生另检验活动线程越界等级；Windows 回读全部五档，POSIX 核对负档 Unsupported，并与 pthread 的设置结果对照；成功须回读普通档 SCHED_OTHER 和优先级 0，受限时原生错误码须一致。
5 五端短等待。每组预热后请求 20 次 2 ms，累计至少 38 ms。Windows 另通过测试翻译单元调用真实的普通计时器回退。--precise-sleep 额外要求各组累计小于 160 ms，仅作调度诊断。
6 五端熵源。覆盖空指针、0、1、32、256、8192、8193 字节和回调返回值。有效写入不得越界，超上限时整块缓冲保持原字节。
7 五端并发。八个同时创建的工作线程各循环 2000 次，线程号须非零、在线程内稳定且线程间互异；16000 个 UUID 均非空且无重复。一个工作线程返回失败时须回收两个线程并报告失败。
8 桌面租约。同一持有线程用第二个空 state 再申请同名租约，普通申请返回 0，原租约仍就绪且未收到停止请求；零等待替换返回 -1，原租约仍就绪并收到停止请求。释放后可重新取得，新租约初始未就绪且没有停止请求。同一进程中，同一角色使用另一隔离后缀时不得重置已发布的就绪状态。Windows 另保留一个未被线程占有的同名互斥量引用，实际可用的租约仍应能够取得。另一线程请求替换时，原持有者观察到停止请求并主动释放，替换者须在时限内成为唯一持有者。检查取得、另一个线程竞争返回 0、非空 state 拒绝复用、ready 置位及撤销、重复释放、释放后重新取得。Linux、macOS 在独占测试名下放置无写端的 FIFO 就绪记录，查询须返回 false 并正常结束。Windows Debug 另在 32 个分配位置注入 CRT 故障，校验无异常逃出、句柄数及堆块恢复。模块列表在前六个普通堆分配位置均须触发 bad_alloc。按窄、宽进程名查询 PID 列表各覆盖前六个位置，均须观察到实际分配失败；按名查首个 PID、按自身 PID 查名称的窄、宽接口各拒绝前两个分配位置。没有分配即可正常返回。每次调用后都与调用前的系统句柄数对照，仅接受预期分配异常。
9 桌面及 Android 命令执行。子进程应返回真实的退出码 7 并交付预期字节；空或含 NUL 的命令应失败。Windows 另有 shell、输入输出别名及并行管道；POSIX 核对包含 NUL 的输出。
10 Windows 子进程与输出。库的控制台输出重定向到独立管道时，向量及键值对均须保留 NUL 两侧的字节。通过当前测试程序启动独立子进程，逐项核对空串、空格、引号、反斜杠和中文参数。捕获 stdout 和 stderr 后须读回约定的完整字节；另将 Win32 标准输出切换到独立管道，从管道读回 OutputVector 写出的标记，监视器须交付真实退出码 23。在系统句柄已发出退出信号且仍持有句柄时，processDefinitelyExited(pid) 须确认退出；相同日志不得覆盖。分离启动的子进程取得其 PID 专属租约、发布就绪并带着租约正常退出，退出码须为 37。父进程确认退出后应能取得该名称；无效参数须失败并清零输出。每个子进程最多等待 10 秒。另创建名字含 U+FFFD 的文件，合法 UTF-8 路径须可用，含孤立 FF 字节的路径须拒绝规范化、打开及属性查询。

11 六端持久文件。对照工作目录验证尚未创建的相对路径；核对空路径及嵌入 NUL。排他创建、写入并刷新后由独立文件流读回，重复创建和目标冲突须保留原字节，成功发布后源文件消失。拒绝普通目录及无效句柄。64 位描述符平台拒绝低 32 位碰巧等于有效描述符的越界值。原生 POSIX 核对父目录刷新；Wasm 在内存文件系统核对拒绝，Node 另在真实临时目录验证整个成功路径，并仅令该宿主描述符的 fsyncSync 失败，核对调用触达、失败返回及恢复后的成功。原生 POSIX 发布悬空相对符号链接后须保留链接文本，桌面 POSIX 拒绝 FIFO。

用例入口在运行前检查登记编号递增、中文场景与预期非空、回调有效。执行时逐项打印编号、名称、预期和实际返回码。Windows 的用例 2、3 同时验证系统窄路径和 UTF-8 路径接口，CSYSTEM_TEST_DIR 应选当前 ANSI 代码页可以表示的目录。默认或 --all 先执行本平台全部瞬时项，再执行已登记的压力项，各项恰好一次。--instant、--stress 按类型选取，--cases 1,3-4 按编号选取。Wasm 有 1、3、4、11；iOS 有 1 至 7 和 11；Android 有 1 至 7、9、11；Linux、macOS 有 1 至 9 和 11，Windows 有 1 至 11。退出码 0 为选中项通过，1 为失败，2 为参数无效或无可运行项。
桌面输入专项另起进程，向 --console-line 或 --console-reader 写入 first、换行、X、换行并关闭管道。前者在库内清行并读回整行，后者由应用先预读、把自己的 getchar 交给库，检查剩余字符和 EOF。两种入口的预期退出码均为 0，换为 Y 或 Xsuffix 均应返回 1。父进程设置超时，防止 EOF 循环掩盖失败。

验证方法与边界
Windows 由 MSVC v120 工程构建与配置匹配的 CSystem、PlatformTest 和 CSystemTest，执行 --all。另以独立进程向两个控制台入口输入 first 和 X 或 Y，检查正反退出码。Wasm32/64 通过 Emscripten 及本工程 CMake 构建，Node 运行同一 C++ 登记表。非法编号的 C++ 返回码为 2，Node 宿主可将其作为异常报告并以 1 退出。
2026-11-24 正式复核。十种配置的构建及功能运行日志时间为北京时间 13:59:38 至 14:00:55。11 个稳定 ID 按各平台登记表实际执行。按每配置一次 --all 统计，瞬时 88 项、压力 8 项，共 96 项，通过 96 项，失败 0 项，取消 0 项。以下其他入口与专项检查单列。
Windows Win32 和 x64 的 Debug、Release 分别链接静态库和 DLL，共八种配置。每种配置的 11 项全部通过，默认入口与 --all 顺序相同。--instant、--stress、混合编号去重和非法编号的实际结果均与预期一致。两个控制台入口分别使用正确、错误和超长第二行核对退出码。五字节文件在 4 GiB + 1 处读取时返回零字节，原缓冲与文件长度保持不变；同一句柄随后可从偏移 1 读回真实内容。默认、显式 1、2、-1 的 UUID 均满足分组格式，0 满足紧凑格式。
用 x64 Release 的隔离构建验证用例本身。将读取偏移截断到 32 位时，用例 2 的 EOF 和哨兵检查报错；只改变参数 -1 的 UUID 连字符时，用例 2 的分组检查报错。两项均返回 1。两种人为错误只存在于临时副本。
Windows x64 Release 另启用 PROCESS_WORK_PORTABLE_IDENTITY，11 项通过。删除 CSYSTEM_TEST_DIR、TMPDIR 后及指定相对目录后，用例 2 分别通过。Win32 与 x64 Release 的可执行文件均另在含代码页 936 双字节字符的路径下改名运行，11 项通过。LogManagerTest 链接本批 x64 Release 静态库，用例 12（归档）、17（排他移动）和 18（相对目录）均返回 0。
Wasm32、Wasm64 的 Release 静态归档由仓库入口重新构建。在 Node 下各通过 1、3、4、11，空压力选择和非法编号返回预期错误。两种位宽的 CTest 均通过 2/2，包括功能宿主及固定计时报告判定。Windows Win32、x64 与 Wasm32、Wasm64 均从 CSystem.cpp 抽取注释示例编译运行。四次均打印绝对日志路径并返回 0，示例直接调用跨平台合计 136 个公开函数名中的 103 个。
六端已分别核对声明、实现、平台注释、测试登记 ID 及压力类型。Linux x86-64 的生产源码和两个测试翻译单元在默认与共享身份配置下均交叉编译为 ELF64 对象。Linux ARM32 硬浮点和 AArch64 分别核对 131 个默认分支声明，未在 ARM 上完成链接或运行。Linux、macOS、Android、iOS 缺少本批实机结果。浏览器和微信宿主、设备断电恢复、交互剪贴板、登录启动项和摄像头采集未实测。
产物 SHA-256。每行依次给出目标、测试程序、实际链接的 CSystem 归档或加载的 DLL。Wasm 取实际运行的 .wasm 模块。本节保留本批结果和产物身份。
Wasm32 Release 静态  44ae53c12d739a5fa0fd6cf838694f0847a480120e79f86cf5baf0a1eb9719d1  d3a214eabcba42c0486840ac0ea9f58547c57e3a847b7410df747a2c59b77f7c
Wasm64 Release 静态  337234ffbff1f2a991beacc5fb0962f36f9ff3d3361c0417000423d11eacab53  fca8952e529190dc3563341cac367cdf57363a41b53dd43baa3543b10f5ec8da
Windows Win32 Debug 静态  a7c45b66a9d511e56ed8ea74fc369e58116c9eb0a4d80142f7cfc904357f2d7d  7745ce2ce489bc0d3a0757993e5e1a0e6497c3c443e3bd9c14744720f428f607
Windows Win32 Debug DLL  96b7716f1070f68cd6fafe70d55321d017c6ffe7efad7745bc74ec4f96d3934e  8c1da571ab928b5c84fe9ed21716d17b45220863cf80f5aa284db3261a6852c6
Windows Win32 Release 静态  717c6dff0f83c3fe775d07e7c4d84f176c40d929b954af3304b20d8a01f12b40  e0f512f4f10256a6b89d21b22994a371440ade3fa19920c2bdb48944d67076e6
Windows Win32 Release DLL  c2f0f9e9c3c680195fb6a3eddcfcdcde7a1cd5445a0a698b5dbf93630be72348  dc35f7d3a39b4ccd3656f3628b8b6fae173b1cabe9cc3de1203ab85a7ed33c7d
Windows x64 Debug 静态  1dd530546fa8d85f5f1e1093bd024ad5c52f47b84291c48a50da768627976b42  f241d7e88e2f1488732e8637eabc3107bdee762226b5eef655f019a9f55930c4
Windows x64 Debug DLL  ab82f55e83fa53596f0e5a9c3263442a820bedc3e9020e6a325b95da79eacbb7  14e671f8844ea650c90d006da916bd497a23405383e97f04b151383bec89faa1
Windows x64 Release 静态  3bf2557d49bc25dd7332b1eac3ba86e090a2dcce5c21097e6cf64d53f749960c  fff3effefcbcc3cb23c4562d5fc35639c8921a567f9036fea9b911856b31b71a
Windows x64 Release DLL  adb572f79bdbeb9989f520b9a0871171cc3b65af8d6f2affa521b2ea680edd71  acd2616d0b89f77fc6a676bc062e198ae3c69d24eefc32ae0f918609be6e32e0
*/

/** 性能用例 P01
CSystemTest --benchmark 会用两条固定路径依次测 GetName 全名、GetName 扩展名、joinPath。内容检查在计时前；批内循环含字符串构造、长度累加和析构，每批末核对总长度。准备过程与 I/O 不参与计时。CMake 可用 -DLIBRARY_TEST_BENCHMARKS=ON 注册，ctest -V -L performance 输出原值。
性能数据与适用范围
当前构建缺少性能测试数据。本次未运行 --benchmark，Windows、Linux（含 ARM）、macOS、Android、iOS、Wasm 均没有本批次的 ns/调用或吞吐量。短等待用例的时间界限只用于判定功能。
GetName 的扫描长度取决于最后一个路径段，返回的字符串可能分配内存。joinPath 需要构造完整结果，频繁使用同一目录和文件名时可在路径改变后再拼接。枚举、迁移和设备查询的开销随目录大小、文件字节数或系统设备数量变化。筛选首个匹配时让回调提前停止。只需固定工作目录时可用 absolutePath；需要解析现存链接时才查询最终路径。重复读取同一文件可复用已打开的句柄和调用方缓冲区，连续小块可在单次 16 MiB 上限内合并。持久化写入涉及系统和存储设备刷新；允许批量提交时，先写完一批，再在业务要求的持久化边界调用 syncDurableFile。返回成功才可向上层确认提交。
P01 可在启用 LIBRARY_TEST_BENCHMARKS 的 Release 构建目录运行 cmake -DMAX_SPREAD_PERCENT=<容许的整数百分比> -P <仓库>/src/CSystemTest/CSystemTest/CheckBenchmarkSeries.cmake。保持目标位宽、编译器、链接方式、主机负载与 CPU 策略一致；先预热一个进程，再串行运行五个独立进程。每进程每项先校准批长，再记录九轮原值。汇总器按每进程 600 秒的时限核对单轮和三项共 27 轮的累计用时。按输出精度扣除舍入误差后，累计下界仍超限的报告也会被拒绝。汇总检查实际工作量的校验和与四分位数，输出进程中位数的中位数及全距（ps/调用）。任一项超过指定的跨进程离散容差即报告失败；省略容差时采用 20%，计算方式为（最高进程中位数－最低进程中位数）/最低进程中位数。每个正式进程也须满足（第三四分位数－第一四分位数）/中位数不超过 20%，可用 -DMAX_IQR_PERCENT 单独设置。缺少本构建在真实负载下的重复性数据。
CopyFileOver 在 POSIX 对普通源和目标做类型与同文件检查后，按 16 KiB 分块复制；文件字节数、缓存命中和目标存储决定耗时。OutputMap、OutputVector 写文件时每次调用打开并关闭一次文件，有多项时可一次传入整个容器。这三项均缺少本构建的性能测试数据。
空参数、关闭、错误返回、所有权回滚和平台不支持分支按正确性判定，无需单列性能数值。其余接口当前均缺少性能测试数据。
报告解析用例通过虚构计时记录运行实际汇总器。完整记录成功；某项指标缺失或重复、零计时样本、按迭代次数还原后单轮或同一进程全部采样的累计用时已超过进程时限的记录、字段截断、九轮原值与中位数不一致、格式正确但工作量校验和错误必须失败。默认容差下，一个进程的中位数从 10 增至 12 ns 时通过，增至 13 ns 时拒绝。五个中位数相同而单进程四分位距达中位数的 40% 时也须拒绝；固定的小数输入在 20% 边界须通过。
*/
#pragma once

/** 时间格式用例：UTC 纪元应得到完整的日期时间。在 64 位 time_t 的非 Windows 目标上，五位年份超出缓冲区时应得到空串。 */

/** 桌面输入专项：向 CSystemTest --console-line 的标准输入提供普通一行、再给 X 和换行并关闭输入。清行后应读到 X，在 EOF 再清行应立即返回。
文件用例检查带 NUL 的源和目标均不发生路径前缀复制或日志创建，同一文件覆盖复制仍保留原字节。覆盖标志分别验证已有文件保持或替换。 */

#include <cstdint>
#include <string>

/** 核对目录迁移、纯工具与文件操作，所有断言满足时返回 0。 */
int RunSystemInterfaceTests();

/** 核对熵源及缓冲区边界，成功返回 0；仅适用于原生目标。 */
int RunSystemEntropyTests();

/** 八线程核对线程号和 UUID，成功返回 0；仅适用于原生目标。 */
int RunSystemConcurrentTests();

/** 核对桌面进程租约及资源清理，成功返回 true。 */
bool SystemLeaseAudit();

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

#ifndef __EMSCRIPTEN__
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

/** 性能测试 P01（--benchmark）
使用两个带分隔符和多点文件名的固定路径，分别测量取文件名、扩展名及拼接。输入准备和内容校验在计时区外，返回字符串分配包含在耗时内。
预热后自动加倍至至少约20毫秒，九轮分别以纳秒/调用报告原值、中位数及四分位数。
*/

/** 当前功能用例
1 路径和纯工具。检查隐藏文件、分隔符、UTC 纪元、时间单位、矩形及二维数组。分别验证字段式矩形和 QRect 风格的四边访问器。在第四个元素抛异常后，存活元素数应恢复为零。
2 原生系统与文件。在本轮独占路径上核对目录设置、二进制内容、句柄复制、EOF、截断和删除。查询本进程名与 PID 的双向匹配，并检验含内嵌 NUL 的窄/宽路径不能打开、重命名、替换、删除或改写前缀文件。分块往返 32771 字节的含零内容。Windows 另核对本进程监视结果、控制台句柄、注册表报告的处理器频率以及查询前后的调度等级。
3 系统接口与目录迁移。另检查两个已入队子目录间的全局终止、大小写扩展名及回调抛异常后的资源释放。覆盖失败不丢失原文件、冲突保留双方内容、空文件、重叠目录、合法迁移和清理。Wasm 执行纯工具部分。
4 线程优先级。非 joinable 线程返回 Invalid。原生端对活动线程设置普通等级，Windows 从操作系统独立回读；非法等级应清空错误码。桌面 Linux 同时确认可以读到 procfs 的进程名称。
5 原生短等待。Windows 分别调用公开的 sleepPrecise 和生产库中的私有普通计时器回退，两条路径均先预热再执行 20 次 2 毫秒等待。每组总时长须至少 38 毫秒；单独用 --precise-sleep 检查每组低于 160 毫秒的上界，建议在空闲机器运行。Windows x86、x64 的静态库和动态库在 Debug、Release 各串行测 3 轮，共 24 轮。每轮 20 次等待合计，高分辨率路径为 48.323–58.836 毫秒，普通计时器回退为 53.105–58.789 毫秒。
6 原生熵源。核对零长度、上限、非法参数、回调返回码和边界保护字节。
7 原生压力。八个线程各调用 2000 次，线程号必须有效且每次 UUID 非空。
8 桌面进程租约。检验冲突、释放及可重用性。Windows Debug 依次在 32 个堆分配位置注入故障，校验句柄与资源回收。
9 桌面及 Android 命令执行。对照真实子进程退出码 7、完整输出和非法命令。Wasm 可选 1、3、4。iOS 可选 1 至 7。Android 可选 1 至 7 及 9。桌面原生端可选 1 至 9。
默认或 --all 执行当前目标全部功能用例。--instant 和 --stress 按类型选择。--cases 1 或 --cases 1,3-4 按本目标有效编号选取，未登记编号、无适用用例以及非法参数返回非零。Wasm 配对包支持 node run-node.js <位宽> 后接相同参数；页面参数 args 为 URL 编码的 JSON 字符串数组。
静态工具无统一初始化。SystemCommand 等待命令完成，返回真实的退出状态，结果可含 NUL。Windows 默认直接创建隐藏进程，POSIX 通过 shell 执行。五端原生 readFile 分段读至 EOF，空文件与失败均返回空串，需区分时使用带状态的文件句柄接口。GetCPUSpeedGHz 查询系统报告的频率，单位 GHz，无法取得时返回 0。setThreadPriority 只对仍可 join 的线程生效，调用方负责防止并发 join 或 detach。移动端身份目录必须在首次登记前设置。桌面默认采用系统进程创建时间；构建时设置 PROCESS_WORK_PORTABLE_IDENTITY=ON 可改用同一目录登记后端，各参与进程须使用相同配置。优先级、系统查询、文件与租约延迟缺少本版性能测试数据，初始化参数校验不单独计时。
Windows 使用 MSVC v120。Linux、macOS、Android、iOS 仅可按源码与构建入口核对；Linux x86-64 另交叉编译并链接，运行需目标设备。Wasm 分别提供 wasm32 与 wasm64，实际可用位宽由宿主决定。
性能数据及适用范围（2026-11-22）
2026-11-22，Intel Core i9-14900HX，Release。两条 72 字节路径交替输入，末段为 33 字节的 session-recording.2026.final.flac。计时包含返回字符串的构造和释放，每批校验总长度。下表依次为各独立进程中位数的中位数、[Q1, Q3] 及全距，单位 ns/调用。
Windows x64（VS2013）绑定逻辑处理器 4，先完整预热两个进程，正式采样九个进程，每进程每项自适应批量测九轮。Node 两种位宽绑定逻辑处理器 8，各先预热一个完整进程，再测五个进程，每进程每项九轮。
Windows x64 / GetName 文件全名　79.313 [78.054, 83.415]，全距 74.055–89.348
Windows x64 / GetName 扩展名　51.177 [48.921, 51.824]，全距 48.779–54.682
Windows x64 / joinPath 拼接　60.319 [57.322, 65.495]，全距 56.231–69.187
Node Wasm32 / GetName 文件全名　70.473 [61.725, 72.010]，全距 57.995–72.124
Node Wasm32 / GetName 扩展名　51.680 [48.653, 54.237]，全距 43.784–54.713
Node Wasm32 / joinPath 拼接　231.841 [218.176, 239.214]，全距 203.718–288.104
Node Wasm64 / GetName 文件全名　89.274 [88.741, 90.793]，全距 87.234–96.098
Node Wasm64 / GetName 扩展名　77.136 [76.529, 77.333]，全距 75.832–89.578
Node Wasm64 / joinPath 拼接　296.111 [293.254, 302.911]，全距 288.771–312.607
上下限保留所有进程。Chrome 和微信开发者工具本次运行功能用例，缺少本版计时。进程查询、目录遍历和桌面打开缺少接口时延、吞吐和峰值内存数据。
配置 CMake 时加入 -DLIBRARY_TEST_BENCHMARKS=ON，构建目录内用 ctest -V -L performance 运行并读取原始九轮、迭代次数与校验值。
双位宽宿主包可执行 node run-node.js 32 --benchmark 或 node run-node.js 64 --benchmark，浏览器及小程序页面参数为 bits=32&benchmark=1 或 bits=64&benchmark=1。
*/

/** 原生用例并发创建同一普通目录，四个调用均须成功，最后清理目录。POSIX 另以反斜线作为目录名字符创建并清理。迁移用例在源和目标目录名末尾各放置一个反斜线，核对目标子文件字节及源目录的清理。剪贴板和登录启动项须独立桌面账户或交互环境核验，本轮未运行。 */
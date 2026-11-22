#pragma once

/** SharedMemory 原生测试说明（2026-11-23）

运行和判定
测试程序链接所选目标 SDK 的 SharedMemory 和 PlatformTest，全部场景通过生产公开接口触发。默认、--all 或 --instant 执行本平台登记的瞬时用例；--cases 1,3-4 只执行所选项。编号必须全部适用于目标。没有压力用例，--stress 返回 2。每项输出编号及返回值，检查失败输出行号和表达式并返回。进程退出 0 表示选中项全部通过，1 表示功能失败，2 表示参数无效或无可运行项。
每项使用进程、时间和序号组成的新名称。普通实例退出作用域释放视图与句柄，显式保留的名称由用例撤销。用例 4、5 等待子进程正常退出最多 15 秒，超时即判失败，仅对本用例启动的子进程请求终止，再等至多 1 秒回收。桌面可终止测试进程取消；强制终止 POSIX 创建者可能留下其独占名称。移动端由 PlatformTest 宿主管理测试目录和设备结果。

用例与独立预期（全部为瞬时）
1　命名视图／六端。排他创建 128 字节后重复创建、等长加入、过大加入、请求 129 字节。按后备实际边界判断：Windows 检查页边界，macOS 默认后备用系统 fstat 作参照，其余目标精确到字节。创建端、只打开端在映射前及读写映射后的 realSize 必须等于同一独立预期。仅查询容量不会发布视图。失败实例不能撤名。包含 NUL 的名称不能打开或加入前缀对象。POSIX 名称长度在默认后备的 30 字节、目录后备的 128 字节边界上可用，多一字节无效。用 128 个已知字节验证双向可见、只打开端在映射前后调用 clear 均保留内容，映射前调用也不打开后备、读后写、close 和重映射；POSIX 再检查撤名并重建后旧实例重复撤名不会破坏新对象。
2　匿名与缺名／六端。不存在的名称以及空名加零容量均无法映射。同时创建两个 16 字节匿名区，后建者初始全零，对其写入不改变先建者。先建者可逐字节清零且不能撤名。将全部 16 字节写为不同的非零值，close 并重映射，Windows 保留全部字节，POSIX 的 16 字节全为零；另一个匿名区仍保留自己的标记。
3　权限／六端。Windows 建立只读节，写升级失败后原读视图和 memory() 保持有效；对仅允许读写映射权限的节按名加入并经不同视图核对回写。POSIX 给对端只读权限，以原生 O_RDWR 实际结果判断是否允许写（兼容特权用户），读端须看到偏移 17 的 0x73。macOS 默认共享内存在创建时指定权限，以 fstat 取得容量；普通文件或 Linux 后备用 fchmod 设置权限。另建零长度对象，首次映射失败，扩至 64 字节并具备写权限后同一实例可重试，另一实例读到末字节 0x52。
4　真正的父子进程／Windows、Linux、Linux ARM、macOS、Android。父进程设首字节为 0x36、次字节为零；全新测试进程按名读取，写回 0x91。父进程必须同时看见正常退出和正确回写。
5　目录引用／Linux、Linux ARM、macOS、Android。在独立子进程切换两次有效配置，尝试不存在的目录应失败并保留上一次选择。随后改名并重建原路径；97 字节后备仅出现在原目录对象中。预置同名子目录应被拒绝。两个实例可读写末字节，配置已封存、创建者析构撤名、临时目录可清理。
6　保护与容量／Windows。三页中间页改只读，realSize 仍为三页，clear 后首尾标记保留；恢复可写后清零。中间页改 PAGE_GUARD 后应保留首字节及保护标志。x64 另保留 4 GiB 只提交第一页，容量超出 uint32_t 时返回 0，但可清零请求的 128 字节，紧随其后的标记不变。
7　显式保留名称／六端。缺名的只打开实例在创建者就绪后重试成功。创建者 preserveName 后退出，旧对端仍能读到末字节。所有旧引用释放后，Windows 不能再打开；POSIX 新实例仍可读取并显式撤名。
8　清零范围／六端。73 字节全部填入不同的非零值，以 17 字节加入，在尚未取视图时调用 clear。再填充前缀，显式取得写视图后调用 clear。两次都用独立预期数组比较全部 73 字节，前 17 字节为零，尾部 56 字节不变。
9　仅授予 FILE_MAP_WRITE／Windows。系统独立打开核实允许写映射、拒绝单独读及组合权限后，检查加入、首次读、首次写、无视图容量查询、清零和重映射；原生及库视图读到相同的末字节。
10　默认析构／六端。创建者直接退出，对端持有的旧视图仍见 0x48，close 并重映射也能读取。Windows 此时新实例可按名加入。POSIX 新打开失败，排他重建的数据从零开始，写入 0x51 不影响旧视图。另一个只查询过容量的实例在创建者退出后才首次映射，仍须读到旧对象的 0x48。释放全部引用后可排他创建，得到零数据。
11　命名存储系统调用／Linux、Linux ARM 静态 SDK。对首次预留和首次设置长度分别注入 EINTR，后续调用真实 libc；要求二者使用同一描述符并重试，预留从偏移 0 开始，每次预留和设置长度均请求 16452 字节，最终容量恰为 16452 字节，另一实例可读回末字节 0x75。分别注入 ENOSPC 和 EFBIG 时创建失败，错误调用仅执行一次，随后相同名称可排他重建。

构建与平台范围
Windows 使用 VS2013 v120，先用 version_debug.sh 或 version_release.sh 准备其指定的 32 位 DLL SDK，再编译对应解决方案。x64 另准备同配置的 64 位 SharedMemory、PlatformTest 及 SDK 头，TestSdkRoot 指向该 SDK；测试 EXE 旁放同批 DLL。静态链接须使用对应静态 SDK 的宏头及库。
Linux、macOS 在本测试 CMakeLists.txt 所在目录运行 cmake -S . -B build -DSDK_ROOT=<同目标SDK> -DCMAKE_BUILD_TYPE=Release，然后 cmake --build build，在 build 内执行 ctest -V。
Linux ARM 使用与被测库相同目标 ABI 的 PlatformTest 和编译器。在 Linux 上可给测试工程传入 -DBIT=arm32 或 -DBIT=arm64 选择 GNU 工具链；交叉构建测试时加 -DCMAKE_TOOLCHAIN_FILE=<同目标工具链>。配置时按实际编译器检查 ARMv7 硬浮点或 AArch64 ABI，选错工具链会使配置失败。在目标 Linux 上运行 ./SharedMemoryTest --all，动态链接时确保加载同一 SDK 的库。适用编号为 1、2、3、4、5、7、8、10。对应配置链接 Linux 静态 SDK 且 nm 可识别其系统调用时再登记 11，执行 ./SharedMemoryTest --cases 11 可单独复测。
Android 加 -DCMAKE_TOOLCHAIN_FILE=<NDK>/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24；连接 adb 设备，必要时指定 PLATFORM_TEST_DEVICE 和 PLATFORM_TEST_RUNTIME_LIBRARIES。iOS 用 -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64，模拟器选择 -DCMAKE_OSX_SYSROOT=iphonesimulator，真机选择 iphoneos 并提供开发团队和设备 UDID。多配置目标使用 cmake --build build --config Release，在 build 内运行 ctest -C Release -V。
Android/iOS 宿主设置 SHARED_MEMORY_TEST_DIR 或 TMPDIR 为绝对可写目录。iOS 运行 1、2、3、7、8、10；App Group 中两个独立签名进程之间的通信需另在设备验证。

功能验证结果（2026-11-23，上海时间）
Windows VS2013 v120，Win32、x64 × Debug、Release × DLL、静态库，共 8 种组合。核对了每种程序的库导入。最终 --all 批次每种运行 1、2、3、4、6、7、8、9、10 九项，共 72 次通过，0 失败、0 取消。--cases 1,3-4 返回 0，未登记的 5 和空压力集合分别返回 2。x64 Release 的 CMake/CTest、默认及 --instant 入口均通过，单独启动 9 个编号也全部通过。
将隔离副本中的 realSize 正常容量人为增加 1 字节，用例 1 在创建者尚未映射时即检出偏差并返回 1，正式库通过相同用例。SharedMemory.cpp 末尾的注释 demo 在 Win32、x64 各以 Debug 和 Release 编译运行，4 次均返回 0。
本批 x64 Release 动态链接实测产物的 SHA-256 前 16 位分别为 SharedMemoryTest.exe 464e094af11a77fa、SharedMemory.dll d68211318b6feef2、PlatformTest.dll 1b651cdb1bd36878。
Linux x86_64 在 Windows 上交叉构建了生产库和测试程序。单配置 Debug、双配置 Debug/Release 均链接成功。静态 Debug 程序包含用例 11 的两个系统调用拦截符号，动态 Release 程序使用共享库。
Linux ARM 已检查 ARMv7 硬浮点及 AArch64 源码分支。两种 ABI 的编译条件通过对应目标的 Clang 前端检查，ARM 软浮点及错误架构被拒绝。测试 CMake 分别指定 arm32、arm64 时均拒绝 x86_64 工具链。本机编译器缺少 ARM 机器码后端，未生成 ARM 目标程序。macOS、Android、iOS 已核对平台源码、构建与测试入口。五个非 Windows 目标均未执行功能用例。

性能用例与判定
可选的 --benchmark 运行四个 C++ 场景，独立于功能选择。两个 4096 字节匿名对象分别保持已映射和未映射；预热、校准后取九批，不把创建和首次映射计入。测试调用方关闭跨翻译单元优化，避免静态链接下重复取地址被折叠。按系统页大小或请求长度计算期望容量。每批检查地址复用次数、累计容量、未映射对象的 memory() 及清零后的回读值。清零场景每次先改写不同偏移，再 clear 并读回。Windows 在当前处理器组内选择进程掩码允许的最低一位，固定计时线程并记录实际处理器组和组内编号。其他目标记录由系统调度。
使用 CMake 3.18 或以上，配置时加 -DLIBRARY_TEST_BENCHMARKS=ON，再执行 cmake --build build --config Release --target check_shared_memory_benchmarks。宿主串行执行两组，每组启动一个完整的预热进程，再启动五个独立采样进程。每个场景须返回正迭代数、每轮一次操作、九个正耗时样本及校验和。所有进程报告的映射容量必须一致。Windows 的预热和采样进程还须报告相同的处理器组和组内编号，缺失、格式错误或中途改变均判失败。取地址场景的校验和须等于迭代数，容量查询须等于迭代数乘容量，清零须为零。宿主按原值从小到大排列，用第 3、5、7 个值复核 Q1、中位数和 Q3；缺样、重复记录或统计值不符使检查失败。宿主报告五个中位数及其中位数（ps/次，1000 ps = 1 ns）。每个场景分别核对三个比值：采样进程内的 Q3/Q1、每组五个进程中位数的最大值/最小值、两组中位数的较大值/较小值。每项均须不超过 1.10。超界时退出失败并保留观测，提示本批无法建立稳定基线。此阈值用于判别采样波动。跨批比较前固定硬件、目标 ABI、优化/链接模式、电源策略、处理器和负载。单次 --benchmark 或 ctest -V -L performance 可查看单进程原始观测。
使用 27 组固定记录检验宿主判定，其中包含 Windows 处理器编号和其他平台系统调度两种有效记录。分位数正确的乱序样本和组间差异恰好为 10% 的样本通过。缺容量、容量变化、缺行、重复行、样本不足、样本格式错误、校验和长度错误、零耗时、零迭代、分位数错误、四个场景各自的校验和值错误，以及进程内、进程间、组间超差都返回失败。缺处理器标记、处理器组或组内编号中途改变、编号越界、调度方式与目标不符也须失败。另在 Windows x64 Release 单独执行计时前的准备代码，读到容量 4096 字节、处理器组 0、组内编号 0。以上检查均未采集接口耗时。

当前版本性能数据与测量范围
本次未运行性能测试。Windows、Linux、Linux ARM、macOS、Android、iOS 均缺少可与当前构建对应的性能测试数据。现有汇总缺少与当前产物的对应记录。

可计时场景                         计时边界与成本
writeWithoutLock，已持有写视图      复用现有地址，每次调用核对返回地址；不含首次打开与映射。
realSize，已持有视图                Windows 逐区域查询，区域越多查询次数越多。POSIX 已打开后读取缓存容量。
realSize，尚无视图                  对已创建的匿名对象反复查询。Windows 每次临时映射、查询并解除；POSIX 读取容量，不需要映射。此场景不测按名首次打开。
clear，4 KiB 已映射可写区           每次写一个标记、清零并读回；包含这两次校验访问。清零工作量随字节数增长，Windows 还检查整段页面保护。

创建、按名加入、首次读写映射、权限升级、close 后重映射及其他长度的 clear 均缺少性能测试数据。跨进程传输延迟还取决于调用方的同步协议，库内的地址获取耗时不能用于推算该延迟。
size、isFailed、memory 仅读取实例状态，preserveName 只设置回收策略，无需单列耗时。mapName 按值返回名称，复制成本随名称长度变化，本次无需单列。setProcessDirectory、unlinkName、close 和析构属于配置或资源回收操作，本次无需单列性能数据。
*/

/** 当前功能用例的稳定编号，各目标登记可运行的场景
*/
enum SharedMemoryTestCaseId
{
    // 命名映射、容量边界和读写视图
    kNamedViews = 1,
    // 缺名和匿名对象的有效期
    kAnonymous = 2,
    // 只读权限和写视图获取
    kReadAccess = 3,
    // 新进程按名打开并回写
    kProcesses = 4,
    // 目录改名和后备文件位置
    kDirectory = 5,
    // 分区保护与容量表示上限
    kMappingExtent = 6,
    // 保留名称与最后引用回收
    kNameLifetime = 7,
    // 短前缀清零的写入范围
    kPartialClear = 8,
    // 仅有写映射权限时加入和读写
    kWriteMappingRight = 9,
    // 创建者默认析构和对端重映射后的名称寿命
    kDefaultNameLifetime = 10,
    // 文件预留与设置长度遭遇信号中断或硬错误
    kStorageInterrupt = 11
};

#ifdef SHM_WRAP_RESERVE
/** Linux 静态库用例一次创建操作的调用记录，仅由测试线程借用
*/
struct StorageCallProbe
{
public:
    // 首次预留的错误码，0 表示执行真实预留
    int m_reserveError;
    // 首次设置长度时写入 errno 的错误码，0 表示执行真实调用
    int m_resizeError;
    // 独立于实现指定的期望字节数
    unsigned m_expectedBytes;
    // 当前线程对该实例的预留次数
    unsigned m_reserveCalls;
    // 当前线程对该实例的长度设置次数
    unsigned m_resizeCalls;
    // 首次预留使用的描述符，-1 表示尚未预留
    int m_descriptor;
    // 所有调用的描述符、长度和预留偏移是否符合预期
    bool m_argumentsMatch;

    /** 结束本线程对本地记录的借用，包括构造抛出异常的路径
    */
    ~StorageCallProbe();
};
#endif
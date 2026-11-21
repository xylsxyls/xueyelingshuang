#pragma once

/** SharedMemory 原生测试说明（2026-11-22）

运行和判定
测试程序链接所选目标 SDK 的 SharedMemory 和 PlatformTest，全部场景通过生产公开接口触发。默认、--all 或 --instant 执行本平台登记的瞬时用例；--cases 1,3-4 只执行所选项。编号必须全部适用于目标。没有压力用例，--stress 返回 2。每项输出编号及返回值，断言失败输出行号和表达式。进程退出 0 表示选中项全部通过，1 表示功能失败，2 表示参数无效或无可运行项。
每项使用进程、时间和序号组成的新名称。普通实例退出作用域释放视图与句柄，显式保留的名称由用例撤销。用例 4、5 等待子进程最多 15 秒，超时终止并回收。桌面可终止测试进程取消；强制终止 POSIX 创建者可能留下其独占名称。移动端由 PlatformTest 宿主管理测试目录和设备结果。

用例与独立预期（全部为瞬时）
1　命名视图／五端。排他创建 128 字节后重复创建、等长加入、过大加入、请求 129 字节。按后备实际边界判断：Windows 检查页边界，macOS 默认后备用系统 fstat 作参照，其余目标精确到字节。失败实例不能撤名。内嵌 NUL 不能打开前缀对象。用 128 个已知字节验证双向可见、只打开端 clear 无副作用、读后写、close 和重映射；POSIX 再检查撤名并重建后旧实例重复撤名不会破坏新对象。
2　匿名与缺名／五端。不存在的名称以及空名加零容量均无法映射。16 字节匿名区可写、清零后逐字节为零、不能撤名。写标记后 close 并重映射，Windows 保留标记，POSIX 重新得到零页。
3　权限／五端。Windows 建立只读节，写升级失败后原读视图和 memory() 保持有效；对仅允许读写映射权限的节按名加入并经不同视图核对回写。POSIX 给对端只读权限，以原生 O_RDWR 实际结果判断是否允许写（兼容特权用户），读端须看到偏移 17 的 0x73。macOS 默认共享内存在创建时指定权限，以 fstat 取得容量；普通文件或 Linux 后备用 fchmod 设置权限。另建零长度对象，首次映射失败，扩至 64 字节并具备写权限后同一实例可重试，另一实例读到末字节 0x52。
4　真正的父子进程／Windows、Linux、macOS、Android。父进程设首字节为 0x36、次字节为零；全新测试进程按名读取，写回 0x91。父进程必须同时看见正常退出和正确回写。
5　目录引用／Linux、macOS、Android。在独立子进程选择私有目录，改名并重建原路径；97 字节后备仅出现在原目录对象中。预置同名子目录应被拒绝。两个实例可读写末字节，配置已封存、创建者析构撤名、临时目录可清理。
6　保护与容量／Windows。三页中间页改只读，realSize 仍为三页，clear 后首尾标记保留；恢复可写后清零。中间页改 PAGE_GUARD 后应保留首字节及保护标志。x64 另保留 4 GiB 只提交第一页，容量超出 uint32_t 时返回 0，但可清零请求的 128 字节，紧随其后的标记不变。
7　显式保留名称／五端。缺名的只打开实例在创建者就绪后重试成功。创建者 preserveName 后退出，旧对端仍能读到末字节。所有旧引用释放后，Windows 不能再打开；POSIX 新实例仍可读取并显式撤名。
8　清零范围／五端。73 字节全部填入不同的非零值，以 17 字节加入后 clear。用独立预期数组比较全部 73 字节，前 17 字节为零，尾部 56 字节不变。
9　仅授予 FILE_MAP_WRITE／Windows。系统独立打开核实允许写映射、拒绝单独读及组合权限后，检查加入、首次读、首次写、无视图容量查询、清零和重映射；原生及库视图读到相同的末字节。
10　默认析构／五端。创建者直接退出，对端持有的旧视图仍见 0x48，close 并重映射也能读取。Windows 此时新实例可按名加入。POSIX 新打开失败，排他重建的数据从零开始，写入 0x51 不影响旧视图。释放全部引用后可排他创建，得到零数据。

构建与平台范围
Windows 使用 VS2013 v120，先用 version_debug.sh 或 version_release.sh 准备其指定的 32 位 DLL SDK，再编译对应解决方案。x64 另准备同配置的 64 位 SharedMemory、PlatformTest 及 SDK 头，TestSdkRoot 指向该 SDK；测试 EXE 旁放同批 DLL。静态链接须使用对应静态 SDK 的宏头及库。
Linux、macOS 在本测试 CMakeLists.txt 所在目录运行 cmake -S . -B build -DSDK_ROOT=<同目标SDK> -DCMAKE_BUILD_TYPE=Release，然后 cmake --build build，在 build 内执行 ctest -V。
Android 加 -DCMAKE_TOOLCHAIN_FILE=<NDK>/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24；连接 adb 设备，必要时指定 PLATFORM_TEST_DEVICE 和 PLATFORM_TEST_RUNTIME_LIBRARIES。iOS 用 -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64，模拟器选择 -DCMAKE_OSX_SYSROOT=iphonesimulator，真机选择 iphoneos 并提供开发团队和设备 UDID。多配置目标使用 cmake --build build --config Release，在 build 内运行 ctest -C Release -V。
Android/iOS 宿主设置 SHARED_MEMORY_TEST_DIR 或 TMPDIR 为绝对可写目录。iOS 运行 1、2、3、7、8、10；App Group 中两个独立签名进程之间的通信需另在设备验证。

功能验证结果（2026-11-22）
Windows VS2013 v120 的 Win32、x64 各自 Debug/Release 分别以 DLL 和静态库链接，共八种组合，均从源码构建。每种组合 --all 执行 1、2、3、4、6、7、8、9、10 共 9 项，72 次通过、0 失败。SharedMemory.cpp 末尾逐行注释的 demo 在 Windows 两种位数及 Debug/Release 四种配置下独立编译，退出码均为 0。编号 5 在 Windows 未登记，--cases 5 及无压力项的 --stress 均返回 2。Windows x64 Release 的独立 CMake/CTest 功能入口通过。Linux x86_64 成功交叉编译并链接，未在 Linux 运行。macOS、Android 和 iOS 仅静态审查，未在设备编译或执行。

性能用例与判定
可选的 --benchmark 运行四个 C++ 场景，独立于功能选择。两个 4096 字节匿名对象分别保持已映射和未映射；预热、校准后取九批，不把创建和首次映射计入。测试调用方关闭跨翻译单元优化，避免静态链接下重复取地址被折叠。按系统页大小或请求长度计算期望容量。每批检查地址复用次数、累计容量、未映射对象的 memory() 及清零后的回读值。清零场景每次先改写不同偏移，再 clear 并读回。Windows 固定在当前进程所允许的最低编号逻辑处理器，输出掩码。
使用 CMake 3.18 或以上，配置时加 -DLIBRARY_TEST_BENCHMARKS=ON，再执行 cmake --build build --config Release --target check_shared_memory_benchmarks。宿主串行执行一次完整预热及五次独立进程采样。每进程保留九批原值、Q1、中位数和 Q3，宿主报告五个中位数及其中位数（ps/次，1000 ps = 1 ns）。每个场景要求所有进程 Q3/Q1 和五个中位数最大/最小均不超过 1.10。超界时退出失败并保留观测，提示本批无法建立稳定基线。此阈值用于判别采样波动。跨批比较前固定硬件、目标 ABI、优化/链接模式、电源策略、处理器和负载；两批中位数也应使用同一容差。单次 --benchmark 或 ctest -V -L performance 可查看单进程原始观测。

性能口径与已有数据（ns/次）
本次未运行性能测试。下表为项目已记录的 Windows x64 样本。现存记录给出的条件是 i9-14900HX、逻辑处理器 8、VS2013 v120 Release /MT，4096 字节匿名后备、单线程、同一进程内复用对象。共三组，每组预热两个进程，随后七个进程各取九批。本次未取得与当前构建哈希配对的原始采样，缺少本次构建可独立核验的性能数据。

场景                          第二组中位数 [Q1, Q3]          第二组最小～最大       三组各自的中位数（按采样顺序）
已有可写视图再取地址           1.051 [0.948, 1.167]           0.947～1.227           1.462 / 1.051 / 0.982
持有视图调用 realSize       390.758 [337.805, 440.851]     319.014～451.135      489.719 / 390.758 / 336.894
未持有视图调用 realSize    1977.368 [1727.368, 2088.959]  1719.745～2279.669    3143.542 / 1977.368 / 1868.280
写标记、clear 4 KiB 并读回   254.041 [234.849, 283.297]     233.223～301.161      344.461 / 254.041 / 250.583

三个组的中位数最大/最小分别为 1.49、1.45、1.68、1.37。这组记录可用于理解操作的相对成本，波动超出当前 10% 的复现判据。以上为调用循环的观测值，含返回值核对或写入和读回标记的成本。
缺少性能测试数据的场景包括创建及按名加入、首次读写映射、只读升级、close 后重映射、清零不同长度或多保护区域、跨进程通知和另四端的全部场景。size、isFailed、memory 只读取本地状态，preserveName 只设置析构策略，无需单列性能指标。mapName 返回字符串副本，成本取决于名称长度。目录配置、显式撤名及析构属于启动或回收路径，本次无单独计时需求。
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
    kDefaultNameLifetime = 10
};
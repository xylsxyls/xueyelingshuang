#pragma once
#include "LogManager/LogManagerAPI.h"
#include <locale>

/** LogManager 测试说明（2026-11-24）

运行与判定
使用 version_release.sh 或 version_debug.sh 准备同位数、同配置的 LogManager 和 PlatformTest SDK。
Windows 使用 LogManagerTest.sln；原生 CMake、Android/iOS 设备宿主和 Wasm 共用 LogManagerTest.cpp。
程序默认或 --all 依次运行该宿主已登记的瞬时和压力用例，--instant、--stress 按类型选择，--cases 1,3,9-12 可选具体项。
启动时检查用例登记；执行时以 UTF-8 输出编号、中文场景和预期；终端和采集工具请选 UTF-8。退出码 0 表示选定用例全部通过，1 表示判定失败，2 表示选择非法或没有可执行项。指定范围包含保留编号 8 时报告一次跳过，其余编号按升序去重执行；只选 8 返回 2。非法编号或本宿主不支持的编号会使整批选择失败。
每批使用新进程或新 Wasm 模块。测试排他创建临时目录，从实体文件独立读回数据，并在结束时清理；链接不重复计数。
移动端由 PlatformTest 设备入口提供可写 TMPDIR。Node 将系统临时目录挂到 NODEFS。无持久化能力的 Wasm 宿主只登记 1、14。
单线程 Wasm 构建不登记原生多线程用例。进程创建、跳过析构的退出和 Windows 删除保护由真实 Windows 子进程验证。

编号、类型和预期
1  瞬时。核对默认 ID、路径、100 MiB、5 卷、100 条检查周期和两个输出策略。采样序号 0 至 39 按独立算式检查负、零、1、7 四种间隔。
2  瞬时。实际禁写后再启用，查询已打开和已关闭的 ID。读回一卷，丢弃项不出现，六次调用中序号 0、2、4 各一条，1、3、5 各零条。另分别在打开和关闭之间切换 writeBeginEnd，两种方向的 BEGIN、END 各按当时开关判断，业务消息始终只有一条。
3  压力。256 字节阈值、每条检查、保留 3 卷，连续写 80 条。结束时必须恰有 3 个实体卷，含第 0 卷及最后一条；deleteFile 清掉当前会话。
4  压力，原生。4 个线程各写 80 个可区分标记。关闭后恰有 1 卷、322 个已提交记录，含 BEGIN/END，按换行逐条解析，完整正文必须与一个尚未出现过的预期标记完全相等，所有线程序号恰有一份，成功提交的卷内无零字节空洞。
5  瞬时。开关边界标记后独立读回每次调用的结果。检查记录前导换行、[1]、正文 CR/LF 转义、无尾部换行、跨映射边界的长记录，以及自定义时间、进程、线程、文件、函数、等级及格式实参；含内嵌 NUL 的函数名按 std::string 的完整长度读回。
6  瞬时，Node。仅令一条正文的宿主 fsyncSync 失败，它保留 [0]。观察下一条两次真实刷新前的文件字节，第一次是带完整正文的 [0]，第二次只改变标志为 [1]。另令宿主只写半段正文并在补写时返回 EIO；读回故障卷，将故障偏移设在固定正文的中点，从文件首字节起逐字节核对原记录、[0]、实际写入的半段、所有剩余零字节及下一条完整 [1] 记录。另在正文已刷新、标志改写后，仅令第二次刷新失败。Node 当前文件可读到 [1]，该条不应输出控制台副本；紧随其后的正常记录须落盘并输出副本。
7  压力，原生。与 4 相同的并发输入，加上 1024 字节滚动阈值；必须产生多个卷，跨卷仍有 322 条完整记录，所有业务标记恰有一份。
9  瞬时。预设非零 ID 后修改原对象，禁写时分别调用 init() 和 init(cfg)，两个目录都应无日志文件，两个路径均查不到会话。重新启用后用 LOGINFO 在预设目录延迟打开 ID 0，再通过显式配置把原预设的非零 ID 打开在另一目录，验证控制台副本和无 BEGIN。Windows 从独立子进程实际继承的标准输出核对，覆盖 DLL 和静态库。显式 ID 使用自己的输出配置。随后用无参 init 打开预设的非零 ID。可选控制台输出失败时，关闭释放 ID，归档卷须含 BEGIN、业务记录和 END。共享 C++ 运行库的测试使用拒写的输出缓冲区并开启异常；Windows 独立 CRT 的 DLL 路径从子进程实际标准输出验证副本。
10 瞬时。1 字节阈值下写 10 条固定格式的消息。周期 3、卷数 -1 时，第 0、1、2、3 卷各含序号 0–2、3–5、6–8、9。周期取 0、负数或 1，卷数分别覆盖 1、2、0、-2 时，只保留第 0、9 卷且各含一条。关闭后核对实体卷名和每卷全部字节。
11 瞬时。五个等级分别通过默认及指定 ID 的普通、EVERY 和 ONCE 宏写入。从实体卷逐条比对已提交标志、实际等级和完整正文，核对所属 ID、序号、重复数及 Debug 和 Release 过滤。一个调用点交替传两个 ID，以名为 logManagerCallIndex 的业务变量作为间隔和消息实参。序号 0、3、6 各写一次，消息实参恰好求值三次，记录含调用方变量的原值。间隔表达式再次进入同一调用点时，外层占用序号 0，内层占用序号 1；间隔为 2 时只能读到外层消息。原生端另让 4 个线程同时首次进入相同的 EVERY 和 ONCE 调用点，64 次调用须分别命中 10 次和 1 次，所有成功求值的消息在实体卷各有一份。
12 瞬时。预放外部文件，打开另一个会话并正常归档。只有本会话的卷进入独立归档目录并含 BEGIN、业务记录和 END，外部文件字节保持原样。
13 压力，Windows。检查 8 个线程各 500 条、4 个进程向同一目录输出、当前卷删除保护、旧卷可删、相对链接、同 ID 重开、多 ID、析构归档、显式关闭、禁归档和删除。目标同名、归档路径被普通文件阻挡时核对源文件。父进程等写入就绪后，通过创建时取得的句柄结束该测试子进程，核对已提交文件的名称和字节；后续正常会话也须保留这些文件。子进程有就绪和退出时限。Windows 读取者持有禁止删除的链接句柄时，滚动仍须写入新卷。过期链接不能命中当前卷查询。释放句柄后分别检查下次滚动、关闭归档和显式删除。一直持有到关闭时，归档只保留实际卷，不带过期链接。无符号链接权限时单独报告相关跳过项。
14 瞬时，Wasm。尝试向纯内存挂载写入，查询应返回 -1，目录中应无实体文件。Node 也运行这一项。
15 压力。配置单卷字节数 -1、逐条检查，真实写入 101 份 1 MiB 正文和一个尾标记。首卷长度必须超过 100 MiB，最后字节匹配尾标记，未生成第 1 卷。
16 瞬时。非法 ID、内嵌 NUL 和目录被文件阻挡时不应注册会话；使用有效配置可重试。已使用的 ID 重复初始化仍绑定原路径，全局禁写后关闭不会补写 END。
17 瞬时。按实体路径反查 ID。排他重建已打开的卷应失败，原记录和随后写入均可读；向已有目标排他移动失败且两处内容均保留。
18 瞬时。以相对路径打开后切换进程工作目录，新目录中预放同名文件。分别执行滚动后删除、滚动后归档。核对两条消息在原目录的两个卷内、绝对及相对查询的结果、新目录同名文件的字节和归档后的两卷。
19 压力。分别把单卷字节数 0、-2 设为默认配置并通过无参 init 打开。逐条检查容量，100 个 1 MiB 载荷使首卷严格超过 100 MiB，下一条短消息独占第 1 卷。
20 瞬时。在首卷写满后，预先写入与第 1、2 卷同名的外部文件。下次应打开第 3 卷，此后保留首卷和第 4 卷；删除会话后两份外部文件应逐字节保持原样。Node 额外拒绝一次新卷创建；本条应写入原卷，下一次仍能滚动。Node 还让旧卷暂时拒绝删除和属性查询，解除拒绝后再滚动，须移除滞留卷并恢复保留数量。
21 瞬时。打开不滚动的预设 ID 后更改路径、ID、容量、卷数和检查间隔，再无参打开第二个 ID。向两者交错写入，前者保留全部五条，后者仅保留首末两条，各自关闭后独立读回。
22 瞬时。在 C++ 全局区域设置中启用三位数字分组。以固定时间、线程 12345 和等长消息写三条，配置单卷阈值恰好等于第一条已提交记录的字节数。第 0 卷须与前两条预期字节完全一致，第 1 卷只含第三条。

当前结果
Windows VS2013 Win32 Release、x64 Release、x64 Debug 的静态库及 x64 Release DLL，四组各通过全部 19 个适用编号（13 个瞬时、6 个压力）。四组均在具备符号链接权限的进程中实际执行用例 13，各完成 179 项判定，零失败、零跳过。
Node / Emscripten 3.1.74 的 wasm32 Release、wasm64 Release、wasm32 Debug 通过正式 CMake/CTest 入口执行全部 18 个适用编号（15 个瞬时、3 个压力），均通过。用例 6 的三次写盘失败、用例 14 和 16 的打开失败，以及用例 20 的新卷创建失败，均为预期故障。
在独立的 Windows x64 Release 对照程序中，将一条 WARNING 改为 INFO 并保留相同正文，用例 11 在比对实体卷等级时返回 1。
另将 Windows x64 Release 的第 2 号消息改为重复第 1 号，卷数及首末消息均正确，用例 10 仍在核对卷内字节时返回 1。在独立 Node wasm32 Release 进程中只改短写空洞前的一个字节，用例 6 在核对故障卷时返回 1。
Windows x64 和 Node wasm32 的 Release 程序都以严格 UTF-8 解码核对了中文用例预期。指定 8-10,9,1,1 时只输出一次跳过 8，依次执行 1、9、10 并返回 0。只选 8、倒序、0、越界、空段、混入其他宿主的编号六类输入均返回 2，没有执行用例。
在 Win32、x64、wasm32、wasm64 的 Release 构建中分别从 LogManager.cpp 取出 main 示例编译运行，示例从相同目录先查询到 ID 0，关闭它后再查询到 ID 7，删除 ID 7 后磁盘上恰有一份已提交的启动消息。Windows x64 Release 也通过 DLL 及导入库运行此示例。另在 x64 和 Node wasm32 的中文含空格目录核对了示例实体卷。
Linux x86_64 使用目标 sysroot 对库、跨端测试及 CSystem 持久文件实现做过 C++11 编译前端检查。Linux、Linux ARM、macOS、Android 和 iOS 没有运行结果。浏览器、微信真机和设备断电未在本轮实测。

本次全量正确性复测的程序身份
下列路径相对于本轮临时构建目录 src/temp/LogManager/audit，记录执行文件 SHA-256 及整套用例墙钟时间。各构建同时运行，时间仅用于复核测试批次。Wasm 同时记录宿主 JS；其余依赖为静态链接，DLL 组另列实际使用的 LogManager.dll。
x64-Release/lib/LogManagerTest.exe  095AFF39AFDFE0A9D263A979961CFB1BB556E2104D5B15BA1A120B63CD185C29  59.160 秒
Win32-Release/lib/LogManagerTest.exe  0810BE05E2C829D42336FF935F30B211B1C9D091F6AC2C44B846965DED19D052  59.953 秒
x64-Debug/lib/LogManagerTestd.exe  DD05F760279C3C14ADD3EA55F58A796B89F06128325F708C6AC81CD623F49C60  109.192 秒
wasm32-Release/build/LogManagerTest.wasm  E9965F054DEBC3312776657F7CD709348158048D1D291D8A1F3C4DDCDA702B13  16.51 秒
    同目录 LogManagerTest.js  79DFCF68A56331ADED8E772AF5B9D0BE95FF6AC87692992B4C5504047B818E24
wasm64-Release/build/LogManagerTest.wasm  221A956ACF4D9A0F75A8116AF7D77E229C1C5BC05F97687FD4E8B7D72488D8B6  22.58 秒
    同目录 LogManagerTest.js  74716B11FB25ED1977497187F5C060EA8C6C4B3D2EE51C3C5CFF64DC6F11523B
wasm32-Debug/build/LogManagerTest.wasm  3346B3BAD5D05F3C8E36F9D3CCCD778AD114F35698C371164BB5746AE8949950  39.06 秒
    同目录 LogManagerTest.js  CF88FBDBD6B979AD6AD1F84DC9DA3E295D5B85864A3B61366A7A32ED87BC81C3
x64-Release/dll-tests/lib/LogManagerTest.exe  A0A816CE2D821E8A403E141CB9AF60926C46917847171895B9C7ABACF8DE5322  28.414 秒（含增量构建）
    同目录 LogManager.dll  836586CDDB73F2C1D47B47A5B5C6D3F09EE6F422EA3DE248E32C55D9D7C58F28

性能入口和数据
本次不运行性能测试。当前版本在七端均缺少性能测试数据。配置复制、开关、初始化失败、重复关闭、按路径查找及显式删除以正确性判定，无需单列微基准。多线程、滚动、归档和控制台场景缺少专项性能实测。
--benchmark 单独运行两个场景。间隔 7 的 needPrintEvery 用独立算式检查命中数。单线程 LOGINFO_EX 向一个预打开、不滚动且无控制台或 BEGIN/END 的本地卷写固定宽度整数，含格式化和两次持久刷新；计时结束后从实体卷核对每条 [1] 和序号。两者均由 PlatformTest 校准批量并记录 9 轮样本、中位数及四分位数。建目录、打开/关闭、读回均在写入计时区间外。
未来对比需固定 Release 链接配置、磁盘和挂载、宿主、处理器及负载。在启用 LIBRARY_TEST_BENCHMARKS 的 CMake 构建目录运行 cmake -DMAX_SPREAD_PERCENT=<事先确定的容差百分比> -P <测试源码目录>/CheckBenchmarkSeries.cmake。先预热一个独立进程，再串行运行五个进程，检查样本完整性和四分位数，报告进程中位数的中位数及全距。各实测进程内以（第三四分位数－第一四分位数）/第一四分位数核验波动，跨五进程以（最大中位数－最小中位数）/最小中位数核验波动。任一超过同一约定容差时返回失败；缺少稳定样本时不发布单值。固定合成样本验证了一致数据通过，并验证错误中位数、单进程内波动超限、跨进程波动超限、缺少容差均失败。未执行日志接口计时。
*/

/** 各端共用稳定编号；宿主只登记能够实际验证的用例 */
enum LogTestId
{
    // 默认配置及采样算式
    LogTestSampling = 1,
    // 启停及实体文件读回
    LogTestReadBack = 2,
    // 滚动和删除
    LogTestRolling = 3,
    // 同卷并发
    LogTestConcurrent = 4,
    // 逐条提交格式
    LogTestFormat = 5,
    // 持久化故障
    LogTestFailedFlush = 6,
    // 并发跨卷
    LogTestConcurrentRoll = 7,
    // 初始化前的默认配置及独立 ID
    LogTestDefaults = 9,
    // 容量、数量和周期边界
    LogTestLimits = 10,
    // 全部公开日志宏
    LogTestMacros = 11,
    // 会话归档和关闭
    LogTestArchive = 12,
    // Windows 子进程退出与归档
    LogTestChildren = 13,
    // 无持久化能力的 Wasm 宿主
    LogTestMemory = 14,
    // 超过默认阈值后的无限容量验证
    LogTestUnlimited = 15,
    // 非法配置、重试、重复初始化和关闭开关
    LogTestInvalid = 16,
    // 排他创建及目标冲突保留原文件
    LogTestExclusive = 17,
    // 相对路径打开后更改工作目录
    LogTestRelativeDir = 18,
    // 无效字节上限在默认边界换卷
    LogTestDefaultBytes = 19,
    // 未来卷号被外部文件占用时继续滚动
    LogTestRollCollision = 20,
    // 运行中替换预设只影响后续会话
    LogTestPresetIsolation = 21,
    // 宿主数字区域设置和精确字节边界
    LogTestExactBytes = 22
};

/** 单个用例的登记信息，执行和报告共用同一条目 */
struct LogCase
{
    // 是否属于压力用例
    bool m_stress;
    // 中文场景与可独立核验的预期
    const wchar_t* m_expectation;
    // 执行真实检查，成功返回零
    int (*m_run)();
};

/** 用逗号人为区分宿主展示用的整数和日志协议中的十进制整数
*/
class LogTestGroupedNumbers : public std::numpunct<char>
{
protected:
    /** 提供千位分隔符
    @return 逗号
    */
    char do_thousands_sep() const override;

    /** 每三位插入一次分隔符
    @return 重复组宽 3
    */
    std::string do_grouping() const override;
};

/** 管理单个测试独占的目录，退出时先关闭日志再清理目录 */
struct LogSession
{
public:
    // 私有目录及测试使用的文件 ID
    LogManagerConfig config;

    /** 创建独占目录，失败时配置路径为空 */
    LogSession();

    /** 释放日志并回收本次创建的目录树 */
    ~LogSession();
};

/** 删除测试自行创建的目录树，遇到链接只删除链接本身
@param [in] path 本次测试排他创建的目录
*/
void RemoveLogTestTree(const std::string& path);

#ifdef _WIN32
/** 链接被外部读取者占用后，分别核验四种收尾路径
*/
enum class LogTestLinkAction
{
    // 释放读取句柄后再次滚动
    RollAgain,
    // 释放读取句柄后关闭并归档
    Archive,
    // 释放读取句柄后显式删除
    Delete,
    // 保持句柄到会话关闭和归档结束
    ArchiveWhilePinned
};

/** 供测试独立核对会话身份及分卷名 */
struct LogTestFileName
{
    // 包含扩展名的观察目标
    std::string m_fileName;
    // 本次会话名，不含卷号和扩展名
    std::string m_baseName;
    // 文件名中的日期
    std::string m_date;
    // 文件名中的时刻
    std::string m_time;
    // 文件所属的子进程 ID
    int32_t m_pid;
    // -1 表示链接，非负值为实体卷号
    int32_t m_fileIndex;
};
/** 验证外部读取者暂时阻止链接更新时的分卷、查询及关闭处理
@return 四种收尾的实体和链接均符合预期时返回 0
*/
int CheckBusyLogLink();

/** 验证独立进程生命周期及归档冲突
@return 所有判定通过返回 0
*/
int CheckLogProcesses();

/** 在测试程序子进程中运行指定场景
@param [in] argc 进程参数数量
@param [in] argv 进程参数，包括父进程的私有目录和同步文件
@return 子进程结果
*/
int RunLogChild(int argc, char** argv);
#endif
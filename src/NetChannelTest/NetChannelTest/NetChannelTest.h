#pragma once

/** NetChannelTest 功能用例与性能口径

无参数或 --all 先执行全部瞬时用例，再执行全部压力用例。--cases 1,4 按编号升序去重执行，--instant 和 --stress 按类型选择。
每个用例通过 SDK 公开入口创建独立实例，输入在内存中生成；预期取自手写大端字节、固定正文和独立的额度计算。
同步调用返回即可判定，失败会打印文件、行号及条件。退出码 0 表示通过，1 表示用例失败，2 表示选择无效。Node 的共用启动器会把非零结果转换为进程退出码 1，具体 C++ 退出码见错误信息。
外层运行器可设置每次功能测试 60 秒超时或取消进程。无外部网络、文件及需跨用例释放的资源。
Windows 使用本工程的 VS 项目及同配置 SDK。其他目标从本目录的 CMakeLists.txt 构建，SDK_ROOT 指向对应位宽的 SDK。
Linux x86 按 BIT=32/64 构建，Linux ARM 按 arm32/arm64 或指定工具链；macOS 和 iOS 按目标选择 64/arm64，Android 使用 NDK 工具链和对应 ABI。显式指定 BIT 时须与编译器的目标架构、指针宽度相符，切换 ABI 使用独立构建目录。工程自带 VS Code 设置预选 Linux x86 的 BIT=32；macOS 手动构建时将 cmake.configureArgs 中的 BIT 改为目标 SDK 的 64 或 arm64。
Wasm 使用 Emscripten 工具链、BIT=wasm32/wasm64 和 LIBRARY_TEST_HOST=ON；在生成目录用 node LibraryTestHost.js NetChannelTest.js --all 运行。

1 瞬时，独立线格式。编码后逐字节比对手写样本；目标少一字节应报告所需容量，整个缓冲区保持原值。READY 时核对路由字段、二进制正文和消费长度。
2 瞬时，双向额度。配置 81920 字节并用尽，信用返还后应可继续发送；超额本地消费保留 pending，对端多返还则进入持续协议错误。
3 压力，分片与截断。遍历样本每个头部和正文切点，断流应返回持续 NC_TRUNCATED；补齐则须还原完整字段和正文，并能读取下一条。
4 瞬时，记录与字节流。第二组手写样本覆盖切点两端、双帧粘连、HELLO 空正文和另外三类逐字节输入；每次只交付一帧并保留后缀。
5 瞬时，协议错误。逐字段更改固定头、类型、通道号和长度。分别提交单字段非法的 HELLO、CONTROL、BYTES 和 CLOSE 给编码器及解码器。发送失败保留目标，接收只消费坏帧的 24 字节头，区分协议错误与超限；后续合法输入不能清除错误。
6 瞬时，编码缓冲。核对大小查询、无效字段、输入输出重叠、相邻边界和地址溢出；允许输出覆盖已备份的记录描述符，实际正文与手写样本相同。分别将正文置于实际输出的两侧、恰好相邻、仅一字节相交、完全相交和未用尾部，逐字节比对整块缓冲区。长度输出与描述符、有效正文或目标区间重叠要拒绝并保留原值。feed 拒绝 consumed 覆盖输入后，同一解码器可接纳原始记录。
7 压力，正文缓冲复用。以 257 字节分片读取 16 KiB 上界。另一个实例从小到大依次接收各档容量前一字节、边界、后一字节的合法正文（256 字节至 16 KiB）。每帧在最后一字节到达前须返回 NC_NEED_MORE，完成后逐字节匹配。再交替读取长、短和空记录 64 轮，核对每轮内容及结束状态。
8 瞬时，信用窗口边界。遍历 64 KiB 至 1 MiB 的全部合法窗口、非法窗口、零值、超额和部分返还，检查两方向独立及错误后的公开状态。

9 瞬时，中途拒绝与流恢复。在半头、半正文、READY 三种状态插入错误参数和空输入。检查已接纳的前缀和借用视图有效，同一实例能继续读完并在正常 EOF 后复用。
10 瞬时，配置与通道隔离。在未开始记账时提交无效本地操作，再配置窗口。部分扣账和返还只改变对应方向，额度归还后配置仍冻结。一个通道发生远端错误时，另一个通道继续正常工作。
11 瞬时，分配失败。仅在使用 MSVC 调试 CRT 的 Windows Debug 进程注册。首条小记录已消费后，拒绝下一条正文缓冲扩容，预期仅消费固定头并持续返回 NC_NO_MEMORY。恢复分配后，新实例正常解码。

--benchmark 单独测量 1024 字节正文的 encode。输入与输出缓冲预先准备，每次调用检查返回值、长度并抽样读取正文。固定输入、构建配置、运行宿主和逻辑核，多次启动独立进程并比较批次中位数。
本轮未运行性能测试。当前版本的编码、分片解码及信用记账均缺少性能测试数据。
构造、析构、单次配置和状态查询无需单列吞吐指标。

本轮功能验证（2026-11-25）：Windows Win32、x64 均分别运行静态库和 DLL。四组 Debug 各通过 11 项（9 项瞬时、2 项压力），四组 Release 各通过 10 项（8 项瞬时、2 项压力）。Node Wasm32、Wasm64 Release 各通过 10 项。第 11 项需要调试 CRT，在这些 Release 配置中未注册。两种 Windows 位宽和两种 Wasm 位宽的示例均输出 41 00 42。Linux x86 的 32/64 位编译器通过 CMake 架构检查，位宽或架构填错会被拒绝；未链接 Linux 版 SDK。Linux ARM、macOS、Android、iOS 已检查源码及构建入口。这五端均缺少目标机运行结果。

逐编号独立启动、按类型分组、混选去重及非法选择退出码均已验证。Wasm32 和 Wasm64 另用未定义行为检测构建，各通过 10 项。Windows x64 Release 单独验证了两项用例的判错能力。在隔离副本中让编码器拒绝合法相邻缓冲时，用例 6 报错；让 257 字节的合法记录无法扩容时，用例 7 报错。换回正式 DLL 后，两项均通过。

函数与用例索引。下列编号可用 --cases 单独运行，私有方法通过真实公共入口覆盖。
NetChannel
  3、4、5、7 号　NetChannel、~NetChannel、feed、peek、consume、finish、acceptHeader、readNumber、valid
  6 号　encode、writeNumber、文件内长度输出重叠检查
  9 号　feed、peek、consume、finish
  11 号　acceptHeader 的分配失败路径
NetChannelCredit
  2、8、10 号　NetChannelCredit、configure、capacity、available、sent、grant、received、pending、consume、status
*/

// 命令行选择使用的稳定用例编号。
enum NetChannelCaseId
{
    // 手写线格式，编码字节完整匹配，借用到消费结束
    NetChannelCaseWire = 1,
    // 双向信用额度，发送和接收独立记账，拒绝重复返还
    NetChannelCaseCredit = 2,
    // 分片与断流，每个切点复原正文，过早结束返回截断
    NetChannelCaseFragments = 3,
    // 连续记录和粘连，交付全部四类记录，输入可以复用
    NetChannelCaseRecordStream = 4,
    // 坏头与超长正文，消费固定头后保留错误
    NetChannelCaseInvalidHeader = 5,
    // 编码缓冲边界，拒绝非法重叠和长度，失败保留输出
    NetChannelCaseEncodeBounds = 6,
    // 大小正文交替复用，最大、短小和空正文逐字节匹配
    NetChannelCaseStorageReuse = 7,
    // 信用窗口边界，按协议接受全范围窗口并保持额度不变量
    NetChannelCaseWindowLimits = 8,
    // 部分输入、借用与可恢复的参数错误
    NetChannelCaseFeedRecovery = 9,
    // 本地拒绝后的配置与独立通道额度
    NetChannelCaseCreditIsolation = 10,
    // 调试 CRT 定点拒绝扩容
    NetChannelCaseAllocationFailure = 11
};

// 供 PlatformTest 调度器选择并执行用例，id 与 stress 为调度器约定字段。
struct NetChannelTestCase
{
    // 已登记的稳定编号
    NetChannelCaseId id;
    // 为 true 时纳入压力分类
    bool stress;
    // 中文场景名称
    const char* m_name;
    // 根据协议或输入独立计算的预期
    const char* m_expected;
    // 调用公开接口并返回本项判定结果
    int (*m_run)();
};
#pragma once

/** NetChannelTest 功能用例与性能口径

无参数或 --all 执行全部功能用例。--cases 1,4 按编号选择，--instant 和 --stress 按类型选择。
每个用例通过 SDK 公开入口创建独立实例，输入在内存中生成；预期取自手写大端字节、固定正文和独立的额度计算。
同步调用返回即可判定，失败会打印文件、行号及条件。退出码 0 表示通过，1 表示用例失败，2 表示选择无效。Node 的共用启动器会把非零结果转换为进程退出码 1，具体 C++ 退出码见错误信息。
外层运行器可设置每次功能测试 60 秒超时或取消进程。无外部网络、文件及需跨用例释放的资源。

1 瞬时，独立线格式。编码后逐字节比对手写样本；目标少一字节应报告所需容量，整个缓冲区保持原值。READY 时核对路由字段、二进制正文和消费长度。
2 瞬时，双向额度。配置 81920 字节并用尽，信用返还后应可继续发送；超额本地消费保留 pending，对端多返还则进入持续协议错误。
3 压力，分片与截断。遍历样本每个头部和正文切点，断流应返回持续 NC_TRUNCATED；补齐则须还原完整字段和正文，并能读取下一条。
4 瞬时，记录与字节流。第二组手写样本覆盖切点两端、双帧粘连、HELLO 空正文和另外三类逐字节输入；每次只交付一帧并保留后缀。
5 瞬时，协议错误。逐字段更改固定头、类型、通道号和长度；只消费坏帧的 24 字节头，区分协议错误与超限；后续合法输入不能清除错误。
6 瞬时，编码缓冲。核对大小查询、无效字段、输入输出重叠、相邻边界和地址溢出；允许输出覆盖已备份的记录描述符，实际正文与手写样本相同。长度输出与描述符、有效正文或目标区间重叠要拒绝并保留原值。feed 拒绝 consumed 覆盖输入后，同一解码器可接纳原始记录。
7 压力，正文缓冲复用。以 257 字节分片读取 16 KiB 上界；同一解码器再交替读取长、短和空记录 64 轮，逐字节核对内容及每轮结束状态。
8 瞬时，信用窗口边界。遍历 64 KiB 至 1 MiB 的全部合法窗口、非法窗口、零值、超额和部分返还，检查两方向独立及错误后的公开状态。

2026-11-25 功能回归。Win32 Debug、Windows x64 Release、Node Wasm32 Release 和 Node Wasm64 Release 均运行全部8项。Linux、Linux ARM、macOS、Android、iOS 检查对应源代码及构建入口，缺少实机运行结果。

--benchmark 单独测量 1024 字节正文的 encode。输入与输出缓冲预先准备，每次调用检查返回值、长度并抽样读取正文。固定输入、构建配置、运行宿主和逻辑核，多次启动独立进程并比较批次中位数。
本轮未运行性能测试。包含长度输出重叠检查的编码、分片解码和信用记账，均缺少当前实现的性能测试数据。
构造、析构、单次配置和状态查询无需单列吞吐指标。内存耗尽返回 NC_NO_MEMORY 的分支未做确定性故障注入。

当前回归验证了 Windows 静态库及 DLL 的实际链接，Wasm32 和 Wasm64 的未定义行为检测构建均通过全部编号用例。Chrome 无界面浏览器的 Wasm32 和 Wasm64 各通过8 项（6 项瞬时、2 项压力）。

函数与用例索引（Win32 Debug）
逐项单独运行并跟踪进入的生产函数，共 21 个具名入口。下列编号可用 --cases 单独运行。
每个函数列出至少一个实际进入它的用例。同名重载可能对应多个编号。输入、预期输出和状态变化见相应的编号说明。仅内联且没有独立符号的函数随调用场景检查。
NetChannel
  3 号　NetChannel、 acceptHeader、 consume、 feed、 finish、 peek、 readNumber、 valid、 ~NetChannel
  6 号　encode、 writeNumber
NetChannelCredit
  2 号　NetChannelCredit、 available、 capacity、 configure、 consume、 grant、 pending、 received、 sent、 status
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
    NetChannelCaseWindowLimits = 8
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
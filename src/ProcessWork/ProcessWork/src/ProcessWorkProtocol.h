#pragma once
#include <stdint.h>

// 非阻塞发送结果；成功只代表正文已原子提交，不代表业务已经处理。
enum ProcessWorkSendResult
{
    // 完整正文已经共享提交，不表示业务处理完成
    PROCESS_WORK_SENT = 0,
    // 仅trySend报告仍观察到空闲容量时的CAS竞争失败，本次未提交
    PROCESS_WORK_BUSY = 1,
    // 参数无效，本次未提交
    PROCESS_WORK_INVALID = -1,
    // 接收器或底层资源不可用，本次未提交
    PROCESS_WORK_UNAVAILABLE = -2,
    // 超过声明正文容量，不截断也不提交
    PROCESS_WORK_TOO_LARGE = -3,
    // 本次本地对象或映射申请失败，未提交
    PROCESS_WORK_NO_MEMORY = -4,
    // 本次观察到正文或队列段没有剩余容量，未提交
    PROCESS_WORK_CAPACITY = -5,
    // 登记/元数据尚未准备好或可用资源正保留给先到等待者，未提交
    PROCESS_WORK_RESOURCE = -6
};

#ifdef _MSC_VER
#define PROCESS_WORK_ALIGN8 __declspec(align(8))
#define PROCESS_WORK_ALIGN64 __declspec(align(64))
#else
#define PROCESS_WORK_ALIGN8 alignas(8)
#define PROCESS_WORK_ALIGN64 alignas(64)
#endif

// v5线格式只包含固定宽度字段，无本地地址和STL对象，32/64位进程布局一致。
struct PROCESS_WORK_ALIGN8 ProcessWorkSharedDirectory
{
    // 正数接收器代次，0空闲，-1正在准备
    int64_t m_session;
    // 操作系统提供的完整进程创建标识
    uint64_t m_identity;
    // 单调分配的接收器代次，达到24位上限后拒绝创建
    int64_t m_nextSession;
};

struct PROCESS_WORK_ALIGN8 ProcessWorkSharedPeer
{
    // 0可登记，正数发送PID，负数正在回收登记
    int32_t m_pid;
    // 保留字段，初始化为0
    int32_t m_reserved;
    // 操作系统提供的完整发送进程创建标识，批准token之前发布
    uint64_t m_identity;
    // 接收器批准的正数登记号，0尚未批准
    int64_t m_token;
};

// 正文槽可以退役，元数据地址在本代通道内始终保留；各槽占用独立缓存行。
struct PROCESS_WORK_ALIGN64 ProcessWorkSharedBlock
{
    // 0可申请，-1未启用；正数是发送进程登记号，正文消费完成后由接收器归还0。
    int64_t m_owner;
    // 只在未启用时递增，达到名称可表示范围后不回绕。
    int64_t m_generation;
    // v5利用原缓存行尾部保存至多48字节的包头及正文，节点标志决定读取位置。
    char m_inlineData[48];
};

// 每段固定30条独立提交，分散到四条缓存行；正文槽仍按实际容量独立计费。
enum { kProcessWorkSegmentCells = 30 };

// 队列段元数据与正文槽独立，不以批量调用改变逐包提交语义。
struct PROCESS_WORK_ALIGN64 ProcessWorkSharedNode
{
    // 0可复用，-1尚未启用，-2当前读取段，-3代次耗尽；正数为登记号。
    int64_t m_owner;
    // 高32位为本节点代次；低32位为后继下标加1，0为无后继。
    int64_t m_link;
    // 高32位是本段代次；低24位为正文下标，bit24标记内联，低32位0表示未提交。
    // 提交后直到整段退役都不清零，旧生产者不能在已消费的位置重复发布。
    int64_t m_cells[kProcessWorkSegmentCells];
};

// 每个实际发送线程拥有独立发布端；请求和授予只在资源竞争慢路径使用。
struct PROCESS_WORK_ALIGN64 ProcessWorkSharedStream
{
    // 正数为所属进程token，负token表示登记构造中；0尚未占用或死亡且排空。
    int64_t m_owner;
    // 所属操作系统线程号；同进程同线程经不同本地通道实例仍归同一流。
    int64_t m_thread;
    // 只有发送线程更新的当前写段：高32位代次，低32位下标加1。
    int64_t m_writeNode;
    // 只有发送线程更新的下一个写单元，0..30。
    int64_t m_writeOffset;
    // 发送调用取消后置1，由接收器确认清理，不以暂停推断死亡。
    int64_t m_cancel;
    // 0空闲，1/2等待正文/段，3/4已排队，5/6已授予正文/段。
    int64_t m_request;
    // 已授予的正文或段下标，request的release发布使其可见。
    int64_t m_grant;
    // 仅接收器维护的等待链后继，0为末尾。
    int64_t m_nextWaiter;
    // 仅接收器推进的当前读段。
    PROCESS_WORK_ALIGN64 int64_t m_head;
    // 当前段的下一读取单元。
    int64_t m_headSlot;
    // 仅在确认进程身份失效后置1；仍需保留已提交正文。
    int64_t m_dead;
    // 每个登记流附带一个初始段，避免空流永久占完正文对应的段池。
    ProcessWorkSharedNode m_first;
};

struct PROCESS_WORK_ALIGN64 ProcessWorkSharedMetadata
{
    // 协议版本，当前为5
    int32_t m_version;
    // 本页的全局页号
    int32_t m_page;
    // 本页256个正文槽的稳定元数据
    ProcessWorkSharedBlock m_blocks[256];
    // 本页256个可复用队列节点
    ProcessWorkSharedNode m_nodes[256];
    // 与正文目录独立增长的发送线程登记及初始队列段。
    ProcessWorkSharedStream m_streams[256];
};

struct PROCESS_WORK_ALIGN64 ProcessWorkSharedControl
{
    // 控制协议版本6要求发送前检查接收本地额度；旧发送者必须拒绝连接
    int32_t m_version;
    // 控制区结构字节数，用于拒绝布局不兼容的参与方
    int32_t m_bytes;
    // 单块声明字节数，含12字节消息头
    int32_t m_blockBytes;
    // 永久保留的初始正文块数
    int32_t m_initial;
    // -1表示无业务配置上限；表示范围和系统资源仍可能耗尽。
    int32_t m_limit;
    // 可用块数阈值，0只由失败申请触发增长
    int32_t m_threshold;
    // 一次扩容计划增加的块数
    int32_t m_increment;
    // 扩容正文空闲回收秒数，0禁用缩容
    int32_t m_idleSeconds;
    // 1可发送，0准备中或停止
    int32_t m_ready;
    // 当前实际启用的正文块数
    int32_t m_active;
    // 已发布的最大正文下标，只增长；不以最大容量预分配目录。
    int32_t m_extent;
    // 0未满，1待报告，2本轮满载已报告
    int32_t m_full;
    // 生产者设置的扩容检查请求，接收器合并清除
    int32_t m_demand;
    // 合并的数据到达提示，不能当作消息数量
    int32_t m_dataSignal;
    // 发送者先置1再复查空间，接收器置0并提示
    int32_t m_spaceSignal;
    // 最大可接纳正文字节数，兼顾共享单块和接收本地单项额度；-1表示连空正文也放不下
    int32_t m_maxPayloadBytes;
    // 单调分配发送流下标；0保留，每个登记只申请一次，不用CAS争抢登记槽。
    PROCESS_WORK_ALIGN64 int64_t m_nextStream;
    // 已完整发布的新流通知代次；暂停的未发布登记不阻挡后面的登记。
    PROCESS_WORK_ALIGN64 int64_t m_streamEpoch;
    // 完整发布等待请求后递增；生产者不覆盖消费者的确认代次。
    PROCESS_WORK_ALIGN64 int64_t m_requestEpoch;
    // 接收器确认没有待授予请求的代次；归还优先直接授予等待者，已授予资源不能被新申请抢走。
    PROCESS_WORK_ALIGN64 int64_t m_servedEpoch;
    // 最后批准的登记号，耗尽不回绕
    PROCESS_WORK_ALIGN64 int64_t m_nextToken;
    // 最多256个同时登记的发送进程，同进程线程共享登记
    ProcessWorkSharedPeer m_peers[256];
};

#undef PROCESS_WORK_ALIGN8
#undef PROCESS_WORK_ALIGN64
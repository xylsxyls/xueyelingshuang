#pragma once
#include "ProcessWorkMacro.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <atomic>
#include <memory>
#include <vector>
#include <stdint.h>

#ifdef _MSC_VER
#define PROCESS_WORK_ALIGN8 __declspec(align(8))
#else
#define PROCESS_WORK_ALIGN8 alignas(8)
#endif

// v2共享入口；进程创建标识与接收器代次共同确定实际控制区名称。
struct PROCESS_WORK_ALIGN8 ProcessWorkDirectory
{
    // 0未就绪，-1准备中，正数为接收器代次；只通过原子操作读写
    int64_t m_session;
    // 进程实例标识：桌面OS创建标识或移动参与者随机登记，发布session前写完
    uint64_t m_identity;
    // 同一映射中不回绕的接收器代次分配器，关闭不会清零
    int64_t m_nextSession;
};

// 由接收器确认的发送进程身份；发送者得到token之前不得占用数据块。
struct PROCESS_WORK_ALIGN8 ProcessWorkPeer
{
    // 0未分配；正数为请求注册的PID
    int32_t m_pid;
    // 明确填充，保持两端32/64位布局一致
    int32_t m_reserved;
    // 发布token前写入的完整创建标识
    uint64_t m_identity;
    // 0待确认；正数为不可复用的注册号
    int64_t m_token;
};

// 数据块只有FREE能被抢占或退役，发布后由唯一接收者释放。
struct PROCESS_WORK_ALIGN8 ProcessWorkBlock
{
    // 0空闲，-1未启用/退役，-2接收方正在复制，正数为发送进程token
    int64_t m_owner;
    // 只在未启用状态修改，发布FREE之后保持不变
    int64_t m_generation;
};

// v2固定布局，无指针、STL对象或进程内锁；64位字段均按8字节对齐。
struct PROCESS_WORK_ALIGN8 ProcessWorkControl
{
    // 版本标记、布局字节数、声明块容量及初始块数
    int32_t m_version;
    int32_t m_bytes;
    int32_t m_blockBytes;
    int32_t m_initial;
    // 最大块数、低水位、增长步长及闲置回收秒数
    int32_t m_limit;
    int32_t m_threshold;
    int32_t m_increment;
    int32_t m_idleSeconds;
    // 1接收就绪，0正在关闭；当前启用块数
    int32_t m_ready;
    int32_t m_active;
    // 满载事件0未触发、1待分派、2本轮已分派；维护请求标记
    int32_t m_full;
    int32_t m_demand;
    // 合并数据通知；空间请求只在等待者需要时通知，避免历史令牌形成空转
    int32_t m_dataSignal;
    int32_t m_spaceSignal;
    // 下一提交票号；环槽CAS发布完整正文后，任何参与者均可帮助推进
    int64_t m_tail;
    // 只由接收任务修改的下一接收票号
    int64_t m_head;
    // 单调注册号，原子读取/发布；耗尽返回不可用，不能回绕复用
    int64_t m_nextToken;
    // 发送进程身份，最多256个同时存活的发送进程
    ProcessWorkPeer m_peers[256];
    // 元数据上限固定，扩缩容只改变数据映射
    ProcessWorkBlock m_blocks[1024];
    // 高位为票号，低11位为块下标加1；0表示此票尚未提交
    int64_t m_ring[1024];
};

// 非阻塞发送结果；成功只代表正文已原子提交，不代表业务已经处理。
enum ProcessWorkSendResult
{
    // 完整正文已经共享提交，不表示业务处理完成
    PROCESS_WORK_SENT = 0,
    // 本次未提交，可以按原顺序有截止地重试
    PROCESS_WORK_BUSY = 1,
    // 参数无效，本次未提交
    PROCESS_WORK_INVALID = -1,
    // 接收器或底层资源不可用，本次未提交
    PROCESS_WORK_UNAVAILABLE = -2,
    // 超过声明正文容量，不截断也不提交
    PROCESS_WORK_TOO_LARGE = -3
};

/** ProcessWork与NetSender共用的v2传输实现；业务通常使用ProcessWork门面。
    open/close由生命周期owner串行执行；完成openSender后trySend可以多线程调用。
*/
class ProcessWorkAPI ProcessWorkChannel
{
public:
    /** 初始化空对象，不启动线程或创建IPC资源。 */
    ProcessWorkChannel();
    /** 静止状态下释放本实例资源；调用方先停止所有借用任务。 */
    ~ProcessWorkChannel();
    /** 准备接收区，尚不对发送者发布就绪。
    @param [in] bytes 单块总容量，含12字节头
    @param [in] initial 固定初始块数
    @param [in] threshold 剩余块数不高于此值时请求增长
    @param [in] increment 每次增加的块数，0禁用增长
    @param [in] idleSeconds 扩容块空闲释放秒数，0不自动缩容
    @param [in] rounds 最大保留增长份数，容量上限initial+increment*rounds
    @return true准备完成，false已回滚本次资源
    */
    bool openReceiver(int32_t bytes, int32_t initial, int32_t threshold, int32_t increment,
        int32_t idleSeconds, int32_t rounds);
    /** 接收任务均已准备后发布入口；只由接收器生命周期owner调用。 */
    void publish();
    /** 撤销接收入口并唤醒等待者；释放映射前仍须加入接收任务。 */
    void stop();
    /** 静止状态释放资源，重复调用安全。 */
    void close();
    /** 打开既有接收区并请求身份登记，不等待登记完成。
    @param [in] pid 真实接收进程号
    @return true已打开，false不存在、协议不兼容或身份不可查询
    */
    bool openSender(int32_t pid);
    /** 检查缓存是否仍对应当前接收器代次，不查询OS进程存活。
    @return true共享入口仍就绪
    */
    bool ready() const;
    /** 查询本代声明的最大正文字节，不使用系统页对齐余量
    生命周期owner不与open/close并发；查询不登记新发送者，也不预留数据块
    @return 就绪时为声明单块容量减12，空包专用或未就绪通道为0
    */
    int32_t payloadCapacity() const;
    /** 验证名称发现记录与已打开通道属于同一进程代次。
    @param [in] identity 名称记录的完整创建标识
    @return true身份一致且本接收器代次仍就绪
    */
    bool matchesIdentity(uint64_t identity) const;
    /** 单次有界申请和提交；BUSY表示未提交，允许同一消息重试。
    @param [in] data 借用正文，length为0时允许空指针
    @param [in] length 正文字节数
    @param [in] type 非负消息类型
    @return ProcessWorkSendResult，不通过异常报告正常失败
    */
    int32_t trySend(const char* data, int32_t length, int32_t type);
    /** 满载时睡眠等待空间，并核对接收进程完整身份。
    @return true接收端仍可用，false关闭或确认换代/退出
    */
    bool waitForSpace();
    /** 唯一接收任务调用，复制FIFO正文、维护注册/死亡回收/扩缩容。
    @param [in] queue 接收正文所有权队列
    @param [in] notification 本地接收任务通知
    @param [in] fullCount 合并后的满载快照，0表示没有待处理事件
    @return 本次复制成功的消息数量
    */
    int32_t pump(LockFreeQueue<char*>& queue, Semaphore& notification, std::atomic<int32_t>& fullCount);
    /** 接收任务空闲时等待通知，有限等待兼顾提交后发送者立即退出。 */
    void waitForData();
    /** 唤醒接收等待，用于StopTask。 */
    void wake();
    /** 当前活动块数快照。
    @return 未初始化时0，运行时为初始块加当前保留扩容块
    */
    int32_t bufferCount() const;
    /** 生成原生名称，最多30个ASCII字节；macOS通知为29字节，身份编码不截断、不哈希。
    @param [in] kind C控制区、B数据块、R数据通知、W空间通知
    @param [in] pid 进程号
    @param [in] identity 完整系统创建标识
    @param [in] session 24位接收器代次
    @param [in] index 块下标
    @param [in] generation 24位块代次
    @return 两端一致的编码名称
    */
    static std::string objectName(char kind, uint32_t pid, uint64_t identity, uint32_t session,
        uint32_t index = 0, uint32_t generation = 0);

private:
    /** 禁止复制独占生命周期。
    @param [in] other 不可复制对象
    */
    ProcessWorkChannel(const ProcessWorkChannel& other);
    /** 禁止复制独占生命周期。
    @param [in] other 不可复制对象
    @return 不可调用
    */
    ProcessWorkChannel& operator=(const ProcessWorkChannel& other);
    /** 创建一块且最后发布FREE。
    @param [in] index 固定元数据下标
    @return true成功，false仍保持未启用
    */
    bool createBlock(int32_t index);
    /** 唯一接收任务进行有界维护。
    @param [in] notification 本地回调通知
    @param [in] fullCount 合并后的满载事件
    */
    void maintain(Semaphore& notification, std::atomic<int32_t>& fullCount);
    /** 唯一接收任务批准新发送者，注册号永不复用。 */
    void approveSenders();
    /** 分批确认死亡，只释放未提交占位，保留已提交消息。 */
    void recoverExitedSenders();
    /** 唯一接收任务管理扩容块的创建和空闲退役。
    @param [in] now 开始扫描空闲状态的单调毫秒
    @param [in] notification 本地回调通知
    @param [in] fullCount 合并满载快照
    */
    void resizeBuffers(uint64_t now, Semaphore& notification, std::atomic<int32_t>& fullCount);
    /** 取得本发送进程已获批准的注册号。
    @return 正数批准号，0尚未批准或无容量
    */
    int64_t senderToken();
    /** 合并数据通知，不为每个包在内核累积一个空唤醒。 */
    void signalData();
    /** 只有等待者请求空间时才发布一次空间提示。 */
    void signalSpace();
    /** 已确认接收进程退出时清理其唯一代次的POSIX对象名称，不碰固定发现入口。
    发送者缓存仍拥有映射引用，已有并发调用不会访问解除映射后的地址。
    */
    void cleanupExitedReceiver();

private:
    // 本地角色、目标PID及进程完整创建标识
    bool m_receiver;
    // stop只撤销ready；直到close完成释放才交出发现入口。
    bool m_stopped;
    int32_t m_pid;
    uint64_t m_identity;
    uint64_t m_selfIdentity;
    // 本地接收器代次、发送进程号及请求登记下标
    int64_t m_session;
    int32_t m_selfPid;
    int32_t m_peerIndex;
    // 本实例拥有的共享入口与控制区映射，指针仅借用这些映射
    std::unique_ptr<SharedMemory> m_directoryMemory;
    std::unique_ptr<SharedMemory> m_controlMemory;
    ProcessWorkDirectory* m_directory;
    ProcessWorkControl* m_control;
    // 完成准备后不改变容器大小；发送者只缓存固定初始块，扩容块每次调用结束即解除映射
    std::vector<std::unique_ptr<SharedMemory> > m_memories;
    // 单一接收任务私有的空闲起点及维护游标；起点0表示刚归还，下一轮维护开始计时
    std::vector<uint64_t> m_idleSince;
    uint64_t m_nextMaintenance;
    int32_t m_peerCursor;
    // 数据到达及空间可用通知，没有跨进程互斥锁
    Semaphore m_dataReady;
    Semaphore m_spaceReady;
    // 多发送线程仅用原子变量轮转起点，不串行化发送
    std::atomic<uint32_t> m_cursor;
    // 映射/分配异常后的重试退让；不改变已提交报文状态。
    std::atomic<bool> m_retryDelay;
};

#undef PROCESS_WORK_ALIGN8
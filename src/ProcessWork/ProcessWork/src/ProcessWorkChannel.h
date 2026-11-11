#pragma once
#include "ProcessWorkMacro.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "LockFreeMap/LockFreeMapAPI.h"
#include <atomic>
#include <memory>
#include <vector>
#include <stdint.h>

#include "ProcessWorkProtocol.h"
#include "ProcessWorkMemory.h"

class ProcessWorkStorage;
class ProcessWorkPage;
class ProcessWorkSharedQueue;
class ProcessWorkProducerState;
class ProcessWorkReceiveQueue;

/** ProcessWork与NetSender共用的v6控制协议传输实现；业务通常使用ProcessWork门面。
    open/close由生命周期owner串行执行；完成openSender后trySend可以多线程调用。
*/
class ProcessWorkAPI ProcessWorkChannel
{
public:
    /** 初始化空对象，不启动线程或创建IPC资源。
    */
    ProcessWorkChannel();

    /** 静止状态下释放本实例资源；调用方先停止所有借用任务。
    */
    ~ProcessWorkChannel();

    /** 准备接收区，尚不对发送者发布就绪。
    @param [in] bytes 单块总容量，含12字节头
    @param [in] initial 固定初始块数
    @param [in] threshold 剩余块数不高于此值时请求增长
    @param [in] increment 每次增加的块数，0禁用增长
    @param [in] idleSeconds 扩容块空闲释放秒数，0不自动缩容
    @param [in] rounds 最大保留增长份数，-1无配置上限；非负值的容量上限initial+increment*rounds
    @return true准备完成，false已回滚本次资源
    */
    bool openReceiver(int32_t bytes, int32_t initial, int32_t threshold, int32_t increment,
        int32_t idleSeconds, int32_t rounds);

    /** 使用显式内存保护策略准备接收区，其余参数与六参数重载相同。
    @param [in] bytes 单块总容量
    @param [in] initial 初始块数
    @param [in] threshold 扩容剩余块数阈值
    @param [in] increment 每次计划增加的块数
    @param [in] idleSeconds 扩容块闲置回收秒数
    @param [in] rounds -1无配置上限
    @param [in] memory 内存余量、扩容分批和失败退避策略
    @return true准备成功，false完整回滚
    */
    bool openReceiver(int32_t bytes, int32_t initial, int32_t threshold, int32_t increment,
        int32_t idleSeconds, int32_t rounds, const ProcessWorkMemoryConfig& memory);

    /** 接收任务均已准备后发布入口；只由接收器生命周期owner调用。
    */
    void publish();

    /** 撤销接收入口并唤醒等待者；释放映射前仍须加入接收任务。
    */
    void stop();

    /** 静止状态释放资源，重复调用安全。
    */
    void close();

    /** 打开既有接收区并请求身份登记，不等待登记完成。
    @param [in] pid 真实接收进程号
    @return true已打开，false不存在、协议不兼容或身份不可查询
    */
    bool openSender(int32_t pid);

    /** 静止且未打开通道时设置生产者登记Map的完整池参数
    @param [in] config 节点、值、增长及保护记录参数
    @return true已保存；false已打开或复制失败
    */
    bool configureProducers(const LockFreeMapConfig& config);

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

    /** 按开关申请并提交；默认有界尝试，BUSY表示尚未提交
    @param [in] data 借用正文，length为0时允许空指针
    @param [in] length 正文字节数
    @param [in] type 非负消息类型
    @param [in] retryContention true持续重试CAS竞争，false允许返回未提交竞争状态
    @param [in] retryCapacity true等待可复用容量；前4096次快速重试，之后每次等待1毫秒
    @param [in] stop 可空的协作停止标记，调用期间保持有效
    @return ProcessWorkSendResult，不通过异常报告正常失败
    */
    int32_t trySend(const char* data, int32_t length, int32_t type, bool retryContention = false,
        bool retryCapacity = false, const std::atomic<bool>* stop = nullptr);

    /** 普通发送在内部不限次数重试竞争及容量不足；4096次容量重试之后每轮休眠1毫秒
    @param [in] data 借用正文
    @param [in] length 正文字节数
    @param [in] type 消息类型
    @param [in] stop 可选协作停止标志，调用期间有效
    @return ProcessWorkSendResult；SENT已完整提交，不返回BUSY；首次身份登记可能等待接收器确认
    */
    int32_t send(const char* data, int32_t length, int32_t type, const std::atomic<bool>* stop = nullptr);

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

    /** 唯一读取线程复制到可复用批次，返回前发布本轮所有副本。
    @param [in,out] queue 接收正文批次和计费所有者
    @param [in,out] fullCount 满载事件快照
    @return 本次复制成功的消息数量；失败的提交保留本地副本供重试
    */
    int32_t pump(ProcessWorkReceiveQueue& queue, std::atomic<int32_t>& fullCount);

    /** 接收任务空闲时等待通知，有限等待兼顾提交后发送者立即退出。
    */
    void waitForData();

    /** 唤醒接收等待，用于StopTask。
    */
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
    /** 共用正文复制及提交，普通路径允许等待已登记的资源授予
    @param [in] data 正文
    @param [in] length 字节数
    @param [in] type 消息类型
    @param [in] retryContention 竞争失败是否无限重试
    @param [in] retryCapacity 容量不足是否公平登记并等待
    @param [in] stop 可选停止标志
    @return ProcessWorkSendResult
    */
    int32_t transmit(const char* data, int32_t length, int32_t type, bool retryContention, bool retryCapacity,
        uint32_t& capacityAttempts, const std::atomic<bool>* stop);

    /** 失败后只读复查剩余资源，不把已满误报为竞争Busy
    @param [in] node true检查段，false检查正文
    @param [in] extent 当前正文最大下标
    @param [in] contended 本次确实经历了CAS竞争
    @return Capacity表示未见空闲；Busy要求存在空闲且刚发生竞争；缺少映射或公平授予保留返回Resource
    */
    int32_t unavailableResource(bool node, uint32_t extent, bool contended);

    /** 等待已登记资源，定期核对关闭、取消及接收进程身份
    @param [in,out] producer 当前线程登记
    @param [in] node true段，false正文
    @param [out] index 授予下标
    @param [in,out] attempts 本次调用共用的容量重试次数
    @param [in] stop 可选停止标志
    @return true取得资源，false关闭或取消
    */
    bool waitForGrant(ProcessWorkProducerState& producer, bool node, uint32_t& index, uint32_t& attempts, const std::atomic<bool>* stop);

    /** 共用空间通知和接收进程存活核对
    @param [in] timeout 一次通知等待毫秒数
    @param [in] probeSpace 是否允许发现空正文后立即返回；已登记请求必须等待自己的授予
    @return true接收端仍可用
    */
    bool waitForSpaceImpl(int32_t timeout, bool probeSpace = true);

    /** 共用FIFO读取及死亡回收路径，兼容逐包队列和接收端批次队列。
    @param [in] queue 逐包模式的输出队列，批次模式为空
    @param [in] notification 回调线程通知
    @param [in,out] fullCount 满载快照
    @param [in] batches 批次计费所有者，与逐包输出互斥
    @return 已复制消息数
    */
    int32_t pumpImpl(LockFreeQueue<char*>* queue, Semaphore& notification, std::atomic<int32_t>& fullCount,
        ProcessWorkReceiveQueue* batches);

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
    @param [in] index 从1开始的正文下标
    @return true成功，false仍保持未启用
    */
    bool createBlock(int32_t index);

    /** 唯一接收任务进行有界维护。
    @param [in] notification 本地回调通知
    @param [in] fullCount 合并后的满载事件
    */
    void maintain(Semaphore& notification, std::atomic<int32_t>& fullCount);

    /** 唯一接收任务批准新发送者，注册号永不复用。
    */
    void approveSenders();

    /** 分批确认死亡，只释放未提交占位，保留已提交消息。
    */
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

    /** 合并数据通知，不为每个包在内核累积一个空唤醒。
    */
    void signalData();

    /** 只有等待者请求空间时才发布一次空间提示。
    */
    void signalSpace();

    /** 已确认接收进程退出时清理其唯一代次的POSIX对象名称，不碰固定发现入口。
    发送者缓存仍拥有映射引用，已有并发调用不会访问解除映射后的地址。
    */
    void cleanupExitedReceiver();

private:
    // 本地角色、目标PID及进程完整创建标识
    bool m_receiver;
    // stop只撤销ready；直到close完成释放才交出发现入口。
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<bool> m_stopped;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 本通道接收进程PID
    int32_t m_pid;
    // 接收进程完整创建标识
    uint64_t m_identity;
    // 发送进程自己的完整创建标识
    uint64_t m_selfIdentity;
    // 本地接收器代次、发送进程号及请求登记下标
    int64_t m_session;
    // 发送进程自己的PID
    int32_t m_selfPid;
    // 本发送进程的登记下标，未登记为-1
    int32_t m_peerIndex;
    // 登记量超过该提示后，发送小批次主动让出调度机会；不决定容量或成功状态
    uint32_t m_yieldThreshold;
    // 本实例拥有的共享入口与控制区映射，指针仅借用这些映射
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::unique_ptr<SharedMemory> m_directoryMemory;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 独占本地控制区映射对象
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::unique_ptr<SharedMemory> m_controlMemory;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 借用发现入口映射地址
    ProcessWorkSharedDirectory* m_directory;
    // 借用本代控制区映射地址
    ProcessWorkSharedControl* m_control;
    // 元数据页只增加；正文映射可以独立回收，发送者不缓存扩容块。
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::unique_ptr<ProcessWorkStorage> m_storage;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 拥有借用存储和控制区的共享队列算法对象
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::unique_ptr<ProcessWorkSharedQueue> m_sharedQueue;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 接收端扩容保护策略，运行期间只读
    ProcessWorkMemoryConfig m_memoryConfig;
    // 每次建立共享队列算法对象时应用，运行中不修改
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeMapConfig m_producerConfig;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 最早允许重试扩容的单调毫秒
    uint64_t m_nextGrowth;
    // 本轮计划尚未实际创建的正文块数
    int32_t m_growthRemaining;
    // 实际发布增长批次的次数，不含初始缓冲块
    uint64_t m_successfulGrowth;
    // 单一接收线程私有维护状态。
    uint64_t m_nextMaintenance;
    // 下一批死亡确认开始的登记下标
    int32_t m_peerCursor;
    // 唯一读取线程借用的稳定元数据页；close在释放存储前撤销，不能缓存可缩容正文映射
    ProcessWorkPage* m_readPage;
    // 上述页的全局页号，UINT32_MAX表示没有缓存
    uint32_t m_readPageIndex;
    // 数据到达及空间可用通知，没有跨进程互斥锁
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    Semaphore m_dataReady;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 共享空间提示后端，不用于互斥
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    Semaphore m_spaceReady;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 下一次允许OS存活检查的单调毫秒
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<uint64_t> m_nextLivenessCheck;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 映射/分配异常后的重试退让；不改变已提交报文状态。
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<bool> m_retryDelay;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
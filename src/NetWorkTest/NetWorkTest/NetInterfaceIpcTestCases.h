#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include <atomic>

// 真实ProcessWork接入、容量及保序测试集合，逐用例独立建立预期并检查实际结果
class NetInterfaceIpcTestCases
{
public:
    /** 64条满窗口、独立控制额度、单通道过载、就绪轮转和清理
    */
    static void inboxIsolation();

    /** 未获租约不登记回调；外部/内部所有权、64租约及延后释放分别收敛
    */
    static void hubLifecycle();

    /** 新旧信号量入口互通，通知计数满时有明确结果
    */
    static void notifications();

    /** 接收端不消费、环容量满及坏布局时立即拒绝且不占用申请区
    */
    static void stalledReceiver();

    /** 验证v2共享提交、关闭不撤销已提交消息以及满载后恢复
    */
    static void publication();

    /** TLS本地片信封、完整代次/编号、坏长度及固定确认窗口
    */
    static void localPackets();

    /** 队列构造/销毁/非法发布/重建保持可恢复，不使用assert停止测试进程
    */
    static void queueLifecycle();

#ifdef __APPLE__
    /** macOS真实映射所有权、容量、重开、名称拒绝及原子操作
    */
    static void appleResources();
#endif

#ifdef _WIN32
    /** 启动本测试拥有的隐藏子进程，用原ProcessWork接收真实IPC
    */
    static void childTransfer();

    /** 内部子进程入口，只由childTransfer调用
    @param [in] parentPid 创建者PID，必须为正数
    @return 0全部消息通过；其他值表示初始化、数据或超时失败
    */
    static int childMain(int32_t parentPid);

    /** 独占子进程验证回调异常不影响另一回调和后续消息
    */
    static void callbackIsolation();

    /** 独占v2通道验证提交FIFO、活写者保留、退出回收与提交后丢通知的补查
    */
    static void readSlotIsolation();

    /** 按声明容量验证打开和完整正文边界，不使用系统页尾余量
    */
    static void ipcCapacity();

    /** 已有外部接收器被拒绝时，回调、任务和实际收发均保持
    */
    static void receiverOwnership();

    /** 外部接收器未启动或小于16452时拒绝C/C++门面，失败可重试
    */
    static void externalReceiverCapacity();

    /** 原生TLS及透明接入共用最小接收容量，不接受半可用连接
    */
    static void localReceiverCapacity();

    /** Hub接收初始化失败只回滚本次资源，既有Post及回调仍可在重试后完成真实发送
    */
    static void managedReceiverRollback();

    /** 无效参数和重复init必须及时失败，原有效接收器保持可用
    */
    static void initializationBounds();

    /** 隔离子进程核对原始send/post负长度、分配溢出、超容量拒绝及之后真实收发。
    */
    static void invalidSendBounds();

    /** 同名事件使通知创建失败后直接重试，原回调及统一Post发送线程仍能完成真实收发
    */
    static void notificationCreationFailure();

    /** 只运行本用例拥有的故障场景
    @param [in] mode callback/read/init/semaphore
    @return 0通过，否则失败
    */
    static int auditChild(const std::string& mode);
private:
    /** 启动隐藏且有总期限的当前EXE子进程，核对真实退出码
    @param [in] mode callback/read/init/semaphore
    */
    static void runAuditChild(const std::wstring& mode);
public:
#endif
};

// 子进程回调由ProcessWork的CTask执行，无额外自建线程。
class NetInterfaceIpcReceiver : public ProcessReceiveCallback
{
public:
    /** 初始化固定发送方和期望序列
    @param [in] parentPid 此次测试创建者PID
    */
    explicit NetInterfaceIpcReceiver(int32_t parentPid);

    /** 按独立生成的长度、顺序和二进制内容检查实际接收
    @param [in] sendPid 发送进程
    @param [in] buffer 本次借用数据
    @param [in] length 实际长度
    @param [in] type 原IPC消息类型
    */
    virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);

public:
    // 回调完成后通知主线程；只有主线程读完成结果
    Semaphore m_done;
    // 实际ProcessWork回调次数，跨线程通过原子量观察
    std::atomic<int32_t> m_count;
    // 本用例首次失败后保留的错误标志，后续成功不能清除
    std::atomic<bool> m_failed;
    // 本用例父进程PID，只用于测试IPC寻址
    int32_t m_parentPid;
};

// 原始IPC故障观察者，仅由本轮隔离子进程使用
class NetInterfaceIpcAuditReceiver : public ProcessReceiveCallback
{
public:
    /** 初始化原始消息故障模式
    @param [in] fail 是否每次回调均抛受控异常
    */
    explicit NetInterfaceIpcAuditReceiver(bool fail);

    /** 计数并检查固定二进制正文，故障模式用于证明接收任务的异常隔离
    @param [in] sendPid 此隔离进程
    @param [in] buffer 借用到返回
    @param [in] length 必须4
    @param [in] type 必须42
    */
    virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);
    // mode只在启动接收线程前设置
    bool m_throw;
    // 由真实接收执行者写，子进程owner读
    std::atomic<uint32_t> m_count;
    // 输入、顺序、线程或回调次数不符合用例独立预期时置位
    std::atomic<bool> m_bad;
};

#if defined(_WIN32) && defined(PROCESS_WORK_RELIABILITY_UNIT_TEST)
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
/** Independent two-message-per-producer callback checker.
*/
class ReliabilityReceiver : public ProcessReceiveCallback
{
public:
    /** Initialize independent receipt counters.
    */
    ReliabilityReceiver();

    /** Validate borrowed data and per-producer order
    @param [in] pid Sending process
    @param [in] buffer Borrowed four-byte body
    @param [in] length Actual body length
    @param [in] type Expected MESSAGE
    */
    void receive(int32_t pid, const char* buffer, int32_t length, MessageType type);
public:
    // 实际ProcessWork回调次数，跨线程通过原子量观察
    std::atomic<int32_t> m_count;
    // 本用例首次失败后保留的错误标志，后续成功不能清除
    std::atomic<bool> m_failed;
    // 实际收到的进程消息数量，用于对照成功提交总数
    std::atomic<uint32_t> m_seen;
    // 启用每生产者先发先到校验，跨生产者不要求全局先后
    bool m_checkOrder;
    // 四个生产者各自下一条预期序号
    int32_t m_next[4];
};

/** Arms allocation failure only on the execution thread of a real product task.
*/
class ReliabilityArmedTask : public CTask
{
public:
    /** Select the real task and isolated allocation fault
    @param [in] inner Task retained until worker completion
    @param [in] kind 1 sender, 2 copy allocation, 3 queue allocation
    */
    ReliabilityArmedTask(const std::shared_ptr<CTask>& inner, int32_t kind);

    /** Execute finite test work or the retained real task.
    */
    void DoTask();

    /** Forward cooperative stop to the real task.
    */
    void StopTask();
public:
    // 本包装任务拥有的实际CTask，执行与停止委托给它
    std::shared_ptr<CTask> m_inner;
    // 本轮固定发送方式选择，区分受测ProcessWork入口
    int32_t m_kind;
};

/** Exposes only quiescent cache state for leak verification.
*/
class ReliabilityCacheSender : public ProcessWork
{
public:
    /** Inspect caches after all sender tasks have stopped
    @return Every cached mapping is prepared
    */
    bool mappingsReady();
};

/** Two sequential direct sends through one shared sender instance.
*/
class ReliabilityCacheTask : public CTask
{
public:
    /** Configure one producer
    @param [in] sender Borrowed sender retained until completion
    @param [in] pid Destination process
    @param [in] producer Independent sequence index
    */
    ReliabilityCacheTask(ReliabilityCacheSender* sender, int32_t pid, int32_t producer);

    /** Execute finite test work or the retained real task.
    */
    void DoTask();
public:
    // 借用本用例可靠缓存发送器，任务回收前保持有效
    ReliabilityCacheSender* m_sender;
    // 本用例接收进程PID，不查找或修改其他进程
    int32_t m_pid;
    // 当前生产任务编号，用于独立FIFO校验
    int32_t m_producer;
};

/** Replaces only this receiver's owned copy task with an armed real copy task.
*/
class ReliabilityLiveReceiver : public ProcessWork
{
public:
    /** Arm a failure before the real copy task begins receiving
    @param [in] kind 2 body allocation, 3 queue allocation
    */
    void armCopyFailure(int32_t kind);

    /** Inspect live shared blocks while allocation failure remains enabled
    @return Both published messages still own their blocks
    */
    bool sharedBlocksHeld();
};
#endif
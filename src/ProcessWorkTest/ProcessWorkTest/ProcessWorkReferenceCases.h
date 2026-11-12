#pragma once
// 完整保留的参考测试素材，不参与当前用例计数。
// 可通过当前ProcessWork接口运行的独有边界见ProcessWorkCompatibilityTests。
// 其余平台解析、网关及已失效私有布局注入的迁移状态见ProcessWorkTest.h。
// 这里仅保留注释原文，避免清理legacy目录时丢失尚未接入的断言和输入。
// PROCESS_WORK_V2_TESTS_BEGIN
//#include "ProcessReceiveCallback.h"
//#include "ReadTask.h"
//#include "SendTask.h"
//#include <functional>
//#include <chrono>
//#include <stdexcept>
//
///** Actual objectName body compiled with only its Darwin name selection enabled. */
//std::string ProcessWorkTestDarwinObjectName(char kind, uint32_t pid, uint64_t identity,
//    uint32_t session, uint32_t index = 0, uint32_t generation = 0);
//
///** Test-only replacement of the self-identity query before publishing a receiver name.
//@param [in] pid Current process ID
//@param [out] identity Actual identity, or zero for the single injected failure
//@return Whether the query succeeded
//*/
//bool ProcessWorkTestSelfIdentity(uint32_t pid, uint64_t& identity);
//
///** 连接既有NetWorkTest用例的期限检查；所有调用仍由main选择，外部控制器还有总截止。 */
//inline void netTestCheckpoint()
//{
//    static const std::chrono::steady_clock::time_point s_start = std::chrono::steady_clock::now();
//    if (std::chrono::steady_clock::now() - s_start > std::chrono::seconds(60))
//    {
//        throw std::runtime_error("existing IPC case deadline");
//    }
//}
//
///** 仅测试的回调适配，所有检查与负载逻辑位于ProcessWork.cpp的main。 */
//class ProcessWorkTestReceiver : public ProcessWork, public ProcessReceiveCallback
//{
//public:
//    /** 登记本实例回调，尚不启动接收器。 */
//    ProcessWorkTestReceiver() { addProcessReceiveCallback(this); }
//    /** 在测试对象捕获的变量销毁前先加入任务。 */
//    ~ProcessWorkTestReceiver() { uninitReceive(); }
//    /** 转发消息到main内测试逻辑。
//    @param [in] pid 发送进程
//    @param [in] data 借用正文
//    @param [in] length 正文字节
//    @param [in] type 测试序号/类型
//    */
//    void receive(int32_t pid, const char* data, int32_t length, MessageType type)
//    {
//        if (m_message) { m_message(pid, data, length, type); }
//    }
//    /** 转发满载快照。
//    @param [in] count 活动块数
//    */
//    void bufferFull(int32_t count) { if (m_full) { m_full(count); } }
//    /** 返回真实Post队列地址，供测试构建只暂停指定队列。
//    @return 不拥有队列的只读标识
//    */
//    const void* postQueueAddress() const { return &m_postQueue; }
//    /** 受控暂停实际共享读取任务，不关闭共享入口。 */
//    void pauseReader()
//    {
//        CTaskThreadManager::Instance().Uninit(m_readThreadId);
//        m_readThreadId = 0;
//    }
//    /** 使用实际ReadTask恢复读取。
//    @return true已投递任务
//    */
//    bool resumeReader()
//    {
//        std::shared_ptr<ReadTask> task(new ReadTask);
//        task->setParam(m_channel.get(), &m_receiveQueue, &m_receiveSemaphore, &m_fullCount);
//        m_readThreadId = CTaskThreadManager::Instance().Init();
//        std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(m_readThreadId);
//        if (!thread) { return false; }
//        thread->PostTask(task);
//        return true;
//    }
//    /** 创建绑定本接收器的真实读任务，供测试包装执行线程。
//    @return 尚未投递的任务
//    */
//    std::shared_ptr<ReadTask> readerTask()
//    {
//        std::shared_ptr<ReadTask> task(new ReadTask);
//        task->setParam(m_channel.get(), &m_receiveQueue, &m_receiveSemaphore, &m_fullCount);
//        return task;
//    }
//    /** 让真实任务在受控故障包装器内执行。
//    @param [in] task 测试CTask，StopTask必须转发停止
//    @return true已投递
//    */
//    bool resumeReaderWithTask(const std::shared_ptr<CTask>& task)
//    {
//        m_readThreadId = CTaskThreadManager::Instance().Init();
//        std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(m_readThreadId);
//        if (!thread) { return false; }
//        thread->PostTask(task);
//        return true;
//    }
//public:
//    // 初始化前设置，运行中不替换；捕获对象保持到接收器停止
//    std::function<void(int32_t, const char*, int32_t, MessageType)> m_message;
//    std::function<void(int32_t)> m_full;
//};
//
///** 只负责承载main内负载的CTask，不创建std::thread。 */
//class ProcessWorkTestTask : public CTask
//{
//public:
//    /** 初始化协作停止及结果标记。 */
//    ProcessWorkTestTask(): m_stop(false), m_done(false), m_failed(false) {}
//    /** 运行main设置的动作；异常记录失败，不主动终止进程。 */
//    void DoTask()
//    {
//        const std::atomic<bool>* old = SendTask::setCurrentStopFlag(&m_stop);
//        try { if (m_action) { m_action(); } }
//        catch (...) { m_failed = true; }
//        SendTask::setCurrentStopFlag(old);
//        m_done = true;
//    }
//    /** 使阻塞send和测试循环能够协作退出。 */
//    void StopTask() { m_stop = true; if (m_stopAction) { m_stopAction(); } }
//public:
//    // 只在投递前设置的测试动作
//    std::function<void()> m_action;
//    // 可选地把停止转发到main包装的真实任务，不执行等待
//    std::function<void()> m_stopAction;
//    // 外部观察的任务状态
//    std::atomic<bool> m_stop;
//    std::atomic<bool> m_done;
//    std::atomic<bool> m_failed;
//};
//
//#ifdef PROCESS_WORK_TEST_ALLOCATION
///** 仅供临时测试构建暂停已核对的名称查找，不修改共享状态。 */
//void ProcessWorkTestNameBoundary();
//#endif
//
///** 设置Apple退出判断的系统响应替身；仅测试构建使用。
//@param [in] queryReturn sysctl返回值
//@param [in] lengthMode -1完整记录，其他值为返回字节数
//@param [in] state 测试进程状态
//@param [in] killReturn kill返回值
//@param [in] error 系统错误码
//*/
//void ProcessWorkTestDarwinResult(int32_t queryReturn, int32_t lengthMode, int32_t state, int32_t killReturn, int32_t error);
//
///** 执行从当前CSystem源码提取的Apple退出判断，仅替代系统响应。
//@param [in] processId 被查询PID
//@return 是否有明确退出证据
//*/
//bool ProcessWorkTestDarwinExited(uint32_t processId);
//
//#ifdef PROCESS_WORK_TEST_ALLOCATION
///** 测试构建临时授权访问计数边界；不进入生产Semaphore声明。 */
//class ProcessWorkTestSemaphoreAccess
//{
//public:
//    /** 在没有并发操作时设置计数，以覆盖实际运行难以积累到的整数边界。
//    @param [in,out] semaphore 被测真实本地通知对象
//    @param [in] count 待测试的已有通知数量
//    */
//    static void set(Semaphore& semaphore, uint64_t count);
//    /** 获取现有锁保护的计数快照。
//    @param [in,out] semaphore 被测通知对象
//    @return 64位计数快照
//    */
//    static uint64_t count(Semaphore& semaphore);
//};
//#endif
//
//#ifdef PROCESS_WORK_TEST_ALLOCATION
///** 只在测试构建中授权访问元素统计边界，不改变真实链表。 */
//class ProcessWorkTestQueueCountAccess
//{
//public:
//    /** 静止状态注入统计快照，消费本轮真实节点后必须恢复0。
//    @param [in,out] queue 被测真实队列
//    @param [in] count 边界计数
//    */
//    static void set(LockFreeQueue<int32_t>& queue, uint64_t count);
//    /** 读取被测队列原子统计。
//    @param [in] queue 被测真实队列
//    @return 64位计数快照
//    */
//    static uint64_t count(const LockFreeQueue<int32_t>& queue);
//};
//#endif
//
//#ifdef PROCESS_WORK_TEST_ALLOCATION
///** 测试构建临时访问线程编号分配器；不进入生产头文件。 */
//class ProcessWorkTestTaskIdAccess
//{
//public:
//    /** 没有并发Init时注入分配边界；恢复前须清理本轮线程。
//    @param [in,out] manager 被测管理器
//    @param [in] count 已分配到的编号
//    */
//    static void set(CTaskThreadManager& manager, uint32_t count);
//    /** 读取实际原子编号。
//    @param [in] manager 被测管理器
//    @return 当前已分配编号
//    */
//    static uint32_t count(const CTaskThreadManager& manager);
//};
//#endif
//
//#include "ProcessWork/ProcessWorkAPI.h"
//#include "Semaphore/SemaphoreAPI.h"
//#include <atomic>
//
//class NetInterfaceIpcTestCases
//{
//public:
//    /** 新旧信号量入口互通，通知计数满时有明确结果
//    */
//    static void notifications();
//
//    /** 接收端不消费、环容量满及坏布局时立即拒绝且不占用申请区
//    */
//    static void stalledReceiver();
//
//    /** 验证v2共享提交、关闭不撤销已提交消息以及满载后恢复
//    */
//    static void publication();
//
//    /** TLS本地片信封、完整代次/编号、坏长度及固定确认窗口
//    */
//    static void localPackets();
//
//    /** 队列构造/销毁/非法发布/重建保持可恢复，不使用assert停止测试进程 */
//    static void queueLifecycle();
//
//#ifdef __APPLE__
//    /** macOS真实映射所有权、容量、重开、名称拒绝及原子操作
//    */
//    static void appleResources();
//#endif
//
//#ifdef _WIN32
//    /** 启动本测试拥有的隐藏子进程，用原ProcessWork接收真实IPC
//    */
//    static void childTransfer();
//
//    /** 内部子进程入口，只由childTransfer调用
//    @param [in] parentPid 创建者PID，必须为正数
//    @return 0全部消息通过；其他值表示初始化、数据或超时失败
//    */
//    static int childMain(int32_t parentPid);
//    /** 独占子进程验证回调异常不影响另一回调和后续消息 */
//    static void callbackIsolation();
//    /** 独占v2通道验证提交FIFO、活写者保留、退出回收与提交后丢通知的补查
//    */
//    static void readSlotIsolation();
//    /** 按声明容量验证打开和完整正文边界，不使用系统页尾余量 */
//    static void ipcCapacity();
//    /** 已有外部接收器被拒绝时，回调、任务和实际收发均保持 */
//    static void receiverOwnership();
//    /** 外部接收器未启动或小于16452时拒绝C/C++门面，失败可重试 */
//    static void externalReceiverCapacity();
//    /** 原生TLS及透明接入共用最小接收容量，不接受半可用连接 */
//    static void localReceiverCapacity();
//    /** Hub接收初始化失败只回滚本次资源，既有Post及回调仍可在重试后完成真实发送 */
//    static void managedReceiverRollback();
//    /** 无效参数和重复init必须及时失败，原有效接收器保持可用 */
//    static void initializationBounds();
//    /** 隔离子进程核对原始send/post负长度、分配溢出、超容量拒绝及之后真实收发。 */
//    static void invalidSendBounds();
//    /** 同名事件使通知创建失败后直接重试，原回调及统一Post发送线程仍能完成真实收发
//    */
//    static void notificationCreationFailure();
//    /** 只运行本用例拥有的故障场景
//    @param [in] mode callback/read/init/semaphore
//    @return 0通过，否则失败
//    */
//    static int auditChild(const std::string& mode);
//private:
//    /** 启动隐藏且有总期限的当前EXE子进程，核对真实退出码
//    @param [in] mode callback/read/init/semaphore
//    */
//    static void runAuditChild(const std::wstring& mode);
//public:
//#endif
//};
//
//// 子进程回调由ProcessWork的CTask执行，无额外自建线程。
//class NetInterfaceIpcReceiver : public ProcessReceiveCallback
//{
//public:
//    /** 初始化固定发送方和期望序列
//    @param [in] parentPid 此次测试创建者PID
//    */
//    explicit NetInterfaceIpcReceiver(int32_t parentPid);
//
//    /** 按独立生成的长度、顺序和二进制内容检查实际接收
//    @param [in] sendPid 发送进程
//    @param [in] buffer 本次借用数据
//    @param [in] length 实际长度
//    @param [in] type 原IPC消息类型
//    */
//    virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);
//
//public:
//    // 回调完成后通知主线程；只有主线程读完成结果
//    Semaphore m_done;
//    std::atomic<int32_t> m_count;
//    std::atomic<bool> m_failed;
//    int32_t m_parentPid;
//};
//
//// 原始IPC故障观察者，仅由本轮隔离子进程使用
//class NetInterfaceIpcAuditReceiver : public ProcessReceiveCallback
//{
//public:
//    /** 初始化原始消息故障模式
//    @param [in] fail 是否每次回调均抛受控异常
//    */
//    explicit NetInterfaceIpcAuditReceiver(bool fail);
//    /** 计数并检查固定二进制正文，故障模式用于证明接收任务的异常隔离
//    @param [in] sendPid 此隔离进程
//    @param [in] buffer 借用到返回
//    @param [in] length 必须4
//    @param [in] type 必须42
//    */
//    virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);
//    // mode只在启动接收线程前设置
//    bool m_throw;
//    // 由真实接收执行者写，子进程owner读
//    std::atomic<uint32_t> m_count;
//    std::atomic<bool> m_bad;
//};
//
//#if defined(_WIN32) && defined(PROCESS_WORK_RELIABILITY_UNIT_TEST)
//#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
///** Independent two-message-per-producer callback checker. */
//class ReliabilityReceiver : public ProcessReceiveCallback
//{
//public:
//    /** Initialize independent receipt counters. */
//    ReliabilityReceiver();
//    /** Validate borrowed data and per-producer order
//    @param [in] pid Sending process
//    @param [in] buffer Borrowed four-byte body
//    @param [in] length Actual body length
//    @param [in] type Expected MESSAGE
//    */
//    void receive(int32_t pid, const char* buffer, int32_t length, MessageType type);
//public:
//    std::atomic<int32_t> m_count;
//    std::atomic<bool> m_failed;
//    std::atomic<uint32_t> m_seen;
//    bool m_checkOrder;
//    int32_t m_next[4];
//};
//
///** Arms allocation failure only on the execution thread of a real product task. */
//class ReliabilityArmedTask : public CTask
//{
//public:
//    /** Select the real task and isolated allocation fault
//    @param [in] inner Task retained until worker completion
//    @param [in] kind 1 sender, 2 copy allocation, 3 queue allocation
//    */
//    ReliabilityArmedTask(const std::shared_ptr<CTask>& inner, int32_t kind);
//    /** Execute finite test work or the retained real task. */
//    void DoTask();
//    /** Forward cooperative stop to the real task. */
//    void StopTask();
//public:
//    std::shared_ptr<CTask> m_inner;
//    int32_t m_kind;
//};
//
///** Exposes only quiescent cache state for leak verification. */
//class ReliabilityCacheSender : public ProcessWork
//{
//public:
//    /** Inspect caches after all sender tasks have stopped
//    @return Every cached mapping is prepared
//    */
//    bool mappingsReady();
//};
//
///** Two sequential direct sends through one shared sender instance. */
//class ReliabilityCacheTask : public CTask
//{
//public:
//    /** Configure one producer
//    @param [in] sender Borrowed sender retained until completion
//    @param [in] pid Destination process
//    @param [in] producer Independent sequence index
//    */
//    ReliabilityCacheTask(ReliabilityCacheSender* sender, int32_t pid, int32_t producer);
//    /** Execute finite test work or the retained real task. */
//    void DoTask();
//public:
//    ReliabilityCacheSender* m_sender;
//    int32_t m_pid;
//    int32_t m_producer;
//};
//
///** Replaces only this receiver's owned copy task with an armed real copy task. */
//class ReliabilityLiveReceiver : public ProcessWork
//{
//public:
//    /** Arm a failure before the real copy task begins receiving
//    @param [in] kind 2 body allocation, 3 queue allocation
//    */
//    void armCopyFailure(int32_t kind);
//    /** Inspect live shared blocks while allocation failure remains enabled
//    @return Both published messages still own their blocks
//    */
//    bool sharedBlocksHeld();
//};
//#endif
// PROCESS_WORK_V2_TESTS_END
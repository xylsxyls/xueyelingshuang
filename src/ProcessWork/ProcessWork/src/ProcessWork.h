#pragma once
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include "ProcessWorkChannel.h"
#include <memory>
#include <map>
#include <vector>
#include <atomic>
#include "ProcessWorkMacro.h"

class ProcessReceiveCallback;

// 名称发现记录，完整名称验证防止编码摘要碰撞误寻址。
struct ProcessWorkNameDirectory
{
    // 0未就绪，负数初始化中，正数真实PID；原子发布
    int32_t m_pid;
    // 逻辑名字节数及完整UTF8名称，最多1023字节
    int32_t m_length;
    // 目标进程完整创建标识，防止旧PID复用使名称永久占用
    uint64_t m_identity;
    char m_name[1024];
};

/** 进程通信类
*/
class ProcessWorkAPI ProcessWork
{
protected:
    /** 构造函数
    */
    ProcessWork();

public:
    /** 单一实例
    @return 返回单一实例
    */
    static ProcessWork& instance();

public:
    /** 增加接收回调类，按先入先执行回调顺序；须在接收器启动前登记，回调借用至uninitReceive完成
    @param [in] callback 接收回调类；空指针在分派时忽略
    */
    void addProcessReceiveCallback(ProcessReceiveCallback* callback);

    /** 设置本接收器发布的逻辑名称，适合Android同一宿主可执行文件中的多个服务进程。
    @param [in] name 1至1023字节且不含NUL；空值恢复默认可执行文件名
    @return 接收器尚未初始化且名称合法为true；运行中返回false
    */
    bool setReceiveName(const std::string& name);

    /** 初始化接收模块
    @param [in] receiveSize 申请的单块字节，至少12且小于INT32_MAX；默认500KiB，最大正文严格为receiveSize减12，不依赖系统页对齐余量
    @param [in] areaCount 固定初始块数1至1024
    @param [in] expandThreshold 剩余空闲块不高于此值时请求扩容，非负
    @param [in] expandCount 每次新增块数，0关闭扩容
    @param [in] idleSeconds 扩容块连续空闲多少秒后释放，0不自动释放
    @param [in] maxExpandTimes 最大同时保留扩容份数，0关闭扩容；初始数加扩容总数不得超过1024
    @return 成功true；参数无效、已有接收器、通知不可用或有界资源准备失败为false，重复调用不破坏原接收器
    失败保留已登记回调及既有发送模块，可直接重试
    */
    bool initReceive(int32_t receiveSize = 500 * 1024, int32_t areaCount = 100,
        int32_t expandThreshold = 0, int32_t expandCount = 0, int32_t idleSeconds = 600, int32_t maxExpandTimes = 0);

    /** 销毁接收模块资源；生命周期操作须串行，不能从接收回调中等待销毁本回调线程
    */
    void uninitReceive();

    /** 清空发送模块指定pid的资源，不涉及存储pid的缓冲区map
    @param [in] destPid 指定pid
    */
    void clearDestPid(int32_t destPid);

    /** 清空发送模块不用的缓存资源
    */
    void clearUseless();

    /** 清空发送模块的所有缓存资源
    */
    void clear();

    /** 初始化发送线程，不可并行
    */
    void initPostThread();

    /** 销毁发送线程并取消尚未提交的消息；须先停止新的post调用，需要排空时先调用waitForPostEnd
    */
    void uninitPostThread();

    /** 向服务端发送字符串；同一线程向同一目标连续send保持提交及接收顺序，跨线程不保证调用开始顺序
    有效消息完成共享内存提交后返回，不等待业务回调；无效参数、底层失败或目标关闭也会提前返回，void接口没有成功状态码
    @param [in] destPid 目标进程pid
    @param [in] buffer 字符串地址，空包填nullptr
    @param [in] length 长度
    @param [in] type 发送数据类型
    */
    void send(int32_t destPid, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

    /** 向服务端发送数据
    @param [in] destPid 目标进程pid
    @param [in] message 发送内容，可以为空字符串，等同于nullptr
    @param [in] type 发送数据类型
    */
    void send(int32_t destPid, const std::string& message, MessageType type = MessageType::MESSAGE);

    /** 按名称取得核验过身份的通道并发送；目标稳定时，同一线程连续send有序，跨线程不保证调用开始顺序
    返回及失败语义与PID入口相同
    @param [in] processName 目标进程名，windows下不带后缀名，linux下全名
    @param [in] buffer 字符串地址
    @param [in] length 长度
    @param [in] type 发送数据类型
    */
    void send(const std::string& processName, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

    /** 向服务端发送数据
    @param [in] processName 目标进程名，windows下不带后缀名，linux下全名
    @param [in] message 发送内容
    @param [in] type 发送数据类型
    */
    void send(const std::string& processName, const std::string& message, MessageType type = MessageType::MESSAGE);

    /** 拷贝到统一后台队列；同一发送线程连续post至同一目标有序，跨线程按实际入队顺序
    @param [in] destPid 目标进程pid
    @param [in] buffer 字符串地址
    @param [in] length 长度
    @param [in] type 发送数据类型
    */
    void post(int32_t destPid, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

    /** 拷贝到统一后台队列，顺序语义与PID缓冲区入口相同
    @param [in] destPid 目标进程pid
    @param [in] message 发送内容
    @param [in] type 发送数据类型
    */
    void post(int32_t destPid, const std::string& message, MessageType type = MessageType::MESSAGE);

    /** 拷贝到统一后台队列；同一线程混合post(name)与post(pid)至同一稳定目标有序
    @param [in] processName 目标进程名，Windows不带后缀，POSIX为完整文件名
    @param [in] buffer 字符串地址
    @param [in] length 长度
    @param [in] type 发送数据类型
    */
    void post(const std::string& processName, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

    /** 拷贝到统一后台队列，名称在后台发送时解析为当前PID
    @param [in] processName 目标进程名，Windows不带后缀，POSIX为完整文件名
    @param [in] message 发送内容
    @param [in] type 发送数据类型
    */
    void post(const std::string& processName, const std::string& message, MessageType type = MessageType::MESSAGE);

    /** 停止新的post调用后排空后台任务，结束后更换线程号；不等待接收回调，底层失败语义与send相同
    */
    void waitForPostEnd();


    /** 非阻塞PID发送；BUSY表示尚未提交，调用者可保持顺序重试。
    @param [in] destPid 目标进程号
    @param [in] buffer 借用正文，length为0时可空
    @param [in] length 正文字节数
    @param [in] type 非负消息类型
    @return ProcessWorkSendResult；SENT表示已提交，不表示业务已处理
    */
    int32_t trySend(int32_t destPid, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

    /** 当前接收器活动块数快照；生命周期owner不与init/uninit并发。
    @return 未初始化时0
    */
    int32_t receiveBufferCount() const;

    /** 查询本进程已就绪接收器的声明最大正文容量，不与init/uninit并发
    查询不改变接收资源、回调或Post任务，不代表空闲额度或目标进程容量
    @return 声明单块容量减12；未就绪或仅允许空包时为0
    */
    int32_t receivePayloadCapacity() const;

private:
    /** 释放本次接收资源。
    @param [in] clearProcessState true额外关闭Post并清理发送缓存及回调；false只回滚初始化
    */
    void releaseReceive(bool clearProcessState);
    /** 准备名称发现记录；发布PID须等待接收任务全部就绪。
    @return true名称记录可用，false由初始化作用域统一回滚
    */
    bool claimReceiveName();
    /** 取得既有锁保护的发送缓存；构造阶段不等待对方注册批准。
    @param [in] pid 目标PID
    @return 可用v2通道或空指针
    */
    std::shared_ptr<ProcessWorkChannel> sender(int32_t pid);
    /** 使用名称映射缓存取得绑定身份和接收代次的通道；已撤销映射允许重新打开。
    @param [in] name 平台约定的进程名
    @return 已核验通道，未就绪返回空指针
    */
    std::shared_ptr<ProcessWorkChannel> resolveName(const std::string& name);
    /** 在已选定通道上等待提交，不在等待期间改选同PID的新接收器。
    @param [in] channel 本次调用持有的通道，可空
    @param [in] buffer 已校验的借用正文，空正文可空
    @param [in] length 非负正文字节数
    @param [in] type 非负消息类型
    */
    void sendChannel(const std::shared_ptr<ProcessWorkChannel>& channel, const char* buffer, int32_t length, MessageType type);
    /** 生成名称发现键，记录中仍核对完整名称，不靠摘要认定身份。
    @param [in] name 完整进程名
    @return 最多30字节的原生共享对象名
    */
    static std::string nameKey(const std::string& name);
    /** 将PID或名称消息复制入唯一队列。
    @param [in] pid PID模式时的目标；名称模式为0
    @param [in] name 名称模式的目标；PID模式为空
    @param [in] buffer 借用正文
    @param [in] length 正文字节数
    @param [in] type 消息类型
    */
    void enqueue(int32_t pid, const std::string& name, const char* buffer, int32_t length, MessageType type);

protected:
    // 接收器资源由生命周期owner创建，读任务借用至加入完成
    std::shared_ptr<ProcessWorkChannel> m_channel;
    std::unique_ptr<SharedMemory> m_nameMemory;
    // 初始化阶段设置，接收运行期间不变。
    std::string m_receiveName;
    bool m_nameOwned;
    // 回调对象由调用方拥有，运行中不增删
    std::vector<ProcessReceiveCallback*> m_callback;
    int32_t m_thisProcessPid;
    // 两个接收任务分别负责共享数据与业务回调，一个Post任务统一发送
    uint32_t m_readThreadId;
    uint32_t m_receiveThreadId;
    uint32_t m_postThreadId;
    // 保留原有缓存读写锁，不增加覆盖消息发送或扩缩容的互斥锁
    ReadWriteMutex m_sendProcessAssignMapMutex;
    std::map<int32_t, std::shared_ptr<ProcessWorkChannel> > m_sendChannels;
    ReadWriteMutex m_sendNameMemoryMapMutex;
    std::map<std::string, std::shared_ptr<SharedMemory> > m_sendNameMemoryMap;
    // 本地收件队列持有独立正文副本，业务回调完成后释放
    LockFreeQueue<char*> m_receiveQueue;
    Semaphore m_receiveSemaphore;
    std::atomic<int32_t> m_fullCount;
    // 本地唯一Post队列；生命周期操作前调用方先停止外部生产者
    std::atomic<bool> m_waitEndPost;
    LockFreeQueue<char*> m_postQueue;
    Semaphore m_postSemaphore;
};

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
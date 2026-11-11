#pragma once
#include "CorrespondParam/CorrespondParamAPI.h"
#include "LockFreeMap/LockFreeMapAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include "ProcessWorkChannel.h"
#include "ProcessWorkLocalQueue.h"
#include "ProcessWorkReceiveQueue.h"
#include "ProcessWorkSendCache.h"
#include "ProcessWorkConfig.h"
#include <memory>
#include <vector>
#include <atomic>
#include "ProcessWorkMacro.h"

class ProcessReceiveCallback;

/** Post只接纳本地副本，不表示共享提交或远端业务完成；失败均未接纳。
*/
enum ProcessWorkPostResult
{
    // 本地副本已接纳，不代表远端收到
    PROCESS_WORK_POSTED = 0,
    // 仅tryPost的CAS竞争失败；未接纳，不表示容量不足
    PROCESS_WORK_POST_BUSY = 1,
    // 参数错误，未创建排队副本
    PROCESS_WORK_POST_INVALID = -1,
    // 本次内存申请失败，未接纳
    PROCESS_WORK_POST_NO_MEMORY = -2,
    // 本地队列不可用，未接纳
    PROCESS_WORK_POST_UNAVAILABLE = -3,
    // 本地队列字节预算或节点容量上限已到，未接纳
    PROCESS_WORK_POST_CAPACITY = -4,
    // 系统余量保护或尚不能安全复用的存储阻止接纳
    PROCESS_WORK_POST_RESOURCE = -5
};

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

    /** 静止期一次配置内部组件，必须先收敛所有调用、清空发送缓存并停止收发线程
    @param [in] config 各组件独立池参数，配置复制且运行期间冻结
    @return true配置完成；false无效、资源不足或尚未静止，失败后可修正配置重试初始化
    */
    bool configure(const ProcessWorkConfig& config);

public:
    /** 增加接收回调类，按先入先执行回调顺序；须在接收器启动前登记，回调借用至uninitReceive完成
    @param [in] callback 接收回调类；空指针在分派时忽略
    @return true已登记；false接收已启动或资源不足，原回调列表保持不变
    */
    bool addProcessReceiveCallback(ProcessReceiveCallback* callback);

    /** 设置本接收器发布的逻辑名称，适合Android同一宿主可执行文件中的多个服务进程。
    @param [in] name 1至1023字节且不含NUL；空值恢复默认可执行文件名
    @return 接收器尚未初始化且名称合法为true；运行中、参数无效或分配失败返回false，保留原名称
    */
    bool setReceiveName(const std::string& name);

    /** 初始化接收模块
    @param [in] receiveSize 申请的单块字节，至少12且小于INT32_MAX；默认500KiB，最大正文严格为receiveSize减12，不依赖系统页对齐余量
    @param [in] areaCount 初始保留块数，正数；实际申请受地址空间及系统内存保护限制
    @param [in] expandThreshold 剩余空闲块不高于此值时请求扩容，非负
    @param [in] expandCount 每次新增块数，0关闭扩容
    @param [in] idleSeconds 扩容块连续空闲多少秒后释放，0不自动释放
    @param [in] maxExpandTimes 最大同时保留扩容份数；0关闭，-1不设配置上限且expandCount必须大于0
    @return 成功true；参数无效、已有接收器、通知不可用或有界资源准备失败为false，重复调用不破坏原接收器
    失败保留已登记回调及既有发送模块，可直接重试
    */
    bool initReceive(int32_t receiveSize = 500 * 1024, int32_t areaCount = 100,
        int32_t expandThreshold = 0, int32_t expandCount = 64, int32_t idleSeconds = 600, int32_t maxExpandTimes = -1);

    /** 使用显式内存保护策略初始化，其他参数与六参数入口相同。
    @param [in] receiveSize 单块字节数，含12字节头
    @param [in] areaCount 初始保留块数
    @param [in] expandThreshold 剩余块数阈值，0只在新申请遇满后扩容
    @param [in] expandCount 每次计划新增块数，按memory预算分批执行
    @param [in] idleSeconds 扩容块闲置回收秒数，0不缩容
    @param [in] maxExpandTimes -1无配置上限；0关闭；正数限定最大保留扩容份数
    @param [in] memory 系统内存余量、每轮申请预算和失败退避策略
    @return true就绪，false未启动接收且保留既有回调登记
    */
    bool initReceive(int32_t receiveSize, int32_t areaCount, int32_t expandThreshold,
        int32_t expandCount, int32_t idleSeconds, int32_t maxExpandTimes, const ProcessWorkMemoryConfig& memory);

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
    @return true已启动或原线程已存在；false资源准备失败，不抛出异常
    */
    bool initPostThread();

    /** Post线程未启动且没有在途正文时配置本地积压预算。
    @param [in] memory 系统余量及本地正文计费预算
    @return true配置完成；false无效或尚未静止
    */
    bool configurePostMemory(const ProcessWorkMemoryConfig& memory);

    /** 按开关尝试接纳本地PID发送副本，默认不等待后台线程释放容量
    @param [in] destPid 目标PID
    @param [in] buffer 借用正文，空正文可为空
    @param [in] length 正文字节数
    @param [in] type 非负消息类型
    @param [in] retryContention true持续重试CAS竞争，false允许返回未提交竞争状态
    @param [in] retryCapacity true等待可复用容量；前4096次快速重试，之后每次等待1毫秒
    @return ProcessWorkPostResult；POSTED后不能重复提交，其余状态均未提交
    */
    int32_t tryPost(int32_t destPid, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE,
        bool retryContention = false, bool retryCapacity = false);

    /** 按开关尝试接纳名称寻址副本，后台发送时才解析名称
    @param [in] processName 不超过1023字节且无NUL的目标名称
    @param [in] buffer 借用正文
    @param [in] length 正文字节数
    @param [in] type 非负消息类型
    @param [in] retryContention true持续重试CAS竞争，false允许返回未提交竞争状态
    @param [in] retryCapacity true等待可复用容量；前4096次快速重试，之后每次等待1毫秒
    @return ProcessWorkPostResult，不代表远端接收
    */
    int32_t tryPost(const std::string& processName, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE,
        bool retryContention = false, bool retryCapacity = false);

    /** 普通PID投递的结果接口；竞争与容量不足内部重试，实际分配等错误直接返回
    @param [in] destPid 目标PID
    @param [in] buffer 调用期间借用的正文；成功后已复制
    @param [in] length 正文字节数，0允许buffer为空
    @param [in] type 非负消息类型
    @return ProcessWorkPostResult；不返回POST_BUSY，只有POSTED表示已接纳
    */
    int32_t postResult(int32_t destPid, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

    /** 普通名称投递的结果接口；目标名称在后台发送时解析
    @param [in] processName 非空且不超过1023字节、不含NUL的目标名称
    @param [in] buffer 调用期间借用的正文；成功后已复制
    @param [in] length 正文字节数
    @param [in] type 非负消息类型
    @return ProcessWorkPostResult；不返回POST_BUSY，不通过异常报告资源失败
    */
    int32_t postResult(const std::string& processName, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

    /** 查询本地Post在途计费，包含预留、排队以及正在发送的正文。
    @return 有限额度的弱一致计费快照；-1模式返回0表示未启用计费，不表示队列为空
    */
    uint64_t postPendingBytes() const;

    /** 查询本地接收在途计费，包含正在执行回调的正文。
    @return 有限额度的弱一致计费快照；-1模式返回0表示未启用计费，不包含共享映射
    */
    uint64_t receivePendingBytes() const;

    /** 销毁发送线程并取消尚未提交的消息；须先停止新的post调用，需要排空时先调用waitForPostEnd
    */
    void uninitPostThread();

    /** 向服务端发送字符串；同一线程向同一目标连续send保持提交及接收顺序，跨线程不保证全局提交或接收顺序
    返回SENT才表示完整共享提交；不等待业务回调，满载和CAS竞争内部不限次数重试
    @param [in] destPid 目标进程pid
    @param [in] buffer 字符串地址，空包填nullptr
    @param [in] length 长度
    @param [in] type 发送数据类型
    @return true已提交；false为参数、系统或生命周期错误，不抛异常
    */
    bool send(int32_t destPid, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

    /** 向服务端发送数据
    @param [in] destPid 目标进程pid
    @param [in] message 发送内容，可以为空字符串，等同于nullptr
    @param [in] type 发送数据类型
    @return true已提交；false为参数、系统或生命周期错误，不抛异常
    */
    bool send(int32_t destPid, const std::string& message, MessageType type = MessageType::MESSAGE);

    /** 按名称取得核验过身份的通道并发送；目标稳定时，同一线程连续send有序，跨线程不保证全局提交或接收顺序
    返回及失败语义与PID入口相同
    @param [in] processName 目标进程名，windows下不带后缀名，linux下全名
    @param [in] buffer 字符串地址
    @param [in] length 长度
    @param [in] type 发送数据类型
    @return true已提交；false为参数、系统或生命周期错误，不抛异常
    */
    bool send(const std::string& processName, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

    /** 向服务端发送数据
    @param [in] processName 目标进程名，windows下不带后缀名，linux下全名
    @param [in] message 发送内容
    @param [in] type 发送数据类型
    @return true已提交；false为参数、系统或生命周期错误，不抛异常
    */
    bool send(const std::string& processName, const std::string& message, MessageType type = MessageType::MESSAGE);

    /** 拷贝到统一后台队列；同一发送线程连续post至同一目标有序，跨线程按实际入队顺序
    未提交的CAS竞争及队列瞬时竞争持续内部重试，没有次数上限
    资源失败不抛异常；返回true才表示已接纳，详细错误码使用postResult查询
    @param [in] destPid 目标进程pid
    @param [in] buffer 字符串地址
    @param [in] length 长度
    @param [in] type 发送数据类型
    @return true已接纳；false为参数、系统或生命周期错误，不抛异常
    */
    bool post(int32_t destPid, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

    /** 拷贝到统一后台队列，顺序语义与PID缓冲区入口相同
    @param [in] destPid 目标进程pid
    @param [in] message 发送内容
    @param [in] type 发送数据类型
    @return true已接纳；false为参数、系统或生命周期错误，不抛异常
    */
    bool post(int32_t destPid, const std::string& message, MessageType type = MessageType::MESSAGE);

    /** 拷贝到统一后台队列；同一线程混合post(name)与post(pid)至同一稳定目标有序
    @param [in] processName 目标进程名，Windows不带后缀，POSIX为完整文件名
    @param [in] buffer 字符串地址
    @param [in] length 长度
    @param [in] type 发送数据类型
    @return true已接纳；false为参数、系统或生命周期错误，不抛异常
    */
    bool post(const std::string& processName, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

    /** 拷贝到统一后台队列，名称在后台发送时解析为当前PID
    @param [in] processName 目标进程名，Windows不带后缀，POSIX为完整文件名
    @param [in] message 发送内容
    @param [in] type 发送数据类型
    @return true已接纳；false为参数、系统或生命周期错误，不抛异常
    */
    bool post(const std::string& processName, const std::string& message, MessageType type = MessageType::MESSAGE);

    /** 停止新的post调用后排空后台任务，结束后更换线程号；不等待接收回调，底层失败语义与send相同
    */
    void waitForPostEnd();

    /** 按开关尝试PID发送；默认不持续重试，BUSY表示尚未提交
    @param [in] destPid 目标进程号
    @param [in] buffer 借用正文，length为0时可空
    @param [in] length 正文字节数
    @param [in] type 非负消息类型
    @param [in] retryContention true持续重试CAS竞争，false允许返回未提交竞争状态
    @param [in] retryCapacity true等待可复用容量；前4096次快速重试，之后每次等待1毫秒
    @return ProcessWorkSendResult；SENT表示已提交，不表示业务已处理
    */
    int32_t trySend(int32_t destPid, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE,
        bool retryContention = false, bool retryCapacity = false);

    /** 按名称发送并独立选择竞争、容量重试；一次调用绑定同一接收代次
    @param [in] processName 目标进程名称
    @param [in] buffer 调用期间借用的正文
    @param [in] length 非负字节数；0允许buffer为空
    @param [in] type 非负消息类型
    @param [in] retryContention CAS失败是否无限重试
    @param [in] retryCapacity 容量不足是否无限重试；4096次后每次等待1毫秒
    @return SENT仅表示已提交，其他结果均未提交
    */
    int32_t trySend(const std::string& processName, const char* buffer, int32_t length,
        MessageType type = MessageType::MESSAGE, bool retryContention = false, bool retryCapacity = false);

    /** 同步PID发送并报告共享提交结果；等待空间期间持续检查对端退出
    @param [in] destPid 目标PID
    @param [in] buffer 调用期间借用的正文
    @param [in] length 正文字节数，0允许buffer为空
    @param [in] type 非负消息类型
    @return ProcessWorkSendResult；不返回BUSY，SENT只表示完整共享提交
    */
    int32_t sendResult(int32_t destPid, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

    /** 同步名称发送并报告结果；一次调用绑定核验过身份的同一接收器代次
    @param [in] processName 非空且不超过1023字节、不含NUL的目标名称
    @param [in] buffer 调用期间借用的正文
    @param [in] length 正文字节数
    @param [in] type 非负消息类型
    @return ProcessWorkSendResult；资源异常转换为错误码，不自动重放已提交消息
    */
    int32_t sendResult(const std::string& processName, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

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

    /** 查询不可变通道缓存；缺失时独立准备候选，竞争失败不影响本次发送。
    @param [in] pid 目标PID
    @return 可用v6通道或空指针
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
    @param [in] retryContention true持续重试CAS竞争，false允许返回未提交竞争状态
    @param [in] retryCapacity true等待可复用容量；前4096次快速重试，之后每次等待1毫秒
    @return 提交结果；按两种开关重试；成功只表示共享提交，不等待远端回调
    */
    int32_t sendChannel(const std::shared_ptr<ProcessWorkChannel>& channel, const char* buffer, int32_t length,
        MessageType type, bool retryContention, bool retryCapacity);

    /** 生成名称发现键，记录中仍核对完整名称，不靠摘要认定身份。
    @param [in] name 完整进程名
    @return 最多30字节的原生共享对象名
    */
    static std::string nameKey(const std::string& name);

    /** 快缓存未命中时经权威Map查找并发送；单独保存慢路径的构造及回滚状态
    @param [in] destPid 已检查为正的目标PID
    @param [in] buffer 借用到返回的正文，空正文允许nullptr
    @param [in] length 正文长度
    @param [in] type 消息类型
    @param [in] retryContention 是否重试竞争
    @param [in] retryCapacity 是否等待容量
    @return 共享提交结果，成功不表示业务已经处理
    */
    int32_t sendUncached(int32_t destPid, const char* buffer, int32_t length, MessageType type,
        bool retryContention, bool retryCapacity);

    /** 将PID或名称消息复制入唯一队列。
    @param [in] pid PID模式时的目标；名称模式为0
    @param [in] name 名称模式的目标；PID模式为空
    @param [in] buffer 借用正文
    @param [in] length 正文字节数
    @param [in] type 消息类型
    @param [in] retryTransient 是否内部无限重试CAS竞争
    @param [in] retryCapacity 是否内部无限重试积压预算不足
    @return ProcessWorkPostResult，所有失败均未提交
    */
    int32_t enqueue(int32_t pid, const std::string& name, const char* buffer, int32_t length,
        MessageType type, bool retryTransient, bool retryCapacity);

    /** 将队列结果转换为Post结果，容量与资源不足不得映射为竞争忙
    @param [in] result 本次队列申请或发布结果
    @return ProcessWorkPostResult，成功以外均未接纳
    */
    static int32_t postQueueResult(LockFreeQueueResult result);

protected:
    // 接收器资源由生命周期owner创建，读任务借用至加入完成
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::shared_ptr<ProcessWorkChannel> m_channel;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 独占本地名称发现映射对象
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::unique_ptr<SharedMemory> m_nameMemory;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 初始化阶段设置，接收运行期间不变。
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::string m_receiveName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 是否拥有名称发布权；同名其他接收器仅按PID工作
    bool m_nameOwned;
    // 回调对象由调用方拥有，运行中不增删
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::vector<ProcessReceiveCallback*> m_callback;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 单例初始化时固定的本进程PID
    int32_t m_thisProcessPid;
    // 两个接收任务分别负责共享数据与业务回调，一个Post任务统一发送
    uint32_t m_readThreadId;
    // 业务回调任务所在的受管线程ID
    uint32_t m_receiveThreadId;
    // 后台发送任务所在的受管线程ID
    uint32_t m_postThreadId;
    // 区分已经接纳后台任务与准备失败后尚待回收的空线程；仅生命周期所有者读写
    bool m_postStarted;
    // 缓存只发布准备完整的对象；发送者持有副本，删除不会关闭在用通道。
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeMap<int32_t, std::shared_ptr<ProcessWorkChannel>> m_sendChannels;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 有界通道借用缓存，清理并发时以保护记录延后回收，不保留线程局部拥有引用
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    ProcessWorkSendCache m_sendCache;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 发布完整初始化后的名称映射，删除不使在用引用失效
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeMap<std::string, std::shared_ptr<SharedMemory>> m_sendNameMemoryMap;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 本地收件队列持有独立正文副本，业务回调完成后释放
    ProcessWorkReceiveQueue m_receiveQueue;
    // 等待业务回调处理的合并满载快照
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<int32_t> m_fullCount;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 本地唯一Post队列；生命周期操作前调用方先停止外部生产者
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<bool> m_waitEndPost;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 拥有本地待发副本与在途计费，停止后才能清理
    ProcessWorkLocalQueue m_postQueue;
    // 初始化时复制的完整组件配置；活动收发期间只读
    ProcessWorkConfig m_components;
};
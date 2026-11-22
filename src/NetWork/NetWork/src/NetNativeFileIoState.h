#pragma once
#include "NetNativeFileIo.h"
#include "NetWorkFilePools.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include <map>
#include <string>
#include <vector>

// 仅文件执行者使用的独占资源所有者；平台系统操作全部调用CSystem静态函数。
class NetFileIoHandle
{
public:
    /** 初始化无效句柄
    */
    NetFileIoHandle();

    /** 关闭自身句柄，不执行隐式持久化
    */
    ~NetFileIoHandle();

    /** 打开系统普通文件
    @param [in] path UTF8路径
    @param [in] mode 系统打开模式
    @return 是否成功
    */
    bool open(const char* path, uint32_t mode);

    /** 在文件执行者独占创建临时普通文件，只有创建成功后才拥有删除权
    @param [in] path 宿主提供的本地UTF8路径，不能取自远端文件名
    @return 是否成功，既有文件/创建失败不授予删除权
    */
    bool openTemporary(const char* path);

    /** 提交NEW／WRITE／TEMPORARY打开的同一源路径，关闭后原子发布；仅TEMPORARY失败自动清理
    @param [in] source 必须对应本句柄创建的路径
    @param [in] destination 宿主选择的本地UTF8目标
    @param [in] replace 是否允许替换
    @return 是否完成提交，拒绝不会删除其他文件
    */
    bool publish(const char* source, const char* destination, bool replace);

    /** 复制借用的系统句柄
    @param [in] native 原句柄
    @param [in] writable 当前工作是否允许写
    @return 是否成功
    */
    bool duplicate(intptr_t native, bool writable);

    /** 转交系统有界位置读
    @param [in] offset 位置
    @param [out] bytes 缓冲
    @param [in] size 容量
    @param [out] received 实际长度
    @return 是否成功
    */
    bool read(uint64_t offset, uint8_t* bytes, uint32_t size, uint32_t* received);

    /** 转交系统完整位置写
    @param [in] offset 位置
    @param [in] bytes 数据
    @param [in] size 长度
    @return 是否成功
    */
    bool write(uint64_t offset, const uint8_t* bytes, uint32_t size);

    /** 当前写入实际持久化
    @return 是否成功
    */
    bool flush();

    /** 读取长度
    @param [out] bytes 长度
    @return 是否成功
    */
    bool size(uint64_t* bytes);

    /** 截断/扩展
    @param [in] bytes 新长度
    @return 是否成功
    */
    bool resize(uint64_t bytes);

    /** 关闭并置无效，临时文件清理仅使用创建前准备的路径，不再分配内存
    @return 是否成功
    */
    bool close();

    /** 最近系统错误
    @return 错误码
    */
    uint32_t lastError() const;
private:
    /** 禁止复制
    @param [in] other 原对象
    */
    NetFileIoHandle(const NetFileIoHandle& other);

    /** 禁止赋值
    @param [in] other 原对象
    @return 不可调用
    */
    NetFileIoHandle& operator=(const NetFileIoHandle& other);
private:
    // 本对象拥有的系统文件句柄，-1表示尚未打开或已关闭
    intptr_t m_handle;
    // 最近一次系统文件操作错误码；0表示无错误
    uint32_t m_error;
    // 当前句柄是否允许写入、截断和持久化
    bool m_writable;
    // 可写文件打开前复制规范源路径；NEW／WRITE可持句柄发布，只有OPEN_TEMPORARY自动删除。
    std::string m_temporary;
#ifdef _WIN32
    // 独占创建前准备Windows删除路径，低内存关闭/析构期间不做编码转换
    std::wstring m_cleanupPath;
#endif
    // 仅独占创建成功才为true，既有文件永远不授予删除权
    bool m_ownsTemporary;
};

// 一个已接纳文件请求的独占输入、输出与完成状态，执行期间不再借用上层数据
struct NetFileIoEntry
{
public:
    // 请求标量副本，借用指针均清空，数据由下列独占字段保留
    NetFileIoRequest m_request;
    // 文件执行者写结果，完成原子发布后只读；bytes在owner的peek时绑定
    NetFileIoResult m_result;
    // 请求源路径的UTF8副本，不再借用调用方字符串
    std::string m_path;
    // 发布目标路径的UTF8副本，仅PUBLISH使用
    std::string m_destination;
    // WRITE输入的独占副本，接纳后至consume计入排队字节预算
    NetBufferLease m_input;
    // 输入有效长度，跨线程执行期间不变
    uint32_t m_inputSize;
    // READ或HASH结果的独占字节，consume前保持有效
    NetBufferLease m_output;
    // 输出有效长度，完成release后供owner读取
    uint32_t m_outputSize;
    // 跨线程取消标志；文件执行者在工作开始及摘要分块间检查
    std::atomic<bool> m_cancelled;
    // 文件执行者release发布，协议owner acquire读取；完成发布为工作线程最后一次访问
    std::atomic<bool> m_completed;

public:
    /** 复制有界请求
    @param [in] request 已验证参数
    */
    explicit NetFileIoEntry(const NetFileIoRequest& request);

    /** 擦除输入/输出
    */
    ~NetFileIoEntry();
};

// 协议owner独占接纳表；无锁队列交给唯一文件执行者，结果以release/acquire发布
class NetFileIoState
{
  public:
    /** 初始化独立存储池及协调状态
    @param [in] pools 启动前选定的实际配置，复制进各组件
    */
    explicit NetFileIoState(const NetWorkFilePools& pools = NetWorkFilePools());

    /** 文件任务已经停止后释放状态，更新存活计数；禁止在任务执行中销毁
    */
    ~NetFileIoState();

    /** 原子关闭接纳并通知文件执行者；已进入提交者结束后才能判定排空。
    */
    void stop();

    /** 合并非阻塞提示；持久许可覆盖等待登记前发布
    */
    void wake();

    /** 文件执行者处理一个请求；不持协调锁做I/O
    @param [in,out] entry 当前独占工作
    */
    void execute(NetFileIoEntry& entry);

    /** 文件执行者中的增量摘要，逐64KiB检查停止/取消
    @param [in,out] entry 工作
    @param [in] file 已打开句柄
    */
    void hash(NetFileIoEntry& entry, NetFileIoHandle& file);

  public:
    // 持久计数提示；等待有100ms兜底，提示失败不会导致停止永久挂起
    Semaphore m_wake;
    // 合并提示；实际队列发布先于置位，空观察后消费提示并再次检查队列
    std::atomic<bool> m_notified;
    // 0开放、1唯一提交者在途、2关闭、3关闭且原提交者仍在途；不是互斥锁
    std::atomic<uint32_t> m_admission;
    // 构造期真实错误码；区分配置非法与系统分配失败，失败时不创建文件任务
    int32_t m_poolStatus;
    // 多生产者能力只使用单生产者子集；指针由owner表保留到完成后consume
    LockFreeQueue<NetFileIoEntry*> m_queued;
    // 实际正文池，晚于所有在途请求归还
    NetBufferPool m_bodies;
    // 真实任务记录，不能在文件执行者仍访问时归还
    NetObjectPool<NetFileIoEntry> m_entryPool;
    // 实际句柄记录，只由文件执行者修改对象
    NetObjectPool<NetFileIoHandle> m_filePool;
    // 仅协议owner访问；cancel/peek/consume串行，StopTask不访问此表
    std::map<uint64_t, NetObjectReference<NetFileIoEntry>> m_entries;
    // 只有文件执行者访问下面句柄表。
    std::map<uint64_t, NetObjectReference<NetFileIoHandle>> m_files;
    // 停止接纳并取消排队工作；执行者收到停止后仍逐项提交完成结果
    std::atomic<bool> m_stop;
    // 执行者已关闭全部文件并完成最后一次状态访问
    std::atomic<bool> m_done;
    // 仅执行者使用；系统等待失败时使排队项得到明确失败结果
    bool m_waitFailed;
    // 已尝试提交的最大请求号；失败的请求号也不得再次提交
    uint64_t m_lastRequest;
    // 对外分配的最大请求号，与外部显式提交编号共同保持单调
    uint64_t m_allocatedRequest;
    // 唯一文件执行者分配的文件编号高水位，关闭后不复用
    uint64_t m_lastFile;
    // 已接纳WRITE副本总字节，仅统计；consume后归还实际缓冲
    uint64_t m_inputBytes;
    // 已经构造的协调状态数，存在任何文件后端时禁止替换默认配置
    static std::atomic<size_t> m_instances;
};

// 单生产者接纳租约；不等待竞争者，关闭位与在途位在同一个CAS字中
class NetFileIoAdmission
{
public:
    /** 尝试取得唯一生产者资格，关闭或误用并发时立即拒绝
    @param [in,out] state 生存期覆盖本次submit
    */
    explicit NetFileIoAdmission(NetFileIoState& state);

    /** 归还在途位并提示停止方复查；不访问已完成的请求对象
    */
    ~NetFileIoAdmission();

    /** 观察本次是否接纳，不能据此允许另一个生产者并发修改表
    @return 本次CAS成功为true
    */
    bool accepted() const;

private:
    /** 禁止复制提交租约
    @param [in] other 原对象
    */
    NetFileIoAdmission(const NetFileIoAdmission& other);

    /** 禁止重复归还在途位
    @param [in] other 原对象
    @return 不可调用
    */
    NetFileIoAdmission& operator=(const NetFileIoAdmission& other);
private:
    // 借用固定状态，生命周期由backend和Task共同持有
    NetFileIoState& m_state;
    // 只有CAS成功的租约负责归还在途计数
    bool m_accepted;
};

// 独占文件句柄表的长任务；停止后排空取消结果并回收所有句柄
class NetFileIoTask : public CTask
{
  public:
    /** 持有协调状态，直到工作及关闭完成
    @param [in] state 独立上下文
    */
    explicit NetFileIoTask(const std::shared_ptr<NetFileIoState>& state);

    /** 唯一文件执行者
    */
    virtual void doTask();

    /** 仅置停止位并唤醒
    */
    virtual void stopTask();

  private:
    // 与后端共享的协调状态，任务退出前保留其生命周期
    std::shared_ptr<NetFileIoState> m_state;
};
#pragma once
#include "NetClientSession.h"
#include "NetSenderSubmission.h"

class NetSenderDispatchMap;

// owner在锁外把本地接纳交给唯一NI会话；三种NetSender共用，不创建线程
class NetInterfaceAPI NetSenderDispatch : private NetSessionListener
{
public:
    /** 绑定不拥有的NI会话，尚不建立连接
    @param [in] session 有效至本对象收敛和释放
    */
    explicit NetSenderDispatch(NetClientSession* session);

    /** 不自动销毁借用会话，拥有者须已shutdown
    */
    ~NetSenderDispatch();

    /** 生命周期开始前借用外层提交端口；Peer默认使用内置单执行者端口
    @param [in] submission 非空且活到本分派器释放
    @return OK或INVALID_STATE／INVALID_ARGUMENT
    */
    int32_t bindSubmission(NetSenderSubmission* submission);

    /** 为下一次连接借用监听者，只允许完全静止时调用
    @param [in] listener 非空，活到回调收敛
    @return OK或错误
    */
    int32_t prepare(NetSessionListener* listener);

    /** owner记录协议已接纳连接，不制造就绪
    @param [in] generation 本次非零代次
    @param [in] product 固定服务产品或0
    @return OK或错误
    */
    int32_t started(uint64_t generation, uint32_t product);

    /** 取得供NI连接借用的监听入口，不转交所有权
    @return 本分派器的协议监听者
    */
    NetSessionListener* listener();

    /** 返回仅承载本地接纳的端口，不暴露协议对象
    @return 拥有者负责外层生命期的端口
    */
    NetSenderSubmission& submission();

    /** owner有界驱动协议及提交，禁止重入
    @return 仍有协议或本地资源
    */
    bool poll();

    /** owner收敛；回调／poll中只标记关闭，不递归收尾
    */
    void shutdown();

    /** 检查当前分派或回调，供拥有者拒绝销毁
    @return 是否正在执行
    */
    bool executing() const;

    /** 检查协议和本地接纳均已释放
    @return 是否可最终销毁
    */
    bool quiescent() const;

    /** 取得本执行者最近内部失败，不代替各次提交返回值
    @return NI_OK或内部失败
    */
    int32_t error() const;

    /** 发起同步请求交付，回调内拒绝且不接纳输入
    @param [in] id 原预留号
    @param [in] bytes 调用时复制
    @param [in] size 字节数
    @return 交付结果，不等待远端响应
    */
    int32_t sendRequest(uint64_t id, const uint8_t* bytes, uint32_t size);

    /** 发起同步流帧交付
    @param [in] frame 调用时复制
    @return 交付结果
    */
    int32_t sendData(const NetInterfaceFrame& frame);

private:
    /** 原生单执行者推进至同步票据完成，不创建线程
    @param [in] ticket 本次接纳票据
    @return 已交付或错误
    */
    int32_t waitSynchronous(uint64_t ticket);

private:
    /** 协议就绪更新提交快照后转发
    @param [in] generation 协议代次
    @param [in] requests 协商请求上限
    @param [in] chunk 协商块上限
    */
    virtual void onSessionReady(uint64_t generation, uint32_t requests, uint32_t chunk);

    /** 只转发第一次且身份匹配的终态
    @param [in] generation 原代次
    @param [in] requestId 线上防重放编号，回调前还原业务预留号
    @param [in] product 产品
    @param [in] type 命令
    @param [in] status RPC结果
    @param [in] bytes 借用体
    @param [in] size 字节数
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 锁外转发本代数据，关闭后不再交付新业务帧
    @param [in] generation 代次
    @param [in] frame 借用帧
    @return 是否接纳
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 先结算尚未交协议的请求，再报告唯一关闭
    @param [in] generation 原代次
    @param [in] reason 原因
    @param [in] detail 诊断
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

    /** 锁外交付本地或协议终态，异常转为可观察失败
    @param [in] metadata 可信接纳身份
    @param [in] status RPC状态
    @param [in] bytes 借用体
    @param [in] size 字节数
    */
    void report(const NetSenderSubmissionCommand& metadata, uint32_t status, const uint8_t* bytes, uint32_t size);

    /** owner处理一个动作，不持提交锁
    @param [in] command 当前固定借用视图
    */
    void execute(const NetSenderSubmissionCommand& command);

    /** 在owner上查找本地预留号对应的线上防重放编号
    @param [in] local 本地业务预留号
    @return 0未进入协议，否则为当前请求的线上号
    */
    uint64_t wireId(uint64_t local) const;

    /** 仅由owner观察完整交付边界；前一个业务帧未交付时不能追加后续业务
    @return 可以继续提交下一动作；false仍背压或已关闭
    */
    bool advanceOutput();

    /** 关闭并保留内部错误
    @param [in] reason 失败原因
    */
    void fail(int32_t reason);

private:
    // 借用会话，由Peer或桌面后端独占
    NetClientSession* m_session;
    // 单执行者缺省接纳表，所有内存在本模块释放
    NetSenderSubmission m_local;
    // 可由NW提供的借用本地端口，不是另一份协议状态
    NetSenderSubmission* m_submission;
    // 连接期间借用用户监听者
    NetSessionListener* m_listener;
    // 防止重入及局部终态回调期间销毁
    bool m_polling;
    // 业务回调深度
    uint32_t m_callbackDepth;
    // 关闭通知去重
    uint64_t m_closedGeneration;
    // 内部失败诊断
    int32_t m_error;
    // 线上号按实际发送顺序递增，不改变业务预留号、原期限或回调身份。
    uint64_t m_nextWire;
    // 独占固定128条的双向编号索引，无运行期树节点分配或反向全表扫描。
    NetSenderDispatchMap* m_mapping;
    // 当前业务帧仍有本层输出后缀；post同样需要维持发送先后。
    bool m_outputPending;
};
#pragma once
#include "NetInterfaceTypes.h"

class NetSenderSubmissionState;

// 可选提交锁／唤醒，默认空供单执行者；函数不得抛异常或在锁内调业务
struct NetSenderSubmissionGate
{
    // 借用至所有提交者和执行者退出
    void* m_context;
    // 成对的短临界区入口；空表示调用方已保证串行
    void (*m_enter)(void* context);
    // 与enter配对，不负责对象生命期
    void (*m_leave)(void* context);
    // 可空的有界通知；失败仍由协议执行者的定时推进兜底
    void (*m_wake)(void* context);
};

// 本地提交动作，不是线协议种类
enum NetSenderSubmissionKind
{
    // 无待执行动作
    NET_SUBMIT_NONE = 0,
    // 将已接纳编号交给唯一协议请求表
    NET_SUBMIT_BEGIN = 1,
    // 提交已拥有的请求体
    NET_SUBMIT_SEND = 2,
    // 请求取消已进入协议层的操作
    NET_SUBMIT_CANCEL = 3,
    // 提交有界流帧
    NET_SUBMIT_DATA = 4,
    // 先于普通数据关闭
    NET_SUBMIT_CLOSE = 5,
    // 尚未进入协议的取消／超时终态
    NET_SUBMIT_TERMINAL = 6
};

// owner借用的稳定动作视图，直到finish前有效；不是可跨线程持有的消息
struct NetSenderSubmissionCommand
{
    // 本地动作
    int32_t m_kind;
    // 连接代次
    uint64_t m_generation;
    // 请求或本地流动作编号
    uint64_t m_id;
    // 请求产品
    uint32_t m_product;
    // 请求类型，0仍是合法业务类型
    uint32_t m_type;
    // 原始单调起点
    uint64_t m_started;
    // 原始期限毫秒
    uint32_t m_timeout;
    // 本地终态或关闭原因
    int32_t m_status;
    // 仅SEND借用的数据
    const uint8_t* m_bytes;
    // 借用数据长度
    uint32_t m_size;
    // 仅DATA有效；payload借用至finish
    NetInterfaceFrame m_frame;
};

// 统一本地接纳，最多128预留、2MiB数据／64流项、128KiB／16控制项；不拥有协议／线程
class NetInterfaceAPI NetSenderSubmission
{
public:
    /** 创建静止的独占接纳表，不创建线程
    */
    NetSenderSubmission();

    /** 释放本模块内存；拥有者须已拒绝新调用并收敛执行者
    */
    ~NetSenderSubmission();

    /** 在任何执行者启动前设置一次提交门，空门保持单执行者
    @param [in] gate enter与leave必须同时为空或同时有效；借用context
    @return OK或INVALID_ARGUMENT／INVALID_STATE
    */
    int32_t configureGate(const NetSenderSubmissionGate& gate);

    /** owner开始新逻辑连接；请求、正文借用及同步回执全部收敛后才允许
    @param [in] generation 此次非零代次
    @param [in] product 固定产品，0表示旧通用连接
    @return OK或INVALID_STATE
    */
    int32_t start(uint64_t generation, uint32_t product);

    /** owner报告真实握手；存在旧代输入或未完成回执时禁止切换代次；已完成回执仍归原调用者读取，不扣线协议额度
    @param [in] generation 真实非零代次
    @param [in] requests 协商的1至128条上限
    @param [in] chunk 协商的1024至65536字节
    @return OK、CANCELLED或PROTOCOL_ERROR
    */
    int32_t ready(uint64_t generation, uint32_t requests, uint32_t chunk);

    /** 预留本地请求及唯一编号；OK后包括未send也有终态
    @param [in] product 非零且符合固定产品
    @param [in] type 业务命令
    @param [in] timeoutMs 1至60000，包含排队时间
    @param [out] id 失败为0，成功非零且不复用
    @return OK仅表示本地接纳，或明确未接纳错误
    */
    int32_t begin(uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& id);

    /** 复制并接管请求体，接纳成功不能再次send
    @param [in] id 本代未发送预留
    @param [in] bytes size非零不可空，借用至返回
    @param [in] size 0至1048560字节
    @return OK或明确未接纳错误，满字节额度可原号重试
    */
    int32_t send(uint64_t id, const uint8_t* bytes, uint32_t size);

    /** 不分配内存的取消接纳，满数据队列也可调用
    @param [in] id 当前未终结请求
    @return OK或NOT_FOUND
    */
    int32_t cancel(uint64_t id);

    /** 复制流帧，ACK／文件取消使用独立有界控制预算
    @param [in] frame 合法DATA／ACK或文件CANCEL，借用至返回
    @return OK或明确未接纳错误
    */
    int32_t data(const NetInterfaceFrame& frame);

    /** 无分配标记停止新接纳，重复关闭保留第一次原因
    @param [in] reason 关闭原因
    */
    void close(int32_t reason);

    /** owner在触碰运输之前观察关闭，防止已取消的连接还去创建socket
    @param [out] reason 首个关闭原因，未关闭时0
    @return 是否已请求关闭
    */
    bool closeRequested(int32_t& reason) const;

    /** 取得一致就绪快照
    @param [out] generation 最近代次
    @param [out] ready 是否真实就绪且未关闭
    */
    void state(uint64_t& generation, bool& ready) const;

    /** owner开始有界一轮，不能在尚有借用动作时调用
    @return OK或INVALID_STATE
    */
    int32_t beginPoll();

    /** owner借用下一动作；停止／取消优先，RPC、DATA及控制帧共用实际接纳FIFO
    @param [out] command 无动作时清零
    @return READY、NEED_MORE或INVALID_STATE
    */
    int32_t next(NetSenderSubmissionCommand& command);

    /** owner结束借用；协议及回调必须在调用本函数前锁外完成
    @param [in] result 本次协议调用结果，LIMIT保留未发送数据
    */
    void finish(int32_t result);

    /** owner取出唯一终态的可信元数据，未知／旧代返回false
    @param [in] generation 原代次
    @param [in] id 原请求号
    @param [out] command 成功填充关联字段，否则清零
    @return 是否第一次结束本请求
    */
    bool complete(uint64_t generation, uint64_t id, NetSenderSubmissionCommand& command);

    /** owner关闭快照，清理普通流帧；请求仍逐个取终态
    @param [in] generation 此次关闭代次
    */
    void disconnected(uint64_t generation);

    /** owner逐个取关闭后剩余的未回报请求
    @param [out] command 可信关联，失败清零
    @return 是否还有待结算请求
    */
    bool takeClosed(NetSenderSubmissionCommand& command);

    /** 只读确认本地接纳及借用动作已经收敛
    @return 没有请求、流输入或owner借用
    */
    bool idle() const;

    /** 观察有界输入预算，用于测试及诊断
    @return 当前仍拥有的请求／普通流输入字节
    */
    uint32_t queuedBytes() const;

    /** 取得统一单调时钟，不创建资源
    @return 单调毫秒
    */
    static uint64_t now();

    /** 接纳同步请求，每票据由其原等待者读取；128个票据满时返回LIMIT且未接纳
    @param [in] id 原预留号
    @param [in] bytes 调用时复制
    @param [in] size 业务体字节数
    @param [out] ticket 接纳后非零，失败为0
    @return 接纳结果；交付结果须继续读取票据
    */
    int32_t sendSynchronous(uint64_t id, const uint8_t* bytes, uint32_t size, uint64_t& ticket);

    /** 接纳同步数据帧，交付期限五秒
    @param [in] frame 调用时复制
    @param [out] ticket 接纳后非零
    @return 接纳结果
    */
    int32_t dataSynchronous(const NetInterfaceFrame& frame, uint64_t& ticket);

    /** 查询是否需要owner检查同步交付，不访问协议对象
    @return 是否仍有待完成的票据
    */
    bool synchronousPending() const;

    /** owner先finish借用，再依据完整运输链结算；未交协议的过期同步流帧移除，已接纳前缀则关闭本代
    @param [in] outputState OK已全部交付、READY仍有后缀或负数失败
    */
    void advanceSynchronous(int32_t outputState);

    /** 等待者读取一次交付结果；不等待远端业务回应
    @param [in] ticket 本次接纳票据
    @param [in] consume 结果完成后是否归还本地票据槽
    @return READY尚未交付、OK已交付或负数失败
    */
    int32_t synchronousResult(uint64_t ticket, bool consume);

private:
    /** 在同一个提交门内接纳请求并可选登记同步票据
    @param [in] id 原预留号
    @param [in] bytes 调用时复制
    @param [in] size 字节数
    @param [in] synchronous 是否登记交付等待
    @param [out] ticket 非同步或失败为0
    @return 接纳结果
    */
    int32_t submitRequest(uint64_t id, const uint8_t* bytes, uint32_t size, bool synchronous, uint64_t& ticket);

    /** 在同一提交门内接纳流帧并可选登记同步票据
    @param [in] frame 调用时复制
    @param [in] synchronous 是否登记交付等待
    @param [out] ticket 非同步或失败为0
    @return 接纳结果
    */
    int32_t submitData(const NetInterfaceFrame& frame, bool synchronous, uint64_t& ticket);

private:
    /** 禁止复制接纳表
    @param [in] other 原实例
    */
    NetSenderSubmission(const NetSenderSubmission& other);

    /** 禁止复制所有权
    @param [in] other 原实例
    @return 不可调用
    */
    NetSenderSubmission& operator=(const NetSenderSubmission& other);

private:
    // 本模块创建和释放，外部只借用统一提交端口
    NetSenderSubmissionState* m_state;
};
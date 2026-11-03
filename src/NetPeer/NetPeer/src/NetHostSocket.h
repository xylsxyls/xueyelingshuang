#pragma once
#include "NetPeerMacro.h"
#include "NetHostTypes.h"
#include "NetInterface/NetInterfaceAPI.h"

class NetHostSocketState;

// 单执行者的原始字节观察者，所有事件只由poll/shutdown交付
class NetPeerAPI NetHostSocketListener
{
public:
    /** 不拥有运输，必须存活至shutdown完成
    */
    virtual ~NetHostSocketListener();

    /** 原始连接已建立，尚未认证；回调内可写/暂停/关闭
    @param [in] peer 此实例内不复用的连接号
    */
    virtual void onConnected(uint64_t peer) = 0;

    /** 输入已复制为有界块并自动暂停，宿主需显式恢复才能继续收取
    @param [in] peer 当前连接
    @param [in] data 借用至回调返回
    @param [in] size 1至16384字节
    */
    virtual void onData(uint64_t peer, const uint8_t* data, uint32_t size) = 0;

    /** 本地在途写释放，不改变远端通道额度
    @param [in] peer 连接号
    */
    virtual void onWritable(uint64_t peer) = 0;

    /** 连接唯一终态；此后旧事件忽略，不允许复用连接号
    @param [in] peer 已回收连接
    @param [in] reason 首个主动NI结果；远端原始非零错误统一INVALID_STATE，正常EOF为0
    @param [in] detail 仅诊断的原始宿主错误，主动关闭为0；不得按NI枚举解释
    */
    virtual void onClosed(uint64_t peer, int32_t reason, int32_t detail) = 0;
};

// 函数表与有界事件协调，不包含Libuv/ProcessWork/线程/文件实现
class NetPeerAPI NetHostSocket
{
public:
    /** 创建尚未分配宿主资源的实例
    @param [in] functions 可空使用预先配置的默认表；非空复制表，context仍借用
    */
    explicit NetHostSocket(const NetHostFunctions* functions = nullptr);

    /** 同一执行者停止并释放，禁止在本实例回调内销毁
    */
    ~NetHostSocket();

    /** 进程启动时一次性配置，必须在创建工作执行者及Socket前完成
    @param [in] functions 完整v1函数表，context保持到所有实例销毁
    @return OK或INVALID_ARGUMENT/INVALID_STATE；成功后不能更换，不存放会话状态
    */
    static int32_t configureDefault(const NetHostFunctions& functions);

    /** 查询启动前已选择的默认传输能力，不创建实例
    @param [in] server true查询监听，false查询主动连接
    @return 默认宿主是否提供该方向；未配置返回false
    */
    static bool defaultSupports(bool server);

    /** 监听一个宿主实例，只能启动一次
    @param [in] address 数值地址，借用到返回
    @param [in] port 0至65535
    @param [in] listener 借用至shutdown完成，不可空
    @param [in] maxPeers 1至256，默认64
    @return OK或负数；未配置宿主为NOT_SUPPORTED
    */
    int32_t listen(const char* address, uint32_t port, NetHostSocketListener* listener, uint32_t maxPeers = 64);

    /** 异步接纳单个连接，只能启动一次；就绪交给poll
    @param [in] address 数值地址，借用到返回
    @param [in] port 1至65535
    @param [in] listener 借用至shutdown完成，不可空
    @param [out] peer 成功非零，失败0
    @return OK或负数
    */
    int32_t connect(const char* address, uint32_t port, NetHostSocketListener* listener, uint64_t& peer);

    /** 非阻塞处理宿主和有界事件，禁止递归驱动；回调异常仅关闭本peer
    @return 仍有活动资源/待处理事件
    */
    bool poll();

    /** 只借用到返回，成功必须完整接纳，不接受部分写
    @param [in] peer 当前已建立连接
    @param [in] data size非零时不可空
    @param [in] size 0至32768
    @return OK完整接纳，LIMIT_EXCEEDED完全未接纳背压，其他负数失败
    */
    int32_t write(uint64_t peer, const uint8_t* data, uint32_t size);

    /** 控制后续输入，回调中可用；每次输入先自动暂停
    @param [in] peer 当前连接
    @param [in] paused 是否暂停
    @return OK或错误
    */
    // 原始EOF已经到达、关闭回调尚未交付时，只更新排队尾部的交付门；终态交付后仍拒绝旧peer。
    int32_t pauseRead(uint64_t peer, bool paused);

    /** 标记关闭，迟到数据不再交付；唯一终态由poll结算
    @param [in] peer 当前连接，不存在无操作
    @param [in] reason 首个关闭原因
    */
    void close(uint64_t peer, int32_t reason = 0);

    /** 回调内只请求停止，外层调用则停止宿主并结算唯一终态
    */
    void shutdown();

    /** 停止并释放宿主，失败保留状态允许宿主修复后重试；成功后不可再启动
    @return OK已释放，INVALID_STATE回调/poll内拒绝或宿主未释放，其他为持续错误
    */
    int32_t uninit();

    /** 查询持续宿主错误，不改变状态
    @return OK或首次负数错误；背压不记为持续错误
    */
    int32_t status() const;

    /** 查询实际监听端口
    @return 端口，未监听0
    */
    uint32_t localPort() const;

    /** 查询尚未交付终态的连接数
    @return 0至256
    */
    uint32_t peerCount() const;

    /** 判断发送未接纳是否为可重试背压
    @param [in] result write结果
    @return 仅LIMIT_EXCEEDED为true
    */
    static bool isBackpressure(int32_t result);

    /** 使用预先配置的宿主密码学随机，不建立物理连接
    @param [out] output size非零时不可空，失败清理有效范围
    @param [in] size 0至8192，非法范围不访问输出
    @return OK、INVALID_ARGUMENT或INVALID_STATE；不存在强随机时拒绝
    */
    static int32_t randomBytes(uint8_t* output, uint32_t size);

private:
    /** 禁止复制宿主所有权
    @param [in] other 原实例
    */
    NetHostSocket(const NetHostSocket& other);

    /** 禁止复制宿主所有权
    @param [in] other 原实例
    @return 不可调用
    */
    NetHostSocket& operator=(const NetHostSocket& other);

private:
    // 独占事件队列和宿主，跨边界不暴露STL
    NetHostSocketState* m_state;
};
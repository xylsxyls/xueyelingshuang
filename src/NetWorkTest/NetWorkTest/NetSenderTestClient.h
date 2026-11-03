#pragma once
#include "NetInterface/NetInterfaceAPI.h"

// 既有场景用例的生命周期适配器；所有操作调用真实单例门面，不实现协议或判定预期
template<class Entry, class Client>
class NetSenderTestClient
{
public:
    /** 创建空测试持有者，实际客户端由prepareGateway经真实门面创建
    */
    NetSenderTestClient();

    /** 回收已静止的测试客户端；用例必须先关闭并收敛回调
    */
    ~NetSenderTestClient();

    /** 取得当前测试提供方的默认持有者
    @return 用例间必须显式releaseGateway清空
    */
    static NetSenderTestClient& instance();

    /** 取得真实客户端，供桌面专用配置使用
    @return 借用指针，不转移所有权
    */
    operator Client*() const;

    /** 准备独立客户端；桌面默认共享本模块管理的IPC接收器，进程内准备协议上下文。
    线程：三种提供方均由唯一生命周期owner串行调用，不与本客户端其他接口并发；禁止在协议回调中调用
    外部接收器仅通过桌面后端在启动前声明，不在共同业务方法中传入进程细节。
    @return NI_OK或负数；可重复调用
    */
    int32_t prepareGateway();

    /** 连接安全网关；桌面IPC经已配置后端接本机网关，进程内接远端数值IP
    线程：三种提供方均由唯一生命周期owner串行调用，不与本客户端其他接口并发；禁止在协议回调中调用
    @param [in] options v1客户端配置；nonce由核心强随机生成
    @param [in] listener 借用至停止；桌面和NetPeer回调在poll执行者，NetWork在内部执行者；禁止销毁或等待
    @return OK接纳，真实就绪由回调；不降级旧协议
    */
    int32_t connectGateway(const NetSenderGatewayOptions& options, NetSessionListener* listener);

    /** NTW3逻辑服务端到端TLS连接；桌面经透明IPC，进程内通过注入的原始字节宿主连接网关
    线程：三种提供方均由唯一生命周期owner串行调用，不与本客户端其他接口并发；禁止在协议回调中调用
    @param [in] product 目标服务号，每对象本代次固定一个目标
    @param [in] options 原v1配置布局，地址/能力/设备/强随机/期限有效，信任根/服务身份内置
    @param [in] listener 借用至停止；请求/取消/文件接口与原方法一致
    @return NI_OK接纳；真实业务TLS及握手完成后才ready
    */
    int32_t connectServiceGateway(uint32_t product, const NetSenderGatewayOptions& options, NetSessionListener* listener);

    /** 连接前设置公开无秘密RPC压缩候选，默认空；不压缩身份秘密和文件内容
    线程：三种提供方均由唯一生命周期owner串行调用，不与本客户端其他接口并发；禁止在协议回调中调用
    @param [in] types count非零时不可空，仅协议许可的公开命令
    @param [in] count 0至64，0清空
    @return OK或负数，实际压缩仍需协商
    */
    int32_t configureCompression(const uint32_t* types, uint32_t count);

    /** 预留唯一编号，先预留再调用业务SDK构造消息
    仅预留编号及原期限，不占发送顺序；同一客户端同目标／代次按成功send／post的提交先后交付。NetWork并发时每个生产线程自身仍保序，线程间不约定先后；业务预留号与内部线上号分开，回调始终返回原业务号。
    线程：NetWork同一客户端允许多生产者并发并可在回调中调用；桌面src/NetSender和NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] product 非零产品
    @param [in] type 命令
    @param [in] timeoutMs 总期限1至60000
    @param [out] requestId 成功非零，否则0
    @return NI_OK或负数
    */
    int32_t beginRequest(uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& requestId);

    /** 投递预留请求到本进程有界发送队列，只能接纳一次
    提交前登记请求上下文；NetWork的内部执行者可能在调用返回前交付回调。
    线程：NetWork同一客户端允许多生产者并发并可在回调中调用；桌面src/NetSender和NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] requestId 当前预留号
    @param [in] bytes 调用期间借用；接纳后由库保留副本
    @param [in] size 最大1048560
    @return OK表示已复制并入队，不表示调用底层网络接口或远端收到；LIMIT_EXCEEDED未接纳可在原期限内重试
    */
    int32_t postRequest(uint64_t requestId, const uint8_t* bytes, uint32_t size);

    /** 取消请求并通过listener报告唯一终态
    线程：NetWork同一客户端允许多生产者并发并可在回调中调用；桌面src/NetSender和NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] requestId 当前号
    @return OK、NOT_FOUND或负数
    */
    int32_t cancelRequest(uint64_t requestId);

    /** 投递上层授权的数据流帧到本进程有界发送队列
    线程：NetWork同一客户端允许多生产者并发并可在回调中调用；桌面src/NetSender和NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] frame DATA／DATA_ACK或文件CANCEL，产品／流号非零
    @return OK表示帧及载荷已复制并入队，不表示网络发送；背压未接纳，可重试
    */
    int32_t postData(const NetInterfaceFrame& frame);

    /** 同步交付预留请求，不等待远端回应；仅NI_OK表示本次数据已全部交给下一责任层
    线程：NetWork允许外部生产者并发调用，同一客户端最多128个有界同步交付票据；桌面和NetPeer仅同一执行者。均禁止在协议回调／poll中调用，原生调用会等待
    提交前登记请求上下文；原生单执行者send内部推进时可交付回调，不能假定回调晚于函数返回。
    桌面完成边界为写入ProcessWork共享内存，Peer／NetWork为实际网络宿主接口接纳。
    禁止在协议回调／poll内同步等待；GUI宜用post。原请求总期限包含等待，同一客户端最多128个同步交付票据，满时未接纳并背压。
    @param [in] requestId 当前未投递的预留号
    @param [in] bytes 调用时复制，size非零时不可空
    @param [in] size 最大1048560字节
    @return OK已交付；负数不保证对端未收到，禁止自动重放；未接纳的LIMIT可重试
    */
    int32_t sendRequest(uint64_t requestId, const uint8_t* bytes, uint32_t size);

    /** 同步交付已授权的流帧，责任边界与sendRequest相同，最多等待五秒
    线程：NetWork允许外部生产者并发调用，同一客户端最多128个有界同步交付票据；桌面和NetPeer仅同一执行者。均禁止在协议回调／poll中调用，原生调用会等待
    @param [in] frame DATA／ACK／文件CANCEL，调用时复制载荷
    @return OK已交付，负数失败；失败不代表远端未收到，不能盲目重复流块
    */
    int32_t sendData(const NetInterfaceFrame& frame);

    /** 非阻塞推进或观察；桌面和NetPeer由单一owner推进，NetWork仅观察内部执行者状态。
    线程：桌面和NetPeer仅唯一owner推进，禁止重入及回调中调用；NetWork允许外部线程并发观察，但禁止本客户端内部协议回调线程调用。均不得与释放并发
    NetWork允许并发beginRequest、postRequest、cancelRequest、sendData及关闭；准备、连接、配置和释放仍串行。
    NetPeer和桌面由后续poll推进队列，NetWork唤醒内部执行者；post不要求创建后台线程。
    @return 本代仍运行（包含连接中），停止或错误为false
    */
    bool pollGateway();

    /** 请求正常关闭；桌面和NetPeer继续poll收敛，NetWork由内部执行者收敛
    线程：NetWork同一客户端允许多生产者并发并可在回调中调用；桌面src/NetSender和NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] reason 默认0
    */
    void closeGateway(int32_t reason = 0);

    /** 请求关闭，不在调用方等待；继续poll／观察至停止，可重连
    线程：NetWork同一客户端允许多生产者并发并可在回调中调用；桌面src/NetSender和NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    */
    void uninitGateway();

    /** 由非GUI生命期owner释放完整上下文，仅在实际停止、生产者退出及回调收敛之后调用
    线程：三种提供方均由唯一生命周期owner串行调用，不与本客户端其他接口并发；禁止在协议回调中调用
    回调/poll或异步资源未收敛时保留对象，继续推进后重试；成功后可再次prepare。
    @return NI_OK已释放或原本为空；负数表示未释放，调用方仍须持有对象
    */
    int32_t releaseGateway();

    /** 真实握手已通过且未关闭
    线程：NetWork支持同一客户端并发查询；桌面src/NetSender和NetPeer仅同一执行者串行查询。可在回调中查询，不得与生命周期变更并发
    @return 是否可提交
    */
    bool gatewayReady() const;

    /** 当前连接代次
    线程：NetWork支持同一客户端并发查询；桌面src/NetSender和NetPeer仅同一执行者串行查询。可在回调中查询，不得与生命周期变更并发
    @return 未建立为0，之后递增
    */
    uint64_t gatewayGeneration() const;

    /** 最近有返回码操作的诊断；状态查询、成功poll及关闭不清除它。多生产者以各次函数直接返回值为准
    线程：NetWork支持同一客户端并发查询；桌面src/NetSender和NetPeer仅同一执行者串行查询。可在回调中查询，不得与生命周期变更并发
    @return NetInterface结果
    */
    int32_t lastGatewayError() const;

private:
    /** 禁止复制测试客户端
    @param [in] other 不可复制实例
    */
    NetSenderTestClient(const NetSenderTestClient& other);

    /** 禁止复制所有权
    @param [in] other 不可复制实例
    @return 不可调用
    */
    NetSenderTestClient& operator=(const NetSenderTestClient& other);

private:
    // 唯一生命周期owner管理，生产者只在其有效期内借用
    Client* m_client;
};

#include "NetSenderTestClient.inl"
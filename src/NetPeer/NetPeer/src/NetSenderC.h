#pragma once
#include "NetPeerMacro.h"
#include "NetSenderHostC.h"
#include "NetSenderFileC.h"
#define netSenderClientCreate netPeerSenderClientCreate
#define netSenderClientConfigureCompression netPeerSenderClientConfigureCompression
#define netSenderClientConnectService netPeerSenderClientConnectService
#define netSenderClientPoll netPeerSenderClientPoll
#define netSenderClientBegin netPeerSenderClientBegin
#define netSenderClientPost netPeerSenderClientPost
#define netSenderClientCancel netPeerSenderClientCancel
#define netSenderClientPostData netPeerSenderClientPostData
#define netSenderClientClose netPeerSenderClientClose
#define netSenderClientState netPeerSenderClientState
#define netSenderClientDestroy netPeerSenderClientDestroy
#define netSenderClientSend netPeerSenderClientSend
#define netSenderClientSendData netPeerSenderClientSendData

#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct NetPeerSenderClientOpaque* NetSenderClientHandle;

    /** 创建独立C接口发送器，不表示会话就绪
    线程：由唯一生命周期owner串行调用，不与本对象其他接口并发；禁止在协议回调中调用
    create／configure／connect／destroy由唯一生命期owner串行调用，禁止在回调中更换配置或销毁。
    NetPeer的提交／poll属于同一执行者；NetWork允许多个生产者提交begin／send／post／cancel／data／close及查询状态，内部唯一CTask驱动协议和回调。
    context须活至实际停止并且destroy成功；生产者先退出，之后由非GUI生命期owner最终回收。同一句柄不能转交另一提供方。
    @param [in] callbacks 原v1回调表，仅复制函数指针，context借用至destroy
    @param [out] handle 失败清空，在创建模块释放
    @return NI_OK或错误；不启动业务网络
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientCreate(const NetInterfaceClientCallbacks* callbacks, NetSenderClientHandle* handle);

    /** 连接前配置公开RPC类型白名单；最多64项，0清空，默认不压缩。
    线程：由唯一生命周期owner串行调用，不与本对象其他接口并发；禁止在协议回调中调用
    @param [in] handle 有效发送器句柄
    @param [in] types 公开RPC类型数组，调用时复制；count为0可为空
    @param [in] count 类型数量0至64，0清空白名单
    @return NI_OK或参数、状态、内存错误
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientConfigureCompression(NetSenderClientHandle handle, const uint32_t* types, uint32_t count);

    /** 启动v3端到端服务连接，断线重建TLS但不重放业务
    线程：由唯一生命周期owner串行调用，不与本对象其他接口并发；禁止在协议回调中调用
    @param [in] handle 有效句柄
    @param [in] product 1至INT32_MAX产品号，内部账号服务0仅由SERVICE专用接口接入
    @param [in] gatewayPid v1布局保留项，必须为0；进程内提供方不使用桌面进程号
    @param [in] options 原v1原生配置，调用时复制必要值；随机context借用至关闭
    @return NI_OK接纳，真实就绪由回调报告；回调/poll内拒绝
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientConnectService(NetSenderClientHandle handle, uint32_t product, int32_t gatewayPid,
    const NetInterfaceClientOptions* options);

    /** 非阻塞推进，同一执行者且不能重入
    线程：NetPeer仅唯一owner推进，禁止重入及回调中调用；NetWork允许外部线程并发观察，但禁止本对象内部协议回调线程调用。均不得与释放并发
    @param [in] handle 有效句柄
    @return NI_READY仍在运行，NI_OK已停止，负数错误
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientPoll(NetSenderClientHandle handle);

    /** 预留唯一请求号，后续send／post只接纳一次；不代表远端已执行
    begin仅预留原号／期限，成功send／post决定同实例同目标／代次的发送顺序；NetWork多生产者各自保序，线程间先后不约定。内部线上号不暴露，回调仍使用原业务号。
    线程：NetWork同实例允许多生产者并发并可在回调中调用；NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] handle 有效句柄
    @param [in] product 当前目标
    @param [in] type 业务命令
    @param [in] timeoutMs 固定总期限1至60000毫秒
    @param [out] requestId 失败清零
    @return NI_OK或错误
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientBegin(NetSenderClientHandle handle, uint32_t product, uint32_t type,
    uint32_t timeoutMs, uint64_t* requestId);

    /** 投递请求体到本进程有界队列；背压时尚未接纳，可在原期限内重试
    调用前登记请求关联；NetWork回调可与提交者并行，不能假定回调晚于返回。
    线程：NetWork同实例允许多生产者并发并可在回调中调用；NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] handle 有效句柄
    @param [in] requestId 当前预留号
    @param [in] bytes 调用时借用，size非零时不可空
    @param [in] size 业务体长度，最大1048560
    @return NI_OK或错误
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientPost(NetSenderClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size);

    /** 取消已接纳请求并给出唯一终态
    线程：NetWork同实例允许多生产者并发并可在回调中调用；NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] handle 有效句柄
    @param [in] requestId 当前号
    @return NI_OK、NI_NOT_FOUND或错误
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientCancel(NetSenderClientHandle handle, uint64_t requestId);

    /** 投递已授权的数据/确认帧到本进程队列，文件必须绑定原连接代次
    线程：NetWork同实例允许多生产者并发并可在回调中调用；NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] handle 有效句柄
    @param [in] frame 调用时借用，不接受普通RPC帧
    @return NI_OK或错误
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientPostData(NetSenderClientHandle handle, const NetInterfaceFrame* frame);

    /** 主动关闭并停止自动恢复，之后继续poll收敛或非回调destroy
    线程：NetWork同实例允许多生产者并发并可在回调中调用；NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] handle 有效句柄
    @param [in] reason 关闭原因，正常为0
    @return NI_OK或错误
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientClose(NetSenderClientHandle handle, int32_t reason);

    /** 查询本代次和就绪，不代表业务身份有效
    线程：NetWork支持同实例并发查询；NetPeer仅同一执行者串行查询。可在回调中查询，不得与生命周期变更并发
    @param [in] handle 有效句柄
    @param [out] generation 最近连接代次，失败0
    @param [out] ready 真实业务握手完成为1，否则0
    @return NI_OK或错误
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientState(NetSenderClientHandle handle, uint64_t* generation, int32_t* ready);

    /** 在创建模块收敛回调、停止桌面IPC并释放；禁止回调/poll内销毁
    线程：由唯一生命周期owner串行调用，不与本对象其他接口并发；禁止在协议回调中调用
    @param [in] handle 有效句柄或null
    @return NI_OK或NI_INVALID_STATE；拒绝时句柄仍有效
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientDestroy(NetSenderClientHandle handle);

    /** 同步交付请求到共享内存或实际宿主网络接口；不等远端响应
    调用前登记请求关联；单执行者send可能在推进过程中交付回调。
    线程：NetWork允许外部生产者并发调用，同实例最多128个有界同步交付票据；NetPeer仅同一执行者。均禁止在协议回调／poll中调用，原生调用会等待
    @param [in] handle 本提供方有效句柄
    @param [in] requestId 未投递预留号
    @param [in] bytes 调用时复制，size非零时不可空
    @param [in] size 最大1048560字节
    @return 0表示已全部交付，负数失败；回调内拒绝且不接纳；失败禁止自动重放
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientSend(NetSenderClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size);

    /** 同步交付流帧，责任边界同Send，交付期限五秒
    线程：NetWork允许外部生产者并发调用，同实例最多128个有界同步交付票据；NetPeer仅同一执行者。均禁止在协议回调／poll中调用，原生调用会等待
    @param [in] handle 本提供方有效句柄
    @param [in] frame 调用时复制的授权流帧
    @return 0已交付或负数失败，不代表远端收到
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientSendData(NetSenderClientHandle handle, const NetInterfaceFrame* frame);

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C"
{
#endif
#define netSenderClientConnect netPeerSenderClientConnect
    /** 启动直接端点连接，配置调用时借用，实际就绪仍由回调通知
    @param [in] handle 本提供方创建且未销毁的有效句柄
    @param [in] options 完整v1配置，随机上下文借用至关闭
    @return NI_OK、NI_READY或负数错误，具体交付边界见说明
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientConnect(NetSenderClientHandle handle, const NetInterfaceClientOptions* options);

#define netSenderClientSendBegin netPeerSenderClientSendBegin
    /** 非阻塞申请请求交付票据；结果为0仅接纳，须查询SendResult至终态
    @param [in] handle 本提供方创建且未销毁的有效句柄
    @param [in] requestId 当前预留请求号
    @param [in] bytes size非零时不可空，接纳时复制
    @param [in] size 正文字节数
    @param [out] ticket 交付票据，输出失败清零；接纳后须查询至终态
    @return NI_OK、NI_READY或负数错误，具体交付边界见说明
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientSendBegin(NetSenderClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t* ticket);

#define netSenderClientSendDataBegin netPeerSenderClientSendDataBegin
    /** 非阻塞申请帧交付票据，返回后不再借用正文
    @param [in] handle 本提供方创建且未销毁的有效句柄
    @param [in] frame 不可空完整帧，接纳时复制
    @param [out] ticket 交付票据，输出失败清零；接纳后须查询至终态
    @return NI_OK、NI_READY或负数错误，具体交付边界见说明
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientSendDataBegin(NetSenderClientHandle handle, const NetInterfaceFrame* frame, uint64_t* ticket);

#define netSenderClientSendResult netPeerSenderClientSendResult
    /** 查询并消费交付票据终态；NI_READY等待，NI_OK才表示交付完成
    @param [in] handle 本提供方创建且未销毁的有效句柄
    @param [in] ticket 交付票据，输出失败清零；接纳后须查询至终态
    @return NI_OK、NI_READY或负数错误，具体交付边界见说明
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientSendResult(NetSenderClientHandle handle, uint64_t ticket);

#define netSenderClientConfigureGateways netPeerSenderClientConfigureGateways
    /** 配置备用网关及公开缓存，须在创建后、连接前由生命期owner串行调用
    @param [in] handle 本提供方有效句柄；不得与销毁并发，回调内拒绝
    @param [in] seeds 最多4096字节，调用时复制，可空清除
    @param [in] cachePath 最多4096字节，调用时复制，可空禁用
    @return NI_OK或参数、状态错误；不创建连接
    */
    NET_PEER_C_EXPORT NetPeerAPI int32_t netSenderClientConfigureGateways(NetSenderClientHandle handle, const char* seeds, const char* cachePath);

#ifdef __cplusplus
}
#endif
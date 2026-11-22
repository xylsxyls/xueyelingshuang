#pragma once
#include "NetWorkBackend.h"
#include "NetWorkTypes.h"
#include "NetWorkHostPools.h"
#include "NetWorkFilePools.h"
#include "NetPeer/NetPeerAPI.h"
#include "NetWorkClientSession.h"
#include "NetWorkEndpointRouter.h"
#ifndef __EMSCRIPTEN__
#include "NetNativeFileIo.h"
#endif

class NetWorkSenderState;

/** 本库主入口，登记宿主并创建独立发送器；不拥有全局业务会话或增加调度层
*/
class NetWorkAPI NetWork
{
public:
    /** 首次连接前设置并行send执行者数量，线程按需创建；只有生命周期owner调用
    @param [in] count 非零线程数，串行post使用另外一个共享执行者
    @return NI_OK或参数/生命周期错误，存在工作者时不得修改
    */
    static int32_t configureWorkers(uint32_t count);

    /** 低层句柄在首次连接前选择执行者类别，仅生命周期owner调用
    @param [in] client 已创建但从未连接的句柄
    @param [in] parallel true为并行send，false为串行post
    @return NI_OK或参数/生命周期错误
    */
    int32_t clientConfigureDispatch(NetWorkClientHandle client, bool parallel);

    /** prepare之后首次连接前选择调度，配置/连接/销毁由一个生命周期owner串行执行
    @param [in] client 已prepare但从未连接的客户端
    @param [in] parallel true分配并行send执行者，false归共享串行post执行者；默认true
    @return NI_OK或参数/生命周期错误；调用方必须只使用所选send或post，不能混用
    */
    int32_t configureDispatch(NetWorkSenderState* client, bool parallel);

    /** 取得本提供方管理入口；不创建会话或线程
    @return 本库唯一管理实例；各客户端上下文仍独立
    */
    static NetWork& instance();

    /** 取得本模块无状态的专属配置入口；启动前使用
    @return 长期借用的配置对象，不得delete
    */
    static NetWorkBackend& backend();

    /** 创建独立的静止客户端，不启动网络或线程
    线程：唯一生命周期owner创建；首次取得管理入口在启动生产者前完成
    @param [in,out] client 必须为空，成功转交对象；失败不覆盖非空输入
    @return NI_OK、NI_INVALID_ARGUMENT或NI_NO_MEMORY；随后prepareGateway及连接
    */
    int32_t createClient(NetWorkSenderState*& client);

    /** 准备独立客户端；桌面默认共享本模块管理的IPC接收器，进程内准备协议上下文。
    线程：三种提供方均由唯一生命周期owner串行调用，不与本客户端其他接口并发；禁止在协议回调中调用
    外部接收器仅通过桌面后端在启动前声明，不在共同业务方法中传入进程细节。
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @return NI_OK或负数；可重复调用
    */
    int32_t prepareGateway(NetWorkSenderState* client);

    /** 连接安全网关；桌面IPC经已配置后端接本机网关，进程内接远端数值IP
    线程：三种提供方均由唯一生命周期owner串行调用，不与本客户端其他接口并发；禁止在协议回调中调用
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] options v1客户端配置；nonce由核心强随机生成
    @param [in] listener 借用至停止；桌面和NetPeer回调在poll执行者，NetWork在内部执行者；禁止销毁或等待
    未启用 pthreads 的 Wasm 返回 NI_NOT_SUPPORTED。
    @return OK接纳，真实就绪由回调；不降级旧协议
    */
    int32_t connectGateway(NetWorkSenderState* client, const NetSenderGatewayOptions& options, NetSessionListener* listener);

    /** NTW3逻辑服务端到端TLS连接；桌面经透明IPC，进程内通过注入的原始字节宿主连接网关
    线程：三种提供方均由唯一生命周期owner串行调用，不与本客户端其他接口并发；禁止在协议回调中调用
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] product 目标服务号，每对象本代次固定一个目标
    @param [in] options 原v1配置布局，地址/能力/设备/强随机/期限有效，信任根/服务身份内置
    @param [in] listener 借用至停止；请求/取消/文件接口与原方法一致
    未启用 pthreads 的 Wasm 返回 NI_NOT_SUPPORTED。
    @return NI_OK接纳；真实业务TLS及握手完成后才ready
    */
    int32_t connectServiceGateway(NetWorkSenderState* client, uint32_t product, const NetSenderGatewayOptions& options, NetSessionListener* listener);

    /** 首次连接前配置TLS待发帧物理池，须先prepareGateway
    线程：仅唯一生命周期owner，不得与本客户端其他调用并发
    @param [in] client 本提供方静止客户端
    @param [in] config 基础池初始容量、上限和增长策略，复制配置
    @return NI_OK或状态/参数/资源错误，配置复制失败保留原配置；池资源在连接创建时申请和验证
    */
    int32_t configureConnectionPools(NetWorkSenderState* client, const NetConnectionPoolConfig& config);

    /** 首次连接前配置应答等待记录的物理池，须先prepareGateway
    线程：仅唯一生命周期owner，不得与本客户端其他调用并发
    @param [in] client 本提供方静止客户端
    @param [in] config 基础池初始容量、上限和增长策略，复制配置
    @return NI_OK或状态/参数/资源错误，失败保留原池
    */
    int32_t configureRequestPool(NetWorkSenderState* client, const LockFreeHashMapPoolConfig& config);

    /** 首次连接前配置提交层物理池，须先prepareGateway
    线程：仅唯一生命周期owner，不得与本客户端其他操作并发；运行后不可更换
    @param [in] client 本提供方创建的静止客户端
    @param [in] pools 实际组件初始容量、增长策略和最大存储，复制配置
    @return NI_OK或参数/资源/状态错误；失败保留原池
    */
    int32_t configureSubmissionPools(NetWorkSenderState* client, const NetSenderSubmissionPools& pools);

    /** 连接前设置公开无秘密RPC压缩候选，默认空；不压缩身份秘密和文件内容
    线程：三种提供方均由唯一生命周期owner串行调用，不与本客户端其他接口并发；禁止在协议回调中调用
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] types count非零时不可空，仅协议许可的公开命令
    @param [in] count 非负项数，0清空
    @return OK或负数，实际压缩仍需协商
    */
    int32_t configureCompression(NetWorkSenderState* client, const uint32_t* types, uint32_t count);

    /** 非阻塞推进或观察；桌面和NetPeer由单一owner推进，NetWork仅观察内部执行者状态。
    线程：桌面和NetPeer仅唯一owner推进，禁止重入及回调中调用；NetWork允许外部线程并发观察，但禁止本客户端内部协议回调线程调用。均不得与释放并发
    NetWork允许并发beginRequest、postRequest、cancelRequest、sendData及关闭；准备、连接、配置和释放仍串行。
    NetPeer和桌面由后续poll推进队列，NetWork唤醒内部执行者；post不要求创建后台线程。
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @return NI_READY本代仍运行（包含连接中），NI_OK已停止，负数为错误
    */
    int32_t pollGateway(NetWorkSenderState* client);

    /** 预留唯一编号，先预留再调用业务SDK构造消息
    仅预留编号及原期限，不占发送顺序；同一客户端同目标／代次按成功send／post的提交先后交付。NetWork并发时每个生产线程自身仍保序，线程间不约定先后；业务预留号与内部线上号分开，回调始终返回原业务号。
    线程：NetWork同一客户端允许多生产者并发并可在回调中调用；桌面src/NetSender和NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] product 非零产品
    @param [in] type 命令
    @param [in] timeoutMs 总期限1至60000
    @param [out] requestId 成功非零，否则0
    @return NI_OK或负数
    */
    int32_t beginRequest(NetWorkSenderState* client, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& requestId);

    /** 投递预留请求到本进程有界发送队列，只能接纳一次
    提交前登记请求上下文；NetWork的内部执行者可能在调用返回前交付回调。
    线程：NetWork同一客户端允许多生产者并发并可在回调中调用；桌面src/NetSender和NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] requestId 当前预留号
    @param [in] bytes 调用期间借用；接纳后由库保留副本
    @param [in] size 最大1048560
    @return OK表示已复制并入队，不表示调用底层网络接口或远端收到；LIMIT_EXCEEDED未接纳可在原期限内重试
    */
    int32_t postRequest(NetWorkSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size);

    /** 取消请求并通过listener报告唯一终态
    线程：NetWork同一客户端允许多生产者并发并可在回调中调用；桌面src/NetSender和NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] requestId 当前号
    @return OK、NOT_FOUND或负数
    */
    int32_t cancelRequest(NetWorkSenderState* client, uint64_t requestId);

    /** 投递上层授权的数据流帧到本进程有界发送队列
    线程：NetWork同一客户端允许多生产者并发并可在回调中调用；桌面src/NetSender和NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] frame DATA／DATA_ACK或文件CANCEL，产品／流号非零
    @return OK表示帧及载荷已复制并入队，不表示网络发送；背压未接纳，可重试
    */
    int32_t postData(NetWorkSenderState* client, const NetInterfaceFrame& frame);

    /** 请求正常关闭；桌面和NetPeer继续poll收敛，NetWork由内部执行者收敛
    线程：NetWork同一客户端允许多生产者并发并可在回调中调用；桌面src/NetSender和NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] reason 默认0
    */
    void closeGateway(NetWorkSenderState* client, int32_t reason = 0);

    /** 请求关闭，不在调用方等待；继续poll／观察至停止，可重连
    线程：NetWork同一客户端允许多生产者并发并可在回调中调用；桌面src/NetSender和NetPeer仅同一执行者串行调用。均不得与配置／连接／释放并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    */
    void uninitGateway(NetWorkSenderState* client);

    /** 由非GUI生命期owner释放完整上下文，仅在实际停止、生产者退出及回调收敛之后调用
    线程：三种提供方均由唯一生命周期owner串行调用，不与本客户端其他接口并发；禁止在协议回调中调用
    回调/poll或异步资源未收敛时保留对象，继续推进后重试；成功后须重新createClient再prepare。
    @param [in,out] client 本提供方创建的客户端或空；成功销毁置空，失败保留
    @return NI_OK已释放或原本为空；负数表示未释放，调用方仍须持有对象
    */
    int32_t destroyClient(NetWorkSenderState*& client);

    /** 真实握手已通过且未关闭
    线程：NetWork支持同一客户端并发查询；桌面src/NetSender和NetPeer仅同一执行者串行查询。可在回调中查询，不得与生命周期变更并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @return 是否可提交
    */
    bool gatewayReady(NetWorkSenderState* client) const;

    /** 当前连接代次
    线程：NetWork支持同一客户端并发查询；桌面src/NetSender和NetPeer仅同一执行者串行查询。可在回调中查询，不得与生命周期变更并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @return 未建立为0，之后递增
    */
    uint64_t gatewayGeneration(NetWorkSenderState* client) const;

    /** 最近有返回码操作的诊断；状态查询、成功poll及关闭不清除它。多生产者以各次函数直接返回值为准
    线程：NetWork支持同一客户端并发查询；桌面src/NetSender和NetPeer仅同一执行者串行查询。可在回调中查询，不得与生命周期变更并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @return NetInterface结果
    */
    int32_t lastGatewayError(NetWorkSenderState* client) const;

    /** 提交预留请求，不等待远端回应；NetWork接纳返回，桌面及NetPeer等待本层交付
    线程：NetWork允许外部生产者并发提交，调用时复制正文；桌面和NetPeer仅同一执行者。均禁止在本对象协议回调／poll中调用
    提交前登记请求上下文；原生单执行者send内部推进时可交付回调，不能假定回调晚于函数返回。
    桌面完成边界为ProcessWork共享内存，NetPeer为网络宿主接纳；NetWork为本会话队列接纳，由选定工作者异步组包加密。
    NetWork须在连接前选择parallel=true的send执行者；同一客户端不得混用send/post。需要交付确认时显式使用beginSendRequest/sendResult。
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] requestId 当前未投递的预留号
    @param [in] bytes 调用时复制，size非零时不可空
    @param [in] size 最大1048560字节
    @return NI_OK表示本提供方接纳边界已完成，不保证远端收到；未接纳的LIMIT可重试，接纳后禁止自动重放
    */
    int32_t sendRequest(NetWorkSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size);

    /** 提交已授权的流帧，责任边界与sendRequest相同；NetWork不等待后台加密或网络交付
    线程：NetWork允许外部生产者并发提交，调用时复制正文；桌面和NetPeer仅同一执行者。均禁止在本对象协议回调／poll中调用
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] frame DATA／ACK／文件CANCEL，调用时复制载荷
    @return NI_OK接纳，负数失败；接纳后的异步失败不代表远端未收到，不能盲目重复流块
    */
    int32_t sendData(NetWorkSenderState* client, const NetInterfaceFrame& frame);

    /** 创建独立客户端；不创建线程
    线程：由本句柄唯一生命周期owner串行调用，不能与提交或回调并发
    @param [in] callbacks v1完整回调表，四个函数均不可空
    @param [out] handle 不可空，失败清零
    @return NI_OK或负数
    */
    int32_t clientCreate(const NetWorkClientCallbacks* callbacks, NetWorkClientHandle* handle);

    /** 收敛回调并释放；context须仍存活
    线程：由本句柄唯一生命周期owner串行调用，不能与提交或回调并发
    @param [in] handle 有效句柄或null；禁止重复销毁
    @return OK；回调/poll或异步资源尚未收敛时INVALID_STATE且句柄仍有效，继续poll后重试
    */
    int32_t clientDestroy(NetWorkClientHandle handle);

    /** 首次连接前配置TLS待发帧物理池
    @param [in] handle 本提供方静止句柄，不得并发调用
    @param [in] config 配置由核心复制
    @return NI_OK或状态/参数/资源错误
    */
    int32_t clientConfigureConnectionPools(NetWorkClientHandle handle, const NetConnectionPoolConfig& config);

    /** 首次连接前配置协议应答记录的物理池
    @param [in] handle 本提供方静止句柄，不得并发调用
    @param [in] config 配置由核心复制
    @return NI_OK或状态/参数/资源错误
    */
    int32_t clientConfigureRequestPool(NetWorkClientHandle handle, const LockFreeHashMapPoolConfig& config);

    /** 静止客户端首次连接前配置实际提交池
    @param [in] handle 本提供方句柄，无并发调用
    @param [in] pools 配置复制；函数及上下文按基础池契约持有
    @return NI_OK或状态/参数/资源错误
    */
    int32_t clientConfigureSubmissionPools(NetWorkClientHandle handle, const NetSenderSubmissionPools& pools);

    /** 连接前配置公开RPC压缩白名单；不改变v1结构，不配置敏感内容或文件类型
    线程：由本句柄唯一生命周期owner串行调用，不能与提交或回调并发
    @param [in] handle 静止的独立客户端
    @param [in] types count非零时不可空，调用时借用
    @param [in] count 非负项数，0清空
    @return NI_OK或错误；实际压缩仍须握手协商
    */
    int32_t clientConfigureCompression(NetWorkClientHandle handle, const uint32_t* types, uint32_t count);

    /** 启动前配置v3备用入口及公开缓存，不改变原v1结构布局
    线程：由本句柄唯一生命周期owner串行调用，不能与提交或回调并发
    @param [in] handle 独立客户端；多个服务用独立句柄
    @param [in] seeds 可空NUL结尾IP:port,[IPv6]:port列表，最大4096字节；空用连接默认地址
    @param [in] cachePath 可空UTF8路径，空为内存缓存
    @return NI_OK或错误；在途、poll及回调期间拒绝
    */
    int32_t clientConfigureGateways(NetWorkClientHandle handle, const char* seeds, const char* cachePath);

    /** 启动或重连，证书/地址调用时借用，随机context仍借用至关闭
    线程：由本句柄唯一生命周期owner串行调用，不能与提交或回调并发
    @param [in] handle 独立客户端
    @param [in] options v1配置
    未启用 pthreads 的 Wasm 返回 NI_NOT_SUPPORTED。
    @return 与NetSession.connect相同
    */
    int32_t clientConnect(NetWorkClientHandle handle, const NetWorkClientOptions* options);

    /** NTW3网关逻辑服务连接，固定内置CA和目标服务身份，复用原请求/文件接口
    线程：由本句柄唯一生命周期owner串行调用，不能与提交或回调并发
    @param [in] handle 独立客户端，必须已停止上一代
    @param [in] product 目标逻辑服务号；外部客户端不能直达内部服务0
    @param [in] options 原v1布局的地址/设备/能力/强随机/期限；证书和域名由库内部决定
    未启用 pthreads 的 Wasm 返回 NI_NOT_SUPPORTED。
    @return NI_OK已接纳，真实业务TLS/握手就绪由ready回调报告
    */
    int32_t clientConnectService(NetWorkClientHandle handle, uint32_t product, const NetWorkClientOptions* options);

    /** 非阻塞观察内部执行者是否已收敛；不驱动Peer，不转移回调线程
    线程：外部线程可并发观察；内部协议回调线程调用被拒绝，不推进Peer
    @param [in] handle 客户端；禁止从本句柄回调重入
    @return READY仍有资源，OK已停止，负数调用错误
    */
    int32_t clientPoll(NetWorkClientHandle handle);

    /** 预留唯一请求编号，再由业务库构造body
    线程：同句柄允许多生产者并发，不得与销毁／配置／连接并发
    @param [in] handle 客户端
    @param [in] product 非零产品
    @param [in] type 命令
    @param [in] timeoutMs 总期限1至60000
    @param [out] requestId 不可空，失败清零
    @return 与NetSession.beginRequest相同
    */
    int32_t clientBegin(NetWorkClientHandle handle, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t* requestId);

    /** 提交预留请求；LIMIT_EXCEEDED尚未接纳，可在原期限内重试
    线程：同句柄允许多生产者并发，不得与销毁／配置／连接并发
    @param [in] handle 客户端
    @param [in] requestId 未发送的预留号
    @param [in] bytes size非零时不可空
    @param [in] size 最大1048560
    @return 与NetSession.sendRequest相同
    */
    int32_t clientPost(NetWorkClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size);

    /** 取消并给出唯一请求终态
    线程：同句柄允许多生产者并发，不得与销毁／配置／连接并发
    @param [in] handle 客户端
    @param [in] requestId 请求号
    @return OK、NOT_FOUND或负数
    */
    int32_t clientCancel(NetWorkClientHandle handle, uint64_t requestId);

    /** 发送已授权的有界数据帧
    线程：同句柄允许多生产者并发，不得与销毁／配置／连接并发
    @param [in] handle 客户端
    @param [in] frame DATA/DATA_ACK，不可空
    @return 与NetSession.sendData相同
    */
    int32_t clientPostData(NetWorkClientHandle handle, const NetInterfaceFrame* frame);

    /** 标记关闭，之后由poll/销毁收敛；回调内允许
    线程：同句柄允许多生产者并发，不得与销毁／配置／连接并发
    @param [in] handle 客户端
    @param [in] reason 关闭原因
    @return OK或INVALID_ARGUMENT
    */
    int32_t clientClose(NetWorkClientHandle handle, int32_t reason);

    /** 读取状态；不是联网/业务授权证明
    线程：同句柄允许多生产者并发，不得与销毁／配置／连接并发
    @param [in] handle 客户端
    @param [out] generation 不可空，最近连接代次
    @param [out] ready 不可空，1允许业务，否则0
    @return OK或负数，失败输出清零
    */
    int32_t clientState(NetWorkClientHandle handle, uint64_t* generation, int32_t* ready);

    /** 并发接纳请求副本到会话队列，不等后台组包加密或远端响应
    线程：同句柄允许多生产者并发，不得与销毁／配置／连接并发；禁止协议回调内等待
    @param [in] handle 本提供方有效句柄
    @param [in] requestId 未投递预留号
    @param [in] bytes 调用时复制，size非零时不可空
    @param [in] size 最大1048560字节
    @return NI_OK已入队，负数未接纳；回调内拒绝；接纳后的异步失败禁止自动重放
    */
    int32_t clientSend(NetWorkClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size);

    /** 并发接纳流帧副本，责任边界同clientSend
    线程：同句柄允许多生产者并发，不得与销毁／配置／连接并发；禁止协议回调内等待
    @param [in] handle 本提供方有效句柄
    @param [in] frame 调用时复制的授权流帧
    @return NI_OK已入队或负数未接纳，不代表网络库接纳或远端收到
    */
    int32_t clientSendData(NetWorkClientHandle handle, const NetInterfaceFrame* frame);

    /** 在创建连接之前一次性登记借用的网络宿主；调用方负责宿主上下文寿命
    线程：唯一初始化执行者串行调用，不与已有连接并发更换
    @param [in] functions 完整v1回调表；复制表项，context仍由宿主拥有
    @return NI_OK或参数／生命期错误，沿用底层宿主登记契约
    */
    static int32_t configureHost(const NetHostFunctions& functions);

    /** 配置原生跨线程宿主桥的实际存储池；生命周期owner在创建任何宿主实例前串行调用
    @param [in] pools 正文、连接、记录、索引及跨线程队列的底层池配置
    @return NI_OK或明确错误；失败不替换已有配置
    */
    static int32_t configureHostPools(const NetWorkHostPools& pools);

    /** 查询本模块的实际指针字节数，不访问会话或宿主
    @return 4或8
    */
    static uint32_t pointerBytes();

    /** 使用已配置宿主的密码学随机，不建立连接或回退为弱随机
    @param [out] output size非零时不可空；有效范围在失败时清零
    @param [in] size 0至8192；非法范围不访问输出
    @return NI_OK、NI_INVALID_ARGUMENT或NI_INVALID_STATE
    */
    static int32_t randomBytes(uint8_t* output, uint32_t size);

    /** 启动前登记原生文件后端；保留已由宿主显式配置的工厂
    @return 原生NI_OK或错误；Wasm为NI_NOT_SUPPORTED，文件能力须另配宿主。不能与执行者启动并发
    */
    static int32_t configureNativeFileIo();

    /** 创建任何原生文件后端前配置其实际存储池；生命周期owner串行调用
    @param [in] pools 输入、结果、记录、句柄及交接队列配置
    @return NI_OK或状态、配置、内存错误；Wasm宿主文件后端为NI_NOT_SUPPORTED
    */
    static int32_t configureFilePools(const NetWorkFilePools& pools);

    /** 一次读取同一客户端的代次及就绪快照，不驱动协议
    线程：NetWork允许生产者查询；其他提供方由唯一执行者调用；均不得与销毁并发
    @param [in] client 本提供方有效客户端
    @param [out] generation 失败清零
    @param [out] ready 失败为false
    @return NI_OK或调用错误
    */
    int32_t gatewayState(NetWorkSenderState* client, uint64_t& generation, bool& ready);

    /** 非阻塞建立请求交付票据；仅sendResult返回NI_OK后才完成send责任交接
    线程：NetWork允许生产者调用；其他提供方唯一执行者；禁止本客户端协议回调内调用
    @param [in] client 本提供方有效客户端
    @param [in] requestId 当前预留请求号
    @param [in] bytes size非零时不可空，接纳时复制
    @param [in] size 正文字节数，受请求额度限制
    @param [out] ticket 失败清零；接纳后须推进并读取最终结果，不得丢弃票据
    @return NI_OK只表示票据已接纳；负数失败，不表示已交付网络
    */
    int32_t beginSendRequest(NetWorkSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t& ticket);

    /** 非阻塞建立流帧交付票据，sendResult为NI_OK时才表示网络宿主或共享内存交付完成
    线程：NetWork允许生产者调用；其他提供方唯一执行者；禁止本客户端协议回调内调用
    @param [in] client 本提供方有效客户端
    @param [in] frame 接纳时复制的帧与正文
    @param [out] ticket 失败清零；成功后必须查询至终态
    @return NI_OK只表示票据接纳，完成需查询sendResult
    */
    int32_t beginSendData(NetWorkSenderState* client, const NetInterfaceFrame& frame, uint64_t& ticket);

    /** 查询并消费交付票据终态，不等待业务应答
    线程：NetWork允许生产者查询；其他提供方唯一执行者；不得与销毁并发
    @param [in] client 创建票据的客户端
    @param [in] ticket 未消费的有效票据，同一票据只有一个查询消费者
    @return NI_READY待交付，NI_OK已交付，负数终态；终态读取后不可重复查询
    */
    int32_t sendResult(NetWorkSenderState* client, uint64_t ticket);

    /** 配置备用网关及公开目录缓存，不创建连接
    线程：唯一生命周期owner在已prepare且静止时调用，不与配置、连接、释放并发；回调内拒绝
    @param [in] client 本提供方创建且已准备的客户端
    @param [in] seeds 网关列表，可空清除；调用时复制，最多4096字节
    @param [in] cachePath 公开目录缓存路径，可空禁用；调用时复制，最多4096字节
    @return NI_OK或参数、状态错误；失败不启动网络
    */
    int32_t configureGateways(NetWorkSenderState* client, const char* seeds, const char* cachePath);

    /** 取得无状态文件协议辅助入口，不创建连接、文件或线程
    线程：首次取得在启动工作线程之前；不同流可独立调用，同一流由唯一执行者串行处理
    @return 本模块长期有效的借用入口，不得delete；流由其create/destroy配对管理
    */
    NetFileProtocol& fileProtocol();

private:
    /** 只允许instance创建管理入口
    */
    NetWork();

    /** 入口不代替调用方关闭仍然存活的会话
    */
    ~NetWork();

    /** 禁止复制管理入口
    @param [in] other 不可复制实例
    */
    NetWork(const NetWork& other);

    /** 禁止赋值管理入口
    @param [in] other 不可复制实例
    @return 不可调用
    */
    NetWork& operator=(const NetWork& other);

};

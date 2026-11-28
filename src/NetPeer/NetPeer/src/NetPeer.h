#pragma once
#include "NetPeerBackend.h"
#include "NetPeerPlatform.h"
#include "NetPeerTypes.h"
#include "NetHostSocket.h"
#include "NetPeerClientBackend.h"
#include "NetTransport.h"
#include "NetRelayTransport.h"
#include "NetLinkTransport.h"
#include "NetGatewayTransport.h"
#include "NetGatewayClientChannel.h"
#include "NetGatewayDirectoryStore.h"
#include "NetGatewayDiscovery.h"
#include "NetEndpointRouter.h"
#include "NetPeerEndpointRouter.h"
#include "NetSession.h"
#include "NetClientSession.h"
#include "NetPeerClientSession.h"
#include "NetFileIo.h"
#include "NetFileIoBackend.h"
#include "NetFileTransfer.h"

class NetPeerSenderState;

/** 本库主入口，登记宿主并创建独立发送器；不拥有全局业务会话或增加调度层
*/
class NetPeerAPI NetPeer
{
public:
    /** 统一初始化接口；本提供方由调用者驱动，不创建工作线程
    @param [in] count 单执行者取 1
    @return 1为NI_OK，0为参数错误，其他为NI_NOT_SUPPORTED
    */
    static int32_t configureWorkers(uint32_t count);

    /** 统一连接前配置入口；本提供方仍由同一调用者推进，不改变send/post的交付边界
    @param [in] client 本提供方已prepare且未连接的有效客户端
    @param [in] parallel 兼容参数，true 和 false 均由所属执行者推进
    @return NI_OK或参数/生命周期错误
    */
    int32_t configureDispatch(NetPeerSenderState* client, bool parallel);

    /** 取得本提供方管理入口；不创建会话或线程
    @return 本库唯一管理实例；各客户端上下文仍独立
    */
    static NetPeer& instance();

    /** 取得本模块无状态的专属配置入口；启动前使用
    @return 长期借用的配置对象，不得delete
    */
    static NetPeerBackend& backend();

    /** 创建独立的静止客户端，不启动网络或线程
    线程：唯一生命周期owner创建；首次取得管理入口在启动生产者前完成
    @param [in,out] client 必须为空，成功转交对象；失败不覆盖非空输入
    @return NI_OK、NI_INVALID_ARGUMENT或NI_NO_MEMORY；随后prepareGateway及连接
    */
    int32_t createClient(NetPeerSenderState*& client);

    /** 准备独立客户端的协议会话与提交队列
    线程：同一提供方的所有客户端共用一个生命周期执行者，由其串行操作，协议回调内禁止调用
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @return NI_OK或负数；可重复调用
    */
    int32_t prepareGateway(NetPeerSenderState* client);

    /** 按预设参数首次准备客户端，失败不发布半初始化对象
    @param [in] client 已创建且尚未准备的客户端
    @param [in] config 三条运输路径各自的配置及可选初始化函数。回调期间再次准备或销毁同一客户端返回 NI_INVALID_STATE
    @return NI_OK或配置、内存、生命周期错误
    */
    int32_t prepareGateway(NetPeerSenderState* client, const NetPeerConfig& config);

    /** 通过已配置的字节宿主连接远端 TLS 网关
    线程：同一提供方的所有客户端共用一个生命周期执行者，由其串行操作，协议回调内禁止调用
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] options v1客户端配置；nonce由核心强随机生成
    @param [in] listener 借用至停止，回调由所属执行者推进协议时交付，回调中禁止销毁客户端或同步等待
    @return OK接纳，真实就绪由回调；不降级旧协议
    */
    int32_t connectGateway(NetPeerSenderState* client, const NetSenderGatewayOptions& options, NetSessionListener* listener);

    /** 通过原始字节宿主连接网关，为目标逻辑服务建立端到端 TLS 连接
    线程：同一提供方的所有客户端共用一个生命周期执行者，由其串行操作，协议回调内禁止调用
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] product 目标服务号，每对象本代次固定一个目标
    @param [in] options 原v1配置布局，地址/能力/设备/强随机/期限有效，信任根/服务身份内置
    @param [in] listener 借用至停止；请求/取消/文件接口与原方法一致
    @return NI_OK接纳；真实业务TLS及握手完成后才ready
    */
    int32_t connectServiceGateway(NetPeerSenderState* client, uint32_t product, const NetSenderGatewayOptions& options, NetSessionListener* listener);

    /** 首次连接前配置TLS待发帧物理池，须先prepareGateway
    线程：仅唯一生命周期owner，不得与本客户端其他调用并发
    @param [in] client 本提供方静止客户端
    @param [in] config 基础池初始容量、上限和增长策略，复制配置
    @return NI_OK或状态/参数/资源错误，配置复制失败保留原配置；池资源在连接创建时申请和验证
    */
    int32_t configureConnectionPools(NetPeerSenderState* client, const NetConnectionPoolConfig& config);

    /** 首次连接前配置应答等待记录的物理池，须先prepareGateway
    线程：仅唯一生命周期owner，不得与本客户端其他调用并发
    @param [in] client 本提供方静止客户端
    @param [in] config 基础池初始容量、上限和增长策略，复制配置
    @return NI_OK或状态/参数/资源错误，失败保留原池
    */
    int32_t configureRequestPool(NetPeerSenderState* client, const LockFreeHashMapPoolConfig& config);

    /** 首次连接前配置提交层物理池，须先prepareGateway
    线程：仅唯一生命周期owner，不得与本客户端其他操作并发；运行后不可更换
    @param [in] client 本提供方创建的静止客户端
    @param [in] pools 实际组件初始容量、增长策略和最大存储，复制配置
    @return NI_OK或参数/资源/状态错误；失败保留原池
    */
    int32_t configureSubmissionPools(NetPeerSenderState* client, const NetSenderSubmissionPools& pools);

    /** 在首次连接前同时替换该客户端的提交池及调度选项
    @param [in] client 已准备、静止的客户端，由其生命周期执行者调用
    @param [in] pools 独立的提交记录、正文和入口队列池配置
    @param [in] options 复制索引、批次、维护和交付参数，策略借用的对象须在使用期间存活
    @return NI_OK 或参数、状态、资源错误；失败保留原配置
    */
    int32_t configureSubmissionPools(NetPeerSenderState* client, const NetSenderSubmissionPools& pools,
        const NetSenderSubmissionOptions& options);

    /** 连接前设置公开无秘密RPC压缩候选，默认空；不压缩身份秘密和文件内容
    线程：同一提供方的所有客户端共用一个生命周期执行者，由其串行操作，协议回调内禁止调用
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] types count非零时不可空，仅协议许可的公开命令
    @param [in] count 非负项数，0清空
    @return OK或负数，实际压缩仍需协商
    */
    int32_t configureCompression(NetPeerSenderState* client, const uint32_t* types, uint32_t count);

    /** 预留唯一编号，先预留再调用业务SDK构造消息
    预留编号和期限后，同一客户端、目标和代次按成功 send 或 post 的提交顺序发送，回调使用原业务编号
    线程：由所属执行者串行调用，协议回调中也可调用，不得与配置、连接或销毁并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] product 非零产品
    @param [in] type 命令
    @param [in] timeoutMs 总期限1至60000
    @param [out] requestId 成功非零，否则0
    @return NI_OK或负数
    */
    int32_t beginRequest(NetPeerSenderState* client, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& requestId);

    /** 投递预留请求到本进程有界发送队列，只能接纳一次
    接纳后由所属执行者继续 poll，业务应答由结果回调报告
    线程：由所属执行者串行调用，协议回调中也可调用，不得与配置、连接或销毁并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] requestId 当前预留号
    @param [in] bytes 调用期间借用；接纳后由库保留副本
    @param [in] size 最大1048560
    @return OK表示已复制并入队，不表示调用底层网络接口或远端收到；LIMIT_EXCEEDED未接纳可在原期限内重试
    */
    int32_t postRequest(NetPeerSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size);

    /** 取消请求并通过listener报告唯一终态
    线程：由所属执行者串行调用，协议回调中也可调用，不得与配置、连接或销毁并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] requestId 当前号
    @return OK、NOT_FOUND或负数
    */
    int32_t cancelRequest(NetPeerSenderState* client, uint64_t requestId);

    /** 投递上层授权的数据流帧到本进程有界发送队列
    线程：由所属执行者串行调用，协议回调中也可调用，不得与配置、连接或销毁并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] frame DATA／DATA_ACK或文件CANCEL，产品／流号非零
    @return OK表示帧及载荷已复制并入队，不表示网络发送；背压未接纳，可重试
    */
    int32_t postData(NetPeerSenderState* client, const NetInterfaceFrame& frame);

    /** 同步提交预留请求，推进至网络宿主接纳全部字节或发生错误
    线程：由所属执行者串行调用，禁止在本客户端协议回调或 poll 中调用
    同步推进期间可能交付回调，调用前须登记业务上下文
    等待计入原请求总期限，交付票据池用尽时返回背压
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] requestId 当前未投递的预留号
    @param [in] bytes 调用时复制，size非零时不可空
    @param [in] size 最大1048560字节
    @return NI_OK 表示网络宿主已接纳，业务应答由回调报告。仅明确未接纳的背压可重试
    */
    int32_t sendRequest(NetPeerSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size);

    /** 同步交付已授权的流帧，责任边界与sendRequest相同，使用本客户端提交选项的交付期限，默认5000毫秒
    线程：由所属执行者串行调用，禁止在本客户端协议回调或 poll 中调用
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] frame DATA／ACK／文件CANCEL，调用时复制载荷
    @return NI_OK 表示网络宿主已接纳，负数表示失败。已接纳帧不应自动重发
    */
    int32_t sendData(NetPeerSenderState* client, const NetInterfaceFrame& frame);

    /** 非阻塞推进当前客户端的队列、网络收发和回调
    线程：仅所属执行者推进，禁止重入及在协议回调中调用，推进期间保持客户端有效
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @return NI_READY本代仍运行（包含连接中），NI_OK已停止，负数为错误
    */
    int32_t pollGateway(NetPeerSenderState* client);

    /** 请求关闭当前代次，随后由所属执行者继续 poll 至停止
    线程：由所属执行者串行调用，协议回调中也可调用，不得与配置、连接或销毁并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] reason 默认0
    */
    void closeGateway(NetPeerSenderState* client, int32_t reason = 0);

    /** 请求关闭，不在调用方等待；继续poll／观察至停止，可重连
    线程：由所属执行者串行调用，协议回调中也可调用，不得与配置、连接或销毁并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    */
    void uninitGateway(NetPeerSenderState* client);

    /** 由非GUI生命期owner释放完整上下文，仅在实际停止、生产者退出及回调收敛之后调用
    线程：同一提供方的所有客户端共用一个生命周期执行者，由其串行操作，协议回调内禁止调用
    回调/poll或异步资源未收敛时保留对象，继续推进后重试；成功后须重新createClient再prepare。
    @param [in,out] client 本提供方创建的客户端或空；成功销毁置空，失败保留
    @return NI_OK已释放或原本为空；负数表示未释放，调用方仍须持有对象
    */
    int32_t destroyClient(NetPeerSenderState*& client);

    /** 真实握手已通过且未关闭
    线程：由所属执行者串行查询，协议回调中也可查询，查询期间保持客户端有效
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @return 是否可提交
    */
    bool gatewayReady(NetPeerSenderState* client) const;

    /** 当前连接代次
    线程：由所属执行者串行查询，协议回调中也可查询，查询期间保持客户端有效
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @return 未建立为0，之后递增
    */
    uint64_t gatewayGeneration(NetPeerSenderState* client) const;

    /** 最近记录的准备、配置、连接或业务提交诊断；状态查询、正常 poll、关闭和交付票据操作保留该值。每次调用以直接返回码为准
    线程：由所属执行者串行查询，协议回调中也可查询，查询期间保持客户端有效
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @return NetInterface结果
    */
    int32_t lastGatewayError(NetPeerSenderState* client) const;

    /** 创建独立客户端；不创建线程
    线程：由本句柄唯一生命周期owner串行调用，不能与提交或回调并发
    @param [in] callbacks v1完整回调表，四个函数均不可空
    @param [out] handle 不可空，失败清零
    @return NI_OK或负数
    */
    int32_t clientCreate(const NetPeerClientCallbacks* callbacks, NetPeerClientHandle* handle);

    /** 使用独立运输配置创建句柄，成功后才发布句柄
    @param [in] callbacks 已填写的v1回调表
    @param [out] handle 成功时取得新对象，失败清零
    @param [in] config 复制运输设置，同步应用会话和派发器配置
    @return NI_OK或具体配置错误
    */
    int32_t clientCreate(const NetPeerClientCallbacks* callbacks, NetPeerClientHandle* handle, const NetPeerConfig& config);

    /** 收敛回调并释放；context须仍存活
    线程：由本句柄唯一生命周期owner串行调用，不能与提交或回调并发
    @param [in] handle 有效句柄或null；禁止重复销毁
    @return OK；回调/poll或异步资源尚未收敛时INVALID_STATE且句柄仍有效，继续poll后重试
    */
    int32_t clientDestroy(NetPeerClientHandle handle);

    /** 首次连接前配置TLS待发帧物理池
    @param [in] handle 本提供方静止句柄，不得并发调用
    @param [in] config 配置由核心复制
    @return NI_OK或状态/参数/资源错误
    */
    int32_t clientConfigureConnectionPools(NetPeerClientHandle handle, const NetConnectionPoolConfig& config);

    /** 首次连接前配置协议应答记录的物理池
    @param [in] handle 本提供方静止句柄，不得并发调用
    @param [in] config 配置由核心复制
    @return NI_OK或状态/参数/资源错误
    */
    int32_t clientConfigureRequestPool(NetPeerClientHandle handle, const LockFreeHashMapPoolConfig& config);

    /** 静止客户端首次连接前配置实际提交池
    @param [in] handle 本提供方句柄，无并发调用
    @param [in] pools 配置复制；函数及上下文按基础池契约持有
    @return NI_OK或状态/参数/资源错误
    */
    int32_t clientConfigureSubmissionPools(NetPeerClientHandle handle, const NetSenderSubmissionPools& pools);

    /** 在首次连接前同时替换该客户端的提交池及调度选项
    @param [in] handle 已准备、静止的客户端，由其生命周期执行者调用
    @param [in] pools 独立的提交记录、正文和入口队列池配置
    @param [in] options 复制索引、批次、维护和交付参数，策略借用的对象须在使用期间存活
    @return NI_OK 或参数、状态、资源错误；失败保留原配置
    */
    int32_t clientConfigureSubmissionPools(NetPeerClientHandle handle, const NetSenderSubmissionPools& pools,
        const NetSenderSubmissionOptions& options);

    /** 连接前配置公开RPC压缩白名单；不改变v1结构，不配置敏感内容或文件类型
    线程：由本句柄唯一生命周期owner串行调用，不能与提交或回调并发
    @param [in] handle 静止的独立客户端
    @param [in] types count非零时不可空，调用时借用
    @param [in] count 非负项数，0清空
    @return NI_OK或错误；实际压缩仍须握手协商
    */
    int32_t clientConfigureCompression(NetPeerClientHandle handle, const uint32_t* types, uint32_t count);

    /** 启动或重连，证书/地址调用时借用，随机context仍借用至关闭
    线程：由本句柄唯一生命周期owner串行调用，不能与提交或回调并发
    @param [in] handle 独立客户端
    @param [in] options v1配置
    @return NI_OK 表示接纳连接，后续由 ready 回调报告就绪，负数表示失败
    */
    int32_t clientConnect(NetPeerClientHandle handle, const NetPeerClientOptions* options);

    /** NTW3网关逻辑服务连接，固定内置CA和目标服务身份，复用原请求/文件接口
    线程：由本句柄唯一生命周期owner串行调用，不能与提交或回调并发
    @param [in] handle 独立客户端，必须已停止上一代
    @param [in] product 目标逻辑服务号；外部客户端不能直达内部服务0
    @param [in] options 原v1布局的地址/设备/能力/强随机/期限；证书和域名由库内部决定
    @return NI_OK已接纳，真实业务TLS/握手就绪由ready回调报告
    */
    int32_t clientConnectService(NetPeerClientHandle handle, uint32_t product, const NetPeerClientOptions* options);

    /** 启动前配置v3备用入口及公开缓存，不改变原v1结构布局
    线程：由本句柄唯一生命周期owner串行调用，不能与提交或回调并发
    @param [in] handle 独立客户端；多个服务用独立句柄
    @param [in] seeds 可空NUL结尾IP:port,[IPv6]:port列表，最大4096字节；空用连接默认地址
    @param [in] cachePath 可空UTF8路径，空为内存缓存
    @return NI_OK或错误；在途、poll及回调期间拒绝
    */
    int32_t clientConfigureGateways(NetPeerClientHandle handle, const char* seeds, const char* cachePath);

    /** 同一执行者非阻塞驱动
    线程：NetPeer仅唯一执行者推进，禁止poll重入或协议回调中调用
    @param [in] handle 客户端
    @return READY仍有资源，OK已停止，负数调用错误
    */
    int32_t clientPoll(NetPeerClientHandle handle);

    /** 预留唯一请求编号，再由业务库构造body
    线程：本句柄仅由一个执行者串行使用，不与销毁／配置／连接并发
    @param [in] handle 客户端
    @param [in] product 非零产品
    @param [in] type 命令
    @param [in] timeoutMs 总期限1至60000
    @param [out] requestId 不可空，失败清零
    @return NI_OK 表示已预留编号，负数表示失败
    */
    int32_t clientBegin(NetPeerClientHandle handle, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t* requestId);

    /** 提交预留请求；LIMIT_EXCEEDED尚未接纳，可在原期限内重试
    线程：本句柄仅由一个执行者串行使用，不与销毁／配置／连接并发
    @param [in] handle 客户端
    @param [in] requestId 未发送的预留号
    @param [in] bytes size非零时不可空
    @param [in] size 最大1048560
    @return NI_OK 表示正文已复制入队，后续由 poll 推进网络交付，负数表示失败
    */
    int32_t clientPost(NetPeerClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size);

    /** 取消并给出唯一请求终态
    线程：本句柄仅由一个执行者串行使用，不与销毁／配置／连接并发
    @param [in] handle 客户端
    @param [in] requestId 请求号
    @return OK、NOT_FOUND或负数
    */
    int32_t clientCancel(NetPeerClientHandle handle, uint64_t requestId);

    /** 将已授权的流帧投递到本客户端的提交队列
    线程：本句柄仅由一个执行者串行使用，不与销毁／配置／连接并发
    @param [in] handle 客户端
    @param [in] frame DATA、DATA_ACK 或文件 CANCEL 帧，不可空，接纳时复制帧与载荷
    @return NI_OK 表示已复制入队，后续由 poll 推进网络交付，负数表示失败
    */
    int32_t clientPostData(NetPeerClientHandle handle, const NetInterfaceFrame* frame);

    /** 标记关闭，之后由poll/销毁收敛；回调内允许
    线程：本句柄仅由一个执行者串行使用，不与销毁／配置／连接并发
    @param [in] handle 客户端
    @param [in] reason 关闭原因
    @return OK或INVALID_ARGUMENT
    */
    int32_t clientClose(NetPeerClientHandle handle, int32_t reason);

    /** 读取状态；不是联网/业务授权证明
    线程：本句柄仅由一个执行者串行使用，不与销毁／配置／连接并发
    @param [in] handle 客户端
    @param [out] generation 不可空，最近连接代次
    @param [out] ready 不可空，1允许业务，否则0
    @return OK或负数，失败输出清零
    */
    int32_t clientState(NetPeerClientHandle handle, uint64_t* generation, int32_t* ready);

    /** 同步交付预留请求至网络宿主，远端应答另由回调报告
    线程：本句柄仅由一个执行者串行使用，不与销毁／配置／连接并发；禁止协议回调内等待
    @param [in] handle 本提供方有效句柄
    @param [in] requestId 未投递预留号
    @param [in] bytes 调用时复制，size非零时不可空
    @param [in] size 最大1048560字节
    @return NI_OK 表示网络宿主已接纳，负数表示失败。回调内调用会被拒绝
    */
    int32_t clientSend(NetPeerClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size);

    /** 同步交付流帧，责任边界同Send，使用本客户端提交选项的交付期限，默认5000毫秒
    线程：本句柄仅由一个执行者串行使用，不与销毁／配置／连接并发；禁止协议回调内等待
    @param [in] handle 本提供方有效句柄
    @param [in] frame 调用时复制的授权流帧
    @return 0已交付或负数失败，不代表远端收到
    */
    int32_t clientSendData(NetPeerClientHandle handle, const NetInterfaceFrame* frame);

    /** 为JS单线程事件循环接纳请求交付票据；不是业务发送成功
    @param [in] handle 本提供方句柄
    @param [in] requestId 未投递的预留号
    @param [in] bytes 调用时复制
    @param [in] size 字节数
    @param [out] ticket 独立输出失败清零。与正文重叠时返回 NI_INVALID_ARGUMENT，保留输入原值。调用方须推进并读取完成结果
    @return 接纳结果；回调内拒绝
    */
    int32_t clientSendBegin(NetPeerClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t* ticket);

    /** 为JS单线程事件循环接纳流帧交付票据
    @param [in] handle 本提供方句柄
    @param [in] frame 调用时复制
    @param [out] ticket 独立输出失败清零。与帧结构或正文重叠时返回 NI_INVALID_ARGUMENT，保留输入原值。
    @return 接纳结果，不表示网络交付
    */
    int32_t clientSendDataBegin(NetPeerClientHandle handle, const NetInterfaceFrame* frame, uint64_t* ticket);

    /** 读取并消费一次交付完成结果，不推进协议，不等待远端回复
    @param [in] handle 本提供方句柄
    @param [in] ticket 当前票据
    @return READY继续事件循环调度、OK已交付或负数失败
    */
    int32_t clientSendResult(NetPeerClientHandle handle, uint64_t ticket);

    /** 在创建连接之前一次性登记借用的网络宿主；调用方负责宿主上下文寿命
    线程：唯一初始化执行者串行调用，不与已有连接并发更换
    @param [in] functions 完整v1回调表；复制表项，context仍由宿主拥有
    @return NI_OK或参数／生命期错误，沿用底层宿主登记契约
    */
    static int32_t configureHost(const NetHostFunctions& functions);

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

    /** 一次读取同一客户端的代次及就绪快照，不驱动协议
    线程：由所属执行者串行查询，协议回调中也可查询，查询期间保持客户端有效
    @param [in] client 本提供方有效客户端
    @param [out] generation 失败清零
    @param [out] ready 失败为false
    @return NI_OK或调用错误
    */
    int32_t gatewayState(NetPeerSenderState* client, uint64_t& generation, bool& ready);

    /** 非阻塞建立请求交付票据；仅sendResult返回NI_OK后才完成send责任交接
    线程：由所属执行者串行调用，禁止在本客户端协议回调或 poll 中调用
    @param [in] client 本提供方有效客户端
    @param [in] requestId 当前预留请求号
    @param [in] bytes size非零时不可空，接纳时复制
    @param [in] size 正文字节数，受请求额度限制
    @param [out] ticket 独立输出失败清零。与正文重叠时返回 NI_INVALID_ARGUMENT，保留输入原值。接纳后须推进并读取最终结果
    @return NI_OK只表示票据已接纳；负数失败，不表示已交付网络
    */
    int32_t beginSendRequest(NetPeerSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t& ticket);

    /** 非阻塞建立流帧交付票据，完成边界与同步sendData相同
    线程：由所属执行者串行调用，禁止在本客户端协议回调或 poll 中调用
    @param [in] client 本提供方有效客户端
    @param [in] frame 接纳时复制的帧与正文
    @param [out] ticket 独立输出失败清零。与帧结构或正文重叠时返回 NI_INVALID_ARGUMENT，保留输入原值。成功后须查询至终态
    @return NI_OK只表示票据接纳，完成需查询sendResult
    */
    int32_t beginSendData(NetPeerSenderState* client, const NetInterfaceFrame& frame, uint64_t& ticket);

    /** 查询并消费交付票据终态，不等待业务应答
    线程：由所属执行者串行查询，协议回调中也可查询，查询期间保持客户端有效
    @param [in] client 创建票据的客户端
    @param [in] ticket 未消费的有效票据，同一票据只有一个查询消费者
    @return NI_READY待交付，NI_OK已交付，负数终态；终态读取后不可重复查询
    */
    int32_t sendResult(NetPeerSenderState* client, uint64_t ticket);

    /** 配置备用网关及公开目录缓存，不创建连接
    线程：唯一生命周期owner在已prepare且静止时调用，不与配置、连接、释放并发；回调内拒绝
    @param [in] client 本提供方创建且已准备的客户端
    @param [in] seeds 网关列表，可空清除；调用时复制，最多4096字节
    @param [in] cachePath 公开目录缓存路径，可空禁用；调用时复制，最多4096字节
    @return NI_OK或参数、状态错误；失败不启动网络
    */
    int32_t configureGateways(NetPeerSenderState* client, const char* seeds, const char* cachePath);

    /** 取得无状态文件协议辅助入口，不创建连接、文件或线程
    线程：首次取得在启动工作线程之前；不同流可独立调用，同一流由唯一执行者串行处理
    @return 本模块长期有效的借用入口，不得delete；流由其create/destroy配对管理
    */
    NetFileProtocol& fileProtocol();

private:
    /** 只允许instance创建管理入口
    */
    NetPeer();

    /** 入口不代替调用方关闭仍然存活的会话
    */
    ~NetPeer();

    /** 禁止复制管理入口
    @param [in] other 不可复制实例
    */
    NetPeer(const NetPeer& other);

    /** 禁止赋值管理入口
    @param [in] other 不可复制实例
    @return 不可调用
    */
    NetPeer& operator=(const NetPeer& other);

};
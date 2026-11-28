#pragma once
#include "NetPeerMacro.h"
#include "NetPeerBackend.h"
#include "NetInterface/NetInterfaceAPI.h"

class NetPeerSenderState;

// 由本提供方创建和释放的客户端，仅作为不透明借用指针传递
typedef NetPeerSenderState NetSenderClient;

// 启动配置对象随提供方选择，通用业务代码不依赖其专属方法
typedef NetPeerBackend NetSenderBackend;

// 统一无状态发送入口；所有会话状态归后端创建的独立客户端，方法只透传
// 同一提供方的创建、准备、配置、连接及销毁共用一个生命周期执行者；不同client也不能并行执行这些操作
class NetPeerAPI NetPeerNetSender
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
    int32_t configureDispatch(NetSenderClient* client, bool parallel);

    /** 取得无业务成员的单例入口；不创建连接或线程
    @return 本库管理入口，首次取得应在启动生产者前完成
    */
    static NetPeerNetSender& instance();

    /** 创建独立的静止客户端，不启动网络或线程
    线程：唯一生命周期owner创建；首次取得管理入口在启动生产者前完成
    @param [in,out] client 必须为空，成功转交对象；失败不覆盖非空输入
    @return NI_OK、NI_INVALID_ARGUMENT或NI_NO_MEMORY；随后prepareGateway及连接
    */
    int32_t createClient(NetSenderClient*& client);

    /** 准备独立客户端的协议会话与提交队列
    线程：同一提供方的所有客户端共用一个生命周期执行者，由其串行操作，协议回调内禁止调用
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @return NI_OK或负数；可重复调用
    */
    int32_t prepareGateway(NetSenderClient* client);

    /** 通过已配置的字节宿主连接远端 TLS 网关
    线程：同一提供方的所有客户端共用一个生命周期执行者，由其串行操作，协议回调内禁止调用
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] options v1客户端配置；nonce由核心强随机生成
    @param [in] listener 借用至停止，回调由所属执行者推进协议时交付，回调中禁止销毁客户端或同步等待
    @return OK接纳，真实就绪由回调；不降级旧协议
    */
    int32_t connectGateway(NetSenderClient* client, const NetSenderGatewayOptions& options, NetSessionListener* listener);

    /** 通过原始字节宿主连接网关，为目标逻辑服务建立端到端 TLS 连接
    线程：同一提供方的所有客户端共用一个生命周期执行者，由其串行操作，协议回调内禁止调用
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] product 目标服务号，每对象本代次固定一个目标
    @param [in] options 原v1配置布局，地址/能力/设备/强随机/期限有效，信任根/服务身份内置
    @param [in] listener 借用至停止；请求/取消/文件接口与原方法一致
    @return NI_OK接纳；真实业务TLS及握手完成后才ready
    */
    int32_t connectServiceGateway(NetSenderClient* client, uint32_t product, const NetSenderGatewayOptions& options, NetSessionListener* listener);

    /** 首次连接前配置TLS待发帧物理池，须先prepareGateway
    线程：仅唯一生命周期owner，不得与本客户端其他调用并发
    @param [in] client 本提供方静止客户端
    @param [in] config 基础池初始容量、上限和增长策略，复制配置
    @return NI_OK或状态/参数/资源错误，配置复制失败保留原配置；池资源在连接创建时申请和验证
    */
    int32_t configureConnectionPools(NetSenderClient* client, const NetConnectionPoolConfig& config);

    /** 首次连接前配置应答等待记录的物理池，须先prepareGateway
    线程：仅唯一生命周期owner，不得与本客户端其他调用并发
    @param [in] client 本提供方静止客户端
    @param [in] config 基础池初始容量、上限和增长策略，复制配置
    @return NI_OK或状态/参数/资源错误，失败保留原池
    */
    int32_t configureRequestPool(NetSenderClient* client, const LockFreeHashMapPoolConfig& config);

    /** 首次连接前配置提交层物理池，须先prepareGateway
    线程：仅唯一生命周期owner，不得与本客户端其他操作并发；运行后不可更换
    @param [in] client 本提供方创建的静止客户端
    @param [in] pools 实际组件初始容量、增长策略和最大存储，复制配置
    @return NI_OK或参数/资源/状态错误；失败保留原池
    */
    int32_t configureSubmissionPools(NetSenderClient* client, const NetSenderSubmissionPools& pools);

    /** 连接前设置公开无秘密RPC压缩候选，默认空；不压缩身份秘密和文件内容
    线程：同一提供方的所有客户端共用一个生命周期执行者，由其串行操作，协议回调内禁止调用
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] types count非零时不可空，仅协议许可的公开命令
    @param [in] count 非负项数，0清空
    @return OK或负数，实际压缩仍需协商
    */
    int32_t configureCompression(NetSenderClient* client, const uint32_t* types, uint32_t count);

    /** 非阻塞推进当前客户端的队列、网络收发和回调
    线程：仅所属执行者推进，禁止重入及在协议回调中调用，推进期间保持客户端有效
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @return NI_READY本代仍运行（包含连接中），NI_OK已停止，负数为错误
    */
    int32_t pollGateway(NetSenderClient* client);

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
    int32_t beginRequest(NetSenderClient* client, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& requestId);

    /** 投递预留请求到本进程有界发送队列，只能接纳一次
    接纳后由所属执行者继续 poll，业务应答由结果回调报告
    线程：由所属执行者串行调用，协议回调中也可调用，不得与配置、连接或销毁并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] requestId 当前预留号
    @param [in] bytes 调用期间借用；接纳后由库保留副本
    @param [in] size 最大1048560
    @return OK表示已复制并入队，不表示调用底层网络接口或远端收到；LIMIT_EXCEEDED未接纳可在原期限内重试
    */
    int32_t postRequest(NetSenderClient* client, uint64_t requestId, const uint8_t* bytes, uint32_t size);

    /** 取消请求并通过listener报告唯一终态
    线程：由所属执行者串行调用，协议回调中也可调用，不得与配置、连接或销毁并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] requestId 当前号
    @return OK、NOT_FOUND或负数
    */
    int32_t cancelRequest(NetSenderClient* client, uint64_t requestId);

    /** 投递上层授权的数据流帧到本进程有界发送队列
    线程：由所属执行者串行调用，协议回调中也可调用，不得与配置、连接或销毁并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] frame DATA／DATA_ACK或文件CANCEL，产品／流号非零
    @return OK表示帧及载荷已复制并入队，不表示网络发送；背压未接纳，可重试
    */
    int32_t postData(NetSenderClient* client, const NetInterfaceFrame& frame);

    /** 请求关闭当前代次，随后由所属执行者继续 poll 至停止
    线程：由所属执行者串行调用，协议回调中也可调用，不得与配置、连接或销毁并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] reason 默认0
    */
    void closeGateway(NetSenderClient* client, int32_t reason = 0);

    /** 请求关闭，不在调用方等待；继续poll／观察至停止，可重连
    线程：由所属执行者串行调用，协议回调中也可调用，不得与配置、连接或销毁并发
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    */
    void uninitGateway(NetSenderClient* client);

    /** 由非GUI生命期owner释放完整上下文，仅在实际停止、生产者退出及回调收敛之后调用
    线程：同一提供方的所有客户端共用一个生命周期执行者，由其串行操作，协议回调内禁止调用
    回调/poll或异步资源未收敛时保留对象，继续推进后重试；成功后须重新createClient再prepare。
    @param [in,out] client 本提供方创建的客户端或空；成功销毁置空，失败保留
    @return NI_OK已释放或原本为空；负数表示未释放，调用方仍须持有对象
    */
    int32_t destroyClient(NetSenderClient*& client);

    /** 真实握手已通过且未关闭
    线程：由所属执行者串行查询，协议回调中也可查询，查询期间保持客户端有效
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @return 是否可提交
    */
    bool gatewayReady(NetSenderClient* client) const;

    /** 当前连接代次
    线程：由所属执行者串行查询，协议回调中也可查询，查询期间保持客户端有效
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @return 未建立为0，之后递增
    */
    uint64_t gatewayGeneration(NetSenderClient* client) const;

    /** 最近记录的准备、配置、连接或业务提交诊断；状态查询、正常 poll、关闭和交付票据操作保留该值。每次调用以直接返回码为准
    线程：由所属执行者串行查询，协议回调中也可查询，查询期间保持客户端有效
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @return NetInterface结果
    */
    int32_t lastGatewayError(NetSenderClient* client) const;

    /** 同步提交预留请求，推进至网络宿主接纳全部字节或发生错误
    线程：由所属执行者串行调用，禁止在本客户端协议回调或 poll 中调用
    同步推进期间可能交付回调，调用前须登记业务上下文
    等待计入原请求总期限，事件循环可使用 beginSendRequest 和 sendResult 查询交付票据
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] requestId 当前未投递的预留号
    @param [in] bytes 调用时复制，size非零时不可空
    @param [in] size 最大1048560字节
    @return NI_OK 表示网络宿主已接纳，业务应答由回调报告。仅明确未接纳的背压可重试
    */
    int32_t sendRequest(NetSenderClient* client, uint64_t requestId, const uint8_t* bytes, uint32_t size);

    /** 同步交付已授权的流帧，使用本客户端提交选项的交付期限，默认 5000 毫秒
    线程：由所属执行者串行调用，禁止在本客户端协议回调或 poll 中调用
    @param [in] client 本提供方创建的有效客户端；使用期间不得销毁
    @param [in] frame DATA／ACK／文件CANCEL，调用时复制载荷
    @return NI_OK 表示网络宿主已接纳，负数表示失败。已接纳帧不应自动重发
    */
    int32_t sendData(NetSenderClient* client, const NetInterfaceFrame& frame);

    /** 一次读取同一客户端的代次及就绪快照，不驱动协议
    线程：由所属执行者串行查询，协议回调中也可查询，查询期间保持客户端有效
    @param [in] client 本提供方有效客户端
    @param [out] generation 失败清零
    @param [out] ready 失败为false
    @return NI_OK或调用错误
    */
    int32_t gatewayState(NetSenderClient* client, uint64_t& generation, bool& ready);

    /** 非阻塞建立请求交付票据；仅sendResult返回NI_OK后才完成send责任交接
    线程：由所属执行者串行调用，禁止在本客户端协议回调或 poll 中调用
    @param [in] client 本提供方有效客户端
    @param [in] requestId 当前预留请求号
    @param [in] bytes size非零时不可空，接纳时复制
    @param [in] size 正文字节数，受请求额度限制
    @param [out] ticket 独立输出失败清零。与正文重叠时返回 NI_INVALID_ARGUMENT，保留输入原值。接纳后须推进并读取最终结果
    @return NI_OK只表示票据已接纳；负数失败，不表示已交付网络
    */
    int32_t beginSendRequest(NetSenderClient* client, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t& ticket);

    /** 非阻塞建立流帧交付票据，sendResult 返回 NI_OK 时网络宿主已接纳
    线程：由所属执行者串行调用，禁止在本客户端协议回调或 poll 中调用
    @param [in] client 本提供方有效客户端
    @param [in] frame 接纳时复制的帧与正文
    @param [out] ticket 独立输出失败清零。与帧结构或正文重叠时返回 NI_INVALID_ARGUMENT，保留输入原值。成功后须查询至终态
    @return NI_OK只表示票据接纳，完成需查询sendResult
    */
    int32_t beginSendData(NetSenderClient* client, const NetInterfaceFrame& frame, uint64_t& ticket);

    /** 查询并消费交付票据终态，不等待业务应答
    线程：由所属执行者串行查询，协议回调中也可查询，查询期间保持客户端有效
    @param [in] client 创建票据的客户端
    @param [in] ticket 未消费的有效票据，同一票据只有一个查询消费者
    @return NI_READY待交付，NI_OK已交付，负数终态；终态读取后不可重复查询
    */
    int32_t sendResult(NetSenderClient* client, uint64_t ticket);

    /** 取得无状态文件协议辅助入口，不创建连接、文件或线程
    线程：首次取得在启动工作线程之前；不同流可独立调用，同一流由唯一执行者串行处理
    @return 本模块长期有效的借用入口，不得delete；流由其create/destroy配对管理
    */
    NetFileProtocol& fileProtocol();

    /** 查询本模块实际指针字节数，不访问客户端或宿主
    @return 4或8
    */
    static uint32_t pointerBytes();

    /** 从已登记的原始字节宿主获取强随机字节
    线程：遵守已登记宿主的执行者约束，不与宿主配置并发
    @param [out] output size非零时不可空；有效范围在熵源失败时清零
    @param [in] size 0至8192；非法范围不访问输出，0允许空指针
    @return NI_OK、NI_INVALID_ARGUMENT或NI_INVALID_STATE；没有弱随机回退
    */
    static int32_t randomBytes(uint8_t* output, uint32_t size);

    /** 取得本提供方的专用配置入口；不创建客户端、线程或连接
    线程：唯一生命周期owner在启动业务执行者前使用，具体方法仍遵守各自配置契约
    通过该入口登记网络宿主或配置客户端的备用网关
    @return 本模块长期有效的借用配置对象，不得delete或跨提供方使用
    */
    static NetSenderBackend& backend();

private:
    /** 仅由instance构造入口
    */
    NetPeerNetSender();

    /** 不在静态析构期代替调用方销毁客户端
    */
    ~NetPeerNetSender();

    /** 禁止复制单例入口
    @param [in] other 不可复制实例
    */
    NetPeerNetSender(const NetPeerNetSender& other);

    /** 禁止赋值单例入口
    @param [in] other 不可复制实例
    @return 不可调用
    */
    NetPeerNetSender& operator=(const NetPeerNetSender& other);
};

// 统一业务名称；真实符号按提供方区分，避免同进程链接时重名
typedef NetPeerNetSender NetSender;
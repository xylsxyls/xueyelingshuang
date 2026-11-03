#pragma once
#include "NetEndpointRouter.h"
#include "NetInterfaceHandshake.h"
#include <map>
#include "NetEndpointFileStream.h"

/** 由路由器端点表唯一拥有的会话状态，关闭链与文件链仅借用
*/
struct NetEndpointPeer
{
public:
    // 本路由器代次，用于关闭队列；不从网络消息自报取得。
    uint64_t m_id;
    // 关闭链中的下一端点，不拥有；只在入队后使用。
    NetEndpointPeer* m_nextClosing;
    // 文件只在第一次关闭推进时清理，不能每个请求都扫描流表。
    bool m_filesCollected;
    // 本端点的文件链与额度；客户端最多32条，服务最多128条。
    NetEndpointFileStream* m_firstFile;
    // 本端点当前登记的文件流数，随双端点链同步增减
    uint32_t m_fileCount;
    // 类型和关闭状态；服务端点没有TCP peer
    bool m_service;
    // true为外部可信通道附加的客户端，使用m_clients回送而不直接访问TCP
    bool m_attached;
    // 已请求关闭，禁止接纳新请求，现有请求按有界关闭队列收敛
    bool m_closing;
    // 显式网络运输中的连接号，服务或附加客户端不使用此TCP号
    uint64_t m_peer;
    // 本地服务绑定的产品号，内部或通用服务为0
    uint32_t m_product;
    // TLS后的应用握手独占对象，服务登记后不使用
    std::unique_ptr<NetInterfaceHandshake> m_handshake;
    // 已验证握手的声明上下文
    NetEndpointOrigin m_origin;
    // 握手协商的平台、能力位、请求上限和文件块字节数，依次占四项
    uint32_t m_negotiated[4];
    // 最近收到完整合法帧的单调时间；业务空闲也需回应心跳
    uint64_t m_activity;
    // 最近发送心跳或初始化连接的单调毫秒，用于响应期限
    uint64_t m_pingTime;
    // 正在等待响应的心跳号，0表示无在途心跳
    uint64_t m_pingId;

public:
    /** 显式初始化全部标量，不依赖v120对含unique_ptr类型的值初始化
    */
    NetEndpointPeer();
};

/** 拥有请求、端点和文件流索引，单执行者推进，不创建线程或网络运输
*/
class NetEndpointRouterState
{
public:
    /** 创建有界路由；保留1为不可用服务槽，保证拒绝也消耗来源编号
    @param [in] services 借用的本地服务提交端口，存活至停止
    @param [in] clients 借用的附加客户端端口，可空；非空时不直接监听TCP
    */
    NetEndpointRouterState(NetEndpointServiceSink* services, NetEndpointServiceSink* clients);

    /** 获取进程内单调毫秒
    @return steady_clock毫秒
    */
    static uint64_t now();

    /** 将已经标记关闭且不在队列中的端点追加到尾部，不分配内存
    @param [in] peer 由m_endpoints拥有，必须保持到出队之后
    */
    void enqueueClosing(NetEndpointPeer* peer);

    /** 取出一个关闭工作，未完成时调用方重新排尾，避免只清理单一端点
    @return 借用端点或空，不删除拥有节点
    */
    NetEndpointPeer* takeClosing();

    /** 擦除本轮转发的暂存明文，最多保留4KiB容量
    */
    void clearPayload();

    /** 双索引插入成功后，无分配地接入双端点链和固定期限堆
    @param [in] stream 已由m_streams拥有的流
    */
    void attachFile(NetEndpointFileStream* stream);

    /** 统一摘除流的全部索引、期限和两端额度，返回独占对象以完成终态通知
    @param [in] id 公开流号
    @return 独占流或空；返回对象析构时擦除秘密
    */
    std::unique_ptr<NetEndpointFileStream> takeFile(uint64_t id);

    /** 更新期限后恢复固定堆，不新建分配节点
    @param [in] stream 仍登记的流
    @param [in] deadline 新单调期限
    */
    void renewFile(NetEndpointFileStream* stream, uint64_t deadline);

    /** 调整堆节点并同步反向下标
    @param [in] left 第一节点
    @param [in] right 第二节点
    */
    void exchangeFile(uint32_t left, uint32_t right);

    /** 从给定下标向上恢复期限堆
    @param [in] index 有效节点
    */
    void riseFile(uint32_t index);

    /** 从给定下标向下恢复期限堆
    @param [in] index 有效节点
    */
    void sinkFile(uint32_t index);

public:
    // 借用显式传输，附加端点模式可空
    NetServerChannel* m_transport;
    // 监听配置借用的宿主密码学随机
    NetInterfaceEntropyFunction m_entropy;
    // 随机上下文借用至停止
    void* m_entropyContext;
    // 物理监听接纳后不允许热换传输
    bool m_listening;
    // 独占的有界请求配额与双端请求映射
    NetInterfaceRequests m_requests;
    // 借用的已认证服务提交端口，存活至路由器停止，空时不能附加服务
    NetEndpointServiceSink* m_services;
    // 借用的附加客户端回送端口，非空时不允许直接监听TCP
    NetEndpointServiceSink* m_clients;
    // 端点唯一拥有表，关闭队列只借用它，不再执行共享引用计数。
    std::map<uint64_t, std::unique_ptr<NetEndpointPeer>> m_endpoints;
    // 真实网络连接号到本路由器端点号的非拥有索引
    std::map<uint64_t, uint64_t> m_peers;
    // 可信附加客户端原始端点号到本地端点号的非拥有索引
    std::map<uint64_t, uint64_t> m_origins;
    // 产品号到本地专属服务端点号的非拥有索引
    std::map<uint32_t, uint64_t> m_products;
    // 无分配的关闭队列头尾，最多1023个已登记端点。
    NetEndpointPeer* m_closingFirst;
    // 有界关闭队列尾，队列为空时与头指针同时为空
    NetEndpointPeer* m_closingLast;
    // 只在单一dispatch期间保存转发明文，不借给后续轮次。
    std::vector<uint8_t> m_payload;
    // 拒绝嵌套推进和同步再次投递；宿主回调只能请求关闭。
    bool m_polling;
    // 当前正在转发一条帧，阻止回调同步重入投递
    bool m_dispatching;
    // 回调/推进中收到停止请求，延迟到借用结束后收敛。
    bool m_stopRequested;
    // 唯一拥有文件流；另一个映射只保存代次，固定堆及双链只借用。
    std::map<uint64_t, std::unique_ptr<NetEndpointFileStream>> m_streams;
    // 按单调到期时间排列的固定最小堆，节点只借用m_streams对象
    NetEndpointFileStream* m_fileExpiry[1024];
    // 期限堆中有效节点数，最大1024
    uint32_t m_fileExpiryCount;
    // 服务代次及本地流号到网关公开流号的非拥有反向索引
    std::map<std::pair<uint64_t, uint64_t>, uint64_t> m_serviceStreams;
    // 已分配的最大端点号，1保留为不可用服务槽，达到UINT64_MAX不再分配
    uint64_t m_lastEndpoint;
    // 内部账号服务端点号，0表示没有登记
    uint64_t m_internal;
    // 通用转发服务端点号，0表示没有登记；产品无专属服务时才使用
    uint64_t m_generic;
    // 本路由器已分配的最大心跳号，耗尽时关闭连接而不复用
    uint64_t m_lastPing;
    // 已分配的最大公开文件流号，耗尽后拒绝新流
    uint64_t m_lastStream;
    // 已交付的最大事件号，转发失败不能当成交付成功而增长
    uint64_t m_lastEvent;
    // 初始化或停止期间保留的路由器状态，负数为错误
    int32_t m_status;
    // 已完成整体停止，不能重新启动此实例
    bool m_stopped;
};
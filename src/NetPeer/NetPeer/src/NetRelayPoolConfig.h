#pragma once
#include "NetPeerMacro.h"
#include "NetInterface/NetInterfaceAPI.h"


/** 原始字节宿主的实际连接槽和输入正文池，每个宿主实例独立配置
*/
struct NetPeerAPI NetHostPoolConfig
{
    // 每次派发最多交付的事件数，剩余事件按原顺序留到下轮
    uint32_t m_eventBudget = 2048;
    // 原始连接槽；最大容量以槽计，-1允许继续增长
    LockFreeHashMapPoolConfig m_peers;
    // 输入数据按长度选择的五个大小级
    NetBufferPoolConfig m_input;

    /** 使用宿主默认的连接预热和物理池预算
    */
    NetHostPoolConfig();
};

// 透明中继输出实际存储配置；控制和数据独立池，共享有序交付链
struct NetPeerAPI NetRelayPoolConfig
{
public:
    // 每轮输入和输出各自最多推进的记录数，正数
    uint32_t m_stepBudget = 64;
    // 等待原始连接建立的最长毫秒数，正数
    uint32_t m_startupMs = 5000;
    // 拒收同一记录的最长毫秒数，正数
    uint32_t m_backpressureMs = 5000;
    // 数据记录池，记录在宿主完整接纳后才归还
    LockFreeHashMapPoolConfig m_dataRecords;
    // 控制记录池，不挪用数据记录槽
    LockFreeHashMapPoolConfig m_controlRecords;
    // 实际数据密文及NTW3头存储
    NetBufferPoolConfig m_dataBytes;
    // 实际控制字节存储
    NetBufferPoolConfig m_controlBytes;

public:
    /** 默认记录池各按约8MiB实际槽大小计算，正文按大小级增长
    */
    NetRelayPoolConfig();

    /** 校验每轮记录数、启动和背压时限
    @return 三个预算均为正数时返回true；物理池按实例创建时检查
    */
    bool valid() const;
};

// 直连TLS会话或端点监听器的运输资源
struct NetPeerAPI NetDirectTransportConfig
{
    // 字节宿主的连接及输入资源
    NetHostPoolConfig m_host;
    // TLS连接对象槽，与宿主连接槽独立
    LockFreeHashMapPoolConfig m_peers;
    // 直连TLS记录及明文字节池
    NetConnectionPoolConfig m_connection;
    // 直连TLS双向密文字节预算
    CEncodeDecodeTlsQueueLimits m_ciphertext;
    // 单帧正文上限，外部网络最大为协议规定的默认上限
    uint32_t m_payloadLimit = NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT;
    // 每条连接每轮最多推进的步骤数，每步至多交付一帧，正数
    uint32_t m_frameBudget = 64;
    // 一轮最多交给宿主的密文字节数，正数
    uint32_t m_outputBudget = 65536;
    // EOF后等待已接纳输入排空的毫秒数，正数
    uint32_t m_drainTimeoutMs = 5000;

    /** 使用直连运输的默认连接池
    */
    NetDirectTransportConfig();
};

// 管理运输的一次配置，服务连接和目录发现各自复制一份
struct NetPeerAPI NetLinkTransportConfig
{
    // 字节宿主的连接及输入资源
    NetHostPoolConfig m_host;
    // 原始中继的控制和数据记录、正文池
    NetRelayPoolConfig m_relay;
    // 已分配的管理连接对象槽
    LockFreeHashMapPoolConfig m_peers;
    // 管理TLS、业务记录和密文资源以及保活策略
    NetInterfaceLinkPools m_link;
    // 每轮各阶段最多推进的记录数，正数
    uint32_t m_stepBudget = 64;
    // 管理记录持续背压的毫秒数，正数
    uint32_t m_backpressureMs = 5000;

    /** 使用管理运输的默认连接池
    */
    NetLinkTransportConfig();
};

// 一条发现链路的轮询和缓存策略
struct NetPeerAPI NetGatewayDiscoveryConfig
{
    // 探测网关时使用的独立链路
    NetLinkTransportConfig m_transport;
    // 暂时无候选或连接失败后的重试间隔，毫秒
    uint32_t m_retryMs = 250;
    // 同一候选失败退避的基础毫秒数，0表示不添加固定退避
    uint32_t m_backoffMs = 500;
    // 随机附加毫秒数小于此值，0或1不添加抖动
    uint32_t m_jitterMs = 1000;
    // 等待一条发现应答的最长毫秒数
    uint32_t m_responseMs = 5000;
    // 成功探测后发起下一次查询的间隔，毫秒
    uint32_t m_refreshMs = 5000;
    // 两次缓存写入的最小毫秒间隔，0允许立即提交
    uint32_t m_cacheWriteMs = 2000;

    /** 核对发现调度期限和本实例管理运输的数值预算
    @return 所有数值预算有效返回true，物理池在创建时验证
    */
    bool valid() const;
};

// 一个客户端的不同运输各有独立预算，创建前可逐项设置
struct NetPeerAPI NetPeerConfig
{
    // 直接连接服务端使用的TLS运输
    NetDirectTransportConfig m_direct;
    // 连接业务服务所属网关使用的管理运输
    NetLinkTransportConfig m_service;
    // 探测候选网关目录使用的管理运输
    NetGatewayDiscoveryConfig m_discovery;
    // 仅在创建时借用，传给初始化配置函数
    void* m_context = nullptr;
    // 可空；在发布客户端前同步调用，在此配置实际会话与派发器。返回NI_OK发布，负值原样返回，正值转为NI_INVALID_STATE。引用仅供本次配置使用
    int32_t (*m_configure)(void*, NetClientSession&, NetSenderDispatch&) = nullptr;

    /** 核对三条路径的数值预算，物理池在各自实际创建时验证
    @return 所有数值预算有效返回true
    */
    bool valid() const;
};
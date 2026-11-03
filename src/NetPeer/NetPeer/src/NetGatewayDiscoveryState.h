#pragma once
#include "NetLinkTransport.h"
#include "NetGatewayDirectoryStore.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <memory>

// 候选值及退避由发现器独占；选择/失败路径不构造字符串或分配容器
struct NetGatewayDiscoveryCandidate
{
    // 保留入口首次出现的地址写法，跨模块只复制固定值
    NetClientDiscoveryPoint m_point;
    // 数值地址键，IPv4与映射IPv6一致，端口单独比较
    uint8_t m_key[16];
    // 单调退避截止，0为可用，UINT64_MAX为本代次强随机不可用
    uint64_t m_failedUntil;
};

// 发现器拥有的内部状态，不跨DLL暴露STL布局
struct NetGatewayDiscoveryState
{
public:
    // 独占的管理运输
    std::unique_ptr<NetLinkTransport> m_link;
    // 可选的异步公开目录缓存
    NetGatewayDirectoryStore m_cache;
    // 只复制强随机上下文和期限
    NetInterfaceTlsOptions m_tls;
    // 调用者提供的数值入口
    NetClientDiscoveryPoint m_seeds[64];
    // 已复制的种子数，启动后不受远端目录删改
    uint32_t m_seedCount;
    // 种子与发现合并后的去重提示
    NetGatewayDiscoveryCandidate m_candidates[128];
    // 已初始化候选数，所有读取严格限制在此范围
    uint32_t m_candidateCount;
    // 本轮认证返回或启动缓存中的节点
    NetInterfaceDirectoryNodeData m_nodes[64];
    // 已验证的完整目录记录数
    uint32_t m_nodeCount;
    // 当前管理探测候选
    NetClientDiscoveryPoint m_current;
    // 当前管理连接号
    uint64_t m_peer;
    // 单调管理请求号
    uint64_t m_request;
    // 等待回复的请求号，0为无
    uint64_t m_waiting;
    // 最近成功提交管理请求的单调时间
    uint64_t m_sentAt;
    // 下一次允许探测的单调时间
    uint64_t m_retryAt;
    // 有界候选的轮转位置
    uint32_t m_next;
    // 是否接纳新探测
    bool m_running;
    // 管理连接是否认证完成
    bool m_ready;
    // 管理运输是否关闭待回收
    bool m_closed;
    // 当前列表是否尚未被本轮认证结果替换
    bool m_cached;
    // 是否已配置异步缓存
    bool m_cacheEnabled;

    /** 初始化静止状态，不启动平台资源
    */
    NetGatewayDiscoveryState();

    /** 无分配合并64种子与64目录，按数值地址去重并保留仍存在候选的退避
    */
    void rebuild();
};
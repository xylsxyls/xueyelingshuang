#pragma once
#include "../../NetServerManager/NetServerManager/NetGatewayNode.h"

// m_boot的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestBoot
{
public:
    // 保留实际成员类型及调用约定。
    typedef uint64_t NetGatewayNode::*Type;
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestBoot::Type netNodeTestMember(NetNodeTestBoot tag);

// m_candidates的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestCandidates
{
public:
    // 保留实际成员类型及调用约定。
    typedef std::map<std::pair<std::string, uint32_t>, NetObjectReference<NetGatewayNodeCandidate>> NetGatewayNode::*Type;
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestCandidates::Type netNodeTestMember(NetNodeTestCandidates tag);

// m_channels的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestChannels
{
public:
    // 保留实际成员类型及调用约定。
    typedef std::map<std::pair<uint64_t, uint64_t>, uint64_t> NetGatewayNode::*Type;
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestChannels::Type netNodeTestMember(NetNodeTestChannels tag);

// m_lastOutbound的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestLastOutbound
{
public:
    // 保留实际成员类型及调用约定。
    typedef uint64_t NetGatewayNode::*Type;
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestLastOutbound::Type netNodeTestMember(NetNodeTestLastOutbound tag);

// m_lastRoute的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestLastRoute
{
public:
    // 保留实际成员类型及调用约定。
    typedef uint64_t NetGatewayNode::*Type;
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestLastRoute::Type netNodeTestMember(NetNodeTestLastRoute tag);

// m_members的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestMembers
{
public:
    // 保留实际成员类型及调用约定。
    typedef std::map<std::string, NetObjectReference<NetGatewayNodeMember>> NetGatewayNode::*Type;
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestMembers::Type netNodeTestMember(NetNodeTestMembers tag);

// m_meshDirty的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestMeshDirty
{
public:
    // 保留实际成员类型及调用约定。
    typedef bool NetGatewayNode::*Type;
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestMeshDirty::Type netNodeTestMember(NetNodeTestMeshDirty tag);

// m_peers的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestPeers
{
public:
    // 保留实际成员类型及调用约定。
    typedef std::map<uint64_t, NetGatewayNodePeer> NetGatewayNode::*Type;
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestPeers::Type netNodeTestMember(NetNodeTestPeers tag);

// m_pendingOpens的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestPendingOpens
{
public:
    // 保留实际成员类型及调用约定。
    typedef std::map<std::pair<uint64_t, uint64_t>, uint64_t> NetGatewayNode::*Type;
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestPendingOpens::Type netNodeTestMember(NetNodeTestPendingOpens tag);

// m_recordBytes的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestRecordBytes
{
public:
    // 保留实际成员类型及调用约定。
    typedef uint64_t NetGatewayNode::*Type;
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestRecordBytes::Type netNodeTestMember(NetNodeTestRecordBytes tag);

// m_routes的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestRoutes
{
public:
    // 保留实际成员类型及调用约定。
    typedef std::map<uint64_t, NetObjectReference<NetGatewayNodeRoute>> NetGatewayNode::*Type;
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestRoutes::Type netNodeTestMember(NetNodeTestRoutes tag);

// m_snapshot的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestSnapshot
{
public:
    // 保留实际成员类型及调用约定。
    typedef std::string NetGatewayNode::*Type;
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestSnapshot::Type netNodeTestMember(NetNodeTestSnapshot tag);

// m_snapshotAt的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestSnapshotAt
{
public:
    // 保留实际成员类型及调用约定。
    typedef uint64_t NetGatewayNode::*Type;
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestSnapshotAt::Type netNodeTestMember(NetNodeTestSnapshotAt tag);

// m_snapshotRevision的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestSnapshotRevision
{
public:
    // 保留实际成员类型及调用约定。
    typedef uint64_t NetGatewayNode::*Type;
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestSnapshotRevision::Type netNodeTestMember(NetNodeTestSnapshotRevision tag);

// select的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestSelect
{
public:
    // 保留实际成员类型及调用约定。
    typedef bool (NetGatewayNode::*Type)(uint64_t peer, uint32_t product, uint64_t instance, uint64_t now, NetGatewayNodeSelection& output) const;
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestSelect::Type netNodeTestMember(NetNodeTestSelect tag);

// open的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestOpen
{
public:
    // 保留实际成员类型及调用约定。
    typedef int32_t (NetGatewayNode::*Type)(uint64_t peer, const NetInterfaceFrame& frame, bool mesh);
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestOpen::Type netNodeTestMember(NetNodeTestOpen tag);

// flushRecords的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestFlushRecords
{
public:
    // 保留实际成员类型及调用约定。
    typedef void (NetGatewayNode::*Type)();
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestFlushRecords::Type netNodeTestMember(NetNodeTestFlushRecords tag);

// pollMesh的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestPollMesh
{
public:
    // 保留实际成员类型及调用约定。
    typedef void (NetGatewayNode::*Type)();
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestPollMesh::Type netNodeTestMember(NetNodeTestPollMesh tag);

// expireMembers的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestExpireMembers
{
public:
    // 保留实际成员类型及调用约定。
    typedef void (NetGatewayNode::*Type)(uint64_t now);
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestExpireMembers::Type netNodeTestMember(NetNodeTestExpireMembers tag);

// remember的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestRemember
{
public:
    // 保留实际成员类型及调用约定。
    typedef int32_t (NetGatewayNode::*Type)(const NetInterfaceDirectoryPoint& point, bool verified);
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestRemember::Type netNodeTestMember(NetNodeTestRemember tag);

// retire的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestRetire
{
public:
    // 保留实际成员类型及调用约定。
    typedef void (NetGatewayNode::*Type)(uint64_t route, uint64_t sender, int32_t reason);
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestRetire::Type netNodeTestMember(NetNodeTestRetire tag);

// queueControl的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestQueueControl
{
public:
    // 保留实际成员类型及调用约定。
    typedef int32_t (NetGatewayNode::*Type)(uint64_t peer, const NetInterfaceFrame& frame, const std::string& bytes);
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestQueueControl::Type netNodeTestMember(NetNodeTestQueueControl tag);

// onLinkClosed的测试侧类型选择，不向生产类增加测试入口。
struct NetNodeTestOnLinkClosed
{
public:
    // 保留实际成员类型及调用约定。
    typedef void (NetGatewayNode::*Type)(uint64_t peer, int32_t reason);
};

/** 读取由测试显式实例化提供的成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetNodeTestOnLinkClosed::Type netNodeTestMember(NetNodeTestOnLinkClosed tag);

// 只在测试中取得原接收入口，不修改生产类。
struct NetNodeTestOnLinkRecord
{
public:
    // 原接收入口签名。
    typedef bool (NetGatewayNode::*Type)(uint64_t peer, const NetInterfaceTunnelRecord& record);
};

/** 取得真实接收成员函数
@param [in] tag 测试成员选择器
@return 原成员指针
*/
NetNodeTestOnLinkRecord::Type netNodeTestMember(NetNodeTestOnLinkRecord tag);

// 仅测试取得真实记录池，用于构造背压及关闭时已有的存储租约。
struct NetNodeTestRecordPool
{
public:
    // 不替换生产池，也不复制其实现
    typedef std::unique_ptr<NetGatewayNodeRecordPool> NetGatewayNode::*Type;
};

/** 取得节点的实际转发池成员
@param [in] tag 测试成员选择器
@return 原成员指针
*/
NetNodeTestRecordPool::Type netNodeTestMember(NetNodeTestRecordPool tag);

// 测试仅从真实池构造前置路由，不给生产代码增加白盒钩子。
struct NetNodeTestRoutePool
{
public:
    // 保留实际成员类型。
    typedef std::unique_ptr<NetObjectPool<NetGatewayNodeRoute>> NetGatewayNode::*Type;
};

/** 取得真实路由池成员
@param [in] tag 测试选择器
@return 成员指针
*/
NetNodeTestRoutePool::Type netNodeTestMember(NetNodeTestRoutePool tag);

#include "NetGatewayNodeTestAccess.inl"
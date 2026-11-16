// 显式实例化仅存在于测试工程，不改变生产类访问属性或符号修饰。
template<class Tag, typename Tag::Type Member>
struct NetNodeTestMember
{
public:
    /** 返回原始成员指针供测试安排状态和观察边界
    @param [in] tag 成员类型选择器
    @return 生产类原始成员指针
    */
    friend typename Tag::Type netNodeTestMember(Tag tag)
    {
        (void)tag;
        return Member;
    }
};

template struct NetNodeTestMember<NetNodeTestBoot, &NetGatewayNode::m_boot>;

template struct NetNodeTestMember<NetNodeTestCandidates, &NetGatewayNode::m_candidates>;

template struct NetNodeTestMember<NetNodeTestChannels, &NetGatewayNode::m_channels>;

template struct NetNodeTestMember<NetNodeTestLastOutbound, &NetGatewayNode::m_lastOutbound>;

template struct NetNodeTestMember<NetNodeTestLastRoute, &NetGatewayNode::m_lastRoute>;

template struct NetNodeTestMember<NetNodeTestMembers, &NetGatewayNode::m_members>;

template struct NetNodeTestMember<NetNodeTestMeshDirty, &NetGatewayNode::m_meshDirty>;

template struct NetNodeTestMember<NetNodeTestPeers, &NetGatewayNode::m_peers>;

template struct NetNodeTestMember<NetNodeTestPendingOpens, &NetGatewayNode::m_pendingOpens>;

template struct NetNodeTestMember<NetNodeTestRecordBytes, &NetGatewayNode::m_recordBytes>;

template struct NetNodeTestMember<NetNodeTestRoutes, &NetGatewayNode::m_routes>;

template struct NetNodeTestMember<NetNodeTestSnapshot, &NetGatewayNode::m_snapshot>;

template struct NetNodeTestMember<NetNodeTestSnapshotAt, &NetGatewayNode::m_snapshotAt>;

template struct NetNodeTestMember<NetNodeTestSnapshotRevision, &NetGatewayNode::m_snapshotRevision>;

template struct NetNodeTestMember<NetNodeTestSelect, &NetGatewayNode::select>;

template struct NetNodeTestMember<NetNodeTestOpen, &NetGatewayNode::open>;

template struct NetNodeTestMember<NetNodeTestFlushRecords, &NetGatewayNode::flushRecords>;

template struct NetNodeTestMember<NetNodeTestPollMesh, &NetGatewayNode::pollMesh>;

template struct NetNodeTestMember<NetNodeTestExpireMembers, &NetGatewayNode::expireMembers>;

template struct NetNodeTestMember<NetNodeTestRemember, &NetGatewayNode::remember>;

template struct NetNodeTestMember<NetNodeTestRetire, &NetGatewayNode::retire>;

template struct NetNodeTestMember<NetNodeTestQueueControl, &NetGatewayNode::queueControl>;

template struct NetNodeTestMember<NetNodeTestOnLinkClosed, &NetGatewayNode::onLinkClosed>;
template struct NetNodeTestMember<NetNodeTestOnLinkRecord, &NetGatewayNode::onLinkRecord>;

template struct NetNodeTestMember<NetNodeTestRecordPool, &NetGatewayNode::m_recordPool>;

template struct NetNodeTestMember<NetNodeTestRoutePool, &NetGatewayNode::m_routePool>;
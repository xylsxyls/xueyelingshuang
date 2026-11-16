// Explicit instantiation is confined to the white-box test target.
template<class Tag, typename Tag::Type Member>
struct NetServiceTestMember
{
public:
    /** Return the original member without changing access or symbol decoration.
    @param [in] tag Member type selector
    @return Original member pointer
    */
    friend typename Tag::Type netServiceTestMember(Tag tag)
    {
        (void)tag;
        return Member;
    }
};

template struct NetServiceTestMember<NetServiceTestProxyState, &NetSenderServiceProxy::m_state>;
template struct NetServiceTestMember<NetServiceTestServiceState, &NetSenderService::m_state>;
template struct NetServiceTestMember<NetServiceTestGatewayState, &NetSenderServiceGateway::m_state>;
template struct NetServiceTestMember<NetServiceTestOutgoingPhase, &NetSenderServiceOutgoingTransport::m_phase>;
template struct NetServiceTestMember<NetServiceTestOutgoingProduct, &NetSenderServiceOutgoingTransport::m_product>;
template struct NetServiceTestMember<NetServiceTestOutgoingControl, &NetSenderServiceOutgoingTransport::control>;
template struct NetServiceTestMember<NetServiceTestOutgoingRecord, &NetSenderServiceOutgoingTransport::record>;
template struct NetServiceTestMember<NetServiceTestOutgoingClosed, &NetSenderServiceOutgoingTransport::closed>;
template struct NetServiceTestMember<NetServiceTestManagedHandler, &NetSenderServiceManaged::m_handler>;
template struct NetServiceTestMember<NetServiceTestManagedListener, &NetSenderServiceManaged::m_listener>;
template struct NetServiceTestMember<NetServiceTestManagedPeers, &NetSenderServiceManaged::m_peers>;
template struct NetServiceTestMember<NetServiceTestManagedProduct, &NetSenderServiceManaged::m_product>;
template struct NetServiceTestMember<NetServiceTestManagedQueue, &NetSenderServiceManaged::m_queue>;
template struct NetServiceTestMember<NetServiceTestManagedRoots, &NetSenderServiceManaged::m_roots>;
template struct NetServiceTestMember<NetServiceTestManagedRouter, &NetSenderServiceManaged::m_router>;
template struct NetServiceTestMember<NetServiceTestManagedPush, &NetSenderServiceManaged::push>;
template struct NetServiceTestMember<NetServiceTestManagedPump, &NetSenderServiceManaged::pumpQueue>;
template struct NetServiceTestMember<NetServiceTestManagedAttach, &NetSenderServiceManaged::attach>;
template struct NetServiceTestMember<NetServiceTestManagedAdd, &NetSenderServiceManaged::addPeer>;
template struct NetServiceTestMember<NetServiceTestManagedClientClosed, &NetSenderServiceManaged::onGatewayClientClosed>;
template struct NetServiceTestMember<NetServiceTestManagedServiceClosed, &NetSenderServiceManaged::onGatewayServiceClosed>;
template struct NetServiceTestMember<NetServiceTestOutgoingOwner, &NetSenderServiceOutgoingTransport::m_owner>;
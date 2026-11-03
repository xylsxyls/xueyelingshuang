template<class Entry, class Client>
NetSenderTestClient<Entry, Client>::NetSenderTestClient() :
m_client(nullptr)
{

}

template<class Entry, class Client>
NetSenderTestClient<Entry, Client>::~NetSenderTestClient()
{
    Entry::instance().destroyClient(m_client);
}

template<class Entry, class Client>
NetSenderTestClient<Entry, Client>& NetSenderTestClient<Entry, Client>::instance()
{
    static NetSenderTestClient s_instance;
    return s_instance;
}

template<class Entry, class Client>
NetSenderTestClient<Entry, Client>::operator Client*() const
{
    return m_client;
}

template<class Entry, class Client>
int32_t NetSenderTestClient<Entry, Client>::prepareGateway()
{
    if (m_client == nullptr)
    {
        const int32_t created = Entry::instance().createClient(m_client);
        if (created != NI_OK) { return created; }
    }
    return Entry::instance().prepareGateway(m_client);
}

template<class Entry, class Client>
int32_t NetSenderTestClient<Entry, Client>::connectGateway(const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    return Entry::instance().connectGateway(m_client, options, listener);
}

template<class Entry, class Client>
int32_t NetSenderTestClient<Entry, Client>::connectServiceGateway(uint32_t product, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    return Entry::instance().connectServiceGateway(m_client, product, options, listener);
}

template<class Entry, class Client>
int32_t NetSenderTestClient<Entry, Client>::configureCompression(const uint32_t* types, uint32_t count)
{
    return Entry::instance().configureCompression(m_client, types, count);
}

template<class Entry, class Client>
int32_t NetSenderTestClient<Entry, Client>::beginRequest(uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& requestId)
{
    return Entry::instance().beginRequest(m_client, product, type, timeoutMs, requestId);
}

template<class Entry, class Client>
int32_t NetSenderTestClient<Entry, Client>::postRequest(uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    return Entry::instance().postRequest(m_client, requestId, bytes, size);
}

template<class Entry, class Client>
int32_t NetSenderTestClient<Entry, Client>::cancelRequest(uint64_t requestId)
{
    return Entry::instance().cancelRequest(m_client, requestId);
}

template<class Entry, class Client>
int32_t NetSenderTestClient<Entry, Client>::postData(const NetInterfaceFrame& frame)
{
    return Entry::instance().postData(m_client, frame);
}

template<class Entry, class Client>
int32_t NetSenderTestClient<Entry, Client>::sendRequest(uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    return Entry::instance().sendRequest(m_client, requestId, bytes, size);
}

template<class Entry, class Client>
int32_t NetSenderTestClient<Entry, Client>::sendData(const NetInterfaceFrame& frame)
{
    return Entry::instance().sendData(m_client, frame);
}

template<class Entry, class Client>
bool NetSenderTestClient<Entry, Client>::pollGateway()
{
    return Entry::instance().pollGateway(m_client) == NI_READY;
}

template<class Entry, class Client>
void NetSenderTestClient<Entry, Client>::closeGateway(int32_t reason)
{
    Entry::instance().closeGateway(m_client, reason);
}

template<class Entry, class Client>
void NetSenderTestClient<Entry, Client>::uninitGateway()
{
    Entry::instance().uninitGateway(m_client);
}

template<class Entry, class Client>
int32_t NetSenderTestClient<Entry, Client>::releaseGateway()
{
    return Entry::instance().destroyClient(m_client);
}

template<class Entry, class Client>
bool NetSenderTestClient<Entry, Client>::gatewayReady() const
{
    return Entry::instance().gatewayReady(m_client);
}

template<class Entry, class Client>
uint64_t NetSenderTestClient<Entry, Client>::gatewayGeneration() const
{
    return Entry::instance().gatewayGeneration(m_client);
}

template<class Entry, class Client>
int32_t NetSenderTestClient<Entry, Client>::lastGatewayError() const
{
    return Entry::instance().lastGatewayError(m_client);
}
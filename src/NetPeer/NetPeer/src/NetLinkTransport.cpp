#include "NetLinkTransport.h"
#include "NetPeerMacro.h"
#include "NetLinkTransportState.h"
#include "NetTransport.h"
#include <chrono>

NetLinkTransport::NetLinkTransport() : m_state(new NetLinkTransportState())
{

}

NetLinkTransport::~NetLinkTransport()
{
    shutdown();
    delete m_state;
}

int32_t NetLinkTransport::listen(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
    NetLinkTransportListener* listener, uint32_t maxPeers)
{
    // 该适配器是TCP公共/节点入口，服务首次注册只允许独立本机IPC适配。
    if (options.m_tls.server != 1 || (options.m_peerRoles & (1U << NI_LINK_SERVICE)) != 0) { return NI_INVALID_ARGUMENT; }
    const int32_t configured = m_state->configure(options, listener);
    return configured == NI_OK ? m_state->m_raw.listen(address, port, m_state, maxPeers) : configured;
}

int32_t NetLinkTransport::connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
    NetLinkTransportListener* listener, uint64_t& peer)
{
    peer = 0;
    if (options.m_tls.server != 0) { return NI_INVALID_ARGUMENT; }
    const int32_t configured = m_state->configure(options, listener);
    if (configured != NI_OK) { return configured; }
    const int32_t connected = m_state->m_raw.connect(address, port, m_state, peer);
    if (connected != NI_OK) { return connected; }
    try
    {
        std::unique_ptr<NetLinkTransportPeer> entry(new NetLinkTransportPeer(m_state->m_options));
        const int32_t status = entry->m_link.status();
        m_state->m_peers.insert(std::make_pair(peer, std::move(entry)));
        if (status < 0) { m_state->close(peer, status); peer = 0; return status; }
        return NI_OK;
    }
    catch (...)
    {
        m_state->m_raw.close(peer, NI_NO_MEMORY);
        peer = 0;
        return NI_NO_MEMORY;
    }
}

bool NetLinkTransport::poll()
{
    if (m_state->m_polling) { return !m_state->m_stopping; }
    m_state->m_polling = true;
    const bool active = m_state->m_raw.poll();
    for (auto& entry : m_state->m_peers)
    {
        try { m_state->pump(entry.first, *entry.second); }
        catch (...) { m_state->close(entry.first, NI_NO_MEMORY); }
    }
    m_state->collectClosed();
    m_state->m_polling = false;
    if (m_state->m_stopping) { shutdown(); }
    return (active || !m_state->m_peers.empty()) && !m_state->m_stopping;
}

int32_t NetLinkTransport::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    auto found = m_state->m_peers.find(peer);
    return m_state->m_stopping || found == m_state->m_peers.end() || (found->second->m_closing || found->second->m_closed) ? NI_INVALID_STATE :
        found->second->m_link.sendControl(frame);
}

int32_t NetLinkTransport::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    auto found = m_state->m_peers.find(peer);
    return m_state->m_stopping || found == m_state->m_peers.end() || (found->second->m_closing || found->second->m_closed) ? NI_INVALID_STATE :
        found->second->m_link.sendRecord(record);
}

void NetLinkTransport::close(uint64_t peer, int32_t reason)
{
    m_state->close(peer, reason);
}

void NetLinkTransport::shutdown()
{
    m_state->m_stopping = true;
    if (m_state->m_polling) { return; }
    m_state->m_polling = true;
    for (auto& entry : m_state->m_peers) { m_state->close(entry.first, 0); }
    m_state->m_raw.shutdown();
    m_state->collectClosed();
    m_state->m_polling = false;
}

uint32_t NetLinkTransport::localPort() const
{
    return m_state->m_raw.localPort();
}

int32_t NetLinkTransport::outputState(uint64_t peer) const
{
    const auto found = m_state->m_peers.find(peer);
    if (found == m_state->m_peers.end() || found->second->m_closing || found->second->m_closed) { return NI_INVALID_STATE; }
    const int32_t local = found->second->m_link.outputState();
    return local == NI_OK ? m_state->m_raw.outputState(peer) : local;
}

// NET_LINK_TRANSPORT_UNIT_BEGIN
//
// #include "NetPeer/NetHostSocketTest.h"
// #include "NetPeer.h"
// #include <cstring>
// #include <chrono>
// #include <cstdio>
//
// NetHostSocketTestDriver::NetHostSocketTestDriver() :
// m_observer(nullptr),
// m_connected(nullptr),
// m_data(nullptr),
// m_writable(nullptr),
// m_closed(nullptr),
// m_writeResult(0),
// m_pauseResult(0),
// m_entropyMode(0),
// m_destroyFailures(0),
// m_active(false),
// m_syncConnect(false),
// m_createFail(false),
// m_fault(0),
// m_closeOnPause(false),
// m_dataOnResume(0),
// m_written(0),
// m_pauses(0),
// m_stops(0),
// m_destroys(0)
// {
//
// }
//
// NetHostFunctions NetHostSocketTestDriver::functions()
// {
//     NetHostFunctions functions = {};
//     functions.structSize = sizeof(functions);
//     functions.version = 1;
//     functions.context = this;
//     functions.create = create;
//     functions.listen = listen;
//     functions.connect = connect;
//     functions.poll = poll;
//     functions.write = write;
//     functions.pauseRead = pauseRead;
//     functions.close = close;
//     functions.shutdown = shutdown;
//     functions.destroy = destroy;
//     functions.localPort = localPort;
//     functions.peerCount = peerCount;
//     functions.entropy = entropy;
//     return functions;
// }
//
// int32_t NetHostSocketTestDriver::create(void* providerContext, void* observerContext, NetHostConnected connected,
//     NetHostData data, NetHostWritable writable, NetHostClosed closed, void** instance)
// {
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(providerContext);
//     *instance = nullptr;
//     if ((driver->m_fault & 1) != 0) { throw 1; }
//     if (driver->m_createFail)
//     {
//         return -1;
//     }
//     driver->m_observer = observerContext;
//     driver->m_connected = connected;
//     driver->m_data = data;
//     driver->m_writable = writable;
//     driver->m_closed = closed;
//     *instance = driver;
//     return 0;
// }
//
// int32_t NetHostSocketTestDriver::listen(void* instance, const char* address, uint32_t port, uint32_t maxPeers)
// {
//     (void)address;
//     (void)port;
//     (void)maxPeers;
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     if ((driver->m_fault & 2) != 0) { throw 1; }
//     driver->m_active = true;
//     return 0;
// }
//
// int32_t NetHostSocketTestDriver::connect(void* instance, const char* address, uint32_t port, uint64_t* peer)
// {
//     (void)address;
//     (void)port;
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     if ((driver->m_fault & 4) != 0) { throw 1; }
//     *peer = 1;
//     driver->m_active = true;
//     if (driver->m_syncConnect)
//     {
//         driver->m_connected(driver->m_observer, 1);
//     }
//     return 0;
// }
//
// int32_t NetHostSocketTestDriver::poll(void* instance)
// {
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     if ((driver->m_fault & 8) != 0) { throw 1; }
//     return driver->m_active ? 1 : 0;
// }
//
// int32_t NetHostSocketTestDriver::write(void* instance, uint64_t peer, const uint8_t* data, uint32_t size)
// {
//     (void)peer;
//     (void)data;
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     if ((driver->m_fault & 16) != 0) { throw 1; }
//     if (driver->m_writeResult == 0)
//     {
//         driver->m_written += size;
//     }
//     return driver->m_writeResult;
// }
//
// int32_t NetHostSocketTestDriver::pauseRead(void* instance, uint64_t peer, int32_t paused)
// {
//     (void)peer;
//     (void)paused;
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     ++driver->m_pauses;
//     if ((driver->m_fault & 32) != 0) { throw 1; }
//     if (paused != 0 && driver->m_closeOnPause)
//     {
//         driver->m_closeOnPause = false;
//         driver->m_closed(driver->m_observer, peer, 0);
//     }
//     if (paused == 0 && driver->m_dataOnResume != 0)
//     {
//         --driver->m_dataOnResume;
//         uint8_t bytes[64];
//         std::memset(bytes, static_cast<uint8_t>(driver->m_dataOnResume), sizeof(bytes));
//         driver->m_data(driver->m_observer, peer, bytes, sizeof(bytes));
//         std::memset(bytes, 0, sizeof(bytes));
//     }
//     return driver->m_pauseResult;
// }
//
// int32_t NetHostSocketTestDriver::close(void* instance, uint64_t peer, int32_t reason)
// {
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     if ((driver->m_fault & 64) != 0) { throw 1; }
//     driver->m_closed(driver->m_observer, peer, reason);
//     return 0;
// }
//
// int32_t NetHostSocketTestDriver::shutdown(void* instance)
// {
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     driver->m_active = false;
//     ++driver->m_stops;
//     if ((driver->m_fault & 128) != 0) { throw 1; }
//     return 0;
// }
//
// int32_t NetHostSocketTestDriver::destroy(void* instance)
// {
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     ++driver->m_destroys;
//     if ((driver->m_fault & 256) != 0) { throw 1; }
//     if (driver->m_destroyFailures != 0)
//     {
//         --driver->m_destroyFailures;
//         return -1;
//     }
//     return 0;
// }
//
// uint32_t NetHostSocketTestDriver::localPort(void* instance)
// {
//     (void)instance;
//     if ((static_cast<NetHostSocketTestDriver*>(instance)->m_fault & 512) != 0) { throw 1; }
//     return 14327;
// }
//
// uint32_t NetHostSocketTestDriver::peerCount(void* instance)
// {
//     return static_cast<NetHostSocketTestDriver*>(instance)->m_active ? 1 : 0;
// }
//
// int32_t NetHostSocketTestDriver::entropy(void* context, uint8_t* output, uint32_t size)
// {
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(context);
//     if (driver->m_entropyMode != 0)
//     {
//         if (size != 0)
//         {
//             output[0] = 0x7c;
//         }
//         if (driver->m_entropyMode == 2)
//         {
//             throw 1;
//         }
//         return -1;
//     }
//     std::memset(output, 0x6d, size);
//     return 0;
// }
//
// #include "NetPeer/NetRelayTransportTest.h"
// #include <algorithm>
// #if defined(_WIN32) && defined(_DEBUG)
// #include <crtdbg.h>
// #endif
//
// NetRelayTransportTestDriver::NetRelayTransportTestDriver() :
// m_capture(true)
// {
//
// }
//
// NetHostFunctions NetRelayTransportTestDriver::relayFunctions()
// {
//     NetHostFunctions result = functions();
//     result.write = capture;
//     return result;
// }
//
// int32_t NetRelayTransportTestDriver::capture(void* instance, uint64_t peer, const uint8_t* data, uint32_t size)
// {
//     const int32_t result = NetHostSocketTestDriver::write(instance, peer, data, size);
//     NetRelayTransportTestDriver* driver = static_cast<NetRelayTransportTestDriver*>(instance);
//     if (result == 0 && driver->m_capture)
//     {
//         driver->m_wire.insert(driver->m_wire.end(), data, data + size);
//     }
//     return result;
// }
//
// NetRelayTransportTestListener::NetRelayTransportTestListener() :
// m_relay(nullptr),
// m_mode(0),
// m_reject(0),
// m_connected(0),
// m_records(0),
// m_attempts(0),
// m_closed(0),
// m_reason(0),
// m_valid(true)
// {
//
// }
//
// void NetRelayTransportTestListener::onRelayConnected(uint64_t peer)
// {
//     ++m_connected;
//     m_relay->poll();
//     if (m_mode == 1)
//     {
//         m_relay->shutdown();
//     }
//     else if (m_mode == 2)
//     {
//         m_relay->close(peer, NI_CANCELLED);
//         m_relay->close(peer, NI_TIMEOUT);
//     }
//     else if (m_mode == 3)
//     {
//         throw 1;
//     }
// }
//
// bool NetRelayTransportTestListener::onRelayRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
// {
//     ++m_attempts;
//     if (m_reject != 0)
//     {
//         if (m_rejected.empty())
//         {
//             m_rejected.assign(record.m_data, record.m_data + record.m_size);
//         }
//         m_valid = m_valid && m_rejected.size() == record.m_size &&
//             std::memcmp(m_rejected.data(), record.m_data, record.m_size) == 0;
//         --m_reject;
//         return false;
//     }
//     ++m_records;
//     if (record.m_size != 0)
//     {
//         m_received.insert(m_received.end(), record.m_data, record.m_data + record.m_size);
//     }
//     m_relay->poll();
//     if (m_mode == 4)
//     {
//         m_relay->shutdown();
//     }
//     else if (m_mode == 5)
//     {
//         m_relay->close(peer, NI_CANCELLED);
//         m_relay->close(peer, NI_TIMEOUT);
//     }
//     else if (m_mode == 6)
//     {
//         throw 1;
//     }
//     return true;
// }
//
// void NetRelayTransportTestListener::onRelayClosed(uint64_t peer, int32_t reason)
// {
//     ++m_closed;
//     m_reason = reason;
//     m_valid = m_valid && m_relay->outputState(peer) == NI_INVALID_STATE;
//     m_relay->poll();
//     m_relay->close(peer);
//     if (m_mode == 7)
//     {
//         m_relay->shutdown();
//         throw 1;
//     }
// }
//
// #include "NetInterface/NetInterfaceAPI.h"
// #include <cstdio>
// #include <cstring>
// #include <vector>
// #if defined(_WIN32) && defined(_DEBUG)
// #include <crtdbg.h>
// #endif
//
// NetInterfaceLinkTestIdentity::NetInterfaceLinkTestIdentity() :
// m_seed(0x47b9af13)
// {
//
// }
//
// NetInterfaceLinkTestIdentity::~NetInterfaceLinkTestIdentity()
// {
//     NetInterface::wipe(m_key.empty() ? nullptr : reinterpret_cast<uint8_t*>(&m_key[0]), static_cast<uint32_t>(m_key.size()));
// }
//
// bool NetInterfaceLinkTestIdentity::load(const char* path)
// {
//     FILE* file = std::fopen(path, "rb");
//     if (file == nullptr)
//     {
//         return false;
//     }
//     uint8_t bytes[24577] = {};
//     const size_t size = std::fread(bytes, 1, sizeof(bytes), file);
//     const bool readable = std::ferror(file) == 0 && size >= 48 && size <= 24576 && std::memcmp(bytes, "NGI3", 4) == 0;
//     std::fclose(file);
//     bool valid = false;
//     if (readable)
//     {
//         uint32_t lengths[3] = {};
//         for (uint32_t index = 0; index < 3; ++index)
//         {
//             for (uint32_t offset = 0; offset < 4; ++offset)
//             {
//                 lengths[index] = (lengths[index] << 8) | bytes[4 + index * 4 + offset];
//             }
//         }
//         uint8_t digest[32] = {};
//         if (lengths[0] == 37 && lengths[1] > 0 && lengths[1] <= 4096 && lengths[2] > 0 && lengths[2] <= 16384 &&
//             static_cast<uint64_t>(lengths[0]) + lengths[1] + lengths[2] + 48 == size &&
//             NetInterface::sha256(bytes, static_cast<uint32_t>(size) - 32, digest, sizeof(digest)) == NI_OK &&
//             std::memcmp(bytes + size - 32, digest, sizeof(digest)) == 0)
//         {
//             try
//             {
//                 m_name.assign(reinterpret_cast<const char*>(bytes + 16), lengths[0]);
//                 m_key.assign(reinterpret_cast<const char*>(bytes + 16 + lengths[0]), lengths[1]);
//                 m_certificate.assign(reinterpret_cast<const char*>(bytes + 16 + lengths[0] + lengths[1]), lengths[2]);
//                 valid = true;
//             }
//             catch (...)
//             {
//                 valid = false;
//             }
//         }
//     }
//     NetInterface::wipe(bytes, sizeof(bytes));
//     return valid;
// }
//
// int32_t NetInterfaceLinkTestIdentity::entropy(void* context, uint8_t* output, uint32_t size)
// {
//     NetInterfaceLinkTestIdentity* identity = static_cast<NetInterfaceLinkTestIdentity*>(context);
//     for (uint32_t index = 0; index < size; ++index)
//     {
//         identity->m_seed ^= identity->m_seed << 13;
//         identity->m_seed ^= identity->m_seed >> 17;
//         identity->m_seed ^= identity->m_seed << 5;
//         output[index] = static_cast<uint8_t>(identity->m_seed);
//     }
//     return 0;
// }
//
// NetInterfaceLinkOptions NetInterfaceLinkTestIdentity::options(bool server)
// {
//     NetInterfaceLinkOptions value = {};
//     value.m_version = 3;
//     value.m_role = server ? NI_LINK_NODE : NI_LINK_CLIENT;
//     value.m_name = server ? m_name.c_str() : nullptr;
//     value.m_peerRoles = server ? 1U << NI_LINK_CLIENT : 0;
//     value.m_tls.server = server ? 1 : 0;
//     value.m_tls.handshakeTimeoutMs = 5000;
//     value.m_tls.certificate = server ? reinterpret_cast<const uint8_t*>(m_certificate.c_str()) : nullptr;
//     value.m_tls.certificateSize = server ? static_cast<uint32_t>(m_certificate.size() + 1) : 0;
//     value.m_tls.privateKey = server ? reinterpret_cast<const uint8_t*>(m_key.c_str()) : nullptr;
//     value.m_tls.privateKeySize = server ? static_cast<uint32_t>(m_key.size() + 1) : 0;
//     value.m_tls.entropy = entropy;
//     value.m_tls.entropyContext = this;
//     return value;
// }
//
// NetInterfaceLinkTestPair::NetInterfaceLinkTestPair(NetInterfaceLinkTestIdentity& identity) :
// m_client(identity.options(false)),
// m_server(identity.options(true))
// {
//
// }
//
// bool NetInterfaceLinkTestPair::handshake()
// {
//     for (uint32_t step = 0; step < 10000; ++step)
//     {
//         if (m_client.progress(1000) < 0 || m_server.progress(1000) < 0 ||
//             move(m_client, m_server) < 0 || move(m_server, m_client) < 0)
//         {
//             return false;
//         }
//         if (m_client.status() == NI_READY && m_server.status() == NI_READY)
//         {
//             return true;
//         }
//     }
//     return false;
// }
//
// int32_t NetInterfaceLinkTestPair::move(NetInterfaceLink& from, NetInterfaceLink& to)
// {
//     NetInterfaceTunnelRecord record = {};
//     const int32_t ready = from.peekOutput(record);
//     if (ready != NI_READY)
//     {
//         return ready;
//     }
//     const int32_t accepted = to.accept(record);
//     if (accepted != NI_OK)
//     {
//         return accepted == NI_LIMIT_EXCEEDED ? NI_NEED_MORE : accepted;
//     }
//     return from.consumeOutput();
// }
//
//
// #include "NetPeer/NetLinkTransportAuditTest.h"
//
//
// NetLinkTransportAuditDriver::NetLinkTransportAuditDriver() : m_paused(false), m_seed(0x837a391b)
//{
//
//}

// int32_t NetLinkTransportAuditDriver::pause(void* instance,uint64_t peer,int32_t paused)
// {
//     static_cast<NetLinkTransportAuditDriver*>(instance)->m_paused=paused!=0;
//     return NetHostSocketTestDriver::pauseRead(instance,peer,paused);
// }
// int32_t NetLinkTransportAuditDriver::random(void* context,uint8_t* output,uint32_t size)
// {
//     NetLinkTransportAuditDriver* driver=static_cast<NetLinkTransportAuditDriver*>(context);
//     for(uint32_t i=0;i<size;++i)
//     {
//         driver->m_seed^=driver->m_seed<<13;
//         driver->m_seed^=driver->m_seed>>17;
//         driver->m_seed^=driver->m_seed<<5;
//         output[i]=static_cast<uint8_t>(driver->m_seed);
//     }
//     return 0;
// }
// NetLinkTransportAuditListener::NetLinkTransportAuditListener() : m_ready(0),m_control(0),m_records(0),m_closed(0),m_reason(0),m_client(nullptr),m_mode(0),m_attempts(0),m_valid(true)
//{
//
//}

// void NetLinkTransportAuditListener::onLinkReady(uint64_t,const NetInterfaceLinkPeer&)
//{
//    ++m_ready;
//}

// bool NetLinkTransportAuditListener::onLinkControl(uint64_t peer, const NetInterfaceFrame& frame)
// {
//     (void)peer;
//     const uint8_t expected[] = {5, 4, 3, 2, 1};
//     m_valid = m_valid && frame.m_messageType == 991 && frame.m_payloadSize == sizeof(expected) &&
//         std::memcmp(frame.m_payload, expected, sizeof(expected)) == 0;
//     ++m_control;
//     m_client->poll();
//     if (m_mode == 4)
//     {
//         m_client->shutdown();
//     }
//     return true;
// }
//
// bool NetLinkTransportAuditListener::onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
// {
//     ++m_attempts;
//     const uint8_t expected[] = {5, 4, 3, 2, 1};
//     m_valid = m_valid && record.m_kind == NI_TUNNEL_BYTES && record.m_product == 7 && record.m_channel == 1 &&
//         record.m_size == sizeof(expected) && std::memcmp(record.m_data, expected, sizeof(expected)) == 0;
//     if (m_mode == 3 && m_attempts <= 3)
//     {
//         return false;
//     }
//     ++m_records;
//     m_client->poll();
//     if (m_mode == 5)
//     {
//         m_client->close(peer, NI_CANCELLED);
//     }
//     if (m_mode == 6)
//     {
//         throw 1;
//     }
//     return true;
// }
//
// void NetLinkTransportAuditListener::onLinkClosed(uint64_t peer, int32_t reason)
// {
//     ++m_closed;
//     m_reason = reason;
//     m_valid = m_valid && m_client->outputState(peer) == NI_INVALID_STATE;
//     m_client->poll();
//     m_client->close(peer);
//     if (m_mode == 8)
//     {
//         throw 1;
//     }
// }
// #define PROOF_CHECK(c) do { if(!(c)){std::printf("FAIL line=%d\n",__LINE__);return 1;} }while(0)
// int main(int argc, char** argv)
// {
//     NetInterfaceLinkTestIdentity identity;
//     PROOF_CHECK(identity.load(argc > 1 ? argv[1] : "/gateway-test-identity.dat"));
//     NetLinkTransportAuditDriver driver;
//     driver.m_syncConnect=true;
//     NetHostFunctions functions=driver.relayFunctions();
//     functions.pauseRead=NetLinkTransportAuditDriver::pause;
//     functions.entropy=NetLinkTransportAuditDriver::random;
//     PROOF_CHECK(NetPeer::configureHost(functions)==NI_OK);
//     for(uint32_t mode=0;mode<10;++mode)
//     {
//         driver.m_wire.clear();
//         driver.m_paused=false;
//         NetInterfaceLinkOptions serverOptions = identity.options(true);
//         NetInterfaceLink server(serverOptions);
//         PROOF_CHECK(server.status()>=0);
//         NetInterfaceLinkOptions options={};
//         options.m_version=3;
//         options.m_role=NI_LINK_CLIENT;
//         options.m_tls.handshakeTimeoutMs=5000;
//         NetLinkTransportAuditListener listener;
//         NetLinkTransport client;
//         listener.m_client = &client;
//         listener.m_mode = mode;
//         uint64_t id=0;
//         PROOF_CHECK(client.connect("127.0.0.1",1,options,&listener,id)==NI_OK);
//         NetInterfaceTunnel decoder;
//         std::vector<uint8_t> remote;
//         uint32_t sent=0;
//         uint32_t offset=0;
//         for(uint32_t spin=0;spin<2000;++spin)
//         {
//             client.poll();
//             PROOF_CHECK(listener.m_closed==0);
//             for(uint32_t step=0;step<128;++step)
//             {
//                 NetInterfaceTunnelRecord record={};
//                 int32_t result=decoder.peek(record);
//                 if(result==NI_READY)
//                 {
//                     PROOF_CHECK(server.progress(1)>=0);
//                     result=server.accept(record);
//                     if(result==NI_LIMIT_EXCEEDED){break;}
//                     PROOF_CHECK(result==NI_OK);
//                     PROOF_CHECK(decoder.consume()==NI_OK);
//                 }
//                 else if(offset<driver.m_wire.size())
//                 {
//                     uint32_t used=0;
//                     result=decoder.feed(driver.m_wire.data()+offset,static_cast<uint32_t>(driver.m_wire.size())-offset,used);
//                     PROOF_CHECK(result>=0);
//                     offset+=used;
//                 }
//                 else{break;}
//             }
//             PROOF_CHECK(server.progress(1)>=0);
//             for(uint32_t step=0;step<64;++step)
//             {
//                 NetInterfaceTunnelRecord record={};
//                 int32_t result=server.peekOutput(record);
//                 if(result!=NI_READY){PROOF_CHECK(result==NI_NEED_MORE);break;}
//                 uint8_t bytes[16408];uint32_t required=0;
//                 PROOF_CHECK(NetInterfaceTunnel::encode(record,bytes,sizeof(bytes),required)==NI_OK);
//                 remote.insert(remote.end(),bytes,bytes+required);
//                 PROOF_CHECK(server.consumeOutput()==NI_OK);
//             }
//             if(!driver.m_paused&&sent<remote.size())
//             {
//                 uint32_t size=(std::min)(16384U,static_cast<uint32_t>(remote.size())-sent);
//                 driver.m_data(driver.m_observer,id,remote.data()+sent,size);
//                 sent+=size;
//             }
//             if(listener.m_ready==1&&server.status()==NI_READY&&sent==remote.size()&&offset==driver.m_wire.size()){break;}
//         }
//         PROOF_CHECK(listener.m_ready==1&&server.status()==NI_READY);
//         client.poll();
//         const uint8_t payload[]={5,4,3,2,1};
//         if(mode!=1)
//         {
//             NetInterfaceTunnelRecord record={};record.m_kind=NI_TUNNEL_BYTES;record.m_product=7;
//             record.m_channel=1;record.m_data=payload;record.m_size=sizeof(payload);
//             PROOF_CHECK(server.sendRecord(record)==NI_OK);
//         }
//         if(mode!=0)
//         {
//             NetInterfaceFrame frame={};frame.m_kind=NI_EVENT;frame.m_messageType=991;
//             frame.m_payload=payload;frame.m_payloadSize=sizeof(payload);
//             PROOF_CHECK(server.sendControl(frame)==NI_OK);
//         }
//         remote.clear();
//         PROOF_CHECK(server.progress(1)>=0);
//         for(uint32_t step=0;step<64;++step)
//         {
//             NetInterfaceTunnelRecord record={};
//             int32_t result=server.peekOutput(record);
//             if(result!=NI_READY){PROOF_CHECK(result==NI_NEED_MORE);break;}
//             uint8_t bytes[16408];uint32_t required=0;
//             PROOF_CHECK(NetInterfaceTunnel::encode(record,bytes,sizeof(bytes),required)==NI_OK);
//             remote.insert(remote.end(),bytes,bytes+required);
//             PROOF_CHECK(server.consumeOutput()==NI_OK);
//         }
//         PROOF_CHECK(!remote.empty()&&remote.size()<16384&&!driver.m_paused);
//         if (mode == 9)
//         {
//             const uint8_t partial[] = {'N', 'T', 'W'};
//             // 同一个原始输入块中，完整记录后追加半个外层头，再报告EOF。
//             remote.insert(remote.end(), partial, partial + sizeof(partial));
//         }
//         driver.m_data(driver.m_observer,id,remote.data(),static_cast<uint32_t>(remote.size()));
//         driver.m_closed(driver.m_observer,id,mode == 7 ? -1234 : 0);
//         for(uint32_t spin=0;spin<16;++spin){client.poll();}
//         std::printf("mode=%u ready=%u records=%u controls=%u closed=%u reason=%d\n",mode,listener.m_ready,
//             listener.m_records,listener.m_control,listener.m_closed,listener.m_reason);
//         const uint32_t expectedRecords = mode == 1 || mode == 4 ? 0U : 1U;
//         const uint32_t expectedControls = mode == 0 ? 0U : 1U;
//         const int32_t expectedReason = mode == 4 ? NI_OK : mode == 5 ? NI_CANCELLED : mode == 6 ? NI_NO_MEMORY :
//             mode == 7 ? NI_INVALID_STATE : NI_TRUNCATED;
//         PROOF_CHECK(listener.m_valid && listener.m_closed == 1 && listener.m_records == expectedRecords &&
//             listener.m_control == expectedControls && listener.m_reason == expectedReason);
//         PROOF_CHECK(mode != 3 || listener.m_attempts == 4);
//     }
//     std::printf("Link transport input drain PASS\n");return 0;
// }
// NET_LINK_TRANSPORT_UNIT_END
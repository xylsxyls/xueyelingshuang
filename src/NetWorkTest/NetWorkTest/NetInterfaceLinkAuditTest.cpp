#include "NetInterface/NetInterfaceAPI.h"
#include "NetLinkTestCases.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetInterfaceTlsTestCases.h"

NetInterfaceLinkTestIdentity::NetInterfaceLinkTestIdentity() :
m_seed(0x47b9af13)
{

}

NetInterfaceLinkTestIdentity::~NetInterfaceLinkTestIdentity()
{
    NetInterface::wipe(m_key.empty() ? nullptr : reinterpret_cast<uint8_t*>(&m_key[0]), static_cast<uint32_t>(m_key.size()));
}

bool NetInterfaceLinkTestIdentity::load(const char* path)
{
    FILE* file = std::fopen(path, "rb");
    if (file == nullptr)
    {
        return false;
    }
    uint8_t bytes[24577] = {};
    const size_t size = std::fread(bytes, 1, sizeof(bytes), file);
    const bool readable = std::ferror(file) == 0 && size >= 48 && size <= 24576 && std::memcmp(bytes, "NGI3", 4) == 0;
    std::fclose(file);
    bool valid = false;
    if (readable)
    {
        uint32_t lengths[3] = {};
        for (uint32_t index = 0; index < 3; ++index)
        {
            for (uint32_t offset = 0; offset < 4; ++offset)
            {
                lengths[index] = (lengths[index] << 8) | bytes[4 + index * 4 + offset];
            }
        }
        uint8_t digest[32] = {};
        if (lengths[0] == 37 && lengths[1] > 0 && lengths[1] <= 4096 && lengths[2] > 0 && lengths[2] <= 16384 &&
            static_cast<uint64_t>(lengths[0]) + lengths[1] + lengths[2] + 48 == size &&
            NetInterface::sha256(bytes, static_cast<uint32_t>(size) - 32, digest, sizeof(digest)) == NI_OK &&
            std::memcmp(bytes + size - 32, digest, sizeof(digest)) == 0)
        {
            try
            {
                m_name.assign(reinterpret_cast<const char*>(bytes + 16), lengths[0]);
                m_key.assign(reinterpret_cast<const char*>(bytes + 16 + lengths[0]), lengths[1]);
                m_certificate.assign(reinterpret_cast<const char*>(bytes + 16 + lengths[0] + lengths[1]), lengths[2]);
                valid = true;
            }
            catch (...)
            {
                valid = false;
            }
        }
    }
    NetInterface::wipe(bytes, sizeof(bytes));
    return valid;
}

int32_t NetInterfaceLinkTestIdentity::entropy(void* context, uint8_t* output, uint32_t size)
{
    NetInterfaceLinkTestIdentity* identity = static_cast<NetInterfaceLinkTestIdentity*>(context);
    for (uint32_t index = 0; index < size; ++index)
    {
        identity->m_seed ^= identity->m_seed << 13;
        identity->m_seed ^= identity->m_seed >> 17;
        identity->m_seed ^= identity->m_seed << 5;
        output[index] = static_cast<uint8_t>(identity->m_seed);
    }
    return 0;
}

NetInterfaceLinkOptions NetInterfaceLinkTestIdentity::options(bool server)
{
    NetInterfaceLinkOptions value = {};
    value.m_version = 3;
    value.m_role = server ? NI_LINK_NODE : NI_LINK_CLIENT;
    value.m_name = server ? m_name.c_str() : nullptr;
    value.m_peerRoles = server ? 1U << NI_LINK_CLIENT : 0;
    value.m_tls.server = server ? 1 : 0;
    value.m_tls.handshakeTimeoutMs = 5000;
    value.m_tls.certificate = server ? reinterpret_cast<const uint8_t*>(m_certificate.c_str()) : nullptr;
    value.m_tls.certificateSize = server ? static_cast<uint32_t>(m_certificate.size() + 1) : 0;
    value.m_tls.privateKey = server ? reinterpret_cast<const uint8_t*>(m_key.c_str()) : nullptr;
    value.m_tls.privateKeySize = server ? static_cast<uint32_t>(m_key.size() + 1) : 0;
    value.m_tls.entropy = entropy;
    value.m_tls.entropyContext = this;
    return value;
}

NetInterfaceLinkTestPair::NetInterfaceLinkTestPair(NetInterfaceLinkTestIdentity& identity) :
m_client(identity.options(false)),
m_server(identity.options(true))
{

}

bool NetInterfaceLinkTestPair::handshake()
{
    for (uint32_t step = 0; step < 10000; ++step)
    {
        if (m_client.progress(1000) < 0 || m_server.progress(1000) < 0 ||
            move(m_client, m_server) < 0 || move(m_server, m_client) < 0)
        {
            return false;
        }
        if (m_client.status() == NI_READY && m_server.status() == NI_READY)
        {
            return true;
        }
    }
    return false;
}

int32_t NetInterfaceLinkTestPair::move(NetInterfaceLink& from, NetInterfaceLink& to)
{
    NetInterfaceTunnelRecord record = {};
    const int32_t ready = from.peekOutput(record);
    if (ready != NI_READY)
    {
        return ready;
    }
    const int32_t accepted = to.accept(record);
    if (accepted != NI_OK)
    {
        return accepted == NI_LIMIT_EXCEEDED ? NI_NEED_MORE : accepted;
    }
    return from.consumeOutput();
}
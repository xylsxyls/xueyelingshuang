#include "NetGatewayAuthorityTest.h"
#include "../../NetServerManager/NetServerManager/NetGatewayAuthority.h"
#include "NetGatewayTestAccess.h"
#include <mbedtls/md.h>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <utility>

/** 只使用操作系统强随机的身份自测适配
@param [in] context 忽略
@param [out] output 强随机字节
@param [in] size 字节数
@return 0成功，非0失败
*/
static int32_t GatewayIdentityEntropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    return NetTransport::randomBytes(output, size) == NI_OK ? 0 : -1;
}

/** 自测断言不输出证书或私钥
@param [in] condition 条件
@param [in] message 固定诊断文案
*/
static void GatewayIdentityRequire(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

/** 构造真实身份自测配置
@param [in] server 是否服务端
@param [in] name 精确逻辑身份
@return 安全TLS配置
*/
static NetInterfaceTlsOptions GatewayIdentityOptions(bool server, const char* name)
{
    NetInterfaceTlsOptions options = {};
    options.server = server ? 1 : 0;
    options.entropy = GatewayIdentityEntropy;
    options.handshakeTimeoutMs = 5000;
    if (!server)
    {
        options.authority = NetInterface::trustAuthority(&options.authoritySize);
        options.hostname = name;
    }
    return options;
}

/** 自测只搬运TLS密文，端点负责认证
@param [in,out] source 发送端
@param [in,out] target 接收端
@param [in] fragment 最大分片
@return 实际传输
*/
static uint32_t GatewayIdentityTransfer(NetInterfaceTls& source, NetInterfaceTls& target, uint32_t fragment)
{
    const uint8_t* data = nullptr;
    uint32_t size = 0;
    GatewayIdentityRequire(source.peekOutput(data, size) >= 0, "identity TLS output error");
    size = (std::min)(size, fragment);
    uint32_t consumed = 0;
    if (size != 0)
    {
        GatewayIdentityRequire(target.feed(data, size, consumed) >= 0, "identity TLS input error");
        GatewayIdentityRequire(source.consumeOutput(consumed) >= 0, "identity TLS consume error");
    }
    return consumed;
}

int32_t NetGatewayAuthorityTest::identity()
{

    std::string previousKey;
    for (uint32_t generation = 0; generation < 2; ++generation)
    {

        std::string key;
        std::string request;
        std::string certificate;
        GatewayIdentityRequire(NetInterfaceCertificate::createRequest("service.1", GatewayIdentityEntropy,
            nullptr, key, request) == NI_OK, "create endpoint identity failed");
        GatewayIdentityRequire(key != previousKey, "leaf rotation reused private key");
        previousKey = key;
        GatewayIdentityRequire(NetGatewayAuthority::issueLocalIdentity("service.1", request,
            GatewayIdentityEntropy, nullptr, certificate) == NI_OK, "authorized local issue failed");
        GatewayIdentityRequire(NetInterfaceCertificate::verifyPair(certificate, key, "service.1",
            GatewayIdentityEntropy, nullptr) == NI_OK, "signed certificate/key do not match trusted identity");
        GatewayIdentityRequire(NetInterfaceCertificate::verify(certificate, "service.2") != NI_OK,
            "certificate authenticated a different product");
        const uint8_t challenge[] = {0x31, 0x00, 0x73, 0x28, 0x82, 0xff};
        std::vector<uint8_t> proof;
        GatewayIdentityRequire(NetInterfaceCertificate::signProof(key, challenge, sizeof(challenge),
            GatewayIdentityEntropy, nullptr, proof) == NI_OK &&
            NetInterfaceCertificate::verifyProof(certificate, "service.1", challenge, sizeof(challenge), proof.data(),
                static_cast<uint32_t>(proof.size())) == NI_OK, "endpoint identity possession proof failed");
        const uint8_t differentChallenge[] = {0x32, 0x00, 0x73, 0x28, 0x82, 0xff};
        GatewayIdentityRequire(NetInterfaceCertificate::verifyProof(certificate, "service.1", differentChallenge,
            sizeof(differentChallenge), proof.data(), static_cast<uint32_t>(proof.size())) != NI_OK,
            "identity proof accepted for a different challenge");
        proof.back() ^= 1;
        GatewayIdentityRequire(NetInterfaceCertificate::verifyProof(certificate, "service.1", challenge,
            sizeof(challenge), proof.data(), static_cast<uint32_t>(proof.size())) != NI_OK, "tampered identity proof accepted");
        std::string otherKey;
        std::string otherRequest;
        GatewayIdentityRequire(NetInterfaceCertificate::createRequest("service.1", GatewayIdentityEntropy,
            nullptr, otherKey, otherRequest) == NI_OK && NetInterfaceCertificate::verifyPair(certificate, otherKey, "service.1",
            GatewayIdentityEntropy, nullptr) != NI_OK, "mismatched private key accepted");
        std::string rejected;
        GatewayIdentityRequire(NetGatewayAuthority::issueLocalIdentity("node.evil", request,
            GatewayIdentityEntropy, nullptr, rejected) != NI_OK && rejected.empty(),
            "CSR promoted a service identity to a node");
        std::string changed = request;
        const size_t position = changed.rfind("\n-----END");
        GatewayIdentityRequire(position > 5, "unexpected test CSR format");
        changed[position - 5] = changed[position - 5] == 'A' ? 'B' : 'A';
        GatewayIdentityRequire(NetGatewayAuthority::issueLocalIdentity("service.1", changed,
            GatewayIdentityEntropy, nullptr, rejected) != NI_OK && rejected.empty(),
            "CSR without private-key proof was signed");
        NetInterfaceTlsOptions server = GatewayIdentityOptions(true, "service.1");
        server.certificate = reinterpret_cast<const uint8_t*>(certificate.c_str());
        server.certificateSize = static_cast<uint32_t>(certificate.size() + 1);
        server.privateKey = reinterpret_cast<const uint8_t*>(key.c_str());
        server.privateKeySize = static_cast<uint32_t>(key.size() + 1);
        NetInterfaceTlsOptions client = GatewayIdentityOptions(false, "service.1");
        client.authority = NetInterface::trustAuthority(&client.authoritySize);
        NetInterfaceTls left(client);
        NetInterfaceTls right(server);
        for (uint32_t step = 0; step < 50000 &&
            (left.status() != NET_INTERFACE_TLS_READY || right.status() != NET_INTERFACE_TLS_READY); ++step)
        {

            left.progress(1);
            right.progress(1);
            GatewayIdentityTransfer(left, right, 17);
            GatewayIdentityTransfer(right, left, 31);
            GatewayIdentityRequire(left.status() >= 0 && right.status() >= 0, "rotated leaf TLS handshake failed");
        }
        GatewayIdentityRequire(left.status() == NET_INTERFACE_TLS_READY && right.status() == NET_INTERFACE_TLS_READY,
            "TLS handshake did not become ready");
        const uint8_t plain[] = {0x01, 0x00, 0x7f, 0xff, 0x34};
        uint32_t written = 0;
        GatewayIdentityRequire(left.write(plain, sizeof(plain), written) >= 0 && written == sizeof(plain),
            "end-to-end TLS write failed");
        uint8_t received[32] = {};
        uint32_t count = 0;
        for (uint32_t step = 0; step < 1000 && count == 0; ++step)
        {
            left.progress(1);
            right.progress(1);
            GatewayIdentityTransfer(left, right, 7);
            GatewayIdentityTransfer(right, left, 11);
            GatewayIdentityRequire(right.read(received, sizeof(received), count) >= 0, "TLS read failed");
        }
        GatewayIdentityRequire(count == sizeof(plain) && std::memcmp(plain, received, count) == 0,
            "rotated endpoint TLS plaintext mismatch");
    }

    return NI_OK;
}

/** 身份管理自测的暂时凭证，仅在本次函数中随机生成，不是生产注册凭证
@param [in] context pair<本次32字节秘密指针,验证计数>
@param [in] statement 完整双挑战声明
@param [in] size 声明字节数
@param [out] output 32字节HMAC
@param [in] capacity 输出容量
@return NI_OK或错误
*/
static int32_t GatewayLinkTestProof(void* context, const uint8_t* statement, uint32_t size, uint8_t* output, uint32_t capacity)
{
    if (context == nullptr || capacity < 32) { return NI_INVALID_ARGUMENT; }
    std::pair<const uint8_t*, uint32_t>* state = static_cast<std::pair<const uint8_t*, uint32_t>*>(context);
    const unsigned char domain[] = "NetInterface service enrollment v3";
    mbedtls_md_context_t hash;
    mbedtls_md_init(&hash);
    int result = mbedtls_md_setup(&hash, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    if (result == 0) { result = mbedtls_md_hmac_starts(&hash, state->first, 32); }
    if (result == 0) { result = mbedtls_md_hmac_update(&hash, domain, sizeof(domain)); }
    if (result == 0) { result = mbedtls_md_hmac_update(&hash, statement, size); }
    if (result == 0) { result = mbedtls_md_hmac_finish(&hash, output); }
    mbedtls_md_free(&hash);
    return result == 0 ? NI_OK : NI_PROTOCOL_ERROR;
}

/** 只用于本机注册自测，生产宿主须单独按传输角色安装验证器
@param [in] context 本次凭证和验证计数
@param [in] peer 规范角色/名称/CSR
@param [in] statement 双挑战声明
@param [in] statementSize 声明长度
@param [in] proof HMAC证明
@param [in] proofSize 32
@param [out] grant 签发公开证书
@param [in] capacity 容量
@param [out] grantSize 实际证书字节数
@return NI_OK或拒绝，不输出秘密
*/
static int32_t GatewayLinkTestVerify(void* context, const NetInterfaceLinkPeer* peer, const uint8_t* statement,
    uint32_t statementSize, const uint8_t* proof, uint32_t proofSize, uint8_t* grant, uint32_t capacity, uint32_t* grantSize)
{
    std::pair<const uint8_t*, uint32_t>* state = static_cast<std::pair<const uint8_t*, uint32_t>*>(context);
    ++state->second;
    *grantSize = 0;
    uint8_t expected[32] = {};
    if (peer == nullptr || peer->m_role != NI_LINK_SERVICE || std::strcmp(peer->m_name, "service.7") != 0 || proofSize != 32 ||
        GatewayLinkTestProof(context, statement, statementSize, expected, sizeof(expected)) != NI_OK)
    {
        return NI_PROTOCOL_ERROR;
    }
    uint8_t difference = 0;
    for (uint32_t index = 0; index < 32; ++index) { difference |= expected[index] ^ proof[index]; }
    if (difference != 0) { return NI_PROTOCOL_ERROR; }
    const std::string request(reinterpret_cast<const char*>(peer->m_claims), peer->m_claimsSize);
    std::string certificate;
    if (NetGatewayAuthority::issueLocalIdentity(peer->m_name, request, GatewayIdentityEntropy, nullptr, certificate) != NI_OK ||
        certificate.size() > capacity)
    {
        return NI_PROTOCOL_ERROR;
    }
    std::memcpy(grant, certificate.data(), certificate.size());
    *grantSize = static_cast<uint32_t>(certificate.size());
    return NI_OK;
}

/** 只复制外层字节；不访问任何业务会话密钥
@param [in,out] from 输出端
@param [in,out] to 输入端
@return 接纳记录数
*/
static uint32_t GatewayLinkTransfer(NetInterfaceLink& from, NetInterfaceLink& to)
{
    uint32_t count = 0;
    for (uint32_t step = 0; step < 8; ++step)
    {
        NetInterfaceTunnelRecord record = {};
        if (from.peekOutput(record) != NI_READY) { break; }
        const int32_t accepted = to.accept(record);
        if (accepted == NI_LIMIT_EXCEEDED) { break; }
        GatewayIdentityRequire(accepted == NI_OK && from.consumeOutput() == NI_OK, "management link transfer failed");
        ++count;
    }
    return count;
}

int32_t NetGatewayAuthorityTest::management()
{
    const std::string serverName = "node.0123456789abcdef0123456789abcdef";
    std::string serverKey;
    std::string serverCsr;
    std::string serverCert;
    GatewayIdentityRequire(NetInterfaceCertificate::createRequest(serverName, GatewayIdentityEntropy, nullptr,
        serverKey, serverCsr) == NI_OK && NetGatewayAuthority::issueLocalIdentity(serverName, serverCsr, GatewayIdentityEntropy, nullptr, serverCert) == NI_OK,
        "create test gateway management identity failed");
    uint8_t credential[32] = {};
    uint8_t badCredential[32] = {};
    GatewayIdentityRequire(GatewayIdentityEntropy(nullptr, credential, sizeof(credential)) == 0 &&
        GatewayIdentityEntropy(nullptr, badCredential, sizeof(badCredential)) == 0, "management test entropy failed");
    for (uint32_t scenario = 0; scenario < 5; ++scenario)
    {
        std::pair<const uint8_t*, uint32_t> serverProof(credential, 0);
        std::pair<const uint8_t*, uint32_t> clientProof(scenario == 4 ? badCredential : credential, 0);
        NetInterfaceLinkOptions server = {};
        server.m_version = 3;
        server.m_role = NI_LINK_NODE;
        server.m_name = serverName.c_str();
        server.m_peerRoles = (1U << NI_LINK_CLIENT) | (1U << NI_LINK_NODE);
        server.m_tls = GatewayIdentityOptions(true, "node.gateway");
        server.m_tls.certificate = reinterpret_cast<const uint8_t*>(serverCert.c_str());
        server.m_tls.certificateSize = static_cast<uint32_t>(serverCert.size() + 1);
        server.m_tls.privateKey = reinterpret_cast<const uint8_t*>(serverKey.c_str());
        server.m_tls.privateKeySize = static_cast<uint32_t>(serverKey.size() + 1);
        if (scenario >= 3) { server.m_peerRoles = 1U << NI_LINK_SERVICE; }
        server.m_serviceVerify = GatewayLinkTestVerify;
        server.m_serviceContext = &serverProof;
        NetInterfaceLinkOptions client = {};
        client.m_version = 3;
        client.m_role = scenario == 0 ? NI_LINK_CLIENT : scenario == 1 ? NI_LINK_NODE : NI_LINK_SERVICE;
        client.m_tls = GatewayIdentityOptions(false, "node.gateway");
        std::string clientName = scenario == 1 ? "node.fedcba9876543210fedcba9876543210" : "service.7";
        std::string clientKey;
        std::string clientCsr;
        std::string clientCert;
        if (scenario != 0)
        {
            GatewayIdentityRequire(NetInterfaceCertificate::createRequest(clientName, GatewayIdentityEntropy, nullptr,
                clientKey, clientCsr) == NI_OK, "create test peer identity failed");
            client.m_name = clientName.c_str();
            client.m_tls.privateKey = reinterpret_cast<const uint8_t*>(clientKey.c_str());
            client.m_tls.privateKeySize = static_cast<uint32_t>(clientKey.size() + 1);
            if (scenario == 1)
            {
                GatewayIdentityRequire(NetGatewayAuthority::issueLocalIdentity(clientName, clientCsr, GatewayIdentityEntropy, nullptr, clientCert) == NI_OK,
                    "create test peer node certificate failed");
                client.m_tls.certificate = reinterpret_cast<const uint8_t*>(clientCert.c_str());
                client.m_tls.certificateSize = static_cast<uint32_t>(clientCert.size() + 1);
            }
            else
            {
                client.m_claims = reinterpret_cast<const uint8_t*>(clientCsr.data());
                client.m_claimsSize = static_cast<uint32_t>(clientCsr.size());
                client.m_serviceProof = GatewayLinkTestProof;
                client.m_serviceContext = &clientProof;
            }
        }
        NetInterfaceLink left(client);
        NetInterfaceLink right(server);
        GatewayIdentityRequire(left.status() >= 0 && right.status() >= 0, "configure v3 management link failed");
        for (uint32_t step = 0; step < 10000 && (left.status() != NI_READY || right.status() != NI_READY); ++step)
        {
            left.progress(1);
            right.progress(1);
            if (left.status() < 0 || right.status() < 0) { break; }
            GatewayLinkTransfer(left, right);
            if (right.status() >= 0) { GatewayLinkTransfer(right, left); }
        }
        if (scenario == 2 || scenario == 4)
        {
            GatewayIdentityRequire(right.status() < 0 && left.status() != NI_READY, "untrusted service became ready");
            GatewayIdentityRequire(serverProof.second == (scenario == 2 ? 0U : 1U), "public transport invoked service signer");
            continue;
        }
        GatewayIdentityRequire(left.status() == NI_READY && right.status() == NI_READY, "v3 management authentication not ready");
        NetInterfaceLinkPeer peer = {};
        GatewayIdentityRequire(right.peerInfo(peer) == NI_READY && peer.m_role == client.m_role,
            "management role not authenticated");
        if (scenario == 3)
        {
            const uint8_t* grant = nullptr;
            uint32_t size = 0;
            GatewayIdentityRequire(left.serviceGrant(grant, size) == NI_READY && size != 0 && serverProof.second == 1 &&
                NetInterfaceCertificate::verifyPair(std::string(reinterpret_cast<const char*>(grant), size), clientKey, clientName,
                    GatewayIdentityEntropy, nullptr) == NI_OK, "enrollment did not return the matching trusted leaf");
        }
        const uint8_t bytes[] = {0x16, 0x03, 0x03, 0x00, 0xff};
        NetInterfaceTunnelRecord record = {};
        record.m_kind = NI_TUNNEL_BYTES;
        record.m_product = 7;
        record.m_channel = UINT64_C(0x1234567887654321);
        record.m_data = bytes;
        record.m_size = sizeof(bytes);
        GatewayIdentityRequire(left.sendRecord(record) == NI_OK, "opaque business record refused after management authentication");
        for (uint32_t step = 0; step < 20; ++step)
        {
            left.progress(2);
            right.progress(2);
            GatewayLinkTransfer(left, right);
            GatewayLinkTransfer(right, left);
        }
        NetInterfaceTunnelRecord received = {};
        GatewayIdentityRequire(right.peekRecord(received) == NI_READY && received.m_channel == record.m_channel &&
            received.m_size == sizeof(bytes) && std::memcmp(received.m_data, bytes, sizeof(bytes)) == 0 && right.consumeRecord() == NI_OK,
            "management link changed opaque business bytes");
        NetInterfaceFrame control = {};
        control.m_kind = NI_REQUEST;
        control.m_messageType = 0x70000001;
        control.m_requestId = UINT64_C(0x1234567887654321);
        control.m_payload = bytes;
        control.m_payloadSize = sizeof(bytes);
        GatewayIdentityRequire(left.sendControl(control) == NI_OK, "authorized control send failed");
        for (uint32_t step = 0; step < 20; ++step)
        {
            left.progress(3);
            right.progress(3);
            GatewayLinkTransfer(left, right);
            GatewayLinkTransfer(right, left);
        }
        NetInterfaceFrame actual = {};
        GatewayIdentityRequire(right.peekControl(actual) == NI_READY && actual.m_requestId == control.m_requestId &&
            actual.m_payloadSize == sizeof(bytes) && std::memcmp(actual.m_payload, bytes, sizeof(bytes)) == 0 &&
            right.consumeControl() == NI_OK, "authenticated control was corrupted");
        for (uint32_t step = 0; step < 30; ++step)
        {
            left.progress(16000);
            right.progress(16000);
            GatewayLinkTransfer(left, right);
            GatewayLinkTransfer(right, left);
        }
        GatewayIdentityRequire(left.status() == NI_READY && right.status() == NI_READY, "idle management heartbeat failed");
        GatewayIdentityRequire(left.progress(2) < 0, "management link accepted a regressing clock");
    }
    return NI_OK;
}
#include "NetInterfaceMacro.h"
#include "NetInterfaceLinkState.h"
#include "NetInterfaceCertificate.h"
#include "NetInterface.h"
#include "NetInterface.h"
#include "NetInterfaceLink.h"
#include "NetInterfaceConnection.h"
#include <algorithm>
#include <cstring>

/** 管理声明固定大端整数
@param [in] data 至少4字节
@return uint32
*/
static uint32_t LinkRead(const uint8_t* data)
{
    return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) |
        (static_cast<uint32_t>(data[2]) << 8) | data[3];
}

/** 管理声明固定大端整数
@param [out] data 至少4字节
@param [in] value 数值
*/
static void LinkWrite(uint8_t* data, uint32_t value)
{
    for (uint32_t index = 0; index < 4; ++index)
    {
        data[3 - index] = static_cast<uint8_t>(value);
        value >>= 8;
    }
}

/** 有界复制PEM，不接受内部NUL
@param [in] data PEM字节
@param [in] size 允许包括结尾NUL
@param [in] maximum 上限
@param [out] value 不含NUL的副本
@return 是否合法
*/
static bool LinkPem(const uint8_t* data, uint32_t size, uint32_t maximum, std::string& value)
{
    if (data == nullptr || size == 0 || size > maximum)
    {
        return false;
    }
    if (data[size - 1] == 0)
    {
        --size;
    }
    if (size == 0 || std::memchr(data, 0, size) != nullptr)
    {
        return false;
    }
    value.assign(reinterpret_cast<const char*>(data), size);
    return true;
}

/** 只允许规范的服务编号名称，防止同编号的歧义拼写
@param [in] name 逻辑名称
@return 是否合法
*/
static bool LinkServiceName(const std::string& name)
{
    if (name.compare(0, 8, "service.") != 0 || name.size() < 9 || name.size() > 18 ||
        (name.size() > 9 && name[8] == '0') || name.find_first_not_of("0123456789", 8) != std::string::npos)
    {
        return false;
    }
    uint64_t value = 0;
    for (size_t index = 8; index < name.size(); ++index)
    {
        value = value * 10 + static_cast<uint32_t>(name[index] - '0');
    }
    return value <= UINT32_MAX;
}

/** 构造双挑战绑定的规范签名声明，签名长度字段在签名前固定为0
@param [in] magic 4字节类型
@param [in] role 声明/接受的角色
@param [in] nonce 服务端挑战32字节
@param [in] clientNonce 客户端新随机数32字节
@param [in] name 精确身份
@param [in] certificate 身份公证书
@param [in] claims 全部声明或接受结果
@return 不含签名的字节
*/
static std::vector<uint8_t> LinkStatement(const char* magic, uint32_t role, const uint8_t* nonce, const uint8_t* clientNonce,
    const std::string& name, const std::string& certificate, const std::vector<uint8_t>& claims)
{
    std::vector<uint8_t> body(88);
    std::memcpy(body.data(), magic, 4);
    LinkWrite(body.data() + 4, role);
    LinkWrite(body.data() + 8, static_cast<uint32_t>(name.size()));
    LinkWrite(body.data() + 12, static_cast<uint32_t>(certificate.size()));
    LinkWrite(body.data() + 16, static_cast<uint32_t>(claims.size()));
    LinkWrite(body.data() + 20, 0);
    std::memcpy(body.data() + 24, nonce, 32);
    std::memcpy(body.data() + 56, clientNonce, 32);
    body.insert(body.end(), name.begin(), name.end());
    body.insert(body.end(), certificate.begin(), certificate.end());
    body.insert(body.end(), claims.begin(), claims.end());
    return body;
}

NetInterfaceLinkWrite::NetInterfaceLinkWrite() :
m_record()
{

}

NetInterfaceLinkWrite::~NetInterfaceLinkWrite()
{
    if (m_bytes)
    {
        NetInterface::wipe(m_bytes.get(), m_record.m_size);
    }
}

NetInterfaceLinkState::NetInterfaceLinkState() :
m_options(),
m_peerRole(0),
m_inputOffset(0),
m_received(),
m_recordReady(false),
m_outputHead(0),
m_outputCount(0),
m_outputBytes(0),
m_spareSize(0),
m_peeked(),
m_outputPeeked(false),
m_phase(NET_LINK_TLS),
m_error(NI_OK),
m_clockStarted(false),
m_started(0),
m_lastTime(0),
m_activity(0),
m_pingTime(0),
m_pingId(0),
m_receivedHello(false),
m_sentHello(false),
m_drainStarted(0),
m_drainResult(NI_OK)
{
    std::memset(m_nonce, 0, sizeof(m_nonce));
    std::memset(m_clientNonce, 0, sizeof(m_clientNonce));
}

NetInterfaceLinkState::~NetInterfaceLinkState()
{
    if (!m_privateKey.empty())
    {
        NetInterface::wipe(reinterpret_cast<uint8_t*>(&m_privateKey[0]), static_cast<uint32_t>(m_privateKey.size()));
    }
    NetInterface::wipe(m_nonce, sizeof(m_nonce));
    NetInterface::wipe(m_clientNonce, sizeof(m_clientNonce));
    if (m_spare)
    {
        NetInterface::wipe(m_spare.get(), m_spareSize);
    }
}

int32_t NetInterfaceLinkState::queueRecord(const NetInterfaceTunnelRecord& record)
{
    if (m_outputCount == 32 || record.m_size > 65536 - m_outputBytes)
    {
        return NI_LIMIT_EXCEEDED;
    }
    try
    {
        std::unique_ptr<uint8_t[]> bytes;
        if (m_spare && m_spareSize == record.m_size)
        {
            bytes.swap(m_spare);
            m_spareSize = 0;
        }
        else
        {
            bytes.reset(new uint8_t[record.m_size]);
        }
        std::memcpy(bytes.get(), record.m_data, record.m_size);
        NetInterfaceLinkWrite& slot = m_output[(m_outputHead + m_outputCount) % 32];
        slot.m_record = record;
        slot.m_record.m_data = nullptr;
        slot.m_bytes.swap(bytes);
        m_outputBytes += record.m_size;
        ++m_outputCount;
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

void NetInterfaceLinkState::consumeQueuedRecord()
{
    NetInterfaceLinkWrite& slot = m_output[m_outputHead];
    // 这里只有不透明密文；密钥/管理明文仍由各自拥有者执行安全擦除。
    std::memset(slot.m_bytes.get(), 0, slot.m_record.m_size);
    m_spare = std::move(slot.m_bytes);
    m_spareSize = slot.m_record.m_size;
    m_outputBytes -= slot.m_record.m_size;
    slot.m_record = NetInterfaceTunnelRecord();
    m_outputHead = (m_outputHead + 1) % 32;
    --m_outputCount;
}

int32_t NetInterfaceLinkState::fail(int32_t error)
{
    if (m_phase != NET_LINK_FAILED)
    {
        m_error = error < 0 ? error : NI_PROTOCOL_ERROR;
        m_phase = NET_LINK_FAILED;
    }
    return m_error;
}

int32_t NetInterfaceLinkState::status() const
{
    if (m_phase == NET_LINK_FAILED)
    {
        return m_error;
    }
    return m_phase == NET_LINK_READY || m_phase == NET_LINK_DRAINING ? NI_READY : NI_NEED_MORE;
}

int32_t NetInterfaceLinkState::configure(const NetInterfaceLinkOptions& options)
{
    const uint32_t roles = (1U << NI_LINK_CLIENT) | (1U << NI_LINK_NODE) | (1U << NI_LINK_SERVICE);
    if (options.m_version != 3 || options.m_role < NI_LINK_CLIENT || options.m_role > NI_LINK_SERVICE ||
        (options.m_tls.server != 0 && options.m_tls.server != 1) || options.m_tls.entropy == nullptr ||
        options.m_claimsSize > 4096 || (options.m_claimsSize != 0 && options.m_claims == nullptr) ||
        (options.m_tls.server != 0 && (options.m_role != NI_LINK_NODE || options.m_peerRoles == 0 ||
            (options.m_peerRoles & ~roles) != 0 || ((options.m_peerRoles & (1U << NI_LINK_SERVICE)) != 0 && options.m_serviceVerify == nullptr))) ||
        (options.m_tls.server == 0 && options.m_peerRoles != 0))
    {
        return fail(NI_INVALID_ARGUMENT);
    }
    m_options = options;
    if (options.m_name != nullptr)
    {
        uint32_t size = 0;
        while (size <= 96 && options.m_name[size] != 0)
        {
            ++size;
        }
        if (size > 96)
        {
            return fail(NI_INVALID_ARGUMENT);
        }
        m_name.assign(options.m_name, size);
    }
    if (options.m_role == NI_LINK_CLIENT)
    {
        if (!m_name.empty())
        {
            return fail(NI_INVALID_ARGUMENT);
        }
    }
    else
    {
        if (!NetInterfaceCertificate::validName(m_name) ||
            (options.m_role == NI_LINK_NODE && m_name.compare(0, 5, "node.") != 0) ||
            (options.m_role == NI_LINK_SERVICE && (!LinkServiceName(m_name) || options.m_serviceProof == nullptr)) ||
            !LinkPem(options.m_tls.privateKey, options.m_tls.privateKeySize, 4096, m_privateKey))
        {
            return fail(NI_INVALID_ARGUMENT);
        }
        if (options.m_role == NI_LINK_NODE)
        {
            if (!LinkPem(options.m_tls.certificate, options.m_tls.certificateSize, 16384, m_certificate) ||
                NetInterfaceCertificate::verifyPair(m_certificate, m_privateKey, m_name, options.m_tls.entropy,
                    options.m_tls.entropyContext) != NI_OK)
            {
                return fail(NI_PROTOCOL_ERROR);
            }
        }
    }
    if (options.m_claimsSize != 0)
    {
        m_claims.assign(options.m_claims, options.m_claims + options.m_claimsSize);
    }
    NetInterfaceTlsOptions tls = options.m_tls;
    tls.authority = NetInterface::trustAuthority(&tls.authoritySize);
    if (tls.server == 0)
    {
        tls.hostname = "node.gateway";
        tls.certificate = nullptr;
        tls.certificateSize = 0;
        tls.privateKey = nullptr;
        tls.privateKeySize = 0;
    }
    else
    {
        tls.certificate = reinterpret_cast<const uint8_t*>(m_certificate.c_str());
        tls.certificateSize = static_cast<uint32_t>(m_certificate.size() + 1);
        tls.privateKey = reinterpret_cast<const uint8_t*>(m_privateKey.c_str());
        tls.privateKeySize = static_cast<uint32_t>(m_privateKey.size() + 1);
    }
    m_control.reset(new NetInterfaceConnection(tls, 16384));
    return m_control->status() < 0 ? fail(m_control->status()) : NI_NEED_MORE;
}

int32_t NetInterfaceLinkState::handshakeFrame(uint16_t kind, uint32_t type, const std::vector<uint8_t>& body)
{
    if (body.size() > 16384)
    {
        return NI_LIMIT_EXCEEDED;
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = kind;
    frame.m_messageType = type;
    frame.m_payload = body.empty() ? nullptr : body.data();
    frame.m_payloadSize = static_cast<uint32_t>(body.size());
    return m_control->send(frame);
}

int32_t NetInterfaceLinkState::prove()
{
    if (NetInterface::entropyBytes(m_options.m_tls.entropy, m_options.m_tls.entropyContext, m_clientNonce, sizeof(m_clientNonce)) != NI_OK)
    {
        return fail(NI_INVALID_STATE);
    }
    std::vector<uint8_t> body = LinkStatement("NLH3", m_options.m_role, m_nonce, m_clientNonce, m_name, m_certificate, m_claims);
    std::vector<uint8_t> signature;
    if (m_options.m_role == NI_LINK_NODE)
    {
        if (NetInterfaceCertificate::signProof(m_privateKey, body.data(), static_cast<uint32_t>(body.size()),
            m_options.m_tls.entropy, m_options.m_tls.entropyContext, signature) != NI_OK)
        {
            return fail(NI_PROTOCOL_ERROR);
        }
    }
    else if (m_options.m_role == NI_LINK_SERVICE)
    {
        signature.resize(32);
        if (m_options.m_serviceProof(m_options.m_serviceContext, body.data(), static_cast<uint32_t>(body.size()),
            signature.data(), static_cast<uint32_t>(signature.size())) != NI_OK)
        {
            return fail(NI_PROTOCOL_ERROR);
        }
    }
    LinkWrite(body.data() + 20, static_cast<uint32_t>(signature.size()));
    body.insert(body.end(), signature.begin(), signature.end());
    const int32_t result = handshakeFrame(NI_HELLO, NET_INTERFACE_LINK_PROOF, body);
    if (result != NI_OK)
    {
        return fail(result);
    }
    m_phase = NET_LINK_ACCEPTANCE;
    return NI_OK;
}

int32_t NetInterfaceLinkState::verifyProof(const NetInterfaceFrame& frame)
{
    if (frame.m_kind != NI_HELLO || frame.m_messageType != NET_INTERFACE_LINK_PROOF || frame.m_productId != 0 ||
        frame.m_requestId != 0 || frame.m_payloadSize < 88 || std::memcmp(frame.m_payload, "NLH3", 4) != 0 ||
        std::memcmp(frame.m_payload + 24, m_nonce, 32) != 0)
    {
        return fail(NI_PROTOCOL_ERROR);
    }
    const uint32_t role = LinkRead(frame.m_payload + 4);
    const uint32_t nameSize = LinkRead(frame.m_payload + 8);
    const uint32_t certSize = LinkRead(frame.m_payload + 12);
    const uint32_t claimSize = LinkRead(frame.m_payload + 16);
    const uint32_t sigSize = LinkRead(frame.m_payload + 20);
    if (role < NI_LINK_CLIENT || role > NI_LINK_SERVICE || (m_options.m_peerRoles & (1U << role)) == 0 ||
        nameSize > 96 || certSize > 16384 || claimSize > 4096 || sigSize > 80 ||
        static_cast<uint64_t>(nameSize) + certSize + claimSize + sigSize + 88 != frame.m_payloadSize)
    {
        return fail(NI_PROTOCOL_ERROR);
    }
    const uint8_t* name = frame.m_payload + 88;
    const uint8_t* cert = name + nameSize;
    const uint8_t* claims = cert + certSize;
    const uint8_t* signature = claims + claimSize;
    std::string peerName(reinterpret_cast<const char*>(name), nameSize);
    std::string peerCert(reinterpret_cast<const char*>(cert), certSize);
    std::vector<uint8_t> statement(frame.m_payload, signature);
    LinkWrite(statement.data() + 20, 0);
    std::vector<uint8_t> grant;
    if (role == NI_LINK_CLIENT)
    {
        if (nameSize != 0 || certSize != 0 || sigSize != 0)
        {
            return fail(NI_PROTOCOL_ERROR);
        }
    }
    else if (role == NI_LINK_NODE)
    {
        if (peerName.compare(0, 5, "node.") != 0 || NetInterfaceCertificate::verifyProof(peerCert, peerName,
            statement.data(), static_cast<uint32_t>(statement.size()), signature, sigSize) != NI_OK)
        {
            return fail(NI_PROTOCOL_ERROR);
        }
    }
    else
    {
        if (!LinkServiceName(peerName) || certSize != 0 || sigSize != 32 || m_options.m_serviceVerify == nullptr)
        {
            return fail(NI_PROTOCOL_ERROR);
        }
        NetInterfaceLinkPeer peer = {};
        peer.m_role = role;
        peer.m_name = peerName.c_str();
        peer.m_claims = claims;
        peer.m_claimsSize = claimSize;
        uint8_t result[8192] = {};
        uint32_t resultSize = 0;
        if (m_options.m_serviceVerify(m_options.m_serviceContext, &peer, statement.data(), static_cast<uint32_t>(statement.size()),
            signature, sigSize, result, sizeof(result), &resultSize) != NI_OK || resultSize == 0 || resultSize > sizeof(result))
        {
            return fail(NI_PROTOCOL_ERROR);
        }
        grant.assign(result, result + resultSize);
    }
    std::memcpy(m_clientNonce, frame.m_payload + 56, sizeof(m_clientNonce));
    std::vector<uint8_t> accepted = LinkStatement("NLA3", role, m_nonce, m_clientNonce, m_name, m_certificate, grant);
    std::vector<uint8_t> proof;
    if (NetInterfaceCertificate::signProof(m_privateKey, accepted.data(), static_cast<uint32_t>(accepted.size()),
        m_options.m_tls.entropy, m_options.m_tls.entropyContext, proof) != NI_OK)
    {
        return fail(NI_PROTOCOL_ERROR);
    }
    LinkWrite(accepted.data() + 20, static_cast<uint32_t>(proof.size()));
    accepted.insert(accepted.end(), proof.begin(), proof.end());
    const int32_t sent = handshakeFrame(NI_HELLO_REPLY, NET_INTERFACE_LINK_ACCEPT, accepted);
    if (sent != NI_OK)
    {
        return fail(sent);
    }
    m_peerName.swap(peerName);
    m_peerCertificate.swap(peerCert);
    m_peerClaims.assign(claims, claims + claimSize);
    m_peerRole = role;
    m_phase = NET_LINK_READY;
    return NI_OK;
}

int32_t NetInterfaceLinkState::verifyAcceptance(const NetInterfaceFrame& frame)
{
    if (frame.m_kind != NI_HELLO_REPLY || frame.m_messageType != NET_INTERFACE_LINK_ACCEPT || frame.m_productId != 0 ||
        frame.m_requestId != 0 || frame.m_payloadSize < 88 || std::memcmp(frame.m_payload, "NLA3", 4) != 0 ||
        LinkRead(frame.m_payload + 4) != m_options.m_role || std::memcmp(frame.m_payload + 24, m_nonce, 32) != 0 ||
        std::memcmp(frame.m_payload + 56, m_clientNonce, 32) != 0)
    {
        return fail(NI_PROTOCOL_ERROR);
    }
    const uint32_t nameSize = LinkRead(frame.m_payload + 8);
    const uint32_t certSize = LinkRead(frame.m_payload + 12);
    const uint32_t grantSize = LinkRead(frame.m_payload + 16);
    const uint32_t sigSize = LinkRead(frame.m_payload + 20);
    if (nameSize > 96 || certSize > 16384 || grantSize > 8192 || sigSize > 80 ||
        static_cast<uint64_t>(nameSize) + certSize + grantSize + sigSize + 88 != frame.m_payloadSize)
    {
        return fail(NI_PROTOCOL_ERROR);
    }
    const uint8_t* name = frame.m_payload + 88;
    const uint8_t* cert = name + nameSize;
    const uint8_t* grant = cert + certSize;
    const uint8_t* signature = grant + grantSize;
    std::string peerName(reinterpret_cast<const char*>(name), nameSize);
    std::string peerCert(reinterpret_cast<const char*>(cert), certSize);
    std::vector<uint8_t> statement(frame.m_payload, signature);
    LinkWrite(statement.data() + 20, 0);
    if (peerName.compare(0, 5, "node.") != 0 || NetInterfaceCertificate::verifyProof(peerCert, peerName, statement.data(),
        static_cast<uint32_t>(statement.size()), signature, sigSize) != NI_OK)
    {
        return fail(NI_PROTOCOL_ERROR);
    }
    if (m_options.m_role == NI_LINK_SERVICE)
    {
        const std::string certificate(reinterpret_cast<const char*>(grant), grantSize);
        if (NetInterfaceCertificate::verifyPair(certificate, m_privateKey, m_name, m_options.m_tls.entropy,
            m_options.m_tls.entropyContext) != NI_OK)
        {
            return fail(NI_PROTOCOL_ERROR);
        }
        m_grant.assign(grant, grant + grantSize);
    }
    else if (grantSize != 0)
    {
        return fail(NI_PROTOCOL_ERROR);
    }
    m_peerRole = NI_LINK_NODE;
    m_peerName.swap(peerName);
    m_peerCertificate.swap(peerCert);
    m_phase = NET_LINK_READY;
    return NI_OK;
}

int32_t NetInterfaceLinkState::progress(uint64_t now)
{
    if (m_phase == NET_LINK_FAILED)
    {
        return m_error;
    }
    if (!m_clockStarted)
    {
        m_clockStarted = true;
        m_started = now;
        m_activity = now;
    }
    else if (now < m_lastTime)
    {
        return fail(NI_INVALID_ARGUMENT);
    }
    m_lastTime = now;
    if (m_phase == NET_LINK_DRAINING)
    {
        if (now - m_drainStarted >= 5000)
        {
            return fail(NI_TIMEOUT);
        }
        if (m_drainResult < 0)
        {
            return m_recordReady ? NI_READY : fail(m_drainResult);
        }
    }
    if (status() != NI_READY && now - m_started >= m_options.m_tls.handshakeTimeoutMs)
    {
        return fail(NI_TIMEOUT);
    }
    for (uint32_t step = 0; step < 64; ++step)
    {
        if (m_inputOffset < m_input.size())
        {
            uint32_t consumed = 0;
            const int32_t fed = m_control->feed(m_input.data() + m_inputOffset,
                static_cast<uint32_t>(m_input.size()) - m_inputOffset, consumed);
            if (fed < 0)
            {
                return fail(fed);
            }
            m_inputOffset += consumed;
        }
        if (m_inputOffset == m_input.size())
        {
            m_input.clear();
            m_inputOffset = 0;
        }
        if (m_phase == NET_LINK_DRAINING && m_input.empty())
        {
            // 最后一个外层块先全部交给TLS；不能因外层队列空就认定TLS内部也已排空。
            m_control->finishInput();
        }
        const int32_t tlsStatus = m_control->progress(now);
        if (tlsStatus < 0 || tlsStatus == NET_INTERFACE_TLS_CLOSED)
        {
            const int32_t terminal = tlsStatus == NET_INTERFACE_TLS_CLOSED || tlsStatus == NET_INTERFACE_TLS_TRUNCATED ? NI_TRUNCATED : tlsStatus;
            if (m_phase != NET_LINK_DRAINING)
            {
                return fail(tlsStatus == NET_INTERFACE_TLS_CLOSED ? NI_TRUNCATED : tlsStatus);
            }
            m_drainResult = terminal;
            return m_recordReady ? NI_READY : fail(terminal);
        }
        if (tlsStatus != NET_INTERFACE_TLS_READY)
        {
            break;
        }
        if (m_phase == NET_LINK_TLS)
        {
            if (m_options.m_tls.server != 0)
            {
                if (NetInterface::entropyBytes(m_options.m_tls.entropy, m_options.m_tls.entropyContext, m_nonce, sizeof(m_nonce)) != NI_OK)
                {
                    return fail(NI_INVALID_STATE);
                }
                const std::vector<uint8_t> challenge(m_nonce, m_nonce + sizeof(m_nonce));
                const int32_t sent = handshakeFrame(NI_HELLO, NET_INTERFACE_LINK_CHALLENGE, challenge);
                if (sent != NI_OK)
                {
                    return fail(sent);
                }
                m_phase = NET_LINK_PROOF;
            }
            else
            {
                m_phase = NET_LINK_CHALLENGE;
            }
        }
        NetInterfaceFrame frame = {};
        const int32_t peeked = m_control->peek(frame);
        if (peeked < 0)
        {
            return fail(peeked);
        }
        if (peeked != NI_READY)
        {
            break;
        }
        if (m_phase == NET_LINK_PROOF || m_phase == NET_LINK_CHALLENGE || m_phase == NET_LINK_ACCEPTANCE)
        {
            if (m_phase == NET_LINK_PROOF)
            {
                if (verifyProof(frame) < 0)
                {
                    return status();
                }
            }
            else if (m_phase == NET_LINK_CHALLENGE)
            {
                if (frame.m_kind != NI_HELLO || frame.m_messageType != NET_INTERFACE_LINK_CHALLENGE ||
                    frame.m_productId != 0 || frame.m_requestId != 0 || frame.m_payloadSize != sizeof(m_nonce))
                {
                    return fail(NI_PROTOCOL_ERROR);
                }
                std::memcpy(m_nonce, frame.m_payload, sizeof(m_nonce));
                if (prove() < 0)
                {
                    return status();
                }
            }
            else if (verifyAcceptance(frame) < 0)
            {
                return status();
            }
            if (m_control->consume() != NI_OK)
            {
                return fail(NI_INVALID_STATE);
            }
            m_activity = now;
        }
        else if ((frame.m_kind == NI_PING || frame.m_kind == NI_PONG) && frame.m_messageType == NET_INTERFACE_LINK_HEARTBEAT &&
            frame.m_productId == 0 && frame.m_requestId != 0 && frame.m_payloadSize == 0)
        {
            if (frame.m_kind == NI_PING && m_phase != NET_LINK_DRAINING)
            {
                NetInterfaceFrame pong = frame;
                pong.m_kind = NI_PONG;
                const int32_t sent = m_control->send(pong);
                if (sent == NI_LIMIT_EXCEEDED)
                {
                    break;
                }
                if (sent != NI_OK)
                {
                    return fail(sent);
                }
            }
            else if (frame.m_kind == NI_PONG && (m_pingTime == 0 || frame.m_requestId != m_pingId))
            {
                return fail(NI_PROTOCOL_ERROR);
            }
            else if (frame.m_kind == NI_PONG)
            {
                m_pingTime = 0;
            }
            m_control->consume();
            m_activity = now;
        }
        else
        {
            if (frame.m_kind == NI_HELLO || frame.m_kind == NI_HELLO_REPLY || frame.m_kind == NI_PING || frame.m_kind == NI_PONG ||
                frame.m_productId != 0 || (frame.m_messageType >= NET_INTERFACE_LINK_CHALLENGE &&
                    frame.m_messageType <= NET_INTERFACE_LINK_HEARTBEAT))
            {
                return fail(NI_PROTOCOL_ERROR);
            }
            break;
        }
    }
    if (status() == NI_READY && m_phase != NET_LINK_DRAINING)
    {
        if (m_pingTime != 0 && now - m_pingTime >= 10000)
        {
            return fail(NI_TIMEOUT);
        }
        if (m_pingTime == 0 && now - m_activity >= 15000)
        {
            if (m_pingId == UINT64_MAX)
            {
                return fail(NI_LIMIT_EXCEEDED);
            }
            NetInterfaceFrame ping = {};
            ping.m_kind = NI_PING;
            ping.m_messageType = NET_INTERFACE_LINK_HEARTBEAT;
            ping.m_requestId = m_pingId + 1;
            const int32_t sent = m_control->send(ping);
            if (sent == NI_OK)
            {
                ++m_pingId;
                m_pingTime = now;
            }
            else if (sent != NI_LIMIT_EXCEEDED)
            {
                return fail(sent);
            }
        }
    }
    return status();
}

// NET_LINK_AUTH_UNIT_BEGIN
// #include "NetInterface.h"
// #include "NetInterface/NetInterfaceLinkAuditTest.h"
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
// #include "NetInterface/NetInterfaceLinkAuthTest.h"
//
// NetInterfaceLinkAuthTestPair::NetInterfaceLinkAuthTestPair(NetInterfaceLinkTestIdentity& identity, bool server) :
// m_link(identity.options(server)),
// m_raw(tls(identity, !server), 16384),
// m_offset(0)
// {
//
// }
//
// NetInterfaceTlsOptions NetInterfaceLinkAuthTestPair::tls(NetInterfaceLinkTestIdentity& identity, bool server)
// {
//     NetInterfaceTlsOptions options = identity.options(server).m_tls;
//     options.authority = NetInterface::trustAuthority(&options.authoritySize);
//     options.hostname = server ? nullptr : "node.gateway";
//     return options;
// }
//
// void NetInterfaceLinkAuthTestPair::word(uint8_t* bytes, uint32_t value)
// {
//     bytes[0] = static_cast<uint8_t>(value >> 24);
//     bytes[1] = static_cast<uint8_t>(value >> 16);
//     bytes[2] = static_cast<uint8_t>(value >> 8);
//     bytes[3] = static_cast<uint8_t>(value);
// }
//
// bool NetInterfaceLinkAuthTestPair::start()
// {
//     NetInterfaceTunnelRecord hello = {};
//     hello.m_kind = NI_TUNNEL_HELLO;
//     NetInterfaceTunnelRecord output = {};
//     return m_link.accept(hello) == NI_OK && m_link.peekOutput(output) == NI_READY &&
//         output.m_kind == NI_TUNNEL_HELLO && m_link.consumeOutput() == NI_OK && step() >= 0;
// }
//
// int32_t NetInterfaceLinkAuthTestPair::step()
// {
//     const int32_t status = m_link.progress(1000);
//     if (status < 0)
//     {
//         return status;
//     }
//     if (m_raw.progress(1000) < 0)
//     {
//         return NI_INVALID_STATE;
//     }
//     NetInterfaceTunnelRecord output = {};
//     const int32_t ready = m_link.peekOutput(output);
//     if (ready < 0)
//     {
//         return ready;
//     }
//     if (ready == NI_READY)
//     {
//         if (output.m_kind != NI_TUNNEL_CONTROL || m_offset > output.m_size)
//         {
//             return NI_INVALID_STATE;
//         }
//         uint32_t consumed = 0;
//         if (m_raw.feed(output.m_data + m_offset, output.m_size - m_offset, consumed) < 0)
//         {
//             return NI_INVALID_STATE;
//         }
//         m_offset += consumed;
//         if (m_offset == output.m_size)
//         {
//             if (m_link.consumeOutput() != NI_OK)
//             {
//                 return NI_INVALID_STATE;
//             }
//             m_offset = 0;
//         }
//     }
//     const uint8_t* bytes = nullptr;
//     uint32_t size = 0;
//     if (m_raw.peekOutput(bytes, size) < 0)
//     {
//         return NI_INVALID_STATE;
//     }
//     if (size != 0)
//     {
//         NetInterfaceTunnelRecord record = {};
//         record.m_kind = NI_TUNNEL_CONTROL;
//         record.m_data = bytes;
//         record.m_size = size < 16384 ? size : 16384;
//         const int32_t accepted = m_link.accept(record);
//         if (accepted == NI_OK)
//         {
//             if (m_raw.consumeOutput(record.m_size) != NI_OK)
//             {
//                 return NI_INVALID_STATE;
//             }
//         }
//         else if (accepted != NI_LIMIT_EXCEEDED)
//         {
//             return accepted;
//         }
//     }
//     return m_link.status();
// }
//
// bool NetInterfaceLinkAuthTestPair::receive(uint16_t kind, uint32_t type, std::vector<uint8_t>& body)
// {
//     for (uint32_t index = 0; index < 10000; ++index)
//     {
//         if (step() < 0)
//         {
//             return false;
//         }
//         NetInterfaceFrame frame = {};
//         const int32_t ready = m_raw.peek(frame);
//         if (ready < 0)
//         {
//             return false;
//         }
//         if (ready == NI_READY)
//         {
//             if (frame.m_kind != kind || frame.m_messageType != type || frame.m_productId != 0 || frame.m_requestId != 0)
//             {
//                 return false;
//             }
//             body.assign(frame.m_payload, frame.m_payload + frame.m_payloadSize);
//             return m_raw.consume() == NI_OK;
//         }
//     }
//     return false;
// }
//
// int32_t NetInterfaceLinkAuthTestPair::settle()
// {
//     for (uint32_t index = 0; index < 10000; ++index)
//     {
//         const int32_t status = step();
//         if (status < 0 || status == NI_READY)
//         {
//             return status;
//         }
//     }
//     return NI_NEED_MORE;
// }
//
// #define LINK_AUTH_CHECK(condition) do { ++checks; if (!(condition)) { std::printf("Link auth failed mode=%u line=%d checks=%u\n", mode, __LINE__, checks); return 1; } } while (0)
//
// /** 使用真实TLS构造畸形管理证明；预期独立于认证实现，正文不输出
// @param [in] argc 参数数量
// @param [in] argv 隔离identity.dat路径
// @return 0通过
// */
// int main(int argc, char** argv)
// {
//     uint32_t checks = 0;
//     uint32_t mode = 0;
//     try
//     {
//         NetInterfaceLinkTestIdentity identity;
// #ifdef __EMSCRIPTEN__
//         LINK_AUTH_CHECK(identity.load(argc > 1 ? argv[1] : "/gateway-test-identity.dat"));
// #else
//         LINK_AUTH_CHECK(argc == 2 && identity.load(argv[1]));
// #endif
// #if defined(_WIN32) && defined(_DEBUG)
//         _CrtMemState before;
//         _CrtMemCheckpoint(&before);
// #endif
//         {
//             // 读取待发输出不应偷偷用0启动TLS期限；计时起点属于首次显式progress。
//             {
//                 NetInterfaceLink link(identity.options(false));
//                 NetInterfaceTunnelRecord output = {};
//                 LINK_AUTH_CHECK(link.peekOutput(output) == NI_READY && output.m_kind == NI_TUNNEL_HELLO &&
//                     link.consumeOutput() == NI_OK && link.peekOutput(output) >= 0);
//                 const int32_t firstProgress = link.progress(1000000);
//                 std::printf("Link auth first explicit progress=%d\n", firstProgress);
//                 LINK_AUTH_CHECK(firstProgress == NI_NEED_MORE && link.progress(1004999) == NI_NEED_MORE &&
//                     link.progress(1005000) == NI_TIMEOUT);
//             }
//             std::vector<uint8_t> previousProof;
//             for (mode = 0; mode < 24; ++mode)
//             {
//                 NetInterfaceLinkAuthTestPair pair(identity, true);
//                 LINK_AUTH_CHECK(pair.start());
//                 std::vector<uint8_t> challenge;
//                 LINK_AUTH_CHECK(pair.receive(NI_HELLO, NET_INTERFACE_LINK_CHALLENGE, challenge) && challenge.size() == 32);
//                 std::vector<uint8_t> body(88, 0);
//                 std::memcpy(body.data(), "NLH3", 4);
//                 NetInterfaceLinkAuthTestPair::word(body.data() + 4, NI_LINK_CLIENT);
//                 std::memcpy(body.data() + 24, challenge.data(), 32);
//                 std::memset(body.data() + 56, 0x39, 32);
//                 NetInterfaceFrame frame = {};
//                 frame.m_kind = NI_HELLO;
//                 frame.m_messageType = NET_INTERFACE_LINK_PROOF;
//                 switch (mode)
//                 {
//                 case 1:
//                     body[0] ^= 1;
//                     break;
//                 case 2:
//                     frame.m_kind = NI_EVENT;
//                     break;
//                 case 3:
//                     frame.m_messageType = NET_INTERFACE_LINK_CHALLENGE;
//                     break;
//                 case 4:
//                     frame.m_productId = 7;
//                     break;
//                 case 5:
//                     frame.m_requestId = 1;
//                     break;
//                 case 6:
//                     body.resize(87);
//                     break;
//                 case 7:
//                     body[24] ^= 1;
//                     break;
//                 case 8:
//                     NetInterfaceLinkAuthTestPair::word(body.data() + 4, 0);
//                     break;
//                 case 9:
//                     NetInterfaceLinkAuthTestPair::word(body.data() + 4, NI_LINK_SERVICE);
//                     break;
//                 case 10:
//                     NetInterfaceLinkAuthTestPair::word(body.data() + 4, NI_LINK_NODE);
//                     break;
//                 case 11:
//                     NetInterfaceLinkAuthTestPair::word(body.data() + 8, UINT32_MAX);
//                     break;
//                 case 12:
//                     NetInterfaceLinkAuthTestPair::word(body.data() + 12, UINT32_MAX);
//                     break;
//                 case 13:
//                     NetInterfaceLinkAuthTestPair::word(body.data() + 16, 4097);
//                     break;
//                 case 14:
//                     NetInterfaceLinkAuthTestPair::word(body.data() + 20, 81);
//                     break;
//                 case 15:
//                     NetInterfaceLinkAuthTestPair::word(body.data() + 8, 1);
//                     body.push_back('x');
//                     break;
//                 case 16:
//                     NetInterfaceLinkAuthTestPair::word(body.data() + 12, 1);
//                     body.push_back('x');
//                     break;
//                 case 17:
//                     NetInterfaceLinkAuthTestPair::word(body.data() + 20, 1);
//                     body.push_back('x');
//                     break;
//                 case 18:
//                     NetInterfaceLinkAuthTestPair::word(body.data() + 16, 4);
//                     body.insert(body.end(), 4, 0x2d);
//                     break;
//                 case 19:
//                     body.push_back(0);
//                     break;
//                 case 20:
//                     LINK_AUTH_CHECK(previousProof.size() == 88 && std::memcmp(previousProof.data() + 24, challenge.data(), 32) != 0);
//                     body = previousProof;
//                     break;
//                 case 22:
//                     frame.m_kind = NI_REQUEST;
//                     frame.m_requestId = 1;
//                     frame.m_messageType = 998;
//                     break;
//                 case 23:
//                     frame.m_kind = NI_PING;
//                     frame.m_requestId = 1;
//                     frame.m_messageType = NET_INTERFACE_LINK_HEARTBEAT;
//                     body.clear();
//                     break;
//                 default: break;
//                 }
//                 frame.m_payload = body.empty() ? nullptr : body.data();
//                 frame.m_payloadSize = static_cast<uint32_t>(body.size());
//                 LINK_AUTH_CHECK(pair.m_raw.send(frame) == NI_OK);
//                 const int32_t result = pair.settle();
//                 const bool allowed = mode == 0 || mode == 18 || mode == 21;
//                 LINK_AUTH_CHECK(result == (allowed ? NI_READY : NI_PROTOCOL_ERROR));
//                 NetInterfaceLinkPeer peer = {};
//                 if (allowed)
//                 {
//                     LINK_AUTH_CHECK(pair.m_link.peerInfo(peer) == NI_READY && peer.m_role == NI_LINK_CLIENT &&
//                         peer.m_claimsSize == (mode == 18 ? 4U : 0U));
//                     if (mode == 0)
//                     {
//                         previousProof = body;
//                     }
//                     if (mode == 21)
//                     {
//                         LINK_AUTH_CHECK(pair.m_raw.send(frame) == NI_OK);
//                         int32_t duplicated = NI_READY;
//                         for (uint32_t retry = 0; retry < 100 && duplicated >= 0; ++retry)
//                         {
//                             duplicated = pair.step();
//                         }
//                         LINK_AUTH_CHECK(duplicated == NI_PROTOCOL_ERROR);
//                     }
//                 }
//                 else
//                 {
//                     LINK_AUTH_CHECK(pair.m_link.peerInfo(peer) == NI_PROTOCOL_ERROR && peer.m_role == 0 &&
//                         peer.m_name == nullptr && pair.m_link.progress(2000) == NI_PROTOCOL_ERROR);
//                 }
//             }
//             for (mode = 24; mode < 33; ++mode)
//             {
//                 NetInterfaceLinkAuthTestPair pair(identity, false);
//                 LINK_AUTH_CHECK(pair.start());
//                 for (uint32_t step = 0; step < 10000 && pair.m_raw.status() != NET_INTERFACE_TLS_READY; ++step)
//                 {
//                     LINK_AUTH_CHECK(pair.step() >= 0);
//                 }
//                 LINK_AUTH_CHECK(pair.m_raw.status() == NET_INTERFACE_TLS_READY && pair.m_link.status() == NI_NEED_MORE);
//                 std::vector<uint8_t> challenge(32, 0x47);
//                 NetInterfaceFrame frame = {};
//                 frame.m_kind = NI_HELLO;
//                 frame.m_messageType = NET_INTERFACE_LINK_CHALLENGE;
//                 switch (mode)
//                 {
//                 case 25:
//                     frame.m_requestId = 1;
//                     break;
//                 case 26:
//                     frame.m_productId = 7;
//                     break;
//                 case 27:
//                     frame.m_messageType = NET_INTERFACE_LINK_PROOF;
//                     break;
//                 case 28:
//                     frame.m_kind = NI_EVENT;
//                     break;
//                 case 29:
//                     challenge.resize(31);
//                     break;
//                 case 30:
//                     challenge.push_back(0);
//                     break;
//                 case 31:
//                     frame.m_messageType = 998;
//                     break;
//                 case 32:
//                     frame.m_kind = NI_HELLO_REPLY;
//                     break;
//                 default: break;
//                 }
//                 frame.m_payload = challenge.data();
//                 frame.m_payloadSize = static_cast<uint32_t>(challenge.size());
//                 LINK_AUTH_CHECK(pair.m_raw.send(frame) == NI_OK);
//                 if (mode == 24)
//                 {
//                     std::vector<uint8_t> proof;
//                     LINK_AUTH_CHECK(pair.receive(NI_HELLO, NET_INTERFACE_LINK_PROOF, proof) && proof.size() == 88 &&
//                         std::memcmp(proof.data(), "NLH3", 4) == 0 && std::memcmp(proof.data() + 24, challenge.data(), 32) == 0);
//                     // 正确双随机数仍不够：没有可信节点证书和签名的接受消息必须拒绝。
//                     std::vector<uint8_t> accepted(88, 0);
//                     std::memcpy(accepted.data(), "NLA3", 4);
//                     NetInterfaceLinkAuthTestPair::word(accepted.data() + 4, NI_LINK_CLIENT);
//                     std::memcpy(accepted.data() + 24, challenge.data(), 32);
//                     std::memcpy(accepted.data() + 56, proof.data() + 56, 32);
//                     frame.m_kind = NI_HELLO_REPLY;
//                     frame.m_messageType = NET_INTERFACE_LINK_ACCEPT;
//                     frame.m_payload = accepted.data();
//                     frame.m_payloadSize = static_cast<uint32_t>(accepted.size());
//                     LINK_AUTH_CHECK(pair.m_raw.send(frame) == NI_OK && pair.settle() == NI_PROTOCOL_ERROR);
//                 }
//                 else
//                 {
//                     LINK_AUTH_CHECK(pair.settle() == NI_PROTOCOL_ERROR);
//                 }
//             }
//             // 节点双向身份由实际签名验证；CLIENT不因管理TLS成功就获得NODE身份。
//             mode = 33;
//             {
//                 NetInterfaceLinkOptions serverOptions = identity.options(true);
//                 serverOptions.m_peerRoles = 1U << NI_LINK_NODE;
//                 NetInterfaceLinkOptions clientOptions = identity.options(true);
//                 clientOptions.m_tls.server = 0;
//                 clientOptions.m_peerRoles = 0;
//                 NetInterfaceLink server(serverOptions);
//                 NetInterfaceLink client(clientOptions);
//                 for (uint32_t step = 0; step < 10000; ++step)
//                 {
//                     LINK_AUTH_CHECK(client.progress(1000) >= 0 && server.progress(1000) >= 0 &&
//                         NetInterfaceLinkTestPair::move(client, server) >= 0 && NetInterfaceLinkTestPair::move(server, client) >= 0);
//                     if (client.status() == NI_READY && server.status() == NI_READY)
//                     {
//                         break;
//                     }
//                 }
//                 NetInterfaceLinkPeer peer = {};
//                 LINK_AUTH_CHECK(client.status() == NI_READY && server.peerInfo(peer) == NI_READY && peer.m_role == NI_LINK_NODE &&
//                     std::strcmp(peer.m_name, identity.m_name.c_str()) == 0 && peer.m_certificateSize == identity.m_certificate.size());
//                 LINK_AUTH_CHECK(client.peerInfo(peer) == NI_READY && peer.m_role == NI_LINK_NODE);
//             }
//             // 服务首次接入不能缺宿主验证；非法名称、缺少本机证明均在启动阶段拒绝。
//             mode = 34;
//             {
//                 NetInterfaceLinkOptions options = identity.options(true);
//                 options.m_peerRoles |= 1U << NI_LINK_SERVICE;
//                 NetInterfaceLink noVerifier(options);
//                 LINK_AUTH_CHECK(noVerifier.status() == NI_INVALID_ARGUMENT);
//                 options = identity.options(false);
//                 options.m_role = NI_LINK_SERVICE;
//                 options.m_name = "service.7";
//                 options.m_tls.privateKey = reinterpret_cast<const uint8_t*>(identity.m_key.c_str());
//                 options.m_tls.privateKeySize = static_cast<uint32_t>(identity.m_key.size() + 1);
//                 NetInterfaceLink noProof(options);
//                 LINK_AUTH_CHECK(noProof.status() == NI_INVALID_ARGUMENT);
//                 options = identity.options(true);
//                 const std::string differentName = "node.00000000000000000000000000000000";
//                 LINK_AUTH_CHECK(differentName != identity.m_name);
//                 options.m_name = differentName.c_str();
//                 NetInterfaceLink wrongIdentity(options);
//                 LINK_AUTH_CHECK(wrongIdentity.status() == NI_PROTOCOL_ERROR);
//             }
//         }
// #if defined(_WIN32) && defined(_DEBUG)
//         _CrtMemState after;
//         _CrtMemState difference;
//         _CrtMemCheckpoint(&after);
//         _CrtMemDifference(&difference, &before, &after);
//         LINK_AUTH_CHECK(difference.lCounts[_NORMAL_BLOCK] == 0 && difference.lSizes[_NORMAL_BLOCK] == 0);
// #endif
//         std::printf("Link auth checks=%u PASS\n", checks);
//         return 0;
//     }
//     catch (...)
//     {
//         std::printf("Link auth unexpected exception mode=%u\n", mode);
//         return 2;
//     }
// }
// NET_LINK_AUTH_UNIT_END
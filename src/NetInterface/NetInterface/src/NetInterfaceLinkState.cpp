#include "NetExecution.h"
#include "NetInterfaceMacro.h"
#include "NetInterfaceLinkState.h"
#include "CStringManager/CStringManagerAPI.h"
#include "NetInterfaceCertificate.h"
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
    if (!CStringManager::startsWith(name, "service.") || name.size() < 9 || name.size() > 18 ||
        (name.size() > 9 && name[8] == '0') || CStringManager::findFirstNotOf(name, "0123456789", 8) != std::string::npos)
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
m_record(),
m_storage(nullptr),
m_next(nullptr)
{

}

NetInterfaceLinkWrite::~NetInterfaceLinkWrite()
{
    NetInterface::wipe(m_bytes.data(), m_record.m_size);
}

NetInterfaceLinkState::NetInterfaceLinkState(const NetInterfaceLinkPools& pools) :
m_options(),
m_peerRole(0),
m_inputOffset(0),
m_received(),
m_recordReady(false),
m_outputBodies(new NetBufferPool(pools.m_outputBodies)),
m_outputRecords(new NetObjectPool<NetInterfaceLinkWrite>(pools.m_outputRecords)),
m_outputHead(nullptr),
m_outputTail(nullptr),
m_peeked(),
m_outputPeeked(false),
m_phase(NET_LINK_TLS),
m_error(NI_OK),
m_clockStarted(false),
m_started(0),
m_lastTime(0),
m_activity(0),
m_pingIntervalMs(pools.m_pingIntervalMs),
m_pongTimeoutMs(pools.m_pongTimeoutMs),
m_drainTimeoutMs(pools.m_drainTimeoutMs),
m_controlStepBudget(pools.m_controlStepBudget),
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
    while (m_outputHead != nullptr)
    {
        consumeQueuedRecord();
    }
}

int32_t NetInterfaceLinkState::queueRecord(const NetInterfaceTunnelRecord& record)
{
    NetObjectReference<NetInterfaceLinkWrite> slot;
    int32_t result = m_outputRecords->create(slot);
    if (result == NI_OK)
    {
        result = m_outputBodies->allocate(record.m_size, slot->m_bytes);
    }
    if (result != NI_OK)
    {
        return result;
    }
    std::memcpy(slot->m_bytes.data(), record.m_data, record.m_size);
    slot->m_record = record;
    slot->m_record.m_data = nullptr;
    slot->m_storage = slot.storage();
    NetInterfaceLinkWrite* accepted = slot.release();
    if (m_outputTail != nullptr)
    {
        m_outputTail->m_next = accepted;
    }
    else
    {
        m_outputHead = accepted;
    }
    m_outputTail = accepted;
    NetExecution::progress();
    return NI_OK;
}

void NetInterfaceLinkState::consumeQueuedRecord()
{
    NetInterfaceLinkWrite* completed = m_outputHead;
    m_outputHead = completed->m_next;
    if (m_outputHead == nullptr)
    {
        m_outputTail = nullptr;
    }
    m_outputRecords->release(completed->m_storage);
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

bool NetInterfaceLinkState::isHeartbeat(const NetInterfaceFrame& frame)
{
    return (frame.m_kind == NI_PING || frame.m_kind == NI_PONG) && frame.m_messageType == NET_INTERFACE_LINK_HEARTBEAT &&
        frame.m_productId == 0 && frame.m_requestId != 0 && frame.m_payloadSize == 0;
}

bool NetInterfaceLinkState::validControl(const NetInterfaceFrame& frame)
{
    return frame.m_productId == 0 && frame.m_payloadSize <= 16384 &&
        (frame.m_kind == NI_REQUEST || frame.m_kind == NI_RESPONSE || frame.m_kind == NI_EVENT || frame.m_kind == NI_CANCEL) &&
        (frame.m_messageType < NET_INTERFACE_LINK_CHALLENGE || frame.m_messageType > NET_INTERFACE_LINK_HEARTBEAT);
}

int32_t NetInterfaceLinkState::status() const
{
    if (m_phase == NET_LINK_FAILED)
    {
        return m_error;
    }
    return m_phase == NET_LINK_READY || m_phase == NET_LINK_DRAINING ? NI_READY : NI_NEED_MORE;
}

int32_t NetInterfaceLinkState::configure(const NetInterfaceLinkOptions& options, const NetInterfaceLinkPools& settings)
{
    const int32_t pools = m_outputBodies->status() == NI_OK ? m_outputRecords->status() : m_outputBodies->status();
    if (pools != NI_OK)
    {
        return fail(pools);
    }
    const uint32_t roles = (1U << NI_LINK_CLIENT) | (1U << NI_LINK_NODE) | (1U << NI_LINK_SERVICE);
    if (!m_controlStepBudget || !m_pingIntervalMs || !m_pongTimeoutMs || !m_drainTimeoutMs ||
        options.m_version != 3 || options.m_role < NI_LINK_CLIENT || options.m_role > NI_LINK_SERVICE ||
        (options.m_tls.server != 0 && options.m_tls.server != 1) || options.m_tls.entropy == nullptr ||
        options.m_claimsSize > 4096 || (options.m_claimsSize != 0 && options.m_claims == nullptr) ||
        (options.m_tls.server != 0 && (options.m_role != NI_LINK_NODE || options.m_peerRoles == 0 ||
            (options.m_peerRoles & ~roles) != 0 || ((options.m_peerRoles & (1U << NI_LINK_SERVICE)) != 0 && options.m_serviceVerify == nullptr))) ||
        (options.m_tls.server == 0 && options.m_peerRoles != 0))
    {
        return fail(NI_INVALID_ARGUMENT);
    }
    m_options = options;
    m_options.m_tls.handshakeTimeoutMs = options.m_tls.effectiveHandshakeTimeout();
    if (options.m_name != nullptr)
    {
        const size_t size = CStringManager::boundedLength(options.m_name, 97);
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
            (options.m_role == NI_LINK_NODE && !CStringManager::startsWith(m_name, "node.")) ||
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
    m_control.reset(new NetInterfaceConnection(tls, 16384, settings.m_controlConnection, settings.m_controlCiphertext));
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
        if (!CStringManager::startsWith(peerName, "node.") || NetInterfaceCertificate::verifyProof(peerCert, peerName,
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
    if (!CStringManager::startsWith(peerName, "node.") || NetInterfaceCertificate::verifyProof(peerCert, peerName, statement.data(),
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
        NetExecution::deadline(m_drainTimeoutMs, now - m_drainStarted);
        if (now - m_drainStarted >= m_drainTimeoutMs)
        {
            return fail(NI_TIMEOUT);
        }
        if (m_drainResult < 0)
        {
            return m_recordReady ? NI_READY : fail(m_drainResult);
        }
    }
    if (status() != NI_READY)
    {
        const uint64_t timeout = m_options.m_tls.handshakeTimeoutMs;
        NetExecution::deadline(timeout, now - m_started);
    }
    if (status() != NI_READY && now - m_started >= m_options.m_tls.handshakeTimeoutMs)
    {
        return fail(NI_TIMEOUT);
    }
    if (m_phase == NET_LINK_READY && m_pingTime != 0 && now - m_pingTime >= m_pongTimeoutMs)
    {
        return fail(NI_TIMEOUT);
    }
    // 把暂存后缀交给 TLS，返回接纳字节数及底层状态。
    const auto feedInput = [this](uint32_t& consumed) -> int32_t
    {
        const int32_t fed = m_control->feed(m_input.data() + m_inputOffset,
            static_cast<uint32_t>(m_input.size()) - m_inputOffset, consumed);
        if (fed >= 0)
        {
            m_inputOffset += consumed;
            if (m_inputOffset == m_input.size())
            {
                m_input.clear();
                m_inputOffset = 0;
            }
        }
        return fed;
    };
    for (uint32_t step = 0; step < m_controlStepBudget; ++step)
    {
        uint32_t consumed = 0;
        if (!m_input.empty())
        {
            const int32_t fed = feedInput(consumed);
            if (fed < 0)
            {
                return fail(fed);
            }
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
        if (!m_input.empty())
        {
            // 入口原先满时，TLS 推进后可能已腾出容量。只重试一次，仍无接纳则等待运输或次数预算。
            if (consumed == 0)
            {
                const int32_t fed = feedInput(consumed);
                if (fed < 0)
                {
                    return fail(fed);
                }
            }
            if (consumed != 0)
            {
                NetExecution::progress();
            }
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
        else if (isHeartbeat(frame))
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
            if (!validControl(frame))
            {
                return fail(NI_PROTOCOL_ERROR);
            }
            break;
        }
        if (step + 1 == m_controlStepBudget) { NetExecution::progress(); }
    }
    if (status() == NI_READY && m_phase != NET_LINK_DRAINING)
    {
        if (m_pingTime == 0 && now - m_activity >= m_pingIntervalMs)
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
        if (m_pingTime != 0)
        {
            NetExecution::deadline(m_pongTimeoutMs, now - m_pingTime);
        }
        else if (now - m_activity < m_pingIntervalMs)
        {
            NetExecution::deadline(m_pingIntervalMs, now - m_activity);
        }
    }
    return status();
}
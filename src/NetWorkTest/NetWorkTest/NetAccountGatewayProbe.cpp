#include "NetSenderTestClient.h"
#include <stdexcept>
#include "NetAccountGatewayProbe.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "CSystem/CSystemAPI.h"
#if defined(_WIN32) && defined(NET_WORK_TEST_ACCOUNT)
#include "NetInterfaceTlsTestCases.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include <chrono>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <windows.h>

NetAccountGatewayProbe::NetAccountGatewayProbe() :
m_sender(nullptr),
m_id(0),
m_stage(0),
m_operation(0),
m_results(0),
m_closes(0),
m_bad(false)
{

}

bool NetAccountGatewayProbe::sendNext(NetInterfaceMessageHandle parameters)
{
    const uint32_t operations[5] = {ACCOUNT_REGISTER_PASSWORD, ACCOUNT_QUERY_STATE, ACCOUNT_CREATE_MATH_ORDER, ACCOUNT_ANSWER_QUESTION,
                                    ACCOUNT_REQUIRE_VIP};
    m_operation = operations[m_stage];
    uint8_t params[8192] = {};
    uint8_t body[8192] = {};
    uint32_t size = 0;
    if (parameters && NetInterface::messageEncode(parameters, params, sizeof(params), &size) != NI_OK)
    {
        return false;
    }
    const bool sent = m_sender->beginRequest(4, m_operation, 8000, m_id) == NI_OK &&
                      m_account.buildRequest(m_operation, m_id, params, size, body, sizeof(body), &size) == NI_OK &&
                      m_sender->postRequest(m_id, body, size) == NI_OK;
    NetInterface::wipe(params, sizeof(params));
    NetInterface::wipe(body, sizeof(body));
    return sent;
}

void NetAccountGatewayProbe::onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
{
    (void)maxRequests;
    (void)maxChunk;
    m_bad = m_account.setConnection(generation) != ACCOUNT_OK;
    NetInterfaceMessageHandle message = nullptr;
    if (NetInterface::messageCreate(&message) != NI_OK)
    {
        m_bad = true;
        return;
    }
    const char password[] = "gateway-fixture-password-123";
    m_bad = m_bad ||
            NetInterface::messageSetValue(message, 0, ACCOUNT_FIELD_PASSWORD, 4, reinterpret_cast<const uint8_t*>(password),
                                        sizeof(password) - 1) != NI_OK ||
            !sendNext(message);
    NetInterface::messageDestroy(message);
}

void NetAccountGatewayProbe::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status,
                                             const uint8_t* data, uint32_t size)
{
    ++m_results;
    m_bad = m_bad || requestId != m_id || product != 4 || type != m_operation || status != NI_RPC_SUCCESS ||
            m_account.applyResponse(generation, requestId, type, status == NI_RPC_SUCCESS, data, size) != ACCOUNT_OK;
    if (m_bad)
    {
        m_sender->closeGateway(NI_PROTOCOL_ERROR);
        return;
    }
    if (m_stage == 4)
    {
        uint8_t authorization[8192] = {};
        uint32_t required = 0;
        m_bad = m_account.buildAuthorization(nullptr, 0, authorization, sizeof(authorization), &required) != NI_OK || required == 0;
        NetInterface::wipe(authorization, sizeof(authorization));
        m_sender->closeGateway(m_bad ? NI_PROTOCOL_ERROR : 0);
        return;
    }
    ++m_stage;
    NetInterfaceMessageHandle message = nullptr;
    if (NetInterface::messageCreate(&message) != NI_OK)
    {
        m_bad = true;
        return;
    }
    if (m_stage == 2)
    {
        uint8_t random[16] = {};
        m_bad = NetInterface::entropyBytes(CSystem::secureRandomBytesCallback, nullptr, random, sizeof(random)) != NI_OK;
        const char digits[] = "0123456789abcdef";
        char key[32];
        for (uint32_t index = 0; index < 16; ++index)
        {
            key[index * 2] = digits[random[index] >> 4];
            key[index * 2 + 1] = digits[random[index] & 15];
        }
        m_bad = m_bad || NetInterface::messageSetValue(message, 0, ACCOUNT_FIELD_IDEMPOTENCY_KEY, 4, reinterpret_cast<const uint8_t*>(key),
                                                     sizeof(key)) != NI_OK;
    }
    else if (m_stage == 3)
    {
        NetInterfaceMessageHandle response = nullptr;
        m_bad = NetInterface::messageCreate(&response) != NI_OK;
        if (response)
        {
            m_bad = m_bad || NetInterface::messageParse(response, data, size) != NI_OK;
            const uint32_t fields[3] = {ACCOUNT_FIELD_ORDER_ID, ACCOUNT_FIELD_QUESTION_ID, ACCOUNT_FIELD_QUESTION};
            for (uint32_t field : fields)
            {
                uint8_t text[256] = {};
                uint32_t length = 0;
                int32_t valueType = 0;
                m_bad = m_bad || NetInterface::messageGetValue(response, 0, field, &valueType, text, sizeof(text) - 1, &length) != NI_OK ||
                        valueType != 4;
                if (field != ACCOUNT_FIELD_QUESTION)
                {
                    m_bad = m_bad || NetInterface::messageSetValue(message, 0, field, 4, text, length) != NI_OK;
                }
                else
                {
                    int32_t left = 0;
                    int32_t right = 0;
                    char operation = 0;
                    std::istringstream parser(reinterpret_cast<const char*>(text));
                    parser >> left >> operation >> right;
                    int32_t answer = operation == '+' ? left + right : operation == '-' ? left - right : left * right;
                    m_bad = m_bad || parser.fail();
                    uint8_t bytes[4];
                    const uint32_t bits = static_cast<uint32_t>(answer);
                    for (uint32_t index = 0; index < 4; ++index)
                    {
                        bytes[index] = static_cast<uint8_t>(bits >> (index * 8));
                    }
                    m_bad = m_bad || NetInterface::messageSetValue(message, 0, ACCOUNT_FIELD_ANSWER, 1, bytes, sizeof(bytes)) != NI_OK;
                }
            }
            NetInterface::messageDestroy(response);
        }
    }
    m_bad = m_bad || !sendNext(message);
    NetInterface::messageDestroy(message);
    if (m_bad)
    {
        m_sender->closeGateway(NI_PROTOCOL_ERROR);
    }
}

bool NetAccountGatewayProbe::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    if (frame.m_kind == NI_EVENT && frame.m_messageType == ACCOUNT_EVENT_SESSION_REPLACED)
    {
        m_account.applyEvent(generation, frame.m_payload, frame.m_payloadSize);
    }
    return true;
}

void NetAccountGatewayProbe::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    (void)generation;
    ++m_closes;
    m_bad = m_bad || reason != 0 || detail != 0;
    m_account.setConnection(0);
}

int NetAccountGatewayProbe::run(int32_t pid)
{
    NetAccountGatewayProbe probe;
    NetSenderTestClient<NetSender, NetSenderClient>& sender = NetSenderTestClient<NetSender, NetSenderClient>::instance();
    probe.m_sender = &sender;
    if (probe.m_account.init(4) != ACCOUNT_OK || sender.prepareGateway() != NI_OK)
    {
        return 70;
    }
    NetSenderGatewayOptions options = {};
    options.m_localGatewayPid = 0;
    if (NetSenderDesktopBackend::configureGateway(sender, pid) != NI_OK)
    {
        throw std::runtime_error("desktop gateway backend configuration");
    }
    options.m_network.structSize = sizeof(NetWorkClientOptions);
    options.m_network.version = 1;
    options.m_network.platform = 1;
    options.m_network.capabilities = 7;
    options.m_network.maxRequests = 4;
    options.m_network.maxChunk = 4096;
    options.m_network.handshakeTimeoutMs = 5000;
    options.m_network.tls = NetInterfaceTlsTestPair::options(false, "localhost");
    options.m_network.tls.entropy = nullptr;
    options.m_network.tls.entropyContext = nullptr;
    std::memset(options.m_network.device, 0x42, 16);
    int32_t result = sender.connectGateway(options, &probe);
    const auto start = std::chrono::steady_clock::now();
    while (result == NI_OK && probe.m_closes == 0 && !probe.m_bad &&
           std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count() < 15)
    {
        sender.pollGateway();
        Sleep(1);
    }
    sender.uninitGateway();
    while (sender.pollGateway()) { CSystem::sleepPrecise(1); }
    sender.releaseGateway();

    std::printf("ACCOUNT_GATEWAY_RESULT results=%u closes=%u bad=%u\n", probe.m_results, probe.m_closes, probe.m_bad ? 1 : 0);
    return result == NI_OK && probe.m_results == 5 && probe.m_closes == 1 && !probe.m_bad ? 0 : 72;
}

#endif
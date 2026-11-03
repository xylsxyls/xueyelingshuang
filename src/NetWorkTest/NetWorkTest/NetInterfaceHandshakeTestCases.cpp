#include "NetInterfaceHandshakeTestCases.h"
#include "NetWorkTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <cstring>
#include <stdexcept>
#include <vector>
#include <algorithm>

// 独立于产品编码器的客户端/服务器固定线向量。
static const uint8_t g_clientHello[64] = {
    0x4E, 0x48, 0x41, 0x32, 0, 0, 0, 2,
    0, 0, 0, 7, 0, 0, 0, 3, 0, 0, 0, 32, 0, 0, 64, 0,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    0, 0, 0, 0, 0, 0, 0, 0
};
static const uint8_t g_serverHello[64] = {
    0x4E, 0x48, 0x52, 0x32, 0, 0, 0, 2,
    0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 16, 0, 0, 16, 0,
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    0, 0, 0, 0, 0, 0, 0, 0
};

/** 带runner取消/时限的断言
@param [in] condition 独立预期
@param [in] reason 失败原因
*/
static void CheckHello(bool condition, const char* reason)
{
    netTestCheckpoint();
    if (!condition)
    {
        throw std::runtime_error(reason);
    }
}

/** 明确构造测试配置，固定随机数只用于协议断言
@param [in] server 是否服务端
@return 客户端32/16384/7，服务端16/4096/3
*/
static NetInterfaceHandshakeOptions HelloOptions(bool server)
{
    NetInterfaceHandshakeOptions result = {};
    result.m_server = server ? 1 : 0;
    result.m_platform = server ? 0 : 3;
    result.m_capabilities = server ? 3 : 7;
    result.m_maxRequests = server ? 16 : 32;
    result.m_maxChunk = server ? 4096 : 16384;
    result.m_timeoutMs = 100;
    for (uint32_t index = 0; index < 16; ++index)
    {
        result.m_nonce[index] = static_cast<uint8_t>(index + (server ? 65 : 1));
        result.m_device[index] = server ? 0 : static_cast<uint8_t>(index + 33);
    }
    return result;
}

/** 构造固定手写向量帧，不借产品makeOutput构造输入
@param [in] reply 是否服务器回复
@param [in] payload 可选变异64字节体
@return 固定元数据
*/
static NetInterfaceFrame HelloFrame(bool reply, const uint8_t* payload = nullptr)
{
    NetInterfaceFrame frame = {};
    frame.m_kind = reply ? NI_HELLO_REPLY : NI_HELLO;
    frame.m_messageType = 2;
    frame.m_payloadSize = 64;
    frame.m_payload = payload ? payload : (reply ? g_serverHello : g_clientHello);
    return frame;
}

void NetInterfaceHandshakeTestCases::negotiation()
{
    NetInterfaceHandshake client(HelloOptions(false), 1000);
    NetInterfaceHandshake server(HelloOptions(true), 1000);
    NetInterfaceFrame frame = {};
    CheckHello(client.peekOutput(frame) == NI_READY && frame.m_kind == NI_HELLO && frame.m_payloadSize == 64 &&
        frame.m_messageType == 2 && frame.m_productId == 0 && frame.m_requestId == 0 &&
        std::memcmp(frame.m_payload, g_clientHello, 64) == 0, "HELLO wire vector");
    const uint8_t* view = frame.m_payload;
    CheckHello(client.peekOutput(frame) == NI_READY && frame.m_payload == view &&
        client.progress(1050) == NI_NEED_MORE, "pending output lost during backpressure");
    CheckHello(client.consumeOutput() == NI_NEED_MORE && client.peekOutput(frame) == NI_NEED_MORE &&
        frame.m_payload == nullptr, "client sent state");
    CheckHello(server.accept(HelloFrame(false), 1051) == NI_NEED_MORE && server.progress(1052) == NI_NEED_MORE,
        "server ready before reply accepted");
    CheckHello(server.peekOutput(frame) == NI_READY && frame.m_kind == NI_HELLO_REPLY &&
        std::memcmp(frame.m_payload, g_serverHello, 64) == 0, "reply wire vector");
    CheckHello(server.consumeOutput() == NI_READY && client.accept(HelloFrame(true), 1053) == NI_READY,
        "handshake completion");
    uint32_t fields[4] = {};
    uint8_t nonce[16] = {};
    uint8_t device[16] = {};
    CheckHello(client.peerInfo(fields, nonce, device) == NI_READY && fields[0] == 3 && fields[1] == 3 &&
        fields[2] == 16 && fields[3] == 4096 && std::memcmp(nonce, g_serverHello + 40, 16) == 0 &&
        std::memcmp(device, g_clientHello + 40, 16) == 0, "client negotiated peer info");
    CheckHello(server.peerInfo(fields, nonce, device) == NI_READY &&
        std::memcmp(nonce, g_clientHello + 24, 16) == 0, "server peer nonce");
    frame.m_kind = NI_REQUEST;
    frame.m_productId = 4;
    frame.m_requestId = UINT64_C(0xFFFFFFFF00000001);
    CheckHello(server.accept(frame, 5000) == NI_OK && client.progress(5000) == NI_READY,
        "authenticated application stage expired as handshake");
    CheckHello(server.consumeOutput() == NI_INVALID_STATE, "double output consume");
}

void NetInterfaceHandshakeTestCases::rejection()
{
    for (int index = 0; index < 15; ++index)
    {
        NetInterfaceHandshake server(HelloOptions(true), 1000);
        std::vector<uint8_t> bytes(g_clientHello, g_clientHello + 64);
        NetInterfaceFrame frame = HelloFrame(false, &bytes[0]);
        switch (index)
        {
        case 0:
        {
            bytes[0] = 0;
            break;
        }
        case 1:
        {
            bytes[7] = 3;
            break;
        }
        case 2:
        {
            bytes[8] = 128;
            break;
        }
        case 3:
        {
            bytes[11] = 6;
            break;
        }
        case 4:
        {
            bytes[15] = 0;
            break;
        }
        case 5:
        {
            bytes[19] = 0;
            break;
        }
        case 6:
        {
            bytes[20] = 1;
            break;
        }
        case 7:
        {
            std::memset(&bytes[24], 0, 16);
            break;
        }
        case 8:
        {
            std::memset(&bytes[40], 0, 16);
            break;
        }
        case 9:
        {
            bytes[56] = 1;
            break;
        }
        case 10:
        {
            frame.m_payloadSize = 63;
            break;
        }
        case 11:
        {
            frame.m_payloadSize = 65;
            break;
        }
        case 12:
        {
            frame.m_productId = 4;
            break;
        }
        case 13:
        {
            frame.m_requestId = 1;
            break;
        }
        case 14:
        {
            frame.m_kind = NI_REQUEST;
            break;
        }
        }
        CheckHello(server.accept(frame, 1001) == NI_PROTOCOL_ERROR &&
            server.accept(HelloFrame(false), 1002) == NI_PROTOCOL_ERROR &&
            server.peekOutput(frame) == NI_PROTOCOL_ERROR, "malformed HELLO recovered or produced output");
    }
    NetInterfaceHandshake early(HelloOptions(false), 1000);
    CheckHello(early.accept(HelloFrame(true), 1001) == NI_PROTOCOL_ERROR, "reply before HELLO sent");
    NetInterfaceHandshake server(HelloOptions(true), 1000);
    CheckHello(server.accept(HelloFrame(false), 1001) == NI_NEED_MORE && server.consumeOutput() == NI_READY &&
        server.accept(HelloFrame(false), 1002) == NI_PROTOCOL_ERROR, "reentrant HELLO accepted");
    for (int index = 0; index < 9; ++index)
    {
        NetInterfaceHandshakeOptions bad = HelloOptions(false);
        switch (index)
        {
        case 0:
        {
            bad.m_server = 2;
            break;
        }
        case 1:
        {
            bad.m_platform = 0;
            break;
        }
        case 2:
        {
            bad.m_capabilities = 0;
            break;
        }
        case 3:
        {
            bad.m_maxRequests = 129;
            break;
        }
        case 4:
        {
            bad.m_maxChunk = 65537;
            break;
        }
        case 5:
        {
            bad.m_timeoutMs = 99;
            break;
        }
        case 6:
        {
            bad.m_timeoutMs = 60001;
            break;
        }
        case 7:
        {
            std::memset(bad.m_nonce, 0, 16);
            break;
        }
        case 8:
        {
            std::memset(bad.m_device, 0, 16);
            break;
        }
        }
        NetInterfaceHandshake invalid(bad, 1000);
        CheckHello(invalid.progress(1001) == NI_INVALID_ARGUMENT, "invalid local HELLO options accepted");
    }
}

void NetInterfaceHandshakeTestCases::deadlines()
{
    NetInterfaceHandshake late(HelloOptions(true), 1000);
    CheckHello(late.progress(1099) == NI_NEED_MORE && late.progress(1100) == NI_TIMEOUT &&
        late.accept(HelloFrame(false), 1101) == NI_TIMEOUT, "fixed HELLO deadline");
    NetInterfaceHandshake rollback(HelloOptions(true), 1000);
    CheckHello(rollback.progress(1001) == NI_NEED_MORE && rollback.progress(1000) == NI_INVALID_ARGUMENT,
        "HELLO clock rollback");
    NetInterfaceHandshake overflow(HelloOptions(false), UINT64_MAX - 99);
    CheckHello(overflow.progress(UINT64_MAX - 99) == NI_INVALID_ARGUMENT, "HELLO deadline overflow");
    for (int index = 0; index < 5; ++index)
    {
        NetInterfaceHandshake client(HelloOptions(false), 1000);
        CheckHello(client.consumeOutput() == NI_NEED_MORE, "send HELLO before malformed reply");
        std::vector<uint8_t> bytes(g_serverHello, g_serverHello + 64);
        switch (index)
        {
        case 0:
        {
            bytes[24] ^= 1;
            break;
        }
        case 1:
        {
            bytes[15] = 1;
            break;
        }
        case 2:
        {
            bytes[19] = 64;
            break;
        }
        case 3:
        {
            bytes[22] = 128;
            break;
        }
        case 4:
        {
            bytes[63] = 1;
            break;
        }
        }
        CheckHello(client.accept(HelloFrame(true, &bytes[0]), 1001) == NI_PROTOCOL_ERROR,
            "reply not bound to current client or increased limits");
    }
}

void NetInterfaceHandshakeTestCases::cBoundary()
{
    uint32_t options[6] = { 0, 3, 7, 32, 16384, 100 };
    uint8_t identities[32];
    std::memcpy(identities, g_clientHello + 24, 32);
    NetInterfaceHandshakeHandle handle = nullptr;
    CheckHello(NetInterface::handshakeCreate(options, 5, identities, 32, 0xFFFFFFFEU, 8, &handle) == NI_INVALID_ARGUMENT &&
        handle == nullptr, "C HELLO option width");
    CheckHello(NetInterface::handshakeCreate(options, 6, identities, 32, 0xFFFFFFFEU, 8, &handle) == NI_NEED_MORE &&
        handle != nullptr, "C HELLO allocation");
    try
    {
        uint32_t fields[6] = {};
        const uint8_t* payload = nullptr;
        CheckHello(NetInterface::handshakePeekOutput(handle, fields, &payload) == NI_READY && fields[0] == NI_HELLO &&
            fields[4] == 2 && fields[5] == 64 && std::memcmp(payload, g_clientHello, 64) == 0, "C HELLO vector");
        CheckHello(NetInterface::handshakeConsumeOutput(handle) == NI_NEED_MORE &&
            NetInterface::handshakeProgress(handle, 0, 9) == NI_NEED_MORE, "64-bit C monotonic carry");
        fields[0] = NI_HELLO_REPLY;
        CheckHello(NetInterface::handshakeAccept(handle, fields, g_serverHello, 1, 9) == NI_READY, "C HELLO reply");
        CheckHello(NetInterface::handshakePeerInfo(handle, fields, identities) == NI_READY &&
            fields[0] == 3 && fields[1] == 3 && fields[2] == 16 && fields[3] == 4096 &&
            std::memcmp(identities, g_serverHello + 40, 16) == 0, "C peer info");
        CheckHello(NetInterface::handshakePeerInfo(nullptr, fields, identities) == NI_INVALID_ARGUMENT &&
            fields[0] == 0 && identities[0] == 0, "C invalid output clearing");
    }
    catch (...)
    {
        NetInterface::handshakeDestroy(handle);
        throw;
    }
    NetInterface::handshakeDestroy(handle);
    NetInterface::handshakeDestroy(nullptr);
}

/** 转发实际TLS密文，固定不规则碎片，不接触明文协议
@param [in,out] source 密文输出连接
@param [in,out] target 密文输入连接
*/
static void TransferHelloCiphertext(NetInterfaceConnection& source, NetInterfaceConnection& target)
{
    const uint8_t* bytes = nullptr;
    uint32_t size = 0;
    CheckHello(source.peekOutput(bytes, size) >= 0, "TLS ciphertext output");
    uint32_t consumed = 0;
    CheckHello(target.feed(bytes, (std::min)(size, 317U), consumed) >= 0, "TLS ciphertext input");
    CheckHello(source.consumeOutput(consumed) >= 0, "consume forwarded ciphertext");
}

/** 下层接纳后才消费握手输出，保持网络背压
@param [in,out] hello 本地握手
@param [in,out] connection 对应实际TLS连接
*/
static void SendHelloFrame(NetInterfaceHandshake& hello, NetInterfaceConnection& connection)
{
    NetInterfaceFrame frame = {};
    if (connection.status() == NET_INTERFACE_TLS_READY && hello.peekOutput(frame) == NI_READY)
    {
        const int32_t result = connection.send(frame);
        CheckHello(result == NI_OK || result == NI_LIMIT_EXCEEDED, "queue encrypted HELLO");
        if (result == NI_OK)
        {
            CheckHello(hello.consumeOutput() >= 0, "consume queued HELLO");
        }
    }
}

void NetInterfaceHandshakeTestCases::tlsPipeline()
{
    NetInterfaceConnection client(NetInterfaceTlsTestPair::options(false, "localhost"));
    NetInterfaceConnection server(NetInterfaceTlsTestPair::options(true, nullptr));
    NetInterfaceHandshake clientHello(HelloOptions(false), 1000);
    NetInterfaceHandshake serverHello(HelloOptions(true), 1000);
    const uint8_t body[] = { 0, 128, 255, 3, 9 };
    bool sent = false;
    uint32_t received = 0;
    for (uint32_t step = 0; step < 20000; ++step)
    {
        CheckHello(client.progress(1000) >= 0 && server.progress(1000) >= 0, "real TLS handshake");
        TransferHelloCiphertext(client, server);
        TransferHelloCiphertext(server, client);
        SendHelloFrame(clientHello, client);
        SendHelloFrame(serverHello, server);
        NetInterfaceFrame frame = {};
        if (server.peek(frame) == NI_READY)
        {
            const int32_t result = serverHello.accept(frame, 1001);
            CheckHello(result >= 0, "encrypted client HELLO rejected");
            if (result == NI_OK)
            {
                CheckHello(frame.m_kind == NI_REQUEST && frame.m_productId == 4 &&
                    frame.m_requestId == UINT64_C(0xFEDCBA9876543210) &&
                    frame.m_payloadSize == sizeof(body) && std::memcmp(frame.m_payload, body, sizeof(body)) == 0,
                    "post-handshake request payload");
                ++received;
                frame.m_kind = NI_RESPONSE;
                CheckHello(server.send(frame) == NI_OK, "echo authenticated application frame");
            }
            CheckHello(server.consume() == NI_OK, "consume server plaintext frame");
        }
        if (client.peek(frame) == NI_READY)
        {
            const int32_t result = clientHello.accept(frame, 1001);
            CheckHello(result >= 0, "encrypted server HELLO rejected");
            if (result == NI_OK)
            {
                CheckHello(sent && received == 1 && frame.m_kind == NI_RESPONSE && frame.m_productId == 4 &&
                    frame.m_requestId == UINT64_C(0xFEDCBA9876543210) &&
                    frame.m_payloadSize == sizeof(body) && std::memcmp(frame.m_payload, body, sizeof(body)) == 0,
                    "post-handshake response correlation");
                CheckHello(client.consume() == NI_OK, "consume complete response");
                return;
            }
            CheckHello(client.consume() == NI_OK, "consume HELLO_REPLY");
        }
        if (!sent && clientHello.progress(1001) == NI_READY)
        {
            frame = NetInterfaceFrame();
            frame.m_kind = NI_REQUEST;
            frame.m_productId = 4;
            frame.m_requestId = UINT64_C(0xFEDCBA9876543210);
            frame.m_messageType = 700;
            frame.m_payloadSize = sizeof(body);
            frame.m_payload = body;
            CheckHello(client.send(frame) == NI_OK, "queue post-handshake request");
            sent = true;
        }
    }
    throw std::runtime_error("TLS application pipeline step limit");
}
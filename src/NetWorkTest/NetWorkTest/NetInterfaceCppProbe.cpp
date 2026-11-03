#include "NetInterfaceCppProbe.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <string.h>

/** 验证唯一公共头通过C++主类入口操作真实编解码与消息对象
@return 0为通过，非零为失败步骤
*/
extern "C" int netInterfaceCppProbe(void)
{
    const uint8_t input[32] = {
        'N', 'I', 'F', '2', 0, 2, 0, 7, 0, 0, 0, 1,
        0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };
    NetInterfaceHandle handle = 0;
    NetInterfaceFrame frame;
    const uint8_t* payload = 0;
    uint32_t fields[6] = {0};
    uint8_t encoded[32];
    uint32_t required = 0;
    uint32_t used = 0;
    int result = 0;
    NetInterfaceMessageHandle message = 0;
    int32_t code = 0;
    NetInterfaceConnectionHandle connection = 0;
    NetInterfaceHandshakeHandle hello = 0;
    const uint32_t helloOptions[6] = { 0, 1, 7, 32, 16384, 100 };
    uint8_t helloIdentities[32] = { 0 };
    NetInterfaceRequestsHandle requests = 0;
    uint32_t requestInput[13] = { 1, 0, 0, 0, 2, 0, 4, 7, 0, 0, 1000, 0, 50 };
    uint32_t requestOutput[16] = { 0 };
    if (NetInterface::create(8, &handle) != NI_OK || handle == 0)
    {
        return 1;
    }
    if (NetInterface::feed(handle, input, 32, &used) != NI_READY || used != 32)
    {
        result = 2;
    }
    else if (NetInterface::peek(handle, &frame) != NI_READY ||
        frame.m_productId != 1 || frame.m_requestId != 5 ||
        frame.m_kind != NI_PING || frame.m_payloadSize != 0 || frame.m_payload != 0)
    {
        result = 3;
    }
    else if (NetInterface::peekFields(handle, fields, &payload) != NI_READY ||
        fields[0] != NI_PING || fields[1] != 1 || fields[2] != 5 || fields[3] != 0 ||
        fields[4] != 0 || fields[5] != 0 || payload != 0 ||
        NetInterface::encodeFields(fields, payload, encoded, 32, &required) != NI_OK ||
        required != 32 || memcmp(input, encoded, 32) != 0)
    {
        result = 4;
    }
    else if (NetInterface::consume(handle) != NI_OK || NetInterface::finish(handle) != NI_OK)
    {
        result = 5;
    }
    NetInterface::destroy(handle);
    fields[0] = 0x10007;
    if (NetInterface::encodeFields(fields, 0, encoded, 32, &required) != NI_INVALID_ARGUMENT ||
        required != 0)
    {
        result = 6;
    }
    if (NetInterface::tlsGetStatus(0, 0) != NET_INTERFACE_TLS_ARGUMENT)
    {
        result = 7;
    }
    if (NetInterface::messageCreate(&message) != NI_OK || message == 0)
    {
        return 8;
    }
    if (NetInterface::messageSetCode(message, 7, 0, 0) != NI_OK ||
        NetInterface::messageEncode(message, encoded, 32, &required) != NI_OK ||
        required != 4 || memcmp(encoded, "\x0a\x02\x08\x07", 4) != 0 ||
        NetInterface::messageParse(message, encoded, required) != NI_OK ||
        NetInterface::messageGetCode(message, &code, 0, 0, &required) != NI_OK || code != 7)
    {
        result = 9;
    }
    NetInterface::messageDestroy(message);
    if (NetInterface::connectionCreate(0, &connection, &code) != NI_INVALID_ARGUMENT ||
        connection != 0 || code != 0 ||
        NetInterface::connectionFeed(0, input, 32, &used) != NI_INVALID_ARGUMENT || used != 0 ||
        NetInterface::connectionPeek(0, fields, &payload) != NI_INVALID_ARGUMENT ||
        fields[0] != 0 || payload != 0 ||
        NetInterface::connectionProgress(0, 0, 0) != NI_INVALID_ARGUMENT ||
        NetInterface::connectionSend(0, fields, 0) != NI_INVALID_ARGUMENT ||
        NetInterface::connectionConsume(0) != NI_INVALID_ARGUMENT ||
        NetInterface::connectionFinishInput(0) != NI_INVALID_ARGUMENT)
    {
        result = 10;
    }
    NetInterface::connectionDestroy(connection);
    helloIdentities[0] = 1;
    helloIdentities[16] = 2;
    if (NetInterface::handshakeCreate(helloOptions, 6, helloIdentities, 32, 1000, 0, &hello) != NI_NEED_MORE ||
        hello == 0 || NetInterface::handshakePeekOutput(hello, fields, &payload) != NI_READY ||
        fields[0] != NI_HELLO || fields[5] != 64 || payload == 0 ||
        NetInterface::handshakeConsumeOutput(hello) != NI_NEED_MORE ||
        NetInterface::handshakeProgress(hello, 1100, 0) != NI_TIMEOUT)
    {
        result = 11;
    }
    NetInterface::handshakeDestroy(hello);
    if (NetInterface::requestsCreate(2, &requests) != NI_OK || requests == 0 ||
        NetInterface::requestsAddEndpoint(requests, 1, 0, 0, 0, 2, 0) != NI_OK ||
        NetInterface::requestsAddEndpoint(requests, 2, 0, 1, 0, 2, 0) != NI_OK ||
        NetInterface::requestsBegin(requests, requestInput, 13, 1, requestOutput) != NI_OK ||
        requestOutput[4] != 1 || requestOutput[5] != 0 || NetInterface::requestsCount(requests) != 1 ||
        NetInterface::requestsCancel(requests, 1, 0, 1, 0, requestOutput) != NI_READY ||
        NetInterface::requestsCount(requests) != 0)
    {
        result = 12;
    }
    NetInterface::requestsDestroy(requests);
    if (NetInterface::rpcEncode(NI_REQUEST, 1000, 0, 0, 0, 0, encoded, 32, &required) != NI_OK ||
        required != 16 || memcmp(encoded, "NRP2\x00\x00\x03\xe8\x00\x00\x00\x00\x00\x00\x00\x00", 16) != 0)
    {
        result = 13;
    }
    fields[0] = NI_PING;
    fields[1] = 0;
    fields[2] = 0x76543210;
    fields[3] = 0xfedcba98;
    fields[4] = fields[5] = 0;
    if (NetInterface::rpcControl(fields, 6, fields) != NI_READY || fields[0] != NI_PONG ||
        fields[2] != 0x76543210 || fields[3] != 0xfedcba98)
    {
        result = 14;
    }
    return result;
}
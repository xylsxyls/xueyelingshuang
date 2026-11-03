#include "NetInterfaceRpcTestCases.h"
#include "NetWorkTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <cstring>
#include <stdexcept>
#include <vector>

/** 核对协议独立预期并保持测试取消点
@param [in] expected 必需成立
@param [in] reason 失败说明
*/
static void CheckRpc(bool expected, const char* reason)
{
    netTestCheckpoint();
    if (!expected)
    {
        throw std::runtime_error(reason);
    }
}

void NetInterfaceRpcTestCases::serviceWire()
{
    uint8_t secret[32];
    uint8_t expected[48] = { 'N', 'I', 'S', '2', 0, 0, 0, 7 };
    for (uint32_t index = 0; index < 32; ++index)
    {
        secret[index] = expected[8 + index] = static_cast<uint8_t>(index + 1);
    }
    uint8_t output[48];
    std::memset(output, 0xa5, sizeof(output));
    CheckRpc(NetInterfaceService::hello(secret, 7, output, 47) == NI_BUFFER_TOO_SMALL && output[0] == 0xa5,
        "service short output mutated");
    CheckRpc(NetInterfaceService::hello(secret, 7, output, sizeof(output)) == NI_OK &&
        std::memcmp(output, expected, 48) == 0, "service independent HELLO bytes");
    NetInterfaceFrame hello = {};
    hello.m_kind = NI_HELLO;
    hello.m_messageType = 3;
    hello.m_productId = 4;
    hello.m_payload = expected;
    hello.m_payloadSize = sizeof(expected);
    uint32_t limit = 0;
    CheckRpc(NetInterfaceService::authenticate(hello, secret, limit) == NI_OK && limit == 7, "service secret rejected");
    for (uint32_t index = 8; index < 48; ++index)
    {
        expected[index] ^= 0x80;
        CheckRpc(NetInterfaceService::authenticate(hello, secret, limit) == NI_PROTOCOL_ERROR && limit == 0,
            "service damaged credential accepted");
        expected[index] ^= 0x80;
    }
    hello.m_requestId = 1;
    CheckRpc(NetInterfaceService::authenticate(hello, secret, limit) == NI_PROTOCOL_ERROR, "service HELLO with request");
    hello.m_requestId = 0;
    expected[7] = 129;
    CheckRpc(NetInterfaceService::authenticate(hello, secret, limit) == NI_PROTOCOL_ERROR, "service excessive limit");
    uint8_t zero[32] = {};
    CheckRpc(NetInterfaceService::hello(zero, 1, output, 48) == NI_INVALID_ARGUMENT, "empty service secret");

    const uint8_t originBytes[] = { 'N', 'O', 'I', '2', 0, 0, 0, 3,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 0xff, 0, 0x81 };
    NetInterfaceOrigin origin = {};
    origin.m_endpoint = UINT64_C(0xfedcba9876543210);
    origin.m_platform = 3;
    std::memcpy(origin.m_device, originBytes + 16, 16);
    uint32_t required = 0;
    CheckRpc(NetInterfaceService::wrap(origin, originBytes + 32, 3, output, 48, required) == NI_OK &&
        required == 35 && std::memcmp(output, originBytes, 35) == 0, "service origin independent bytes");
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_REQUEST;
    frame.m_requestId = UINT64_C(0x200000005);
    frame.m_productId = 4;
    frame.m_payload = originBytes;
    frame.m_payloadSize = sizeof(originBytes);
    NetInterfaceOrigin parsed = {};
    NetInterfaceFrame business = {};
    CheckRpc(NetInterfaceService::unwrap(frame, parsed, business) == NI_OK &&
        parsed.m_endpoint == origin.m_endpoint && parsed.m_platform == 3 &&
        std::memcmp(parsed.m_device, origin.m_device, 16) == 0 && business.m_payloadSize == 3 &&
        business.m_requestId == frame.m_requestId && std::memcmp(business.m_payload, originBytes + 32, 3) == 0,
        "service origin parse lost context");
    frame.m_payloadSize = 31;
    CheckRpc(NetInterfaceService::unwrap(frame, parsed, business) == NI_PROTOCOL_ERROR && parsed.m_endpoint == 0 &&
        business.m_payload == nullptr, "service truncated context retained stale values");
    NetInterfaceOrigin empty = {};
    CheckRpc(NetInterfaceService::wrap(empty, nullptr, 0, output, 48, required) == NI_OK, "service empty error origin");
    frame.m_payload = output;
    frame.m_payloadSize = 32;
    CheckRpc(NetInterfaceService::unwrap(frame, parsed, business) == NI_PROTOCOL_ERROR, "request without origin");
    frame.m_kind = NI_RESPONSE;
    CheckRpc(NetInterfaceService::unwrap(frame, parsed, business) == NI_OK && parsed.m_endpoint == 0 &&
        business.m_payload == nullptr, "response without root origin");
    std::vector<uint8_t> body(1048576, 0x49);
    std::vector<uint8_t> wrapped(body.size() + 32);
    CheckRpc(NetInterfaceService::wrap(origin, body.data(), static_cast<uint32_t>(body.size()), wrapped.data(),
        static_cast<uint32_t>(wrapped.size()), required) == NI_OK && required == 1048608,
        "service full RPC payload cannot fit local context");
    CheckRpc(NetInterfaceService::wrap(origin, body.data(), 1048577, wrapped.data(),
        static_cast<uint32_t>(wrapped.size()), required) == NI_LIMIT_EXCEEDED, "service oversized business accepted");
    NetInterfaceConnection excessive(NetInterfaceTlsTestPair::options(false, "localhost"), 1048609);
    NetInterfaceConnection none(NetInterfaceTlsTestPair::options(false, "localhost"), 0);
    CheckRpc(excessive.status() == NI_INVALID_ARGUMENT && none.status() == NI_INVALID_ARGUMENT,
        "local transport extra budget unrestricted");
}

void NetInterfaceRpcTestCases::wireVectors()
{
    const uint8_t request[] = { 'N', 'R', 'P', '2', 0, 0, 0x03, 0xe8,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10, 0, 0xff, 0x80, 0 };
    uint8_t bytes[32] = {};
    uint32_t required = 0;
    CheckRpc(NetInterfaceRpc::encode(NI_REQUEST, 1000, UINT64_C(0xfedcba9876543210),
        request + 16, 4, bytes, sizeof(bytes), required) == NI_OK && required == sizeof(request) &&
        std::memcmp(bytes, request, required) == 0, "independent request vector mismatch");
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_REQUEST;
    frame.m_productId = 4;
    frame.m_requestId = UINT64_C(0x123456789abcdef0);
    frame.m_messageType = 900;
    frame.m_payload = request;
    frame.m_payloadSize = sizeof(request);
    NetInterfaceRpcView view = {};
    CheckRpc(NetInterfaceRpc::parse(frame, view) == NI_OK && view.m_value == 1000 &&
        view.m_parent == UINT64_C(0xfedcba9876543210) && view.m_size == 4 &&
        std::memcmp(view.m_data, request + 16, 4) == 0, "request view mismatch");
    const uint8_t response[] = { 'N', 'R', 'P', '2', 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0 };
    CheckRpc(NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_TIMEOUT, 0, nullptr, 0,
        bytes, sizeof(bytes), required) == NI_OK && required == sizeof(response) &&
        std::memcmp(bytes, response, required) == 0, "independent timeout response vector");
    frame.m_kind = NI_RESPONSE;
    frame.m_payload = response;
    frame.m_payloadSize = sizeof(response);
    CheckRpc(NetInterfaceRpc::parse(frame, view) == NI_OK && view.m_value == NI_RPC_TIMEOUT &&
        view.m_parent == 0 && view.m_data == nullptr && view.m_size == 0, "timeout has no business bytes");
    uint32_t fields[6] = { NI_REQUEST, 4, 0x9abcdef0, 0x12345678, 900, sizeof(request) };
    CheckRpc(NetInterface::rpcParse(fields, 6, request, fields) == NI_OK && fields[0] == 1000 &&
        fields[1] == 0x76543210 && fields[2] == 0xfedcba98 && fields[3] == 16 && fields[4] == 4,
        "aliased flat ABI does not retain full parent ID");
    CheckRpc(NetInterface::rpcEncode(NI_REQUEST, 1000, 0x76543210, 0xfedcba98,
        request + 16, 4, bytes, sizeof(bytes), &required) == NI_OK &&
        std::memcmp(bytes, request, required) == 0, "C encoder differs");
}

void NetInterfaceRpcTestCases::rejection()
{
    uint8_t bytes[32];
    std::memset(bytes, 0x55, sizeof(bytes));
    uint32_t required = 99;
    CheckRpc(NetInterfaceRpc::encode(NI_REQUEST, 1, 0, nullptr, 0, bytes, 15, required) ==
        NI_BUFFER_TOO_SMALL && required == 16 && bytes[0] == 0x55 && bytes[15] == 0x55,
        "insufficient output changed bytes");
    CheckRpc(NetInterfaceRpc::encode(NI_REQUEST, 0, 0, nullptr, 0, bytes, 32, required) ==
        NI_INVALID_ARGUMENT && required == 0, "zero deadline");
    CheckRpc(NetInterfaceRpc::encode(NI_REQUEST, 60001, 0, nullptr, 0, bytes, 32, required) ==
        NI_INVALID_ARGUMENT, "deadline limit");
    CheckRpc(NetInterfaceRpc::encode(NI_RESPONSE, 7, 0, nullptr, 0, bytes, 32, required) ==
        NI_INVALID_ARGUMENT && NetInterfaceRpc::encode(NI_RESPONSE, 0, 1, nullptr, 0, bytes, 32, required) ==
        NI_INVALID_ARGUMENT, "invalid response fields");
    CheckRpc(NetInterfaceRpc::encode(NI_RESPONSE, 1, 0, bytes, 1, nullptr, 0, required) ==
        NI_INVALID_ARGUMENT && NetInterfaceRpc::encode(NI_CANCEL, 1, 0, nullptr, 0, bytes, 32, required) ==
        NI_INVALID_ARGUMENT, "error leaked payload or control used RPC body");
    CheckRpc(NetInterface::rpcEncode(65539, 1, 0, 0, nullptr, 0, bytes, 32, &required) ==
        NI_INVALID_ARGUMENT && required == 0, "kind truncation");
    CheckRpc(NetInterfaceRpc::encode(NI_RESPONSE, 0, 0, nullptr, 1, bytes, 32, required) ==
        NI_INVALID_ARGUMENT, "null data");
    std::vector<uint8_t> maximum(NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT - 16, 0x80);
    CheckRpc(NetInterfaceRpc::encode(NI_REQUEST, 60000, 0, maximum.data(),
        static_cast<uint32_t>(maximum.size()), nullptr, 0, required) == NI_BUFFER_TOO_SMALL &&
        required == NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT, "maximum business body");
    CheckRpc(NetInterfaceRpc::encode(NI_REQUEST, 1, 0, maximum.data(),
        static_cast<uint32_t>(maximum.size()) + 1, nullptr, 0, required) == NI_INVALID_ARGUMENT &&
        required == 0, "oversized business body");
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_REQUEST;
    frame.m_productId = 4;
    frame.m_requestId = 1;
    frame.m_payload = bytes;
    frame.m_payloadSize = 16;
    NetInterfaceRpcView view = {};
    CheckRpc(NetInterfaceRpc::encode(NI_REQUEST, 1, 0, nullptr, 0, bytes, 32, required) == NI_OK,
        "test valid request setup");
    for (uint32_t length = 0; length < 16; ++length)
    {
        frame.m_payloadSize = length;
        view.m_value = 99;
        CheckRpc(NetInterfaceRpc::parse(frame, view) == NI_PROTOCOL_ERROR && view.m_value == 0 &&
            view.m_data == nullptr, "truncated RPC prefix accepted or output not cleared");
    }
    frame.m_payloadSize = 16;
    bytes[0] = 'X';
    CheckRpc(NetInterfaceRpc::parse(frame, view) == NI_PROTOCOL_ERROR, "wrong envelope magic");
    bytes[0] = 'N';
    frame.m_productId = 0;
    CheckRpc(NetInterfaceRpc::parse(frame, view) == NI_PROTOCOL_ERROR, "product zero");
    frame.m_productId = 4;
    frame.m_requestId = 0;
    CheckRpc(NetInterfaceRpc::parse(frame, view) == NI_PROTOCOL_ERROR, "request zero");
    frame.m_requestId = 1;
    frame.m_kind = NI_RESPONSE;
    frame.m_payloadSize = 17;
    CheckRpc(NetInterfaceRpc::parse(frame, view) == NI_PROTOCOL_ERROR, "remote error leaked data");
    uint32_t output[5] = { 1, 2, 3, 4, 5 };
    CheckRpc(NetInterface::rpcParse(nullptr, 6, bytes, output) == NI_INVALID_ARGUMENT &&
        output[0] == 0 && output[4] == 0, "C error did not clear output");
    uint32_t control[6] = { NI_PING, 0, 0x76543210, 0xfedcba98, 0, 0 };
    CheckRpc(NetInterface::rpcControl(control, 6, control) == NI_READY && control[0] == NI_PONG &&
        control[2] == 0x76543210 && control[3] == 0xfedcba98, "heartbeat reply lost full ID");
    CheckRpc(NetInterface::rpcControl(control, 6, control) == NI_OK && control[0] == 0 && control[3] == 0,
        "PONG produced recursive response");
    control[0] = NI_PING;
    control[2] = 1;
    control[5] = 1;
    CheckRpc(NetInterface::rpcControl(control, 6, control) == NI_PROTOCOL_ERROR && control[0] == 0,
        "heartbeat with body accepted");
    control[0] = NI_CANCEL;
    control[1] = 4;
    control[2] = 1;
    CheckRpc(NetInterface::rpcControl(control, 6, control) == NI_OK, "valid cancel fields");
}
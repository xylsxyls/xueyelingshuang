#include "NetGatewayChannelStateTest.h"
#include <stdexcept>
#include <algorithm>
#include "NetInterface/NetInterfaceAPI.h"
#include "ProtoMessageManager/ProtoMessageManagerAPI.h"
#include "NetClientSessionTest.h"
#include <cstdio>
#include <cstring>
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif
int NetGatewayChannelStateTest::check()
{
    uint32_t checks = 0;
    const auto Check = [&checks](bool pass, const char* message)
    {
        ++checks;
        if (!pass) { throw std::runtime_error(message); }
    };
    NetInterfaceTlsOptions options = {};
    options.handshakeTimeoutMs = 5000;
    options.entropy = [](void*, uint8_t* bytes, uint32_t size) -> int32_t { std::memset(bytes, 0x41, size); return NI_OK; };
    try
    {
        for (uint32_t mode = 0; mode < 4; ++mode)
        {
            NetGatewayChannelTestTransport transport;
            NetGatewayChannelTestListener listener;
            NetGatewayClientChannel channel(101, &transport);
            listener.m_channel = &channel;
            listener.m_throw = true;
            transport.m_stopThrow = true;
            transport.m_closeDuringStop = true;
            transport.m_pollThrow = mode == 0;
            transport.m_connectResult = mode == 2 ? NI_NOT_FOUND : NI_OK;
            transport.m_zeroPeer = mode == 3;
            uint64_t peer = 99;
            const int32_t connected = channel.connect("127.0.0.1", 1, options, &listener, peer);
            if (mode >= 2)
            {
                Check(connected == (mode == 2 ? NI_NOT_FOUND : NI_PROTOCOL_ERROR) && peer == 0 && listener.m_closed == 0 &&
                    transport.m_stops == 1 && transport.m_listener == nullptr, "rejected connect cleanup/false terminal");
            }
            else
            {
                Check(connected == NI_OK && peer == 1, "accepted connect");
                if (mode == 1) { channel.close(1, NI_CANCELLED); }
                Check(!channel.poll() && listener.m_closed == 1 && listener.m_reason == (mode == 0 ? NI_NO_MEMORY : NI_CANCELLED),
                    "shutdown exception lost terminal or first reason");
            }
            for (uint32_t index = 0; index < 1000; ++index)
            {
                Check(!channel.poll(), "closed poll revived");
            }
            channel.shutdown();
            Check(transport.m_stops == 1 && listener.m_closed == (mode < 2 ? 1U : 0U) && listener.m_other == 0,
                "duplicate stop/callback");
        }
        {
            NetGatewayChannelTestTransport transport;
            NetGatewayChannelTestListener listener;
            NetGatewayClientChannel channel(101, &transport);
            transport.m_busy = true;
            uint64_t peer = 0;
            Check(channel.connect("127.0.0.1", 1, options, &listener, peer) == NI_OK && channel.poll(), "busy OPEN setup");
            uint8_t expected[256];
            const uint32_t expectedSize = transport.m_controlSize;
            std::memcpy(expected, transport.m_control, expectedSize);
#if defined(_MSC_VER) && defined(_DEBUG)
            static uint32_t s_allocations = 0;
            const auto Hook = [](int operation, void*, size_t, int block, long, const unsigned char*, int) -> int
            {
                if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK) { ++s_allocations; }
                return 1;
            };
            _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(Hook);
#endif
            bool unchanged = true;
            for (uint32_t index = 0; index < 4096; ++index)
            {
                unchanged = channel.poll() && transport.m_controlSize == expectedSize &&
                    std::memcmp(expected, transport.m_control, expectedSize) == 0 && unchanged;
            }
#if defined(_MSC_VER) && defined(_DEBUG)
            _CrtSetAllocHook(previous);
            Check(s_allocations == 0, "busy OPEN reallocated");
#endif
            Check(unchanged && transport.m_controls == 4097, "busy OPEN changed bytes/phase");
            transport.m_busy = false;
            Check(channel.poll() && transport.m_controls == 4098, "OPEN not accepted after backpressure");
            Check(channel.poll() && transport.m_controls == 4098, "OPEN replayed after acceptance");
            channel.shutdown();
            Check(listener.m_closed == 1 && listener.m_reason == NI_OK, "normal close result");
        }
        for (uint32_t mode = 0; mode < 10; ++mode)
        {
            NetGatewayChannelTestTransport transport;
            NetGatewayChannelTestListener listener;
            NetGatewayClientChannel channel(101, &transport);
            uint64_t peer = 0;
            Check(channel.connect("127.0.0.1", 1, options, &listener, peer) == NI_OK && channel.poll(), "response setup");
            ProtoMessage message;
            message.setCode(NI_OK);
            message.setKeyValue(NI_GATEWAY_CHANNEL, Variant(static_cast<uint64_t>(mode == 3 ? 2 : 1)));
            message.setKeyValue(NI_GATEWAY_WINDOW, Variant(static_cast<int32_t>(mode == 4 ? 65537 : 65536)));
            std::string body;
            Check(message.toString(body), "response encoding");
            NetInterfaceFrame frame = {};
            frame.m_kind = NI_RESPONSE;
            frame.m_messageType = NI_GATEWAY_OPEN;
            frame.m_requestId = 1;
            frame.m_payload = reinterpret_cast<const uint8_t*>(body.data());
            frame.m_payloadSize = static_cast<uint32_t>(body.size());
            if (mode == 0) { frame.m_productId = 99; }
            if (mode == 1) { frame.m_requestId = 2; }
            if (mode == 2) { frame.m_payload = nullptr; }
            if (mode == 5) { frame.m_payloadSize = 16385; }
            Check(transport.m_listener->onLinkControl(7, frame), "response consumption");
            if (mode >= 6)
            {
                if (mode == 6)
                {
                    Check(transport.m_listener->onLinkControl(7, frame), "duplicate OPEN consumption");
                }
                else if (mode == 7)
                {
                    NetInterfaceTunnelRecord record = {};
                    record.m_kind = NI_TUNNEL_BYTES;
                    record.m_channel = 1;
                    record.m_product = 101;
                    record.m_data = reinterpret_cast<const uint8_t*>(body.data());
                    record.m_size = 16385;
                    Check(transport.m_listener->onLinkRecord(7, record), "oversized record consumption");
                }
                else
                {
                    message.setKeyValue(NI_GATEWAY_CREDIT_BYTES, Variant(static_cast<int32_t>(1)));
                    if (mode == 9) { message.setKeyValue(NI_GATEWAY_CHANNEL, Variant(static_cast<uint64_t>(2))); }
                    Check(message.toString(body), "event encoding");
                    frame.m_kind = NI_EVENT;
                    frame.m_requestId = 0;
                    frame.m_messageType = mode == 8 ? NI_GATEWAY_CREDIT : NI_GATEWAY_CHANNEL_CLOSE;
                    frame.m_payload = reinterpret_cast<const uint8_t*>(body.data());
                    frame.m_payloadSize = static_cast<uint32_t>(body.size());
                    Check(transport.m_listener->onLinkControl(7, frame), "event consumption");
                }
            }
            Check(!channel.poll() && listener.m_closed == 1 && listener.m_reason == NI_PROTOCOL_ERROR && listener.m_other == 0,
                "invalid control/record accepted");
            channel.shutdown();
        }
        std::printf("Business channel lifecycle checks=%u PASS\n", checks);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::printf("Business channel checks=%u FAIL %s\n", checks, error.what());
    }
    catch (...)
    {
        std::printf("Business channel checks=%u FAIL escaped exception\n", checks);
    }
    return 1;
}

void NetGatewayChannelStateTest::run()
{
    if (check() != 0)
    {
        throw std::runtime_error("NetGatewayChannelStateTest failed");
    }
}
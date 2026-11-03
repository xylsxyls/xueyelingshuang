#include "NetTunnelTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <algorithm>
#include <cstring>

void NetTunnelTestCases::portableRecords()
{
    const uint8_t cipher[] = {0x16, 0x03, 0x03, 0x00, 0xff, 0x10, 0x42};
    const uint32_t fields[] = {NI_TUNNEL_BYTES, 7, 0x76543210U, 0xfedcba98U, sizeof(cipher)};
    uint8_t wire[31] = {};
    uint32_t size = 0;
    NetInterfaceTlsTestPair::require(NetInterface::tunnelEncode(fields, cipher, nullptr, 0, &size) == NI_BUFFER_TOO_SMALL &&
        size == sizeof(wire), "route envelope size probe failed");
    NetInterfaceTlsTestPair::require(NetInterface::tunnelEncode(fields, cipher, wire, sizeof(wire), &size) == NI_OK,
        "encode route envelope failed");
    const uint8_t fixed[] = {'N','T','W','3',3,3,0,0,0,0,0,7,0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10,0,0,0,7};
    NetInterfaceTlsTestPair::require(std::memcmp(wire, fixed, sizeof(fixed)) == 0 &&
        std::memcmp(wire + sizeof(fixed), cipher, sizeof(cipher)) == 0, "outer route changed opaque TLS bytes or wire vector");
    for (uint32_t fragment = 1; fragment <= sizeof(wire); ++fragment)
    {
        NetInterfaceTunnelHandle decoder = nullptr;
        NetInterfaceTlsTestPair::require(NetInterface::tunnelCreate(&decoder) == NI_OK, "create route decoder failed");
        uint32_t offset = 0;
        int32_t result = NI_NEED_MORE;
        while (offset < sizeof(wire))
        {
            uint32_t consumed = 0;
            const uint32_t count = (std::min)(fragment, static_cast<uint32_t>(sizeof(wire)) - offset);
            result = NetInterface::tunnelFeed(decoder, wire + offset, count, &consumed);
            NetInterfaceTlsTestPair::require(consumed == count && result >= 0, "fragmented route record rejected");
            offset += consumed;
        }
        uint32_t actual[5] = {};
        const uint8_t* payload = nullptr;
        NetInterfaceTlsTestPair::require(result == NI_READY && NetInterface::tunnelPeek(decoder, actual, &payload) == NI_READY &&
            std::memcmp(fields, actual, sizeof(fields)) == 0 && std::memcmp(cipher, payload, sizeof(cipher)) == 0,
            "route record corrupted the 64-bit channel or TLS bytes");
        uint32_t consumed = 99;
        NetInterfaceTlsTestPair::require(NetInterface::tunnelFeed(decoder, wire, sizeof(wire), &consumed) == NI_READY && consumed == 0,
            "unconsumed route record lost backpressure");
        NetInterfaceTlsTestPair::require(NetInterface::tunnelConsume(decoder) == NI_OK && NetInterface::tunnelFinish(decoder) == NI_OK,
            "route record completion failed");
        NetInterfaceTlsTestPair::require(NetInterface::tunnelFeed(decoder, wire, sizeof(wire) - 1, &consumed) == NI_NEED_MORE &&
            NetInterface::tunnelFinish(decoder) == NI_TRUNCATED, "truncated route record accepted");
        NetInterface::tunnelDestroy(decoder);
    }
    for (uint32_t variant = 0; variant < 4; ++variant)
    {
        uint8_t invalid[24] = {};
        std::memcpy(invalid, wire, sizeof(invalid));
        if (variant == 0) { invalid[4] = 2; }
        if (variant == 1) { invalid[6] = 1; }
        if (variant == 2) { std::memset(invalid + 12, 0, 8); }
        if (variant == 3) { invalid[20] = 0xff; }
        NetInterfaceTunnel decoder;
        uint32_t consumed = 0;
        NetInterfaceTlsTestPair::require(decoder.feed(invalid, sizeof(invalid), consumed) < 0 && consumed == sizeof(invalid),
            "invalid version, flags, channel or oversized body accepted");
    }
    uint8_t sentinel[31];
    std::memset(sentinel, 0xab, sizeof(sentinel));
    NetInterfaceTlsTestPair::require(NetInterface::tunnelEncode(fields, cipher, sentinel, sizeof(sentinel) - 1, &size) ==
        NI_BUFFER_TOO_SMALL && sentinel[0] == 0xab && sentinel[30] == 0xab, "size probe wrote into caller buffer");
    NetInterfaceTlsTestPair::require(NetInterface::tunnelEncode(fields, sentinel + 24, sentinel, sizeof(sentinel), &size) ==
        NI_INVALID_ARGUMENT && sentinel[0] == 0xab, "overlapping envelope memory was accepted");
}
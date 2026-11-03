#include "NetInterfaceMacro.h"
#include "NetInterface.h"

// 公开根是整个网络发行系列的稳定信任锚，不随节点身份重建改变。
static const char g_networkAuthority[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIDFTCCAf2gAwIBAgIUNr7MWN6soOVUJlBfxQjnNDCgSmswDQYJKoZIhvcNAQEL\n"
"BQAwGjEYMBYGA1UEAwwPQ29vayBHYXRld2F5IENBMB4XDTI2MTAxNDAyMzUwMloX\n"
"DTM2MTAxMTAyMzUwMlowGjEYMBYGA1UEAwwPQ29vayBHYXRld2F5IENBMIIBIjAN\n"
"BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA7SRDR09NFIWyPnj+AybUMPQ0IIYi\n"
"YdNpjBlDPOrJEu8/8H/2B6YbfObr1E/3SrglgVeVZneTkpfVybd6nZ9uzot+7gqm\n"
"OQZYXWZIdv/+dItoqFGFYIBhQOcvwIokdrBeg13nRXGY2S9AiPgRtAdKLda1gKJh\n"
"eUMDf1X2E76oFWDnEzMbgvdceo+f8EVgPQBIndWFOVDuO3rIQFb6y9j/Iw9CW5Pt\n"
"CBbX5foSM4BaRkRkoIh9IWOmfD1PidWrWTbfq7nxOvCr1m/sv/HSoa5seM6lOmJz\n"
"VjMW5NvZzCt0gwG9zKX33FssDYeEcwMmEpkIZsytoTgQcy+G+gRV2iVBlQIDAQAB\n"
"o1MwUTAdBgNVHQ4EFgQUG6c74EL3DU+dQaJ4x0vFFIZjbjUwHwYDVR0jBBgwFoAU\n"
"G6c74EL3DU+dQaJ4x0vFFIZjbjUwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0B\n"
"AQsFAAOCAQEAmKRdL8Yo6X18gy1wnyfcXbKAgEdTBYvMWFQ/exZ/opPEF1sB04Hp\n"
"0qOh3O0phuBZrQj9XaL8qrOY4rz+M+uFn/511i5jKBmMAfit7zrRrysRPBB/aZKK\n"
"4jeapKJwh+7d4kpFmZMGeniOIJCQ4hNUwSKR52bjxzbS57xBhtgc8aBv5VXj6Yei\n"
"ytLNldA2lP328YwccMesOWeq6qM9Kcd86+syo3/kl0+38mFWpDok2oTyQ8HhhJ4J\n"
"hD8MbABWKMCEFK1GqPchhm2JHDtwHRwcnbTiQpbUdFOBCr3SU5SYjWcg1Y+yU869\n"
"8bRWfr9/8RKJdMKLmwzUrf6yTmgBbPfllw==\n"
"-----END CERTIFICATE-----\n";

const uint8_t* NetInterface::trustAuthority(uint32_t* size)
{
    if (size != nullptr)
    {
        *size = static_cast<uint32_t>(sizeof(g_networkAuthority));
    }
    return reinterpret_cast<const uint8_t*>(g_networkAuthority);
}
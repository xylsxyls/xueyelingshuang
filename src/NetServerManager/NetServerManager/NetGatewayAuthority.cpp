#include "NetGatewayAuthority.h"
#include "CEncodeDecode/CEncodeDecodeAPI.h"
#include <algorithm>
#include "NetInterface/NetInterfaceAPI.h"
#include <cstring>
#include <ctime>

// 固定签名私钥由本轮已授权的本机迁移写入此处；构建和身份重建不生成新根。
static const char g_gatewaySigningKey[] =
"-----BEGIN PRIVATE KEY-----\n"
"MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDtJENHT00UhbI+\n"
"eP4DJtQw9DQghiJh02mMGUM86skS7z/wf/YHpht85uvUT/dKuCWBV5Vmd5OSl9XJ\n"
"t3qdn27Oi37uCqY5BlhdZkh2//50i2ioUYVggGFA5y/AiiR2sF6DXedFcZjZL0CI\n"
"+BG0B0ot1rWAomF5QwN/VfYTvqgVYOcTMxuC91x6j5/wRWA9AEid1YU5UO47eshA\n"
"VvrL2P8jD0Jbk+0IFtfl+hIzgFpGRGSgiH0hY6Z8PU+J1atZNt+rufE68KvWb+y/\n"
"8dKhrmx4zqU6YnNWMxbk29nMK3SDAb3MpffcWywNh4RzAyYSmQhmzK2hOBBzL4b6\n"
"BFXaJUGVAgMBAAECggEAJQcJfkPynCa4CysnNmTEsxQeY4muKqoxC9ASAJwEY3F6\n"
"AVpplw4nspv37MHDsiYIrSZ4zHQVVclhd0qv2ZbAXX3IhvCgvbyw5XyIChS0rYvq\n"
"/oUkF0pUZSbz/NNVGDc3urlZ2C0Bmb/YCsr5MKwGuBuTYY95Os7s5+ALOVAGvLeE\n"
"xpka5N/N2s2ljzPXPAJ+BV9M9yjvWrNrqXu+0JgOYB8TQfR4n6H7WRBFLDw5FMfZ\n"
"dJYjQVvpR3X1Gc8nIGNjf1xfTAre2HKJkWXJISbSVZtZx/0ssoLihwgQ6Suhm6e8\n"
"+ciekF3yZiwOWbBJuIEiq4dC+7QhFXDTfX1BDKqHbQKBgQD75CrH2o3QQwgqBgiO\n"
"3iJG0Or++9yEiyBGLJIBjasSZ2z7kbnSi7XlBBg+EtznmBwAl0jkhEuYd1yFivhi\n"
"CmYQUYNmI1x7ndtL65Of8JP9F+5h1UBhwDHsE7V9rr9mvAEZnDORRzvuVlwGukrs\n"
"VDnVf0rfszBbYW9PsJf39hc09wKBgQDxAoFMFAS157ywG8lT9q2F603IpiD3y42C\n"
"R56j03Wsr0oIHrEWAVRXMyWxsPosoeBJG1iRMjd8PC9ajNrpTcCG1YaOe+sTV+R1\n"
"Ge6CLDaxNnR0cfFkrPSoYUIuJxCsANvAGGP7b0IlUiOynTW54uzqlpp7xkZuNObx\n"
"tNP/x0u20wKBgCJyBy+hQXdTU1B71rvCwKkydJA93N/7LoSqH6U6vEQeBJc6TFDw\n"
"bgVNVAtZBQBemqWyCnPFa5kD6ZERefneNirRGsji2QjGg3CUXHGosnQPy7dxNAaq\n"
"A6mm4FC5js77sqLP63VYamejQ55f51L+CZKaf6CrnldAsboSChYMfwvtAoGAFLrX\n"
"bjjGkvLkyu0I6xzrnfo4TYrkiQvMFymb3pGArKLDJmGlb7QbkgiHNHp5ej9OMFuW\n"
"yZ5dIWV32T0GU0zpw284TKch+B72OpBJcI7nAWUirSVj0voBbT44WAnqKDFGtYYj\n"
"i2vtb5JaGbHl/iAjqqXbkcvjIPWR67ZXCcgVnhsCgYEAwzNkBh3WdwhBEvR8F2iG\n"
"v/2dddsRL8tK+zwnMg5NvN/Q6lr+6T0sC9MC5A3EDV0VLTZkpnPc5hlDjrOq9KsC\n"
"vvxjcbPX02bDIdF1nWnEKPHwynFd4o1xN2Ja6xrwkRAuKPeLlnTVO8RIPCM4mCS/\n"
"gSMnbKoIjkvy9lDYohhxvJE=\n"
"-----END PRIVATE KEY-----\n";

/** 用UTC表示证书期限，不受本机时区影响
@param [in] value Unix秒
@param [out] output 至少15字节的年月日时分秒
@return 是否成功
*/
static bool GatewayAuthorityDate(time_t value, char* output)
{
    tm utc = {};
#ifdef _WIN32
    if (gmtime_s(&utc, &value) != 0)
#else
    if (gmtime_r(&value, &utc) == nullptr)
#endif
    {
        return false;
    }
    return std::strftime(output, 15, "%Y%m%d%H%M%S", &utc) == 14;
}

int32_t NetGatewayAuthority::issueLocalIdentity(const std::string& name, const std::string& request,
    NetInterfaceEntropyFunction entropy, void* context, std::string& certificate)
{
    certificate.clear();
    if (!NetInterfaceCertificate::validName(name) ||
        (name.compare(0, 5, "node.") != 0 && name.compare(0, 8, "service.") != 0) ||
        request.empty() || request.size() > 4096 || request.find('\0') != std::string::npos || entropy == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    const time_t now = std::time(nullptr);
    char before[15] = {};
    char after[15] = {};
    if (now < 0 || !GatewayAuthorityDate(now - 300, before) || !GatewayAuthorityDate(now + 365 * 86400, after))
    {
        return NI_INVALID_STATE;
    }
    uint32_t rootSize = 0;
    const uint8_t* rootPem = NetInterface::trustAuthority(&rootSize);
    const uint8_t alias[] = "node.gateway";
    const CEncodeDecodeBytes aliases[] = {{alias, sizeof(alias)}};
    CEncodeDecodeCertificateIssue options = {};
    options.m_name.m_bytes = reinterpret_cast<const uint8_t*>(name.c_str());
    options.m_name.m_size = static_cast<uint32_t>(name.size() + 1);
    options.m_request.m_bytes = reinterpret_cast<const uint8_t*>(request.c_str());
    options.m_request.m_size = static_cast<uint32_t>(request.size() + 1);
    options.m_authority.m_bytes = rootPem;
    options.m_authority.m_size = rootSize;
    options.m_privateKey.m_bytes = reinterpret_cast<const uint8_t*>(g_gatewaySigningKey);
    options.m_privateKey.m_size = sizeof(g_gatewaySigningKey);
    options.m_notBefore.m_bytes = reinterpret_cast<const uint8_t*>(before);
    options.m_notBefore.m_size = sizeof(before);
    options.m_notAfter.m_bytes = reinterpret_cast<const uint8_t*>(after);
    options.m_notAfter.m_size = sizeof(after);
    options.m_dnsNames = aliases;
    options.m_dnsNameCount = name.compare(0, 5, "node.") == 0 ? 1 : 0;
    options.m_tlsServer = true;
    options.m_tlsClient = true;
    options.m_entropy = entropy;
    options.m_entropyContext = context;
    uint8_t bytes[8192] = {};
    uint32_t length = 0;
    const int32_t result = CEncodeDecode::p256IssueCertificate(options, bytes, sizeof(bytes), &length);
    if (result != CENCODE_OK)
    {
        return result == CENCODE_MEMORY ? NI_NO_MEMORY : NI_PROTOCOL_ERROR;
    }
    try
    {
        certificate.assign(reinterpret_cast<const char*>(bytes), length - 1);
    }
    catch (...)
    {
        certificate.clear();
        return NI_NO_MEMORY;
    }
    const int32_t verified = NetInterfaceCertificate::verify(certificate, name);
    if (verified != NI_OK)
    {
        certificate.clear();
    }
    return verified;
}

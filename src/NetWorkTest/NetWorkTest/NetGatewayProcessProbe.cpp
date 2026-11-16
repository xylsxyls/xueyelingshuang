#include "NetGatewayProcessProbe.h"
#include "NetGatewayServiceProbe.h"
#include "NetGatewayManagedProbe.h"
#include "CSystem/CSystemAPI.h"
#include <string>
#include <vector>

/** 有界解析十进制参数，不调用抛异常的数值转换
@param [in] text 纯十进制数字
@param [in] maximum 包含在内的数值上限
@param [out] value 成功返回解析数值
@return 每个字符及范围是否有效
*/
static bool GatewayProbeNumber(const std::string& text, uint32_t maximum, uint32_t& value)
{
    if (text.empty() || text.size() > 10 || text.find_first_not_of("0123456789") != std::string::npos)
    {
        return false;
    }
    uint64_t parsed = 0;
    for (char digit : text)
    {
        parsed = parsed * 10 + digit - '0';
    }
    if (parsed > maximum)
    {
        return false;
    }
    value = static_cast<uint32_t>(parsed);
    return true;
}

int NetGatewayProcessProbe::run(int argc, char** argv)
{
    std::vector<std::string> arguments;
    if (!CSystem::commandArguments(argc, argv, arguments))
    {
        return 2;
    }
    if (arguments.size() == 3 && (arguments[0].compare(0, 20, "--self-test-managed=") == 0 ||
        arguments[0].compare(0, 29, "--self-test-managed-internal=") == 0 ||
        arguments[0].compare(0, 21, "--self-test-capacity=") == 0) &&
        arguments[1].compare(0, 12, "--stop-file=") == 0 && arguments[2].compare(0, 13, "--run-for-ms=") == 0)
    {
        uint32_t pid = 0;
        uint32_t duration = 0;
        if (!GatewayProbeNumber(arguments[0].substr(arguments[0].find('=') + 1), INT32_MAX, pid) ||
            !GatewayProbeNumber(arguments[2].substr(13), 60000, duration)) { return 2; }
        return NetGatewayManagedProbe::run(static_cast<int32_t>(pid), arguments[1].substr(12), duration,
            arguments[0].find("-internal=") != std::string::npos, arguments[0].find("-capacity=") != std::string::npos);
    }
    if (arguments.size() == 3 && (arguments[0].compare(0, 20, "--self-test-service=") == 0 ||
        arguments[0].compare(0, 21, "--self-test-internal=") == 0 || arguments[0].compare(0, 18, "--self-test-query=") == 0) &&
        arguments[1].compare(0, 12, "--stop-file=") == 0 && arguments[2].compare(0, 13, "--run-for-ms=") == 0)
    {
        uint32_t pid = 0;
        uint32_t duration = 0;
        if (!GatewayProbeNumber(arguments[0].substr(arguments[0].find('=') + 1), INT32_MAX, pid) ||
            !GatewayProbeNumber(arguments[2].substr(13), NetGatewayServiceProbe::MaximumDurationMs, duration)) { return 2; }
        const bool internal = arguments[0].compare(0, 21, "--self-test-internal=") == 0;
        const bool query = arguments[0].compare(0, 18, "--self-test-query=") == 0;
        return NetGatewayServiceProbe::run(static_cast<int32_t>(pid), arguments[1].substr(12), duration, internal ? 0 : 1, query);
    }
    return 2;
}
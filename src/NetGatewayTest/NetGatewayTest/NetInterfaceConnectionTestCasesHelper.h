#pragma once
#include <cstdint>

/** 同模块跨类使用的局部辅助，不保存可变业务状态
*/
class NetInterfaceConnectionTestCasesHelper
{
public:
    /** 与真实会话相同的单调时间，仅供已验证帧的握手Mock
    @return 单调毫秒
    */
    static uint64_t recoveryTestNow();
};
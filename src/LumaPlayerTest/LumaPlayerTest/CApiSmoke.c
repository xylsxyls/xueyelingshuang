#define LUMAPLAYERCORE_USE_C_API
#include "LumaPlayerCore/LumaPlayerCoreAPI.h"

/* 纯C编译和运行验证，不允许通过C++编译器掩盖C头问题 */
int RunPureCApiSmoke(void)
{
    LumaPlayerCoreCConfig config;
    LumaPlayerCoreHandle handle;
    int result = lumaPlayerCoreDefaultConfig(&config);
    if (result != LumaPlayerCoreCResultSuccess || config.m_defaultRatePermille != 1000)
    {
        return 1;
    }
    handle = lumaPlayerCoreCreate();
    if (handle == 0)
    {
        return 2;
    }
    result = lumaPlayerCoreInitWithConfig(handle, &config);
    lumaPlayerCoreUninit(handle);
    lumaPlayerCoreDestroy(handle);
    return result;
}
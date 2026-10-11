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

#ifdef LUMAPLAYER_CORE_LINK_CHECK
/** 纯C入口，链接检查目标使用全部Core对象而不由静态归档隐藏缺失符号
@return 零表示纯C初始化和清理检查通过
*/
int main(void)
{
    return RunPureCApiSmoke();
}
#endif
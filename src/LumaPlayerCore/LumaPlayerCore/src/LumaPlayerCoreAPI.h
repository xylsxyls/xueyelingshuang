#pragma once
#ifdef _WIN32
#define _LumaPlayerCoreAPI
#endif

// 调用方在首次包含前定义LUMAPLAYERCORE_USE_C_API选择纯C接口，默认使用C++接口
#ifdef LUMAPLAYERCORE_USE_C_API
#include "LumaPlayerCoreC.h"
#else
#include "LumaPlayerCore.h"
#endif

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"LumaPlayerCored.lib")
#else
#pragma comment(lib,"LumaPlayerCore.lib")
#endif
#endif
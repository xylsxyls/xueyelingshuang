#pragma once
#ifdef _WIN32
#define _LumaPlayerCoreAPI
#endif
#include "LumaPlayerCore.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"LumaPlayerCored.lib")
#else
#pragma comment(lib,"LumaPlayerCore.lib")
#endif
#endif
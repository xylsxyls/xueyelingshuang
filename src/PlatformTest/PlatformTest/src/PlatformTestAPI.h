#pragma once
#ifdef _WIN32
#define _PlatformTestAPI
#endif
#include "PlatformTest.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"PlatformTestd.lib")
#else
#pragma comment(lib,"PlatformTest.lib")
#endif
#endif
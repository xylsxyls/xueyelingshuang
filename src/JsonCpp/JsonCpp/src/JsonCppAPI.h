#pragma once
#ifdef _WIN32
#define _JsonCppAPI
#endif
#include "json/json.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"JsonCppd.lib")
#else
#pragma comment(lib,"JsonCpp.lib")
#endif
#endif
#pragma once
#define _JsonCppAPI
#include "json/json.h"

#ifdef _DEBUG
#pragma comment(lib,"JsonCppd.lib")
#else
#pragma comment(lib,"JsonCpp.lib")
#endif
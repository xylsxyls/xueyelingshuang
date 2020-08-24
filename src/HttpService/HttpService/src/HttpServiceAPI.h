#pragma once
#ifdef _WIN32
#define _HttpServiceAPI
#endif
#include "HttpService.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"HttpServiced.lib")
#else
#pragma comment(lib,"HttpService.lib")
#endif
#endif
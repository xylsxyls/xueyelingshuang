#pragma once
#ifdef _WIN32
#define _HttpRequestAPI
#endif
#include "HttpRequest.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"HttpRequestd.lib")
#else
#pragma comment(lib,"HttpRequest.lib")
#endif
#endif
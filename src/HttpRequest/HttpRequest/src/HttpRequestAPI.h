#pragma once
#define _HttpRequestAPI
#include "HttpRequest.h"

#ifdef _DEBUG
#pragma comment(lib,"HttpRequestd.lib")
#else
#pragma comment(lib,"HttpRequest.lib")
#endif
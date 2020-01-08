#pragma once
#define _HttpServiceAPI
#include "HttpService.h"
#include "HttpInterface.h"

#ifdef _DEBUG
#pragma comment(lib,"HttpServiced.lib")
#pragma comment(lib,"civetwebd.lib")
#else
#pragma comment(lib,"HttpService.lib")
#pragma comment(lib,"civetweb.lib")
#endif
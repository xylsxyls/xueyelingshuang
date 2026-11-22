#ifndef NET_WORK_API_INCLUDED
#define NET_WORK_API_INCLUDED
#ifdef _WIN32
#define _NetWorkAPI
#endif
#ifdef __cplusplus
#include "NetWork.h"


#else
// 仅共享值类型；核心功能入口只提供C++，不在这里选择NetSender。
#include "NetWorkTypes.h"
#endif

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"NetWorkd.lib")
#else
#pragma comment(lib,"NetWork.lib")
#endif
#endif

#endif
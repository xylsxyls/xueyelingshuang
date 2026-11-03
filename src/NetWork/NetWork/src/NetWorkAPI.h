#ifndef NET_WORK_API_INCLUDED
#define NET_WORK_API_INCLUDED
#ifdef _WIN32
#define _NetWorkAPI
#endif
#ifdef __cplusplus
#include "NetWork.h"

#ifdef NET_GATEWAY_TEST_ACCESS
// 仅静态白盒测试请求这些声明；正式调用方不得依赖内部布局。
#include "NetEndpointFileStream.h"
#include "NetEndpointRouterState.h"
#include "NetNativeFileIoState.h"
#include "NetWorkClientState.h"
#include "NetWorkHostBridge.h"
#include "NetWorkHostBridgeTest.h"
#endif
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
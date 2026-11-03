#ifndef LIBUV_TCP_API_INCLUDED
#define LIBUV_TCP_API_INCLUDED
#ifdef _WIN32
#define _LibuvTcpAPI
#endif
#include "LibuvTcp.h"
#include "LibuvChannel.h"

#ifdef NET_GATEWAY_TEST_ACCESS
// 仅静态白盒测试请求这些声明；正式调用方不得依赖内部布局。
#include "LibuvChannelModels.h"
#endif

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"LibuvTcpd.lib")
#else
#pragma comment(lib,"LibuvTcp.lib")
#endif
#endif

#endif
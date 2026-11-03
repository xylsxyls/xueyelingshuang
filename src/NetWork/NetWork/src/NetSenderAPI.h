#pragma once
#ifdef _WIN32
#define _NetWorkAPI
#endif

// 独立的业务入口；只选择本门面的C或C++声明，不引入核心主类
#if defined(NET_SENDER_USE_C) || !defined(__cplusplus)
#include "NetWork/NetSenderC.h"
#else
#include "NetWork/NetSender.h"
#endif

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"NetWorkd.lib")
#else
#pragma comment(lib,"NetWork.lib")
#endif
#endif
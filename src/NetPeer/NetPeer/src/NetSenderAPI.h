#pragma once
#ifdef _WIN32
#define _NetPeerAPI
#endif

// 独立的业务入口；只选择本门面的C或C++声明，不引入核心主类
#if defined(NET_SENDER_USE_C) || !defined(__cplusplus)
#include "NetPeer/NetSenderC.h"
#else
#include "NetPeer/NetSender.h"
#endif

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"NetPeerd.lib")
#else
#pragma comment(lib,"NetPeer.lib")
#endif
#endif
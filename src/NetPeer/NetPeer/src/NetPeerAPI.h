#ifndef NET_PEER_API_INCLUDED
#define NET_PEER_API_INCLUDED
#ifdef _WIN32
#define _NetPeerAPI
#endif
#ifdef __cplusplus
#include "NetPeer.h"


#else
// 仅共享值类型；核心功能入口只提供C++，不在这里选择NetSender。
#include "NetPeerTypes.h"
#endif

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"NetPeerd.lib")
#else
#pragma comment(lib,"NetPeer.lib")
#endif
#endif

#endif
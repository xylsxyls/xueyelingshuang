#ifndef NET_PEER_API_INCLUDED
#define NET_PEER_API_INCLUDED
#ifdef _WIN32
#define _NetPeerAPI
#endif
#ifdef __cplusplus
#include "NetPeer.h"

#ifdef NET_GATEWAY_TEST_ACCESS
// 仅静态白盒测试请求这些声明；正式调用方不得依赖内部布局。
#include "NetEndpointFileStream.h"
#include "NetEndpointRouterState.h"
#include "NetFileIoTest.h"
#include "NetFileTransferState.h"
#include "NetGatewayClientChannelState.h"
#include "NetGatewayDirectoryStoreState.h"
#include "NetGatewayDirectoryStoreTest.h"
#include "NetGatewayDiscoveryState.h"
#include "NetHostSocketState.h"
#include "NetHostSocketTest.h"
#include "NetLinkTransportState.h"
#include "NetPeerClientState.h"
#include "NetPeerTest.h"
#include "NetRelayTransportState.h"
#include "NetSessionState.h"
#include "NetTransportState.h"
#endif
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
#ifndef NET_SENDER_API_INCLUDED
#define NET_SENDER_API_INCLUDED
#ifdef _WIN32
#define _NetSenderAPI
#endif
#include "NetSenderPlatform.h"
#include "NetSender.h"

// 网关进程及服务接入所需的公开IPC辅助声明；不包含内部NetSenderIpc核心
#include "NetSenderLegacy.h"
#include "NetSenderDesktopBackend.h"
#include "NetSenderIpcChannel.h"
#include "NetSenderLocalPacket.h"
#include "NetSenderLocal.h"
#include "NetSenderLink.h"
#include "NetSenderServiceGateway.h"
#include "NetSenderServiceOutgoingTransport.h"
#include "NetSenderGatewayTransport.h"
#include "NetSenderService.h"
#include "NetSenderServiceProxy.h"
#include "NetSenderClientChannel.h"

#ifdef NET_GATEWAY_TEST_ACCESS
// 仅静态白盒测试请求这些声明；正式调用方不得依赖内部布局。
#include "NetSenderLinkAuditTest.h"
#include "NetSenderLocalHub.h"
#include "NetSenderLocalHubTest.h"
#include "NetSenderLocalState.h"
#include "NetSenderServiceGatewayState.h"
#include "NetSenderServiceGatewayTest.h"
#include "NetSenderServiceManaged.h"
#include "NetSenderServiceOutgoingTest.h"
#include "NetSenderServiceProxyState.h"
#include "NetSenderServiceProxyTest.h"
#include "NetSenderServiceQueue.h"
#include "NetSenderServiceQueueTest.h"
#include "NetSenderServiceState.h"
#include "NetSenderGatewayLaunch.h"
#endif

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"NetSenderd.lib")
#else
#pragma comment(lib,"NetSender.lib")
#endif
#endif

#endif
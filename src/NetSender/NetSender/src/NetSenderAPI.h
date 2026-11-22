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



#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"NetSenderd.lib")
#else
#pragma comment(lib,"NetSender.lib")
#endif
#endif

#endif
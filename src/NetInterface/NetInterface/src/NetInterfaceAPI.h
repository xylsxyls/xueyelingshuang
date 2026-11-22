#ifndef NET_INTERFACE_API_INCLUDED
#define NET_INTERFACE_API_INCLUDED
#ifdef _WIN32
#define _NetInterfaceAPI
#endif
#include "NetHostTypes.h"
#ifdef __cplusplus
#include "NetFileProtocol.h"
#include "NetInterface.h"
#include "NetExecution.h"
#include "NetBufferPool.h"
#include "NetInterfaceHandshake.h"
#include "NetInterfaceTunnel.h"
#include "NetClientDiscovery.h"
#include "NetInterfaceCertificate.h"
#include "NetInterfaceLink.h"
#include "NetInterfaceCredit.h"
#include "NetInterfaceDirectory.h"
#include "NetInterfaceTls.h"
#include "NetInterfaceConnection.h"
#include "NetInterfaceRequests.h"
#include "NetInterfaceRpc.h"
#include "NetInterfaceService.h"
#include "NetInterfaceFile.h"
#include "NetInterfaceFileFlow.h"
#include "NetInterfaceEvent.h"
#include "NetInterfaceTransport.h"
#include "NetSession.h"
#include "NetGatewayClientChannel.h"
#include "NetEndpointRouter.h"
#include "NetClientSession.h"
#include "NetSenderClientTypes.h"
#include "NetSenderSubmission.h"
#include "NetSenderDispatch.h"


#else
// 其他库的纯C ABI可借用这些值类型；NetInterface本身仅有C++调用接口
#include "NetInterfaceHandles.h"
#endif

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"NetInterfaced.lib")
#else
#pragma comment(lib,"NetInterface.lib")
#endif
#endif

#endif
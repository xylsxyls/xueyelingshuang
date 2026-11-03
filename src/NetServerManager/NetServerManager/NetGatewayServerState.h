#pragma once
#include "NetWork/NetWorkAPI.h"

// 仅保留私有兼容名称；完整布局只由静态测试入口提供。
struct NetEndpointPeer;

// Private compatibility aliases for the migrated endpoint implementation.
typedef NetEndpointRouterState NetGatewayServerState;
typedef NetEndpointPeer NetGatewayEndpoint;
#pragma once
#include "NetPeerMacro.h"
#include "NetHostTypes.h"
#include "NetInterface/NetInterfaceAPI.h"

// 独立客户端的连接参数；只包含值与借用回调，不声明C导出函数
typedef NetInterfaceClientOptions NetPeerClientOptions;

// 独立客户端的事件回调表；context借用至客户端释放
typedef NetInterfaceClientCallbacks NetPeerClientCallbacks;

// 本库管理的独立客户端上下文；布局仅在实现中可见
typedef struct NetPeerClientOpaque* NetPeerClientHandle;
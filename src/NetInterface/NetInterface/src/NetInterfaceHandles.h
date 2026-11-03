#pragma once
#include "NetInterfaceTypes.h"
#include "NetInterfaceClientTypes.h"
#include "NetInterfaceLinkTypes.h"
#include "NetInterfaceGatewayTypes.h"
#include "NetInterfaceTlsTypes.h"
#include "NetInterfaceFileTypes.h"
#include "NetInterfaceTunnelTypes.h"

// 模块拥有的工作句柄与调用方值记录；纯数据声明，不提供C函数接口
// 帧解码工作对象，必须由创建模块配对释放，不跨Wasm实例传递
typedef struct NetInterfaceOpaque* NetInterfaceHandle;

// TLS工作对象，必须由创建模块配对释放，不跨Wasm实例传递
typedef struct NetInterfaceTlsOpaque* NetInterfaceTlsHandle;

// TLS及完整帧连接，必须由创建模块配对释放，不跨Wasm实例传递
typedef struct NetInterfaceConnectionOpaque* NetInterfaceConnectionHandle;

// 增量摘要工作对象，必须由创建模块配对释放，不跨Wasm实例传递
typedef struct NetInterfaceHashOpaque* NetInterfaceHashHandle;

// protobuf消息工作对象，必须由创建模块配对释放，不跨Wasm实例传递
typedef struct NetInterfaceMessageOpaque* NetInterfaceMessageHandle;

// 通道字节额度工作对象，必须由创建模块配对释放，不跨Wasm实例传递
typedef struct NetInterfaceCreditOpaque* NetInterfaceCreditHandle;

// 有界服务目录工作对象，必须由创建模块配对释放，不跨Wasm实例传递
typedef struct NetInterfaceDirectoryOpaque* NetInterfaceDirectoryHandle;

// 服务目录节点值记录；不包含借用指针，可按值复制
typedef struct NetInterfaceDirectoryNodeData
{
    // 调用者填写sizeof(NetInterfaceDirectoryNodeData)。
    uint32_t m_structSize;
    // 当前为1，未知版本明确拒绝。
    uint32_t m_version;
    // 精确节点名，最多37字节及NUL。
    char m_name[38];
    // 数值IP，最多39字节及NUL。
    char m_address[40];
    // 1至65535。
    uint32_t m_port;
    // 1至256。
    uint32_t m_capacity;
    // 本次节点启动的非零代次。
    uint64_t m_boot;
} NetInterfaceDirectoryNodeData;

// 有界文件协议流，必须由创建模块配对释放，不跨Wasm实例传递
typedef struct NetInterfaceFileFlowOpaque* NetInterfaceFileFlowHandle;

// 业务身份握手工作对象，必须由创建模块配对释放，不跨Wasm实例传递
typedef struct NetInterfaceHandshakeOpaque* NetInterfaceHandshakeHandle;

// 请求及唯一终态记录表，必须由创建模块配对释放，不跨Wasm实例传递
typedef struct NetInterfaceRequestsOpaque* NetInterfaceRequestsHandle;

// 本库创建和释放的管理连接
typedef struct NetInterfaceLinkOpaque* NetInterfaceLinkHandle;

// 本库创建和释放的外层记录解码器
typedef struct NetInterfaceTunnelOpaque* NetInterfaceTunnelHandle;

// 节点证书请求与私钥PEM的单项最大容量
#define NET_INTERFACE_IDENTITY_PEM_BYTES 4096U
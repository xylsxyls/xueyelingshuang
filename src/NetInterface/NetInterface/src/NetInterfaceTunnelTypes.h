#pragma once
#include "NetInterfaceTypes.h"

// NTW3只运输TLS管理字节或业务端生成的TLS密文；路由授权由管理会话负责。
typedef enum NetInterfaceTunnelKind
{
    // 无载荷的外层协议开场，只允许一次
    NI_TUNNEL_HELLO = 1,
    // 通道0的管理TLS字节，角色和路由命令在TLS内验证
    NI_TUNNEL_CONTROL = 2,
    // 已授权业务通道的端到端TLS密文字节
    NI_TUNNEL_BYTES = 3,
    // 指定业务通道的关闭记录，不关闭其他通道
    NI_TUNNEL_CLOSE = 4
} NetInterfaceTunnelKind;

/** NTW3外层记录视图，不拥有载荷或改变业务TLS密文
*/
typedef struct NetInterfaceTunnelRecord
{
    // 外层记录类别，取NetInterfaceTunnelKind枚举值
    uint32_t m_kind;
    // 逻辑服务号；HELLO和CONTROL必须为0，业务记录按已授权通道解释
    uint32_t m_product;
    // 本次管理会话内的通道号，HELLO和CONTROL为0
    uint64_t m_channel;
    // 借用载荷，m_size为0时可空；编码或accept只在调用期间读取
    const uint8_t* m_data;
    // 载荷字节数，上限16384；HELLO为0，其他类别遵循各自记录约束
    uint32_t m_size;
} NetInterfaceTunnelRecord;

#define NET_INTERFACE_TUNNEL_HEADER_SIZE 24U
#define NET_INTERFACE_TUNNEL_PAYLOAD_LIMIT 16384U
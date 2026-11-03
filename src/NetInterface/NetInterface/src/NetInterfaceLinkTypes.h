#pragma once
#include "NetInterfaceTlsTypes.h"

// 管理身份与业务账号身份分离；角色只在管理认证通过后生效。
typedef enum NetInterfaceLinkRole
{
    // 客户端网关管理连接，仅取得公共服务路由，不代表账号授权
    NI_LINK_CLIENT = 1,
    // 已认证网关节点，用于可信节点间发现与转发
    NI_LINK_NODE = 2,
    // 仅本机授权业务服务，须提供独立服务凭证证明
    NI_LINK_SERVICE = 3
} NetInterfaceLinkRole;

/** 管理认证回调中的身份视图，全部指针仅在本次回调有效
*/
typedef struct NetInterfaceLinkPeer
{
    // 本端或已认证对端的管理角色，取NetInterfaceLinkRole枚举值
    uint32_t m_role;
    // 回调期间借用的身份名称，CLIENT为空，SERVICE为service.编号
    const char* m_name;
    // 回调期间借用的身份公证书字节，不包含私钥
    const uint8_t* m_certificate;
    // 公证书有效字节数，无证书角色为0
    uint32_t m_certificateSize;
    // 角色声明字节；配置时复制，验证回调中只借用到回调返回
    const uint8_t* m_claims;
    // 角色声明有效字节数，0时m_claims可空
    uint32_t m_claimsSize;
} NetInterfaceLinkPeer;

// 服务证明由服务端专用宿主提供；32字节凭证不能编入客户端/公共SDK/Wasm。
typedef int32_t (*NetInterfaceLinkServiceProof)(void* context, const uint8_t* statement, uint32_t statementSize,
    uint8_t* proof, uint32_t capacity);

// 仅本机授权服务监听安装此回调；公网必须在角色掩码上拒绝SERVICE。
// statement含验证端新挑战和全部角色/名称/CSR声明；grant通常为签发叶证书。
typedef int32_t (*NetInterfaceLinkServiceVerify)(void* context, const NetInterfaceLinkPeer* peer,
    const uint8_t* statement, uint32_t statementSize, const uint8_t* proof, uint32_t proofSize,
    uint8_t* grant, uint32_t capacity, uint32_t* grantSize);

/** 管理TLS及角色认证配置，字节配置被复制，回调上下文保持借用
*/
typedef struct NetInterfaceLinkOptions
{
    // 必须3；这是新增协议契约，不改变v1 ABI或允许隐式明文降级。
    uint32_t m_version;
    // server=1仅网关NODE管理监听；客户端0向网关建立管理TLS。
    NetInterfaceTlsOptions m_tls;
    // 本端或已认证对端的管理角色，取NetInterfaceLinkRole枚举值
    uint32_t m_role;
    // 服务端允许的1<<角色掩码，公网仅CLIENT/NODE；客户端必须0。
    uint32_t m_peerRoles;
    // NODE的唯一证书名、SERVICE的service.<编号>，CLIENT为空。
    const char* m_name;
    // 角色声明字节；配置时复制，验证回调中只借用到回调返回
    const uint8_t* m_claims;
    // 角色声明有效字节数，0时m_claims可空
    uint32_t m_claimsSize;
    // SERVICE角色生成凭证证明的回调，其他角色可空，不泄露原始凭证
    NetInterfaceLinkServiceProof m_serviceProof;
    // 本机管理监听验证服务证明的回调，公网禁止接纳SERVICE
    NetInterfaceLinkServiceVerify m_serviceVerify;
    // 借用的服务证明上下文，存活至管理连接销毁
    void* m_serviceContext;
} NetInterfaceLinkOptions;

#define NET_INTERFACE_LINK_CHALLENGE 0x4e4c0001U
#define NET_INTERFACE_LINK_PROOF 0x4e4c0002U
#define NET_INTERFACE_LINK_ACCEPT 0x4e4c0003U
#define NET_INTERFACE_LINK_HEARTBEAT 0x4e4c0004U
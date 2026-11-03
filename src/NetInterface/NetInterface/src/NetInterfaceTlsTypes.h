#pragma once
#include "NetInterfaceMacro.h"
#include <stdint.h>

// TLS实例状态；负数为持续错误，必须销毁实例后重新连接
typedef enum NetInterfaceTlsStatus
{
    // 本次输入已接受或输出已消费
    NET_INTERFACE_TLS_OK = 0,
    // 尚在握手，需输送网络字节并再次progress
    NET_INTERFACE_TLS_HANDSHAKE = 1,
    // 已验证服务器身份，可以收发应用数据
    NET_INTERFACE_TLS_READY = 2,
    // 对端已发送经过认证的close_notify
    NET_INTERFACE_TLS_CLOSED = 3,
    // 等待输入或输出空间，未发生错误
    NET_INTERFACE_TLS_WANT_IO = 4,
    // 空参数、越界长度或缺失安全配置
    NET_INTERFACE_TLS_ARGUMENT = -101,
    // 内存分配失败
    NET_INTERFACE_TLS_MEMORY = -102,
    // 证书、域名、完整性或TLS协议验证失败
    NET_INTERFACE_TLS_PROTOCOL = -103,
    // 宿主提供的单调时间已超过握手期限
    NET_INTERFACE_TLS_TIMEOUT = -104,
    // 未完成认证关闭的TCP断开
    NET_INTERFACE_TLS_TRUNCATED = -105,
    // 当前状态不允许此操作
    NET_INTERFACE_TLS_STATE = -106
} NetInterfaceTlsStatus;

// 由宿主提供操作系统级密码学随机源；返回0表示完整填充，非0终止握手
typedef int32_t (*NetInterfaceEntropyFunction)(void* context, uint8_t* output, uint32_t size);

// 创建时借用的配置；证书和域名在create中复制，随机回调及context借用至destroy
typedef struct NetInterfaceTlsOptions
{
    // 0为客户端，1为服务端；服务端TLS不替代业务账号认证
    int32_t server;
    // 客户端必须提供受信根PEM或DER；PEM字节数包含结尾NUL
    const uint8_t* authority;
    // 受信根字节数，最大128KiB
    uint32_t authoritySize;
    // 服务端证书链PEM或DER，客户端可空
    const uint8_t* certificate;
    // 证书链字节数，最大128KiB，PEM包含结尾NUL
    uint32_t certificateSize;
    // 服务端未加密私钥PEM或DER，只在create中借用
    const uint8_t* privateKey;
    // 私钥字节数，最大32KiB，PEM包含结尾NUL
    uint32_t privateKeySize;
    // 客户端必须指定DNS域名，1至253字节，禁止跳过域名验证
    const char* hostname;
    // 密码学随机回调，必须有效，不得使用时间或普通伪随机数
    NetInterfaceEntropyFunction entropy;
    // 随机回调上下文，可空，生命周期覆盖TLS实例
    void* entropyContext;
    // 握手时限，100至60000毫秒
    uint32_t handshakeTimeoutMs;
} NetInterfaceTlsOptions;
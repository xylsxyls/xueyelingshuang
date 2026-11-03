#pragma once
#include "NetInterfaceTypes.h"
#include "NetInterfaceTlsTypes.h"

// 客户端v1配置；协议公共层拥有类型，传输由调用者选择，布局不再修改
typedef struct NetInterfaceClientOptions
{
    // 必须为当前v1结构的sizeof值
    uint32_t structSize;
    // 当前为1，不兼容扩展使用新版本入口
    uint32_t version;
    // 借用至连接调用返回；由运输适配解释
    const char* address;
    // 端口1至65535，IPC适配按自身契约解释地址
    uint32_t port;
    // 身份及强随机参数，随机上下文借用至本代停止
    NetInterfaceTlsOptions tls;
    // 桌面1、手机2、小程序3，和传输方式无关
    uint32_t platform;
    // 应用握手能力位
    uint32_t capabilities;
    // 协商的在途请求上限
    uint32_t maxRequests;
    // 协商的数据块字节上限
    uint32_t maxChunk;
    // 应用握手期限，毫秒
    uint32_t handshakeTimeoutMs;
    // 产品合理作用域持久化的非零设备标识，不是会话nonce
    uint8_t device[16];
} NetInterfaceClientOptions;

// 回调在所属执行者交付，cancel可同步交付结果；表复制，context借用至destroy成功，禁止重入销毁/等待/抛异常
typedef struct NetInterfaceClientCallbacks
{
    // 当前v1结构的sizeof值
    uint32_t structSize;
    // 当前为1
    uint32_t version;
    // 借用的调用者上下文
    void* context;
    // TLS/应用握手均通过后报告就绪及协商容量
    void (*ready)(void* context, uint64_t generation, uint32_t maxRequests, uint32_t maxChunk);
    // 已接纳请求的唯一传输终态，bytes仅借用至返回
    void (*result)(void* context, uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status,
        const uint8_t* bytes, uint32_t size);
    // 数据/事件借用至返回；0表示无副作用背压，非0表示接纳
    int32_t (*frame)(void* context, uint64_t generation, const NetInterfaceFrame* frame);
    // 所有请求终态之后唯一关闭通知
    void (*closed)(void* context, uint64_t generation, int32_t reason, int32_t detail);
} NetInterfaceClientCallbacks;

// 历史源码别名，不包含/链接NetWork；两种门面继续保持原v1 ABI
typedef NetInterfaceClientOptions NetWorkClientOptions;
typedef NetInterfaceClientCallbacks NetWorkClientCallbacks;
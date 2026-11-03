#pragma once
#include "NetInterfaceMacro.h"
#include <stdint.h>

// 增量解码结果；负数需要调用方处理错误
typedef enum NetInterfaceResult
{
    // 操作成功，没有待消费消息
    NI_OK = 0,
    // 当前消息尚需更多输入
    NI_NEED_MORE = 1,
    // 一条完整消息等待显式消费
    NI_READY = 2,
    // 指针、配置或操作参数不合法
    NI_INVALID_ARGUMENT = -1,
    // 线格式错误，应断开当前连接
    NI_PROTOCOL_ERROR = -2,
    // 声明长度超过实例预算
    NI_LIMIT_EXCEEDED = -3,
    // 输出空间不足，输出缓冲不变
    NI_BUFFER_TOO_SMALL = -4,
    // 内存分配失败，应断开当前连接
    NI_NO_MEMORY = -5,
    // 输入结束时仍有不完整消息
    NI_TRUNCATED = -6,
    // 当前状态不支持此操作
    NI_INVALID_STATE = -7,
    // 指定消息字段不存在
    NI_NOT_FOUND = -8,
    // 当前操作超过固定单调截止时间
    NI_TIMEOUT = -9,
    // 控制取消先到达，后续完整请求不得执行
    NI_CANCELLED = -10,
    // 目标平台或兼容接口不提供此能力
    NI_NOT_SUPPORTED = -11
} NetInterfaceResult;

// 帧种类；身份鉴权及请求生命周期由会话层负责
typedef enum NetInterfaceKind
{
    // TLS认证完成后的应用能力协商
    NI_HELLO = 1,
    // 应用能力协商响应
    NI_HELLO_REPLY = 2,
    // 发往产品服务的请求
    NI_REQUEST = 3,
    // 先前请求的结果
    NI_RESPONSE = 4,
    // 已授权对端发送的事件
    NI_EVENT = 5,
    // 取消在途请求
    NI_CANCEL = 6,
    // 心跳探测
    NI_PING = 7,
    // 心跳响应
    NI_PONG = 8,
    // 有界数据块，不承载整个大文件
    NI_DATA = 9,
    // 传输确认或额度更新
    NI_DATA_ACK = 10
} NetInterfaceKind;

// RPC传输结果由C/C++共用；业务错误仍在protobuf正文中表达。
typedef enum NetInterfaceRpcStatus
{
    // 传输请求成功，业务结果仍由正文中的产品错误码决定
    NI_RPC_SUCCESS = 0,
    // 目标服务或通道当前不可用
    NI_RPC_UNAVAILABLE = 1,
    // 服务达到有界容量，当前请求未接纳
    NI_RPC_BUSY = 2,
    // 请求超过原始RPC总期限
    NI_RPC_TIMEOUT = 3,
    // 请求已被取消，不能据此认定远端副作用回滚
    NI_RPC_CANCELLED = 4,
    // 传输或远端处理失败，业务结果可能需要查询核实
    NI_RPC_FAILED = 5,
    // 目标拒绝来源或授权，不能作为有效账号证明
    NI_RPC_UNAUTHORIZED = 6
} NetInterfaceRpcStatus;

// 借用消息视图；不跨DLL传递需要调用方释放的内存
typedef struct NetInterfaceFrame
{
    // 网关解释的产品编号
    uint32_t m_productId;
    // 完整64位请求编号，不转换为JS浮点数
    uint64_t m_requestId;
    // 原业务消息类型，既有编号不改动
    uint32_t m_messageType;
    // NetInterfaceKind中的一种
    uint16_t m_kind;
    // 包括嵌入零字节的消息体长度
    uint32_t m_payloadSize;
    // 借用到consume/reset/destroy为止；空消息体为null
    const uint8_t* m_payload;
} NetInterfaceFrame;

#define NET_INTERFACE_HEADER_SIZE 32U
#define NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT 1048576U
#define NET_INTERFACE_MAX_PAYLOAD_LIMIT 16777216U
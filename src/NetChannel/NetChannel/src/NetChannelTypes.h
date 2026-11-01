#pragma once
#include <stdint.h>

// 字节通道的统一结果；成功、等待、可消费和错误分别判断
typedef enum NetChannelResult
{
    // 操作完成，没有待消费记录
    NC_OK = 0,
    // 需要更多输入
    NC_NEED_MORE = 1,
    // 完整记录等待显式消费
    NC_READY = 2,
    // 参数不合法，当前输入未接纳
    NC_INVALID_ARGUMENT = -1,
    // 对端字节或额度违约，当前代次不得继续
    NC_PROTOCOL_ERROR = -2,
    // 超过容量；发送窗口忙时未扣账
    NC_LIMIT_EXCEEDED = -3,
    // 输出缓冲不足，输出未修改
    NC_BUFFER_TOO_SMALL = -4,
    // 本模块分配失败
    NC_NO_MEMORY = -5,
    // EOF仍有未完成记录
    NC_TRUNCATED = -6,
    // 当前生命周期不允许操作
    NC_INVALID_STATE = -7,
    // 指定对象或字段不存在
    NC_NOT_FOUND = -8,
    // 固定总期限已到，不因重试延长
    NC_TIMEOUT = -9,
    // 已取消，不表示远端副作用已回滚
    NC_CANCELLED = -10,
    // 宿主或兼容入口未提供此能力
    NC_NOT_SUPPORTED = -11
} NetChannelResult;

// NTW3记录种类；字段只描述路由，不证明身份或权限
typedef enum NetChannelKind
{
    // 无载荷、产品和通道均为0的版本协商起点
    NC_HELLO = 1,
    // 通道0的非空管理TLS字节
    NC_CONTROL = 2,
    // 非0逻辑通道的非空业务密文
    NC_BYTES = 3,
    // 非0逻辑通道的四字节关闭原因
    NC_CLOSE = 4
} NetChannelKind;

// 借用视图；data仅在声明的消费/调用期限内有效，禁止跨模块释放
typedef struct NetChannelRecord
{
    // NetChannelKind值
    uint32_t m_kind;
    // 路由产品号，不能作为授权依据
    uint32_t m_product;
    // 本物理代次内的逻辑通道，完整64位
    uint64_t m_channel;
    // 借用记录体，size为0时可空
    const uint8_t* m_data;
    // 记录体字节数，不超过16KiB
    uint32_t m_size;
} NetChannelRecord;

#define NET_CHANNEL_HEADER_SIZE 24U
#define NET_CHANNEL_PAYLOAD_LIMIT 16384U
#define NET_CHANNEL_DEFAULT_WINDOW 65536U
#define NET_CHANNEL_MAX_WINDOW 1048576U
#pragma once
#include "NetBufferPoolConfig.h"

// 单连接TLS输出资源；控制与普通数据独立保留，但控制不能截断已开始的帧
struct NetInterfaceAPI NetConnectionPoolConfig
{
public:
    // 普通帧记录的初值、上限与增长策略
    LockFreeHashMapPoolConfig m_dataRecords;
    // 控制帧记录的初值、上限与增长策略
    LockFreeHashMapPoolConfig m_controlRecords;
    // 普通帧实际编码缓冲
    NetBufferPoolConfig m_dataBytes;
    // 控制帧实际编码缓冲
    NetBufferPoolConfig m_controlBytes;
    // 本连接接收端消费后保留的解码正文缓冲字节数，0不保留，-1保留至单帧上限
    int32_t m_decoderRetentionBytes;
    // 白名单RPC尝试压缩的最小正文长度，默认1024字节，0允许所有非空正文参与
    uint32_t m_compressionMinPayload;
    // 控制队列最多连续优先交付的帧数，默认32，至少1
    uint32_t m_controlBurst;
    // 可进入控制队列的最大正文字节数，默认4096；0仅空正文，-1采用单帧协议上限
    int32_t m_controlPayloadLimit;
    // 每次progress最多交给TLS的完整排队帧数，默认512，至少1
    uint32_t m_progressFrameBudget;
    // 每次progress最多执行的TLS读写推进轮数，默认64，至少1
    uint32_t m_progressStepBudget;
    // 发送端Zstandard等级0至9，默认1，0使用Compress的默认等级
    int32_t m_compressionLevel;

public:
    /** 默认物理记录预算各约8MiB；正文按大小类增长，无附加帧数阈值
    */
    NetConnectionPoolConfig();

    /** 校验帧缓存、编码及交付调度参数，物理池另由所属组件验证
    @return 参数在编解码器支持范围内时为true
    */
    bool validCodecSettings() const;
};
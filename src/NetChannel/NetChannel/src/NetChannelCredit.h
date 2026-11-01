#pragma once
#include "NetChannelMacro.h"
#include "NetChannelTypes.h"

// 单执行者维护一个逻辑通道的双向额度；仅在实际交付/消费边界扣账，不解释业务身份
class NetChannelAPI NetChannelCredit
{
public:
    /** 创建每方向64KiB窗口
    */
    NetChannelCredit();

    /** 使用前配置协商窗口，错误不修改原窗口
    @param [in] bytes 64KiB至1MiB且为16KiB倍数
    @return NC_OK、参数或状态错误
    */
    int32_t configure(uint32_t bytes);

    /** 查询固定协商容量
    @return 窗口字节数
    */
    uint32_t capacity() const;

    /** 查询尚可发送额度
    @return 可用字节数，持续错误后为0
    */
    uint32_t available() const;

    /** 只在下层已接纳记录副本后扣账
    @param [in] size 1至16KiB
    @return NC_OK、无副作用NC_LIMIT_EXCEEDED或错误
    */
    int32_t sent(uint32_t size);

    /** 核对远端真实消费后返还的额度
    @param [in] size 非0且不超过已发送未返还量
    @return NC_OK或持续协议错误
    */
    int32_t grant(uint32_t size);

    /** 记录接收端已经拥有且只接纳一次的密文字节
    @param [in] size 1至16KiB，累计不超过窗口
    @return NC_OK或持续协议错误
    */
    int32_t received(uint32_t size);

    /** 查询已接收且尚待返还的额度
    @return 字节数，持续错误后为0
    */
    uint32_t pending() const;

    /** 仅在管理队列已拥有额度消息后扣除待返还量
    @param [in] size 非0且不超过pending
    @return NC_OK或无副作用参数错误
    */
    int32_t consume(uint32_t size);

    /** 查询不可通过后续grant恢复的协议状态
    @return NC_OK或持续错误
    */
    int32_t status() const;

private:
    // 本代协商容量
    uint32_t m_capacity;
    // 是否已发生接纳，发生后不可改窗口
    bool m_used;
    // 已发送且尚未由远端确认消费的字节量，始终不超过容量
    uint32_t m_outstanding;
    // 已接收、尚待交管理队列的返还量
    uint32_t m_pending;
    // 第一个持续协议错误
    int32_t m_error;
};
#pragma once
#include "NetInterfaceTypes.h"
#include "NetChannel/NetChannelAPI.h"
class ProtoMessage;

// 每逻辑通道双向窗口；网关核对同一额度，避免宿主原生socket隐藏发送队列无限积累。
class NetInterfaceAPI NetInterfaceCredit
{
public:
    /** 缺省每方向64KiB，OPEN协商后可在使用前调整
    */
    NetInterfaceCredit();

    /** 当前可发送密文字节；失败后为0
    @return 0至已协商容量
    */
    uint32_t available() const;

    /** 使用前设置协商窗口；必须64KiB至1MiB且为16KiB倍数
    @param [in] bytes 协商字节数
    @return NI_OK或参数/状态错误，失败不修改原窗口
    */
    int32_t configure(uint32_t bytes);

    /** 当前已协商窗口
    @return 字节数
    */
    uint32_t capacity() const;

    /** 读取可选管理窗口字段；旧端点缺字段保持64KiB
    @param [in] message 同源protobuf
    @param [out] bytes 成功窗口
    @param [in] group 消息组号
    @return NI_OK或非法字段协议错误
    */
    static int32_t window(const ProtoMessage& message, uint32_t& bytes, int32_t group = 0);

    /** 记录下层已接纳的密文，背压时不扣额度
    @param [in] size 1至16384
    @return NI_OK、NI_LIMIT_EXCEEDED无副作用或错误
    */
    int32_t sent(uint32_t size);

    /** 校验管理通道返回的额度，不能超过确实已发送的字节
    @param [in] size 1至已协商容量
    @return NI_OK或持续协议错误
    */
    int32_t grant(uint32_t size);

    /** 记录已经由接收端有界缓存拥有的密文；同一记录只能调用一次
    @param [in] size 1至16384
    @return NI_OK或持续协议错误
    */
    int32_t received(uint32_t size);

    /** 待返回的已接收额度，尚未交管理队列时保留
    @return 0至已协商容量
    */
    uint32_t pending() const;

    /** 管理队列已经拥有额度消息后才消费
    @param [in] size 不大于pending的非零值
    @return NI_OK或参数错误
    */
    int32_t consume(uint32_t size);

    /** 持续协议状态，错误后不能靠后续grant恢复
    @return NI_OK或协议错误
    */
    int32_t status() const;

private:
    // protobuf窗口字段解析保留本层，字节记账只由NetChannel实现
    NetChannelCredit m_credit;
};
#pragma once
#include "NetInterfaceTypes.h"

/** 事件信封解析视图，不拥有原始帧内存
*/
struct NetInterfaceEventView
{
    // 已认证服务指定的根来源号，撤销分组时0表示匹配的全部来源
    uint64_t m_target;
    // 0定向业务通知；1撤销16字节文件授权分组（仅已认证服务可发）。
    uint32_t m_flags;
    // 事件正文视图，只借用到原始帧失效
    const uint8_t* m_data;
    // 正文有效字节数，撤销分组时固定16字节
    uint32_t m_size;
};

// 服务→网关的事件信封；公网客户端不能采用此头指定接收者或权限。
class NetInterfaceAPI NetInterfaceEvent
{
  public:
    /** 编码事件，下层接纳前调用方仍拥有输入
    @param [in] target 已知根连接号；撤销分组可用0表示所有匹配根
    @param [in] flags 0定向通知，1撤销分组
    @param [in] data 原业务体/16字节非零分组
    @param [in] size 最多1048552字节
    @param [out] output 容量不足可空；禁止与data重叠
    @param [in] capacity 空间
    @param [out] required 所需长度或0
    @return OK、BUFFER_TOO_SMALL或INVALID_ARGUMENT
    */
    static int32_t encode(uint64_t target, uint32_t flags, const uint8_t* data, uint32_t size, uint8_t* output, uint32_t capacity,
                          uint32_t& required);

    /** 解析已认证服务事件；源角色、产品、接收者仍由网关验证
    @param [in] frame EVENT/非零事件号
    @param [out] view 借用字段或清零
    @return OK或PROTOCOL_ERROR
    */
    static int32_t parse(const NetInterfaceFrame& frame, NetInterfaceEventView& view);
};
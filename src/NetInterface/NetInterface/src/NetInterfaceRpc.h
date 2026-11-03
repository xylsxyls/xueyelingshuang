#pragma once
#include "NetInterfaceTypes.h"

// 固定16字节NRP2信封后的借用业务数据。
struct NetInterfaceRpcView
{
    // REQUEST为剩余毫秒数，RESPONSE为NetInterfaceRpcStatus
    uint32_t m_value;
    // 仅已认证本地服务的内部REQUEST可非零；外部入口必须另外拒绝非零值
    uint64_t m_parent;
    // 不含16字节信封；生命周期与输入相同
    const uint8_t* m_data;
    // 不含16字节信封的业务正文长度，最多1048560字节
    uint32_t m_size;
};

// 原生和Wasm共同解释RPC信封；不从信封采用来源、PID、账号或权限。
class NetInterfaceAPI NetInterfaceRpc
{
  public:
    /** 编码网络/本地RPC消息体，固定字节序，不分配内存
    @param [in] kind 仅REQUEST或RESPONSE
    @param [in] value 请求1至60000毫秒或响应状态0至6
    @param [in] parent 内部父路由号；响应必须为0
    @param [in] data 业务二进制，size非0时不可空；错误响应必须为空
    @param [in] size 最大1048560
    @param [out] output 调用者空间，必须与data不重叠；不足时不改写
    @param [in] capacity 空间字节数
    @param [out] required 合法参数所需空间，否则0
    @return OK、BUFFER_TOO_SMALL或INVALID_ARGUMENT
    */
    static int32_t encode(uint16_t kind, uint32_t value, uint64_t parent, const uint8_t* data, uint32_t size, uint8_t* output,
                          uint32_t capacity, uint32_t& required);

    /** 校验完整RPC帧并借用业务部分；只管线格式，权限由真实来源路由表验证
    @param [in] frame 完整REQUEST/RESPONSE，产品和请求号必须非零
    @param [out] view 成功填充，否则清空
    @return OK或PROTOCOL_ERROR
    */
    static int32_t parse(const NetInterfaceFrame& frame, NetInterfaceRpcView& view);

    /** 校验无体控制帧并生成PING回复；宿主仍核对PONG是否匹配已发探测
    @param [in] frame PING/PONG要求product和type为0；CANCEL要求product非零；请求号均非零
    @param [out] reply 仅PING输出PONG，其余清零，允许与frame为同一对象
    @return READY有回复，OK无需回复，PROTOCOL_ERROR拒绝
    */
    static int32_t control(const NetInterfaceFrame& frame, NetInterfaceFrame& reply);
};
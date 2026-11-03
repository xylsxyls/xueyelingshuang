#pragma once
#include "NetInterfaceTypes.h"

// 来源由网关从根连接附加。设备和平台是声明，不代替账号会话验证。
struct NetInterfaceOrigin
{
    // 网关依据真实根连接赋予的来源代次，客户端不能自行指定
    uint64_t m_endpoint;
    // 握手中的平台声明，不能单独作为可信授权依据
    uint32_t m_platform;
    // 客户端声明的稳定设备号，仍须由Account验证账号会话归属
    uint8_t m_device[16];
};

// 仅已认证SERVICE业务TLS内使用，携带NOI2来源且业务体为空；不交给业务回调。
#define NET_INTERFACE_SERVICE_ROOT_CLOSED 0x4e530001U

// 仅用于经过TLS认证的本地服务通道；不开放给公网客户端注册。
class NetInterfaceAPI NetInterfaceService
{
  public:
    /** 编码服务注册凭据；产品号放在HELLO头中，0为Account
    @param [in] secret 32字节独立服务秘密，不可全零
    @param [in] limit 1至128个在途请求
    @param [out] output 至少48字节
    @param [in] capacity 输出容量
    @return OK或负数，失败不写输出
    */
    static int32_t hello(const uint8_t* secret, uint32_t limit, uint8_t* output, uint32_t capacity);

    /** 验证完整本地HELLO及其产品对应秘密，固定32字节比较
    @param [in] frame HELLO/type3/request0/payload48
    @param [in] secret 网关配置的该产品32字节秘密
    @param [out] limit 验证成功后的请求限额；失败清零
    @return OK或PROTOCOL_ERROR，不区分错误凭据细节
    */
    static int32_t authenticate(const NetInterfaceFrame& frame, const uint8_t* secret, uint32_t& limit);

    /** 添加网关签派的32字节上下文，不改原业务体内容
    @param [in] origin 来源；全零仅限没有根上下文的控制或失败响应
    @param [in] data 原体，仅size0可空
    @param [in] size 原体不超过1MiB
    @param [out] output 至少size+32且不与data重叠
    @param [in] capacity 容量
    @param [out] required 所需字节数
    @return OK或负数
    */
    static int32_t wrap(const NetInterfaceOrigin& origin, const uint8_t* data, uint32_t size, uint8_t* output, uint32_t capacity,
                        uint32_t& required);

    /** 拆出可信网关本地通道上的来源；公网消息不得调用本入口采信来源
    @param [in] frame 本地请求、响应或取消，含32字节前缀
    @param [out] origin 来源；失败清零
    @param [out] business 保留原头且移除前缀的借用视图；失败清零
    @return OK或负数
    */
    static int32_t unwrap(const NetInterfaceFrame& frame, NetInterfaceOrigin& origin, NetInterfaceFrame& business);
};
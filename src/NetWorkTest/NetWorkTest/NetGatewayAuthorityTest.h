#pragma once
#include <stdint.h>

// 仅由NetWorkTest执行实际签发、TLS与管理链路认证；生产进程不保留自测入口。
class NetGatewayAuthorityTest
{
public:
    /** 验证CSR签名、证书身份和真实TLS轮换，失败抛出固定测试诊断
    @return NI_OK表示全部独立断言通过
    */
    static int32_t identity();

    /** 验证节点持有证明、角色限制和本机服务注册，失败抛出固定测试诊断
    @return NI_OK表示全部独立断言通过
    */
    static int32_t management();
};
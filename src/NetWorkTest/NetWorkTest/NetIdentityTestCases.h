#pragma once

// 服务身份、证书与授权边界测试集合，逐用例独立建立预期并检查实际结果
class NetIdentityTestCases
{
public:
    /** 固定根公证书、跨平台P256 CSR和强随机源失败拒绝
    */
    static void portableIdentity();

    /** 真实签发、身份混淆和CSR签名拒绝、轮换叶证书后的TLS往返
    */
    static void authorityAndTls();

    /** 真实网关进程执行管理认证、节点证明和授权服务首次签发
    */
    static void managementAuthentication();
};
#pragma once
#include "NetInterfaceTlsTypes.h"
#include "NetInterfaceTypes.h"
#include <string>
#include <vector>

// 仅公开证书、独立身份密钥和证明操作；不包含CA签名私钥或通用签发入口。
class NetInterfaceAPI NetInterfaceCertificate
{
public:
    /** 严格校验网络身份DNS名称，不允许通配符、DN注入或空标签
    @param [in] name 小写ASCII名称，最长96字节
    @return 是否合法
    */
    static bool validName(const std::string& name);

    /** 同模块/静态兼容入口；跨DLL使用CertificateAPI固定缓冲，不转交STL释放权
    @param [in] name 逻辑身份，服务端签发时仍须独立授权
    @param [in] entropy 宿主密码学随机源
    @param [in] context 随机上下文
    @param [out] privateKey 成功为PEM，失败清空；不得与name/request为同一对象，不得记录日志
    @param [out] request 成功为PEM，失败清空
    @return NI_OK或负数错误
    */
    static int32_t createRequest(const std::string& name, NetInterfaceEntropyFunction entropy, void* context,
        std::string& privateKey, std::string& request);

    /** 验证固定CA、精确DNS身份、有效期和叶证书约束
    @param [in] certificate 不含NUL的PEM，最大16KiB，仅一张叶证书
    @param [in] name 期望身份，不能使用远端证书自报名称
    @return NI_OK或负数错误
    */
    static int32_t verify(const std::string& certificate, const std::string& name);

    /** 验证证书后检查私钥匹配，损坏的持久身份不得静默使用
    @param [in] certificate 叶证书
    @param [in] privateKey P256私钥PEM，最大4KiB
    @param [in] name 期望身份
    @param [in] entropy 强随机源
    @param [in] context 随机上下文
    @return NI_OK或负数错误
    */
    static int32_t verifyPair(const std::string& certificate, const std::string& privateKey, const std::string& name,
        NetInterfaceEntropyFunction entropy, void* context);

    /** 同模块/静态签名入口；挑战由验证端随机生成，跨DLL不转交vector释放权
    @param [in] privateKey 本实例P256私钥
    @param [in] data 包含验证端新随机数和全部角色/地址声明，不能只签角色名
    @param [in] size 1至16384字节
    @param [in] entropy 强随机源
    @param [in] context 随机上下文
    @param [out] signature 成功为ECDSA签名，失败清空
    @return NI_OK或负数错误；签名不替代宿主对挑战期限和唯一消费的检查
    */
    static int32_t signProof(const std::string& privateKey, const uint8_t* data, uint32_t size,
        NetInterfaceEntropyFunction entropy, void* context, std::vector<uint8_t>& signature);

    /** 先验证期望角色的证书，再验证全部挑战声明的签名
    @param [in] certificate 叶证书
    @param [in] name 验证端确定的完整身份
    @param [in] data 本次挑战与声明
    @param [in] size 1至16384字节
    @param [in] signature 借用签名
    @param [in] signatureSize 1至80字节
    @return NI_OK或负数错误
    */
    static int32_t verifyProof(const std::string& certificate, const std::string& name, const uint8_t* data, uint32_t size,
        const uint8_t* signature, uint32_t signatureSize);
};
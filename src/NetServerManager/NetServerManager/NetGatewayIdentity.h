#pragma once
#include <stdint.h>
#include <string>

// 网关本机节点身份；只在服务端使用，CA签名根不保存在文件中。
class NetGatewayIdentity
{
public:
    /** 创建未加载身份
    */
    NetGatewayIdentity();

    /** 擦除本实例的节点私钥副本
    */
    ~NetGatewayIdentity();

    /** 读取并验证现存身份，缺失时自动创建；损坏不覆盖、不降级
    @param [in] directory 当前网关可信数据目录UTF8路径，通常为gateway
    @return NI_OK或负数错误；成功后m_name/m_certificate/m_privateKey可用于管理TLS
    */
    int32_t loadOrCreate(const std::string& directory);

private:
    /** 有界读取固定格式，验证摘要、CA、名称和私钥匹配
    @param [in] path 已存在普通文件UTF8路径
    @return NI_OK或负数错误
    */
    int32_t load(const std::string& path);

    /** 禁止复制持有节点私钥的身份对象
    @param [in] other 不可复制对象
    */
    NetGatewayIdentity(const NetGatewayIdentity& other);

    /** 禁止赋值节点私钥与身份
    @param [in] other 不可赋值对象
    @return 不可调用
    */
    NetGatewayIdentity& operator=(const NetGatewayIdentity& other);

public:
    // 通过证书匹配校验的节点身份名称
    std::string m_name;
    // 本节点公开证书PEM副本
    std::string m_certificate;
    // 本节点私钥PEM副本，仅服务端使用，析构时擦除
    std::string m_privateKey;
};
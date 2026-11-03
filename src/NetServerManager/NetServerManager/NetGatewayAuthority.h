#pragma once
#include "NetInterface/NetInterfaceAPI.h"
#include <string>

// 仅编入服务端网关；固定CA私钥绝不能进入NetInterface、NetWork、客户端或Wasm。
class NetGatewayAuthority
{
public:
    /** 固定身份自测，不接收外部CSR、不输出任何签名身份或秘密
    @return NI_OK通过，失败抛出固定诊断异常供入口报告
    */
    static int32_t selfTest();

    /** 固定管理链路自测，验证节点和授权本机服务；不接受外部输入或输出签名身份
    @return NI_OK或固定诊断异常
    */
    static int32_t linkSelfTest();

    /** 为已经经过宿主授权的本机身份签发；不是公共网络API
    @param [in] name 宿主决定的node.<id>或service.<产品号>名称，不采用CSR自报角色
    @param [in] request P256/SHA256签名CSR，最大4KiB
    @param [in] entropy 强随机源
    @param [in] context 随机上下文
    @param [out] certificate 成功为一年有效期叶证书，失败清空
    @return NI_OK或负数错误，拒绝名称不符和无有效持有证明的CSR
    */
    static int32_t issueLocalIdentity(const std::string& name, const std::string& request,
        NetInterfaceEntropyFunction entropy, void* context, std::string& certificate);
};
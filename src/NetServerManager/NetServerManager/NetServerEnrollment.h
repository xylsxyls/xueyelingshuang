#pragma once
#include "NetInterface/NetInterfaceAPI.h"

// 仅由服务器应用显式编译的注册凭证模块。不是公共SDK，不得加入客户端或Wasm。
// 此凭证只授权本机服务注册，不具有CA签名能力；CA私钥仍只属于NetServerManager。
class NetServerEnrollment
{
public:
    /** 为新挑战及完整服务声明计算注册证明
    @param [in] context 忽略
    @param [in] statement 管理协议规范声明，包含双随机数/角色/名称/CSR
    @param [in] size 1至16KiB
    @param [out] proof 至少32字节
    @param [in] capacity 输出容量
    @return NI_OK或错误
    */
    static int32_t proof(void* context, const uint8_t* statement, uint32_t size, uint8_t* proof, uint32_t capacity);

    /** 固定长度比较服务声明证明；签发宿主还须限制本机角色及CSR
    @param [in] statement 规范声明
    @param [in] size 声明长度
    @param [in] proof 收到的证明
    @param [in] proofSize 必须32
    @return NI_OK或统一拒绝错误
    */
    static int32_t verify(const uint8_t* statement, uint32_t size, const uint8_t* proof, uint32_t proofSize);
};
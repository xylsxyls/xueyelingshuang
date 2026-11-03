#pragma once
#include "NetInterfaceTlsTypes.h"

// 单次操作借用的强随机源；不拥有宿主上下文。
struct NetInterfaceCertificateEntropy
{
    // 借用的密码学随机回调，返回0表示完整填充
    NetInterfaceEntropyFunction m_function;
    // 随机回调上下文，仅借用到单次证书或签名操作结束
    void* m_context;
};
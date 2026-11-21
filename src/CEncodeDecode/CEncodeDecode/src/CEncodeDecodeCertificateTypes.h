#pragma once
#include "CEncodeDecodeTypes.h"

// 一次P-256叶证书签发参数，所有范围借用至调用返回；初始化为{}后逐项填写。
// 仅做证书算法校验，不决定业务上谁有权取得哪个身份。
struct CEncodeDecodeCertificateIssue
{
    // 1至253字节DNS名称加末尾NUL；CSR必须仅含相同CN。
    CEncodeDecodeBytes m_name;
    // 最多4096字节PEM CSR，包含末尾NUL，必须有有效SHA256/ECDSA签名。
    CEncodeDecodeBytes m_request;
    // 最多16KiB的签发者证书PEM，包含末尾NUL，必须是CA。
    CEncodeDecodeBytes m_authority;
    // 最多16KiB的签发者私钥PEM，包含末尾NUL，必须与CA配对。
    CEncodeDecodeBytes m_privateKey;
    // UTC有效起点，恰好14位YYYYMMDDhhmmss加末尾NUL。
    CEncodeDecodeBytes m_notBefore;
    // UTC有效终点，与起点同格式，必须晚于起点。
    CEncodeDecodeBytes m_notAfter;
    // 除CN外的DNS SAN列表，最多8个，每项最多253字节加末尾NUL。
    const CEncodeDecodeBytes* m_dnsNames;
    // 附加DNS名称数量，0时列表允许空。
    uint32_t m_dnsNameCount;
    // 是否加入TLS服务端EKU；用途由调用者的授权策略决定。
    bool m_tlsServer;
    // 是否加入TLS客户端EKU。
    bool m_tlsClient;
    // 必须提供的强随机源，用于签名和随机序列号。
    CEncodeDecodeEntropyFunction m_entropy;
    // 随机源上下文，仅本次调用借用。
    void* m_entropyContext;
};
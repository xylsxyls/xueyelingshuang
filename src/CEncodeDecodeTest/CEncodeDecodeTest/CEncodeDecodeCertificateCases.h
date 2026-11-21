#pragma once

// 证书算法独立测试，临时CA仅在本次测试内生成，不借用生产签发私钥。
class CEncodeDecodeCertificateCases
{
public:
    /** 验证P-256请求、签名、签发、信任、容量、别名、日期和随机失败
    */
    static void run();
};
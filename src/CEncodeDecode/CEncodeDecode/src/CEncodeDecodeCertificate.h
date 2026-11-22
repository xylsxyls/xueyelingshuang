#pragma once
#include "CEncodeDecodeTypes.h"
#include "CEncodeDecodeCertificateTypes.h"
#include <string>

// 单次证书运算借用的随机回调，不跨线程保存。
struct CEncodeDecodeCertificateEntropy
{
    // 必须完整填充才返回0，异常由CEncodeDecode捕获。
    CEncodeDecodeEntropyFunction m_function;
    // 本次运算期间有效的宿主上下文。
    void* m_context;
};

// 内部P-256证书算法实现；业务只从CEncodeDecode调用，不拥有网关身份、信任根或私钥。
class CEncodeDecodeCertificate
{
public:
    /** 校验证书和 TLS 共用的 ASCII DNS 标签语法
    @param [in] name 调用期间借用的域名字节，不能带末尾根点
    @param [in] size 名称字节数，不含结尾 NUL
    @return 总长 1 至 253 且每个标签合法时返回 true
    */
    static bool validDnsName(const uint8_t* name, size_t size);

    /** 验证CSR并完成叶证书签发，算法参数由主类调用方提供
    @param [in] options 借用的签发参数
    @param [out] certificate PEM输出
    @param [in] capacity 输出容量
    @param [out] required 含NUL所需长度
    @return CEncodeDecodeResult
    */
    static int32_t issue(const CEncodeDecodeCertificateIssue& options, uint8_t* certificate,
        uint32_t capacity, uint32_t* required);

    /** 生成独占P-256私钥及CN请求，输出提交规则见CEncodeDecode::p256CreateRequest
    @param [in] name DNS名称字节
    @param [in] nameSize 名称长度
    @param [in] entropy 强随机源
    @param [in] context 随机上下文
    @param [out] privateKey 私钥PEM
    @param [in] keyCapacity 私钥容量
    @param [out] keySize 包括NUL的私钥长度
    @param [out] request CSR PEM
    @param [in] requestCapacity CSR容量
    @param [out] requestSize 包括NUL的CSR长度
    @return CEncodeDecodeResult
    */
    static int32_t createRequest(const uint8_t* name, uint32_t nameSize, CEncodeDecodeEntropyFunction entropy,
        void* context, uint8_t* privateKey, uint32_t keyCapacity, uint32_t* keySize,
        uint8_t* request, uint32_t requestCapacity, uint32_t* requestSize);

    /** 签名已经由调用者域分隔的摘要
    @param [in] privateKey 私钥PEM
    @param [in] digest 32字节SHA256
    @param [in] entropy 强随机源
    @param [in] context 随机上下文
    @param [out] signature DER签名缓冲
    @param [in] capacity 输出容量
    @param [out] required 所需长度
    @return CEncodeDecodeResult
    */
    static int32_t signDigest(const std::string& privateKey, const uint8_t* digest,
        CEncodeDecodeEntropyFunction entropy, void* context, uint8_t* signature, uint32_t capacity, uint32_t* required);

    /** 验证单张P-256叶证书，可在同次解析中继续验证摘要签名
    @param [in] certificate 证书PEM
    @param [in] authority 信任根PEM，包含NUL
    @param [in] authoritySize 根字节数
    @param [in] name 预期DNS名称
    @param [in] digest 32字节摘要；不验签时为空
    @param [in] signature DER签名；不验签时为空
    @param [in] signatureSize 签名字节数；不验签时为0
    @return CEncodeDecodeResult
    */
    static int32_t verify(const std::string& certificate, const uint8_t* authority, uint32_t authoritySize,
        const std::string& name, const uint8_t* digest, const uint8_t* signature, uint32_t signatureSize);

    /** 仅验证证书和私钥配对，信任验证由调用者另行完成
    @param [in] certificate 证书PEM
    @param [in] privateKey 私钥PEM
    @param [in] entropy 强随机源
    @param [in] context 随机上下文
    @return CEncodeDecodeResult
    */
    static int32_t verifyPair(const std::string& certificate, const std::string& privateKey,
        CEncodeDecodeEntropyFunction entropy, void* context);
};
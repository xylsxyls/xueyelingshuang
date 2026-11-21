#pragma once
#include "CEncodeDecodeTypes.h"
#include "CEncodeDecodeMbedConfig.h"
#include <mbedtls/pk.h>
#include <mbedtls/ctr_drbg.h>
#include <string>

// 每次操作独占密钥和DRBG；只支持RSA-2048/OAEP-SHA256，不解析旧逐字节格式
class RsaAlgorithm
{
public:
    /** 初始化安全空上下文，不生成密钥
    @param [in] entropy 宿主强随机回调，原生端可省略以使用操作系统的安全随机源，Wasm 必须提供
    @param [in] context 借用到本实例析构的回调上下文
    */
    RsaAlgorithm(CEncodeDecodeEntropyFunction entropy, void* context);

    /** 释放并擦除密钥和DRBG状态
    */
    ~RsaAlgorithm();

    /** 生成2048位RSA密钥，指数65537
    @param [out] publicKey PEM公钥，失败清空
    @param [out] privateKey PEM私钥，失败清空；不能与publicKey相同
    @return 完整生成返回true
    */
    bool generate(std::string& publicKey, std::string& privateKey);

    /** 加密最多190字节文本
    @param [in] publicKey PEM RSA-2048公钥，最多16KiB
    @param [in] text 零结尾文本，不可空；首个零结束
    @return 256字节二进制密文；失败返回空串
    */
    std::string encrypt(const std::string& publicKey, const char* text);

    /** 解密OAEP-SHA256并校验完整填充
    @param [in] privateKey 未加密PEM RSA-2048私钥，最多16KiB
    @param [in] ciphertext 必须为256字节二进制
    @return 明文；失败或合法空明文返回空串
    */
    std::string decrypt(const std::string& privateKey, const std::string& ciphertext);

private:
    /** 为本次操作播种，拒绝缺失、失败或抛异常的随机源
    @return 成功返回true
    */
    bool seed();

    /** 解析并检查精确RSA-2048类型及密钥一致性
    @param [in] pem 有效PEM文本，不允许内嵌零
    @param [in] privateKey true读取私钥，false读取公钥
    @return 校验成功返回true
    */
    bool parse(const std::string& pem, bool privateKey);

    /** 将宿主强随机源适配到第三方，异常和部分失败不越过C边界
    @param [in] context 有效RsaAlgorithm实例
    @param [out] output 必须完整填充的内存
    @param [in] size 不超过8192的字节数
    @return 0成功，非0失败并擦除输出
    */
    static int entropyBytes(void* context, unsigned char* output, size_t size);

    /** 禁止复制独占密钥
    @param [in] other 源实例
    */
    RsaAlgorithm(const RsaAlgorithm& other);

    /** 禁止复制独占密钥
    @param [in] other 源实例
    @return 不可调用
    */
    RsaAlgorithm& operator=(const RsaAlgorithm& other);

private:
    // 本实例独占的解析或生成密钥
    mbedtls_pk_context m_key;
    // 本实例独占的密码学随机流
    mbedtls_ctr_drbg_context m_random;
    // 本次操作的强随机回调，原生端未指定时在构造函数中选用系统随机源
    CEncodeDecodeEntropyFunction m_entropy;
    // 调用期间借用的上下文
    void* m_entropyContext;
};
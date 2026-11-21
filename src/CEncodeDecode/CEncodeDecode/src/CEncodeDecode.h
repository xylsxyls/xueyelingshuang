#pragma once
#include "CEncodeDecodeMacro.h"
#include "CEncodeDecodeTypes.h"
#include "CEncodeDecodeTlsTypes.h"
#include "CEncodeDecodeCertificateTypes.h"
#include <string>

class CEncodeDecodeTls;
class CEncodeDecodeHash;
typedef struct x509_st X509;

// 通用密码运算入口；无全局可变密钥，独立调用可并发，TLS实例由调用者串行驱动
// MD5、历史AES/SM4仅保留旧数据兼容；网关通信使用mbedTLS认证加密通道
class CEncodeDecodeAPI CEncodeDecode
{
public:
    /** 验证CSR持钥证明并签发P-256叶证书，不负责业务身份授权
    @param [in] options 输入PEM、信任根私钥、名称、时间和用途；详见各字段契约
    @param [out] certificate PEM缓冲，包含结束NUL；不得与任何输入存储重叠，失败保持正文
    @param [in] capacity 1至8192字节
    @param [out] required 成功或容量不足返回含NUL的长度，前置别名／DNS数组检查失败不写入；不得与输入输出重叠
    @return CEncodeDecodeResult；别名或DNS数组元数据错误保持required，其他失败置0；独立调用可并发，不跨DLL分配输出对象
    */
    static int32_t p256IssueCertificate(const CEncodeDecodeCertificateIssue& options, uint8_t* certificate,
        uint32_t capacity, uint32_t* required);

    /** 生成P-256私钥和SHA256签名CSR；不同调用可并发，不保留输入
    @param [in] name 1至253字节DNS名称，仅字母数字、点、连字符，CN不接受转义或附加属性
    @param [in] nameSize 名称字节数，不包含NUL
    @param [in] entropy 必须提供强随机回调，不回退至伪随机数
    @param [in] context 本次调用借用
    @param [out] privateKey 私钥PEM含NUL，调用者负责保密和擦除
    @param [in] keyCapacity 1至4096；不足时两个正文输出均保持原值
    @param [out] keySize 成功或容量不足时返回含NUL的长度，其余失败为0
    @param [out] request CSR PEM含NUL
    @param [in] requestCapacity 1至4096
    @param [out] requestSize 同keySize；所有输入和输出范围不得重叠，别名错误不修改长度
    @return CEncodeDecodeResult；容量不足重试会重新生成密钥，不缓存上次私钥
    */
    static int32_t p256CreateRequest(const uint8_t* name, uint32_t nameSize, CEncodeDecodeEntropyFunction entropy,
        void* context, uint8_t* privateKey, uint32_t keyCapacity, uint32_t* keySize,
        uint8_t* request, uint32_t requestCapacity, uint32_t* requestSize);

    /** 使用P-256私钥签名SHA256摘要，签名使用DER编码；独立调用可并发
    @param [in] privateKey 不含NUL的PEM文本，最多4096字节
    @param [in] digest 恰好32字节，调用方负责业务域分隔，不得指向signature存储
    @param [in] entropy 必须提供强随机回调
    @param [in] context 本次调用借用
    @param [out] signature DER签名缓冲，失败不改变正文，不得与输入或required重叠
    @param [in] capacity 1至80字节，推荐80
    @param [out] required 成功或容量不足返回长度，其他失败为0；不得与输入输出重叠
    @return CEncodeDecodeResult；别名错误保持required，不跨DLL分配输出对象
    */
    static int32_t p256SignDigest(const std::string& privateKey, const uint8_t* digest,
        CEncodeDecodeEntropyFunction entropy, void* context, uint8_t* signature, uint32_t capacity, uint32_t* required);

    /** 验证单张非CA P-256叶证书的信任、时间、DNS名称和数字签名用途，可同时验签
    @param [in] certificate 不含NUL的PEM文本，最多16KiB，不接受叶证书链
    @param [in] authority 调用方指定的信任根PEM，包含结束NUL；本库无内置信任根
    @param [in] authoritySize 1至16KiB
    @param [in] name 1至253字节的预期DNS名称
    @param [in] digest 可选32字节SHA256摘要，不验签时为空
    @param [in] signature 可选DER签名，必须与digest一起提供
    @param [in] signatureSize 验签时1至80，不验签时为0
    @return CEncodeDecodeResult；独立调用可并发，不保留参数
    */
    static int32_t p256VerifyCertificate(const std::string& certificate, const uint8_t* authority, uint32_t authoritySize,
        const std::string& name, const uint8_t* digest = nullptr, const uint8_t* signature = nullptr, uint32_t signatureSize = 0);

    /** 验证P-256证书与私钥匹配；不替代p256VerifyCertificate的信任和时间检查
    @param [in] certificate 不含NUL的PEM文本，最多16KiB
    @param [in] privateKey 不含NUL的PEM文本，最多4096字节
    @param [in] entropy 必须提供强随机回调
    @param [in] context 本次调用借用
    @return CEncodeDecodeResult；独立调用可并发
    */
    static int32_t p256VerifyPair(const std::string& certificate, const std::string& privateKey,
        CEncodeDecodeEntropyFunction entropy, void* context);

    /** 创建独占SHA256状态，不使用全局锁；同一状态的所有操作须串行
    @param [out] hash 成功交给调用者，失败为空；最终用hashDestroy释放
    @return CENCODE_OK、ARGUMENT、MEMORY或STATE，不抛异常
    */
    static int32_t hashCreate(CEncodeDecodeHash** hash);

    /** 追加SHA256输入，不保留正文；不同hash可以并发
    @param [in,out] hash 本库创建且未完成的独占状态
    @param [in] input 本次借用，size为0时可空
    @param [in] size 单次最多16MiB，累计最多2^61-1字节
    @return CENCODE_OK或错误码；非法参数不改变状态，算法错误持续保留
    */
    static int32_t hashUpdate(CEncodeDecodeHash* hash, const uint8_t* input, uint32_t size);

    /** 完成SHA256，仅能成功完成一次；不足容量时可重试
    @param [in,out] hash 本库创建的状态
    @param [out] output 至少32字节，失败保持原内容
    @param [in] capacity 输出容量，只有前32字节被写入
    @return CENCODE_OK或错误码；成功后仍需hashDestroy
    */
    static int32_t hashFinish(CEncodeDecodeHash* hash, uint8_t* output, uint32_t capacity);

    /** 擦除并释放SHA256状态，所有使用者必须已停止
    @param [in] hash 本库创建的对象，可空；返回后不得继续使用
    */
    static void hashDestroy(CEncodeDecodeHash* hash);

    /** 一次计算SHA256，不分配C++容器，不保留输入
    @param [in] input 本次借用；空正文可用nullptr
    @param [in] size 最多16MiB
    @param [out] output 32字节摘要，允许与输入重叠；失败不改变内容
    @param [in] capacity 至少32，尾部保持原值
    @return CENCODE_OK或错误码；独立调用可并发
    */
    static int32_t sha256(const uint8_t* input, uint32_t size, uint8_t* output, uint32_t capacity);

    /** 按片段顺序计算SHA256，避免域标记和正文拼接；独立调用可并发
    @param [in] parts 借用片段数组，count为0时可空，片段可为空
    @param [in] count 0至16，全部片段合计最多16MiB
    @param [out] output 32字节摘要，允许与输入重叠；失败保持不变
    @param [in] capacity 至少32，尾部不改变
    @return CENCODE_OK或错误码，不创建C++容器或长期借用
    */
    static int32_t sha256Parts(const CEncodeDecodeBytes* parts, uint32_t count, uint8_t* output, uint32_t capacity);

    /** 计算完整HMAC-SHA256，调用方负责密钥来源和验证策略
    @param [in] key 本次借用的密钥，空密钥可用nullptr
    @param [in] keySize 最多16KiB；业务不得将空密钥视为安全默认值
    @param [in] input 本次借用的消息，size为0可空
    @param [in] size 最多16MiB
    @param [out] output 32字节认证值，允许与输入/密钥重叠；失败不变
    @param [in] capacity 至少32
    @return CENCODE_OK或错误码；独立调用可并发，不抛异常
    */
    static int32_t hmacSha256(const uint8_t* key, uint32_t keySize, const uint8_t* input, uint32_t size,
        uint8_t* output, uint32_t capacity);

    /** 按片段顺序计算HMAC-SHA256，不合并复制正文；独立调用可并发
    @param [in] key 借用密钥，keySize为0时可空；调用方负责密钥安全策略
    @param [in] keySize 最多16KiB
    @param [in] parts 借用片段数组，count为0时可空，片段可为空
    @param [in] count 0至16，全部片段合计最多16MiB
    @param [out] output 32字节认证值，允许与密钥和输入重叠；失败保持不变
    @param [in] capacity 至少32，尾部不改变
    @return CENCODE_OK或错误码，第三方临时状态在本次调用内释放
    */
    static int32_t hmacSha256Parts(const uint8_t* key, uint32_t keySize, const CEncodeDecodeBytes* parts,
        uint32_t count, uint8_t* output, uint32_t capacity);

    /** 通过PBKDF2-HMAC-SHA256派生32字节；同步计算，不在网络owner热路径执行
    @param [in] password 本次借用的密码字节，允许内部NUL
    @param [in] passwordSize 1至1024
    @param [in] salt 本次借用的盐，调用方保证按业务生成
    @param [in] saltSize 1至128
    @param [in] iterations 1至2000000；上限用于控制CPU工作量，不表示所有值均满足密码策略
    @param [out] output 32字节结果，允许与输入重叠；失败不变
    @param [in] capacity 至少32
    @return CENCODE_OK或错误码；独立调用可并发
    */
    static int32_t pbkdf2Sha256(const uint8_t* password, uint32_t passwordSize, const uint8_t* salt, uint32_t saltSize,
        uint32_t iterations, uint8_t* output, uint32_t capacity);

    /** 擦除调用方可写内存，使用第三方保证不被优化删除的实现
    @param [in,out] bytes 独占可写缓冲，可空；调用方保证size有效
    @param [in] size 擦除字节数，为0时无操作
    */
    static void wipe(uint8_t* bytes, uint32_t size);

    /** 创建无socket、无线程的独占TLS字节通道
    @param [in] options 证书创建时复制；随机回调上下文借用至destroyTls
    @return 由本库释放的实例；无内存返回空，配置错误由实例status查询
    */
    static CEncodeDecodeTls* createTls(const CEncodeDecodeTlsOptions& options);

    /** 用指定的双向密文队列上限创建 TLS 字节通道
    @param [in] options 证书创建时复制，随机回调上下文借用至销毁
    @param [in] limits 此实例独立使用的密文限额，创建后可释放参数对象
    @return 由本库释放的实例，配置错误由 status 查询
    */
    static CEncodeDecodeTls* createTls(const CEncodeDecodeTlsOptions& options, const CEncodeDecodeTlsQueueLimits& limits);

    /** 释放TLS实例并擦除密钥；同实例所有调用必须已经结束
    @param [in] tls 本库创建的实例，可空，返回后失效
    */
    static void destroyTls(CEncodeDecodeTls* tls);

    /** 安全调用宿主强随机源，不回退至时间或普通伪随机数
    @param [in] entropy 必须有效的强随机回调，size为0时允许空
    @param [in] context 本次调用借用的上下文
    @param [out] output size字节；回调失败擦除，size为0时可空
    @param [in] size 最多8192字节；非法长度时不访问output
    @return 完整生成返回true，参数错误、异常或源失败返回false
    */
    static bool entropyBytes(CEncodeDecodeEntropyFunction entropy, void* context, uint8_t* output, uint32_t size);

    /** 计算旧数据校验用MD5，不用于密码存储、签名或防篡改认证
    @param [in] src 任意字节，可含NUL；本次调用借用
    @return 32字节小写十六进制，分配失败返回空
    */
    static std::string MD5Encode(const std::string& src);

    /** 按历史AES-128-ECB、零填充和大写HEX格式编码，保持Package兼容
    @param [in] key 取前16字节，不足补零；不是口令派生函数
    @param [in] src 不含NUL的文本，长度不超过INT_MAX-16字节
    @return 密文HEX；失败返回空，不提供完整性认证或随机化
    */
    static std::string AESEncode(const std::string& key, const std::string& src);

    /** 解码历史AES文本，明文在第一个NUL处结束
    @param [in] key 与编码完全相同的密钥
    @param [in] src HEX长度须为32的非零整数倍，大小写均接受
    @return 明文；错误、分配失败或合法空明文均返回空，不能用于认证
    */
    static std::string AESDecode(const std::string& key, const std::string& src);

    /** 生成RSA-2048密钥，公用指数65537；旧自定义短密钥格式已移除
    @param [out] publicKey PEM公钥，失败清空；不得与privateKey为同一对象
    @param [out] privateKey 未加密的PEM私钥，由调用者保密存储并及时擦除
    @param [in] entropy 原生端可省略以使用操作系统的安全随机源，Wasm 必须提供宿主强随机回调
    @param [in] context 回调上下文，仅本次调用借用
    @return 成功为true；参数、随机源或分配失败为false
    */
    static bool RSAKey(std::string& publicKey, std::string& privateKey,
        CEncodeDecodeEntropyFunction entropy = nullptr, void* context = nullptr);

    /** 用RSA-2048 OAEP-SHA256加密短文本，MGF1使用SHA256，标签为空
    @param [in] publicKey PEM公钥，必须为2048位RSA，不超过16KiB且不含NUL
    @param [in] src NUL结尾的文本，最多190字节，可为空字符串，不可为空指针
    @param [in] entropy 原生端可省略以使用操作系统的安全随机源，Wasm 必须提供宿主强随机回调
    @param [in] context 本次调用借用的随机上下文
    @return 256字节二进制密文，可含NUL；失败返回空，不再生成旧文本格式
    */
    static std::string RSAEncode(const std::string& publicKey, const char* src,
        CEncodeDecodeEntropyFunction entropy = nullptr, void* context = nullptr);

    /** 用RSA-2048 OAEP-SHA256解密；随机源用于RSA盲化
    @param [in] privateKey 未加密PEM私钥，最多16KiB，不接受旧短密钥格式
    @param [in] src 必须为RSAEncode返回的256字节二进制密文
    @param [in] entropy 原生端可省略以使用操作系统的安全随机源，Wasm 必须提供宿主强随机回调
    @param [in] context 本次调用借用的随机上下文
    @return 明文；错误或合法空明文均返回空，不可将空值当成认证成功
    */
    static std::string RSADecode(const std::string& privateKey, const std::string& src,
        CEncodeDecodeEntropyFunction entropy = nullptr, void* context = nullptr);

    /** 创建SM2密钥；需要在本库构建中启用USE_OPENSSL并准备OpenSSL SDK
    @param [out] publicKeyX 32字节X坐标，可按hasPrefix增加0x04前缀
    @param [out] publicKeyY 32字节Y坐标
    @param [out] privateKey 32字节私钥；三个输出须为不同对象
    @param [in] hasPrefix 为true时X长度33，否则32
    @return 成功为true；未启用功能或失败为false并清空输出
    */
    static bool SM2Key(std::string& publicKeyX, std::string& publicKeyY, std::string& privateKey, bool hasPrefix = true);

    /** SM2加密并返回C1、C2、C3三个二进制分段；调用者自行按协议排列
    @param [in] publicKeyX 32字节X，或以0x04开头的33字节X
    @param [in] publicKeyY 32字节Y
    @param [in] src 非空任意字节，最多INT32_MAX，可含NUL
    @param [out] c1 X和Y共64字节，输入X带前缀时为65字节
    @param [out] c2 与明文等长的密文
    @param [out] c3 32字节SM3认证值；三个输出互不别名，也不能引用任一输入
    @return 成功为true；普通失败清空输出，别名错误保持原对象不变
    */
    static bool SM2Encode(const std::string& publicKeyX, const std::string& publicKeyY, const std::string& src,
        std::string& c1, std::string& c2, std::string& c3);

    /** 验证SM2密文完整性后解密；未启用USE_OPENSSL时返回空
    @param [in] privateKey 32字节私钥
    @param [in] c1 64字节坐标，或以0x04开头的65字节点
    @param [in] c2 非空密文，最多INT32_MAX字节
    @param [in] c3 32字节SM3认证值
    @return 验证成功的二进制明文；任何失败返回空，不返回部分明文
    */
    static std::string SM2Decode(const std::string& privateKey, const std::string& c1,
        const std::string& c2, const std::string& c3);

    /** 历史SM4-ECB无填充加密；不提供完整性认证
    @param [in] key 恰好16字节，可含NUL
    @param [in] src 非空且长度为16的整数倍，可含NUL
    @return 等长二进制密文，错误或分配失败返回空
    */
    static std::string SM4Encode(const std::string& key, const std::string& src);

    /** 历史SM4-ECB无填充解密
    @param [in] key 恰好16字节，可含NUL
    @param [in] ciphertext 非空且长度为16的整数倍，可含NUL
    @return 等长明文；错误或分配失败返回空，不校验密钥是否正确
    */
    static std::string SM4Decode(const std::string& key, const std::string& ciphertext);

    /** 从DER或PEM文件读取证书，不代表验证该证书的信任关系
    @param [in] path 非空本地路径，不得含NUL；需要USE_OPENSSL
    @return 独占证书，调用releaseCert释放；读取或解析失败返回空
    */
    static X509* getCert(const std::string& path);

    /** 释放本库读取的证书，不得与同证书的其他操作并发
    @param [in] cert getCert返回的证书，可空；返回后失效
    */
    static void releaseCert(X509* cert);

    /** 提取椭圆曲线证书公钥的编码字节，保留原曲线点格式
    @param [in] cert 本次调用借用的证书，可空；需要USE_OPENSSL
    @return EC公钥字节；非EC证书、空值或错误返回空，不转移证书所有权
    */
    static std::string getPublicKey(X509* cert);
};
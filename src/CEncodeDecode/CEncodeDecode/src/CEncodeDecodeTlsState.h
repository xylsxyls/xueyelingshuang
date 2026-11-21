#pragma once
#include "CEncodeDecodeTlsTypes.h"
#include "CEncodeDecodeMbedConfig.h"
#include <mbedtls/ssl.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/pk.h>

// 密文队列；默认使用内置环形存储。较大的显式限额按实际输入分块增长。
class CEncodeDecodeTlsQueue
{
  public:
    /** 创建空队列
    @param [in] limit 待处理字节上限，-1 由宿主取消此上限
    */
    explicit CEncodeDecodeTlsQueue(int64_t limit);

    /** 释放队列按需分配的全部密文块
    */
    ~CEncodeDecodeTlsQueue();

    /** 复制可容纳的输入前缀
    @param [in] data 输入内存，size为0时可空
    @param [in] size 字节数
    @param [out] accepted 已复制字节数，达到限额时可能只接纳前缀
    @return 存储分配失败时返回 false
    */
    bool append(const uint8_t* data, uint32_t size, uint32_t& accepted);

    /** 复制并消费队首字节
    @param [out] output 有效输出内存
    @param [in] size 容量
    @return 已读取字节数
    */
    uint32_t take(uint8_t* output, uint32_t size);

    /** 借用连续队首
    @param [out] data 无数据为空，消费之前有效
    @return 连续字节数
    */
    uint32_t peek(const uint8_t*& data) const;

    /** 消费连续队首
    @param [in] size 不超过peek的字节数
    @return 参数合法返回true
    */
    bool consume(uint32_t size);

  private:
    // 每个附加块地址固定，保证已借出的输出前缀在消费前不会因后续追加失效。
    struct Block
    {
        uint8_t bytes[32768];   // 此块独占的密文
        uint32_t begin;         // 首个未消费字节
        uint32_t end;           // 已写入区间的末尾
        Block* next;            // 下一块，由队列释放
    };
    CEncodeDecodeTlsQueue(const CEncodeDecodeTlsQueue& other) = delete;
    CEncodeDecodeTlsQueue& operator=(const CEncodeDecodeTlsQueue& other) = delete;
    // 默认配置在内置环形缓冲内完成所有操作。
    uint8_t m_data[32768];
    // 队首偏移
    uint32_t m_head;
    uint32_t m_inlineSize; // 内置缓冲的已用字节
    int64_t m_limit;       // 允许的待处理字节数
    uint64_t m_size;       // 全部块待处理字节总数
    Block* m_first;        // 额外存储的首块
    Block* m_last;         // 额外存储的尾块
};

// TLS内部实现和私密数据，外部只能通过CEncodeDecodeTls操作
class CEncodeDecodeTlsState
{
  public:
    /** 初始化全部第三方上下文，不进行网络 I/O
    @param [in] limits 双向密文队列在本实例上应用的限额
    */
    explicit CEncodeDecodeTlsState(const CEncodeDecodeTlsQueueLimits& limits);

    /** 释放上下文并擦除缓存明文
    */
    ~CEncodeDecodeTlsState();

    /** 验证配置、配置证书与随机源
    @param [in] options 调用时借用，随机上下文借用至析构
    @return 当前状态或持续错误
    */
    int32_t configure(const CEncodeDecodeTlsOptions& options);

    /** 记录持续错误
    @param [in] status 公共负数错误
    @param [in] detail Mbed TLS原始错误
    @return 公共错误
    */
    int32_t fail(int32_t status, int32_t detail);

    /** 将待发明文转换为有界密文
    @return OK、WANT_IO或持续错误
    */
    int32_t flush();

    /** Mbed TLS发送回调，仅复制到队列
    @param [in] context 有效CEncodeDecodeTlsState
    @param [in] data 密文，本次调用借用
    @param [in] size 字节数
    @return 已复制的字节数，未接收任何字节时返回 MBEDTLS_ERR_SSL_WANT_WRITE，分配失败时返回 MBEDTLS_ERR_SSL_ALLOC_FAILED
    */
    static int sendBytes(void* context, const unsigned char* data, size_t size);

    /** Mbed TLS接收回调，仅从队列读取
    @param [in] context 有效CEncodeDecodeTlsState
    @param [out] data 输出缓冲
    @param [in] size 容量
    @return 已读取长度、EOF或WANT_READ
    */
    static int receiveBytes(void* context, unsigned char* data, size_t size);

    /** 将宿主强随机源接入每实例CTR-DRBG；回调异常就地捕获并擦除部分输出，不能越过第三方C调用栈
    @param [in] context 有效CEncodeDecodeTlsState
    @param [out] data 输出缓冲
    @param [in] size 必须完整生成的随机字节数
    @return 0为成功，否则熵源失败
    */
    static int entropyBytes(void* context, unsigned char* data, size_t size);

  public:
    // 每个实例独占SSL连接上下文
    mbedtls_ssl_context m_ssl;
    // 每个实例独占配置，不与其他线程共享可变配置
    mbedtls_ssl_config m_config;
    // 每连接独立随机生成器
    mbedtls_ctr_drbg_context m_random;
    // 客户端信任根或服务端证书链
    mbedtls_x509_crt m_certificate;
    // 仅服务端持有私钥
    mbedtls_pk_context m_key;
    // 宿主强随机回调
    CEncodeDecodeEntropyFunction m_entropy;
    // 借用的随机上下文
    void* m_entropyContext;
    // 待解密密文队列
    CEncodeDecodeTlsQueue m_input;
    // 待发送密文队列
    CEncodeDecodeTlsQueue m_output;
    // 在WANT_WRITE重试期间保持同一份明文
    uint8_t m_plaintext[16384];
    // 待转换明文总量
    uint32_t m_plaintextSize;
    // 已转换明文偏移
    uint32_t m_plaintextOffset;
    // 首次交给mbedTLS后冻结输入，WANT_WRITE重试必须保持原指针、长度和字节
    bool m_plaintextFrozen;
    // 公共当前状态
    int32_t m_status;
    // 最后一个底层错误
    int32_t m_detail;
    // 底层已读至EOF
    bool m_inputFinished;
    // 本端关闭已开始，拒绝新明文
    bool m_closeStarted;
    // close_notify已排队
    bool m_closeSent;
    // 是否已记录握手起始时间
    bool m_clockStarted;
    // 第一次progress单调时间
    uint64_t m_startedMs;
    // 上次progress单调时间
    uint64_t m_lastMs;
    // 配置的握手毫秒时限
    uint32_t m_timeoutMs;
};
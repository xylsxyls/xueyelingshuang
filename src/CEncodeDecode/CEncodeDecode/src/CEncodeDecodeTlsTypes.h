#pragma once
#include "CEncodeDecodeMacro.h"
#include "CEncodeDecodeTypes.h"

// TLS 接口返回码。status() 返回负数表示持续错误，单次操作也可返回可纠正的参数或状态错误
typedef enum CEncodeDecodeTlsStatus
{
    // 本次输入已接受或输出已消费
    CENCODE_TLS_OK = 0,
    // 尚在握手，需输送网络字节并再次progress
    CENCODE_TLS_HANDSHAKE = 1,
    // 已验证服务器身份，可以收发应用数据
    CENCODE_TLS_READY = 2,
    // 对端已发送经过认证的close_notify
    CENCODE_TLS_CLOSED = 3,
    // 等待输入或输出空间，未发生错误
    CENCODE_TLS_WANT_IO = 4,
    // 空参数、越界长度或缺失安全配置
    CENCODE_TLS_ARGUMENT = -101,
    // 内存分配失败
    CENCODE_TLS_MEMORY = -102,
    // 证书、域名、完整性或TLS协议验证失败
    CENCODE_TLS_PROTOCOL = -103,
    // 宿主提供的单调时间已超过握手期限
    CENCODE_TLS_TIMEOUT = -104,
    // 未完成认证关闭的TCP断开
    CENCODE_TLS_TRUNCATED = -105,
    // 当前状态不允许此操作
    CENCODE_TLS_STATE = -106
} CEncodeDecodeTlsStatus;

// 创建时借用的配置；证书和域名在create中复制，随机回调及context借用至destroy
typedef struct CEncodeDecodeTlsOptions
{
    // 0为客户端，1为服务端；服务端TLS不替代业务账号认证
    int32_t server;
    // 客户端必须提供受信根PEM或DER；PEM字节数包含结尾NUL
    const uint8_t* authority;
    // 受信根字节数，最大128KiB
    uint32_t authoritySize;
    // 服务端证书链PEM或DER，客户端可空
    const uint8_t* certificate;
    // 证书链字节数，最大128KiB，PEM包含结尾NUL
    uint32_t certificateSize;
    // 服务端未加密私钥PEM或DER，只在create中借用
    const uint8_t* privateKey;
    // 私钥字节数，最大32KiB，PEM包含结尾NUL
    uint32_t privateKeySize;
    // 客户端预期 DNS 域名，共 1 至 253 字节；标签 1 至 63 字节，可带末尾根点
    const char* hostname;
    // 必填的密码学熵源回调，同步接收 entropyContext 并将 size 个随机字节写入 output，全部填满返回 0，失败返回非零
    CEncodeDecodeEntropyFunction entropy;
    // 随机回调上下文，可空，生命周期覆盖TLS实例
    void* entropyContext;
    // 握手时限，0 采用默认 10000 毫秒，可指定 100 至 60000 毫秒
    uint32_t handshakeTimeoutMs;
#ifdef __cplusplus
    /** 为即将创建的 TLS 实例设置握手时限
    @param [in] timeoutMs 握手时限，单位毫秒，0 使用默认值 10000，也可指定 100 至 60000
    @return 保存成功返回 true，超出允许范围返回 false 并保留原值
    */
    bool setHandshakeTimeout(uint32_t timeoutMs)
    {
        if ((timeoutMs != 0 && timeoutMs < 100) || timeoutMs > 60000) { return false; }
        handshakeTimeoutMs = timeoutMs;
        return true;
    }
#endif
} CEncodeDecodeTlsOptions;

#ifdef __cplusplus
/** 配置创建 TLS 实例时使用的输入和输出密文队列上限
*/
class CEncodeDecodeTlsQueueLimits
{
public:
    /** 将输入和输出密文队列的上限分别初始化为 32768 字节
    */
    CEncodeDecodeTlsQueueLimits() :
    m_input(32768),
    m_output(32768)
    {

    }

    /** 同时设置接收和发送方向可缓存的密文字节数
    @param [in] inputBytes 输入队列待处理密文的字节上限，正数指定上限，-1 允许按需增长
    @param [in] outputBytes 输出队列待交付密文的字节上限，正数指定上限，-1 允许按需增长
    @return 两个值均有效时保存并返回 true，任一值为 0 或小于 -1 时返回 false 并保留原配置
    */
    bool set(int64_t inputBytes, int64_t outputBytes)
    {
        if (inputBytes == 0 || inputBytes < -1 || outputBytes == 0 || outputBytes < -1)
        {
            return false;
        }
        m_input = inputBytes;
        m_output = outputBytes;
        return true;
    }

    /** 查询输入队列可缓存多少尚未处理的密文
    @return 配置的字节上限，-1 表示按需增长
    */
    int64_t inputBytes() const
    {
        return m_input;
    }

    /** 查询输出队列可缓存多少尚未交付的密文
    @return 配置的字节上限，-1 表示按需增长
    */
    int64_t outputBytes() const
    {
        return m_output;
    }

private:
    // 输入队列待处理密文的字节上限
    int64_t m_input;
    // 输出队列待交付密文的字节上限
    int64_t m_output;
};
#endif
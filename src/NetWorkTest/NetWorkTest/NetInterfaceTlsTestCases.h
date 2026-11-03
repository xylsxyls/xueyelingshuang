#pragma once
#include "NetInterface/NetInterfaceAPI.h"
#include <memory>
#include <vector>

// 隔离的双端TLS测试连接，使用公开测试证书和操作系统随机源，不打开网络端口
class NetInterfaceTlsTestPair
{
public:
    /** 创建客户端和服务端，配置错误抛出测试异常
    @param [in] hostname 客户端验证域名
    @param [in] wrongAuthority true使用不签发该服务端证书的根
    */
    NetInterfaceTlsTestPair(const char* hostname, bool wrongAuthority = false);

    /** 在确定性碎片调度中执行实际TLS握手
    @param [in] fragment 每次最多转移的密文字节数，至少1
    @return 双端成功为true，证书或协议错误为false；超出步数抛异常
    */
    bool handshake(uint32_t fragment);

    /** 从一端复制密文到另一端，保留未消费后缀
    @param [in,out] source 输出端
    @param [in,out] destination 输入端
    @param [in] fragment 最大复制字节数
    @return 实际转移字节数
    */
    static uint32_t transfer(NetInterfaceTls& source, NetInterfaceTls& destination,
        uint32_t fragment);

    /** 构建只用于测试的配置
    @param [in] server 是否服务端
    @param [in] hostname 客户端域名，调用和会话创建时借用
    @param [in] wrongAuthority 是否使用错误根
    @return 指向公开测试证书的配置
    */
    static NetInterfaceTlsOptions options(bool server, const char* hostname,
        bool wrongAuthority = false);

    /** 为真实TLS握手提供平台密码学随机数
    @param [in] context 忽略的上下文
    @param [out] output 必须完整填充
    @param [in] size 字节数
    @return 0成功，负数失败
    */
    static int32_t strongRandom(void* context, uint8_t* output, uint32_t size);

    /** 模拟操作系统熵源故障，验证拒绝降级
    @param [in] context 忽略的上下文
    @param [out] output 不写入
    @param [in] size 请求长度
    @return 始终失败
    */
    static int32_t failedRandom(void* context, uint8_t* output, uint32_t size);

    /** 断言并附带具体失败说明
    @param [in] condition 待检查条件
    @param [in] message 失败说明
    */
    static void require(bool condition, const char* message);

public:
    // 测试拥有的客户端
    std::unique_ptr<NetInterfaceTls> m_client;
    // 测试拥有的服务端
    std::unique_ptr<NetInterfaceTls> m_server;
};

// 验证安全边界与有界I/O的稳定编号用例
class NetInterfaceTlsTestCases
{
public:
    /** 实际碎片握手、二进制数据和认证关闭
    */
    static void handshakeAndClose();

    /** 域名不符及错误信任根都必须拒绝
    */
    static void certificateRejection();

    /** AEAD拒绝篡改，不得交付任何未认证明文
    */
    static void tamperRejection();

    /** 成功接收后再次输入同一TLS记录必须拒绝重放
    */
    static void replayRejection();

    /** 截断、无认证关闭、超时和时间倒退保持失败
    */
    static void failures();

    /** C++主类入口创建、空参数、熵源失败及缓冲边界
    */
    static void cApiAndEntropy();

    /** 部分熵输出和宿主异常均拒绝；C/C++创建无异常逃逸且失败状态不可恢复
    */
    static void entropyExceptionBoundary();

    /** 一MiB数据触发环形队列、记录边界及持续背压
    */
    static void boundedTransfer();
};
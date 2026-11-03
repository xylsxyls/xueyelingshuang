#pragma once
#include "NetInterfaceTlsTypes.h"

class NetInterfaceTlsState;

// 非阻塞TLS字节通道，拥有内部状态；不拥有socket，不建线程，同一实例须串行调用
class NetInterfaceAPI NetInterfaceTls
{
  public:
    /** 验证并复制配置；构造结果由status查询
    @param [in] options 证书仅调用时借用，随机回调上下文借用至析构
    */
    explicit NetInterfaceTls(const NetInterfaceTlsOptions& options);

    /** 在本模块释放TLS内存，并擦除密钥及待发明文
    */
    ~NetInterfaceTls();

    /** 输入网络密文，输入缓冲满时只接受前缀
    @param [in] data 仅size为0时可空，本次调用借用
    @param [in] size 输入字节数
    @param [out] consumed 实际复制的字节数，剩余部分由宿主保留
    @return OK、WANT_IO或持续错误
    */
    int32_t feed(const uint8_t* data, uint32_t size, uint32_t& consumed);

    /** 推进握手和待发数据，宿主有输入或输出空间时调用
    @param [in] monotonicMs 宿主单调毫秒时间，不得倒退；无输入也须定时调用以检查超时
    @return HANDSHAKE、READY、CLOSED或持续错误
    */
    int32_t progress(uint64_t monotonicMs);

    /** 借用一段待发密文，发送完成之前不得consume；本方法不消费
    @param [out] data 下次consumeOutput或析构之前有效，无数据时为空
    @param [out] size 连续可用字节数；环形队列可能需多次取出
    @return OK或持续错误；错误时仍可获取待发TLS警报
    */
    int32_t peekOutput(const uint8_t*& data, uint32_t& size) const;

    /** 移除宿主已复制或成功发送的密文前缀
    @param [in] size 不得超过上次peek返回的连续长度
    @return OK或ARGUMENT
    */
    int32_t consumeOutput(uint32_t size);

    /** 排队最多一条TLS记录的明文，满时施加背压
    @param [in] data 仅size为0时可空，本次调用复制，不在返回后借用
    @param [in] size 明文字节数，超过16384时只接受前缀
    @param [out] accepted 已排队字节数，不代表送达；失败后由业务请求幂等重试
    @return OK、WANT_IO、STATE或持续错误
    */
    int32_t write(const uint8_t* data, uint32_t size, uint32_t& accepted);

    /** 读取经完整性验证的应用明文
    @param [out] output 有效可写缓冲，仅capacity为0时可空
    @param [in] capacity 可写字节数
    @param [out] received 实际输出字节数
    @return OK、WANT_IO、CLOSED、STATE或持续错误
    */
    int32_t read(uint8_t* output, uint32_t capacity, uint32_t& received);

    /** 声明底层TCP已结束；允许先排空已到达的完整TLS记录
    @return OK或持续错误，后续read拒绝没有close_notify的截断
    */
    int32_t finishInput();

    /** 在已排队明文之后发送认证关闭通知，仍须排空密文输出
    @return OK、WANT_IO、STATE或持续错误
    */
    int32_t close();

    /** 查询握手、连接或持续错误状态
    @return NetInterfaceTlsStatus
    */
    int32_t status() const;

    /** 查询底层Mbed TLS错误，便于日志定位；不得当作业务认证结果
    @return 最近底层错误，0表示无错误
    */
    int32_t detail() const;

public:
    /** 查询本层待发数据是否全部交给下层；只由协议执行者调用
    @return OK全部交付，READY仍有本层待发数据，负数失败；不表示远端收到
    */
    int32_t outputState() const;

  private:
    /** 禁止复制独占TLS会话
    @param [in] other 不可复制的实例
    */
    NetInterfaceTls(const NetInterfaceTls& other);

    /** 禁止复制独占TLS会话
    @param [in] other 不可复制的实例
    @return 无实现
    */
    NetInterfaceTls& operator=(const NetInterfaceTls& other);

  private:
    // 隔离第三方TLS类型与公共ABI，实例独占并在本模块销毁
    NetInterfaceTlsState* m_state;
};
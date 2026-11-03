#pragma once
#include "NetInterfaceTypes.h"

// TLS后应用握手，不替代证书验证或账号认证；客户端平台/设备是声明而非可信权限。
struct NetInterfaceHandshakeOptions
{
    // 0客户端、1服务器
    uint32_t m_server;
    // 客户端1桌面、2手机App、3微信小程序；服务器填0
    uint32_t m_platform;
    // 位0多路请求必需；位1数据块、位2业务SDK、位3压缩可选，其余位不得设置
    uint32_t m_capabilities;
    // 1至128，协商取双方较小值
    uint32_t m_maxRequests;
    // 1024至65536，协商取双方较小值
    uint32_t m_maxChunk;
    // 100至60000毫秒，从TLS就绪时开始
    uint32_t m_timeoutMs;
    // 宿主CSPRNG产生的本连接新随机数，不可全零
    uint8_t m_nonce[16];
    // 客户端持久设备标识，不可全零；服务器填零
    uint8_t m_device[16];
};

// 无socket、线程、动态分配的单连接握手状态机；原生和Wasm同源。
class NetInterfaceAPI NetInterfaceHandshake
{
  public:
    /** TLS就绪后创建；校验失败保持负数状态
    @param [in] options 本次复制的本地能力和随机数
    @param [in] nowMs 宿主单调毫秒时间
    */
    NetInterfaceHandshake(const NetInterfaceHandshakeOptions& options, uint64_t nowMs);

    /** 检查握手截止时间及单调时钟回退，错误保持到实例销毁
    @param [in] nowMs 同一宿主单调时间
    @return READY、NEED_MORE或负数错误
    */
    int32_t progress(uint64_t nowMs);

    /** 校验一帧；握手前禁止普通业务，握手后禁止重复握手
    @param [in] frame 已由NetInterface完成拆包的借用帧
    @param [in] nowMs 同一宿主单调时间
    @return READY已完成握手，NEED_MORE待发送回复，OK是握手后的普通帧，负数断开
    */
    int32_t accept(const NetInterfaceFrame& frame, uint64_t nowMs);

    /** 查看需要发送的固定64字节握手消息
    @param [out] frame 借用到consumeOutput/销毁，未就绪时清空
    @return READY存在输出，否则NEED_MORE或持续错误
    */
    int32_t peekOutput(NetInterfaceFrame& frame) const;

    /** 仅在下层已接纳完整帧时消费，背压时保留不丢弃
    @return 客户端NEED_MORE等待回复，服务器READY；没有输出时INVALID_STATE
    */
    int32_t consumeOutput();

    /** 取已协商能力；调用方仍须登录/授权，不从设备类型推导VIP
    @param [out] fields 平台、能力、在途上限、块大小共4个uint32
    @param [out] peerNonce 对端连接随机数16字节
    @param [out] clientDevice 本次客户端声明设备标识16字节
    @return READY或尚未就绪/失败状态
    */
    int32_t peerInfo(uint32_t fields[4], uint8_t peerNonce[16], uint8_t clientDevice[16]) const;

  private:
    /** 锁定错误并丢弃待发握手
    @param [in] result 负数错误
    @return 同一错误
    */
    int32_t fail(int32_t result);

    /** 生成本实例固定长度输出
    @param [in] reply false客户端HELLO，true服务器HELLO_REPLY
    */
    void makeOutput(bool reply);

  private:
    // 本连接配置副本，无宿主指针
    NetInterfaceHandshakeOptions m_options;
    // 单调时钟及固定截止时间
    uint64_t m_lastMs;
    // TLS后应用握手的固定单调毫秒期限，不随收到无关数据延长
    uint64_t m_deadlineMs;
    // 对端随机数、客户端设备和最终能力
    uint8_t m_peerNonce[16];
    // 握手中声明的客户端设备号，不独立构成登录或VIP权限
    uint8_t m_clientDevice[16];
    // 已协商的平台、能力、请求上限和块大小，按peerInfo约定顺序存放
    uint32_t m_agreed[4];
    // 固定输出，pending期间不能消费外来握手
    uint8_t m_output[64];
    // 待发HELLO或HELLO_REPLY类型，0表示当前无输出
    uint16_t m_outputKind;
    // NEED_MORE/READY或持续错误
    int32_t m_status;
};
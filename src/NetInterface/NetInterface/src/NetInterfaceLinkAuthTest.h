#pragma once
#include "NetInterface.h"
#include "NetInterface/NetInterfaceLinkAuditTest.h"

// 一端为真实管理协议，另一端仅做真实TLS/帧收发，便于注入未经管理层认可的消息。
class NetInterfaceLinkAuthTestPair
{
public:
    /** 创建实际受测管理端和允许测试直接构造管理帧的TLS端
    @param [in,out] identity 隔离节点身份和测试随机源
    @param [in] server 受测管理端是否为服务端
    */
    NetInterfaceLinkAuthTestPair(NetInterfaceLinkTestIdentity& identity, bool server);

    /** 交换固定外层HELLO并初始化推进时间
    @return 是否成功，后续不重发HELLO
    */
    bool start();

    /** 推进一次双方TLS/管理状态和有限字节，不跳过任何加密层
    @return 管理端当前状态或错误
    */
    int32_t step();

    /** 有界等待并复制TLS端观察到的一帧，确认类别和类型后消费
    @param [in] kind 期望帧类别
    @param [in] type 期望消息类型
    @param [out] body 完整帧正文副本
    @return 是否收到指定帧
    */
    bool receive(uint16_t kind, uint32_t type, std::vector<uint8_t>& body);

    /** 继续推进直至管理就绪或失败；没有对端回包时不自发推进时间
    @return 管理状态
    */
    int32_t settle();

    /** 构造与管理端相反方向的真实TLS配置，客户端仍检查内置根和域名
    @param [in,out] identity 测试身份和随机源
    @param [in] server TLS端方向
    @return 创建期间借用的配置
    */
    static NetInterfaceTlsOptions tls(NetInterfaceLinkTestIdentity& identity, bool server);

    /** 独立构造测试大端32位字段，不调用被测协议编码辅助
    @param [out] bytes 至少四字节的已分配字段
    @param [in] value 数值
    */
    static void word(uint8_t* bytes, uint32_t value);

public:
    // 完整受测管理协议，持有自己的状态
    NetInterfaceLink m_link;
    // 对端仅采用真实TLS和NIF2帧，测试可以构造畸形管理正文
    NetInterfaceConnection m_raw;
    // 管理端外层输出向TLS端喂入时的偏移，整条消费前不丢弃未读字节
    uint32_t m_offset;
};
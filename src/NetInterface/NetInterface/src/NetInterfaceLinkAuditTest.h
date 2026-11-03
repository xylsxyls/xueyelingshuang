#pragma once
#include "NetInterface.h"
#include <memory>
#include <string>

// 只读取隔离测试产生的叶身份，不包含CA私钥或固定测试身份
class NetInterfaceLinkTestIdentity
{
public:
    /** 初始化可预测测试随机状态；禁止用于产品
    */
    NetInterfaceLinkTestIdentity();

    /** 清理本测试拥有的叶私钥副本
    */
    ~NetInterfaceLinkTestIdentity();

    /** 有界读取NGI3隔离身份，核对长度及摘要
    @param [in] path 本次隔离测试生成的identity.dat路径
    @return 是否读取成功，不输出身份内容
    */
    bool load(const char* path);

    /** 生成仅供可重复测试的字节
    @param [in] context 本测试身份
    @param [out] output 有效缓冲
    @param [in] size 字节数
    @return 0
    */
    static int32_t entropy(void* context, uint8_t* output, uint32_t size);

    /** 生成借用本测试身份的管理配置
    @param [in] server 是否服务端
    @return v3节点/匿名客户端配置，借用至本对象析构
    */
    NetInterfaceLinkOptions options(bool server);

public:
    // 隔离节点名称
    std::string m_name;
    // 隔离叶私钥，仅供当前测试
    std::string m_key;
    // 对应固定根签发的公开叶证书
    std::string m_certificate;
    // 可预测随机状态，不用于任何真实连接
    uint32_t m_seed;
};

// 同一线程的真实TLS管理对，桥只转交外层记录，不模拟认证实现
class NetInterfaceLinkTestPair
{
public:
    /** 创建两侧实际管理状态
    @param [in,out] identity 身份及随机源借用到本对象析构
    */
    explicit NetInterfaceLinkTestPair(NetInterfaceLinkTestIdentity& identity);

    /** 有界推进两侧直到双方完成管理认证
    @return 是否就绪
    */
    bool handshake();

    /** 转交一条已编码外层记录；目的端不主动推进，便于制造EOF同批输入
    @param [in,out] from 源协议
    @param [in,out] to 目的协议
    @return OK转交、NEED_MORE无输出或失败
    */
    static int32_t move(NetInterfaceLink& from, NetInterfaceLink& to);

public:
    // 已认证匿名客户端
    NetInterfaceLink m_client;
    // 已认证网关节点
    NetInterfaceLink m_server;
};
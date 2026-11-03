#pragma once
#include "NetEndpointRouter.h"

/** 拥有一条文件流的双端点绑定和秘密副本，索引借用其地址
*/
struct NetEndpointFileStream
{
public:
    // 网关公开流号和固定到期堆下标；堆及端点链仅借用本对象。
    uint64_t m_id;
    // 本流在固定期限堆中的下标，摘除前保持与堆一致
    uint32_t m_expiryIndex;
    // 0为客户端链，1为服务链；一条流同时属于两个互不相同的端点。
    NetEndpointFileStream* m_previous[2];
    // 分别指向客户端链和服务链的下一流，不拥有节点
    NetEndpointFileStream* m_next[2];
    // 真实根来源及服务代次；后续帧不能更换归属。
    uint64_t m_client;
    // 本代已认证服务端点号，服务换代后不允许迁移旧流
    uint64_t m_service;
    // 服务端流号及单调到期期限。
    uint64_t m_local;
    // 由授权有效期换算并限制后的单调毫秒期限
    uint64_t m_deadline;
    // 授权产品及完整秘密元数据，析构擦除。
    uint32_t m_product;
    // 独占的完整传输授权信息，含秘密令牌，析构时擦除
    NetInterfaceFileInfo m_info;

public:
    /** 初始化本代流的双端映射
    @param [in] client 根连接
    @param [in] service 已认证服务
    @param [in] product 非零产品
    @param [in] info 服务端原始信息，stream为服务端本地号
    @param [in] deadline 固定单调期限
    */
    NetEndpointFileStream(uint64_t client, uint64_t service, uint32_t product, const NetInterfaceFileInfo& info, uint64_t deadline);

    /** 擦除秘密
    */
    ~NetEndpointFileStream();
};

// 栈上协议秘密的退出擦除；不拥有存储，不抛异常，也不引入动态分配。
class NetEndpointSecretScope
{
public:
    /** 借用当前作用域中有效的秘密区域
    @param [in,out] bytes 需在离开作用域时擦除的区域
    @param [in] size 字节数
    */
    NetEndpointSecretScope(void* bytes, uint32_t size);

    /** 正常或异常退出均擦除
    */
    ~NetEndpointSecretScope();

private:
    /** 不可复制借用清理器
    @param [in] other 禁止复制
    */
    NetEndpointSecretScope(const NetEndpointSecretScope& other);

    /** 不可复制借用清理器
    @param [in] other 禁止复制
    @return 不可调用
    */
    NetEndpointSecretScope& operator=(const NetEndpointSecretScope& other);

private:
    // 借用的栈内存及其固定长度。
    void* m_bytes;
    // 借用秘密区域的固定字节数，必须覆盖需清理的有效范围
    uint32_t m_size;
};
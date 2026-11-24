#pragma once
#include "NetEndpointRouter.h"

/** 拥有一条文件流的双端点绑定和秘密副本，索引借用其地址
*/
struct NetEndpointFileStream
{
public:
    // 网关公开流号和到期堆下标；堆及端点链仅借用本对象。
    uint64_t m_id;
    // 本流在期限堆中的下标，摘除前保持与堆一致
    size_t m_expiryIndex;
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
#pragma once
#include "NetSenderMacro.h"
#include "NetInterface/NetInterfaceAPI.h"

// 本地密文的独占物理池租约；从回调到协议消费全程占槽，移动不复制正文
class NetSenderAPI NetSenderLocalBuffer
{
public:
    /** 建立空缓冲
    */
    NetSenderLocalBuffer();

    /** 转交实际池槽，原对象清空
    @param [in,out] other 原独占缓冲
    */
    NetSenderLocalBuffer(NetSenderLocalBuffer&& other);

    /** 释放原槽再转交，允许自移动
    @param [in,out] other 原独占缓冲
    @return 当前对象
    */
    NetSenderLocalBuffer& operator=(NetSenderLocalBuffer&& other);

    /** 归还池槽；所属池须持续存活
    */
    ~NetSenderLocalBuffer();

    /** 从指定实际池申请空间，不预留额外业务计数
    @param [in,out] pool 生命周期覆盖缓冲的物理池
    @param [in] size 有效长度，申请成功后调用者须完整填充
    @return NI_OK或具体池错误；非空缓冲返回INVALID_STATE
    */
    int32_t allocate(NetBufferPool& pool, uint32_t size);

    /** 交换租约及长度，不复制正文
    @param [in,out] other 另一独占缓冲
    */
    void swap(NetSenderLocalBuffer& other);

    /** 清空长度并归还物理池槽，不销毁所属池
    */
    void clear();

    /** 获取有效字节数
    @return 当前长度
    */
    uint32_t size() const;

    /** 获取独占槽地址；不得跨clear或移动保留
    @return 借用地址或nullptr
    */
    uint8_t* data() const;

    /** 查询是否为空
    @return 长度为0返回true
    */
    bool empty() const;

private:
    /** 禁止复制独占槽
    @param [in] other 原对象
    */
    NetSenderLocalBuffer(const NetSenderLocalBuffer& other);

    /** 禁止复制独占槽
    @param [in] other 原对象
    @return 不可调用
    */
    NetSenderLocalBuffer& operator=(const NetSenderLocalBuffer& other);

private:
    // 物理槽持续占用到协议消费或失败清理结束
    NetBufferLease m_lease;
    // 已申请的有效长度
    uint32_t m_size;
};
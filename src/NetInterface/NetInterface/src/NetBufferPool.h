#pragma once
#include "NetObjectPool.h"

class NetBufferPool;

#include "NetBufferPoolConfig.h"

// 池槽中的固定尺寸正文，无额外堆分配；有效长度由上层持有者管理
template <size_t Capacity>
struct NetBufferPoolBlock
{
public:
    /** 不清零尚未使用的缓冲，提交者必须完整覆盖有效长度
    */
    NetBufferPoolBlock();

public:
    // 原位存储；归还前由正文拥有者擦除实际使用部分
    uint8_t m_bytes[Capacity];
};

// 独占缓冲租约；不可复制，所属池必须晚于全部租约销毁
class NetInterfaceAPI NetBufferLease
{
public:
    /** 建立空租约
    */
    NetBufferLease();

    /** 归还池槽，不擦除内容；正文拥有者负责有效部分的擦除
    */
    ~NetBufferLease();

    /** 交换独占槽，不分配、复制或归还正文；允许与自身交换
    @param [in,out] other 另一租约，所属池均须持续存活
    */
    void swap(NetBufferLease& other);

    /** 清空租约并归还实际池槽
    */
    void reset();

    /** 查询可写缓冲，仅租约有效时可访问
    @return 借用指针或nullptr
    */
    uint8_t* data() const;

    /** 查询实际分配容量
    @return 池级字节数，空租约为0
    */
    uint32_t capacity() const;

    /** 接管池新创建的槽；仅供池工厂使用
    @param [in,out] pool 生命期覆盖租约的池
    @param [in] storage 新槽的不透明所有权
    @param [in] bytes 新槽中的缓冲
    @param [in] size 实际池级容量
    */
    void adopt(NetBufferPool& pool, void* storage, uint8_t* bytes, uint32_t size);

private:
    /** 禁止复制独占租约
    @param [in] other 原租约
    */
    NetBufferLease(const NetBufferLease& other);

    /** 禁止复制池槽所有权
    @param [in] other 原租约
    @return 不可调用
    */
    NetBufferLease& operator=(const NetBufferLease& other);

private:
    // 借用的所属池
    NetBufferPool* m_pool;
    // 独占槽，reset归还一次
    void* m_storage;
    // 槽内可写正文
    uint8_t* m_bytes;
    // 实际池级容量
    uint32_t m_capacity;
};

// 多生产者正文池；大小类隔离使小消息不会预占最大正文尺寸
class NetInterfaceAPI NetBufferPool
{
public:
    /** 构造实际物理池；配置一次，之后不可变
    @param [in] config 五级基础池配置
    */
    explicit NetBufferPool(const NetBufferPoolConfig& config);

    /** 检查所有大小类初始化
    @return NI_OK或具体资源/参数错误
    */
    int32_t status() const;

    /** 从能容纳正文的最小级分配；不挪用其他级的保留容量
    @param [in] size 0至1048704，0字节不分配
    @param [out] lease 必须为空，失败保持空
    @return NI_OK或容量/内存/参数错误
    */
    int32_t allocate(uint32_t size, NetBufferLease& lease);

    /** 归还此前转交的独占槽，仅供租约使用
    @param [in] size 分配时的实际池级
    @param [in] storage 原池槽，不可重复归还
    */
    void release(uint32_t size, void* storage);

    /** 查询单一大小类实际内存
    @param [in] index 0至4，其他值返回空统计
    @return 基础池统计
    */
    LockFreeHashMapPoolStats statistics(uint32_t index);

private:
    /** 从指定物理池创建并转交槽，不增加正文引用层
    @param [in,out] pool 对应大小类的池
    @param [out] lease 空租约
    @return NI结果
    */
    template <size_t Capacity> int32_t acquire(NetObjectPool<NetBufferPoolBlock<Capacity>>& pool, NetBufferLease& lease);

private:
    // 默认策略准备结果；失败时禁止分配，保留原始内存错误
    int32_t m_configurationStatus;
    // 小正文池
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    NetObjectPool<NetBufferPoolBlock<64>> m_small;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 短报文池
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    NetObjectPool<NetBufferPoolBlock<1024>> m_short;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 中等正文池
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    NetObjectPool<NetBufferPoolBlock<16448>> m_medium;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 流块及帧元数据池
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    NetObjectPool<NetBufferPoolBlock<131072>> m_chunk;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 最大请求正文池
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    NetObjectPool<NetBufferPoolBlock<1048704>> m_large;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

#include "NetBufferPool.inl"
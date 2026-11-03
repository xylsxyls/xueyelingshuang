#pragma once
#include <cstdint>

// 一份业务号/线上号关联，不拥有请求正文；空闲槽复用local字段串接空闲链。
struct NetSenderDispatchEntry
{
    // 活动线上编号，0表示空闲。
    uint64_t m_wire;
    // 活动业务编号；空闲时为下一个槽的下标加一，0结束。
    uint64_t m_local;
};

// 固定128个关联、两个256槽开放寻址索引；删除后回移，无墓碑或运行期分配。
class NetSenderDispatchMap
{
public:
    /** 初始化空表，不分配外部资源
    */
    NetSenderDispatchMap();

    /** 同时登记两个非零且唯一编号，失败不改变任一索引
    @param [in] wire 实际提交顺序的线上编号
    @param [in] local 上层预留编号
    @return 是否已登记，重复或128条满时false
    */
    bool insert(uint64_t wire, uint64_t local);

    /** 读取对应业务号
    @param [in] wire 线上编号
    @return 业务号，未找到为0
    */
    uint64_t local(uint64_t wire) const;

    /** 读取对应线上号
    @param [in] local 业务编号
    @return 线上号，未找到为0
    */
    uint64_t wire(uint64_t local) const;

    /** 原子摘除两个索引并归还槽，未知编号无副作用
    @param [in] wire 线上编号
    */
    void erase(uint64_t wire);

    /** 丢弃全部编号映射，恢复完整空闲链
    */
    void clear();

    /** 查询实际关联数，不遍历槽
    @return 0至128
    */
    uint32_t size() const;

private:
    /** 混合64位编号两半，返回固定索引起点
    @param [in] key 非零编号
    @return 0至255
    */
    static uint32_t hash(uint64_t key);

    /** 查到既有键或第一个空槽，循环有256步上限
    @param [in] key 查询编号
    @param [in] byLocal 是否使用业务号索引
    @return 索引下标，无法找到槽时256
    */
    uint32_t bucket(uint64_t key, bool byLocal) const;

    /** 删除单侧索引并回移其探测链，调用时关联仍有效
    @param [in] key 既有键
    @param [in] byLocal 是否业务号侧
    */
    void remove(uint64_t key, bool byLocal);

private:
    // 关联只有此处一份，两个索引均借用槽号。
    NetSenderDispatchEntry m_entries[128];
    // 0为空，否则为关联槽下标加一，装载率最多一半。
    uint16_t m_wire[256];
    // 业务号反查索引，约束与线上号索引相同。
    uint16_t m_local[256];
    // 空闲链首槽下标加一，0表示全部使用。
    uint32_t m_free;
    // 当前活动槽数，删除不触发分配。
    uint32_t m_count;
};
#pragma once
#include "NetInterfaceTypes.h"
#include <unordered_map>

// 单执行者双向编号索引；每条关联对应一个已接纳请求池槽，不重复设置请求窗口
class NetSenderDispatchMap
{
public:
    /** 初始化空表，索引随已接纳请求记录增长
    */
    NetSenderDispatchMap();

    /** 同时登记两个非零且唯一编号，失败不改变任一索引
    @param [in] wire 实际提交顺序的线上编号
    @param [in] local 上层预留编号
    @return NI_OK已登记，重复/零编号为INVALID_ARGUMENT，分配失败为NO_MEMORY
    */
    int32_t insert(uint64_t wire, uint64_t local);

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

    /** 丢弃全部编号映射，保留桶容量供后续复用
    */
    void clear();

    /** 查询实际关联数，不遍历槽
    @return 当前活动关联数
    */
    uint32_t size() const;

private:
    // 线上号到业务号，不拥有正文；只为已经取得请求池槽的记录建立索引
    std::unordered_map<uint64_t, uint64_t> m_wire;
    // 业务号到线上号，插入失败必须回滚上面的索引
    std::unordered_map<uint64_t, uint64_t> m_local;
};
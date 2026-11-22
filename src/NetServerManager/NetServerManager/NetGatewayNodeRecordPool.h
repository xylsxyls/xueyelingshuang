#pragma once
#include "NetGatewayNodeRecord.h"

// 节点全部方向共用的真实转发存储，调用者在节点唯一执行者内串行使用
class NetGatewayNodeRecordPool
{
public:
    /** 使用各大小级正文默认池及实际记录槽预算
    */
    NetGatewayNodeRecordPool();

    /** 初始化指定的实际池，不叠加排队条数或字节计数门槛
    @param [in] bodies 正文大小级存储配置
    @param [in] records 实际记录槽配置
    */
    NetGatewayNodeRecordPool(const NetBufferPoolConfig& bodies, const LockFreeHashMapPoolConfig& records);

    /** 查询全部池的初始化结果
    @return NI_OK或配置、内存、平台错误
    */
    int32_t status() const;

    /** 完整复制一条NTW3正文，失败不留下半条队列项
    @param [in] data 非空密文，借用到返回
    @param [in] size 1至16384字节，属于协议记录长度
    @param [out] output 必须为空；成功拥有记录及正文，最后引用释放后归还容量
    @return NI_OK或参数、真实池容量、分配错误
    */
    int32_t copy(const uint8_t* data, uint32_t size, NetObjectReference<NetGatewayNodeRecord>& output);

    /** 取得默认记录池参数，调用者可在初始化前修改
    @return 约8MiB实际记录槽预算，初始128槽
    */
    static LockFreeHashMapPoolConfig defaultRecords();

private:
    /** 禁止复制实际池
    @param [in] other 不可复制对象
    */
    NetGatewayNodeRecordPool(const NetGatewayNodeRecordPool& other);

    /** 禁止赋值实际池
    @param [in] other 不可赋值对象
    @return 不可调用
    */
    NetGatewayNodeRecordPool& operator=(const NetGatewayNodeRecordPool& other);

private:
    // 必须比记录池晚销毁
    NetBufferPool m_bodies;
    // 每条记录占用真实槽，拒绝伪装成内存池的独立计数限额
    NetObjectPool<NetGatewayNodeRecord> m_records;
};
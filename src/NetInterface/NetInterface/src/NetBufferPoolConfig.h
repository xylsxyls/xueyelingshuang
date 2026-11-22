#pragma once
#include "NetInterfaceTypes.h"
#include "LockFreeHashMap/LockFreeHashMapAPI.h"

// 分级正文池配置；每级直接采用基础池容量、增长及保护记录契约
struct NetInterfaceAPI NetBufferPoolConfig
{
public:
    // 64、1024、16448、131072、1048704字节五级；元数据也占实际内存
    LockFreeHashMapPoolConfig m_classes[5];

public:
    /** 默认预分配小正文，较大正文按需增长；每级最大槽数按8MiB实际槽大小计算，包含槽内元数据
    */
    NetBufferPoolConfig();

    /** 查询默认扩容策略准备结果
    @return NI_OK或策略分配失败的NI_NO_MEMORY
    */
    int32_t status() const;

private:
    // 构造期间分配失败以状态交给实际池，不从配置构造向外抛出
    int32_t m_status;
};
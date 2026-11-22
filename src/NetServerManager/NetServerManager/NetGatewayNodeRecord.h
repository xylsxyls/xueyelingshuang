#pragma once
#include "NetInterface/NetInterfaceAPI.h"

// 节点转发的一条独占密文，记录和正文分别占用真实池槽
struct NetGatewayNodeRecord
{
public:
    // 完整正文租约，所属节点池晚于所有记录释放
    NetBufferLease m_bytes;
    // 正文有效字节数，不包含正文池的容量对齐
    uint32_t m_size;

public:
    /** 创建未填充记录
    */
    NetGatewayNodeRecord();

    /** 擦除有效密文后归还正文槽
    */
    ~NetGatewayNodeRecord();
};
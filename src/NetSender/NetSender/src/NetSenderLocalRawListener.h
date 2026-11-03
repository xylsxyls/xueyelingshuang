#pragma once
#include "NetSenderMacro.h"
#include "NetInterface/NetInterfaceAPI.h"

// 本机路由只复制NTW3。回调ready仅表示IPC可投递，身份和业务TLS仍由端点验证。
class NetSenderAPI NetSenderLocalRawListener
{
public:
    /** 不拥有本机传输
    */
    virtual ~NetSenderLocalRawListener();

    /** 本机投递资源可用，不将PID或共享内存代次当作身份
    @param [in] peer 独立IPC代次
    @param [in] pid 本机对端声明，仅用于发送地址
    */
    virtual void onRawLocalReady(uint64_t peer, int32_t pid) = 0;

    /** 借用外层记录，无密钥，不解密
    @param [in] peer IPC代次
    @param [in] record 借用至返回
    @return false无副作用背压；true接纳
    */
    virtual bool onRawLocalRecord(uint64_t peer, const NetInterfaceTunnelRecord& record) = 0;

    /** 回收后唯一终态
    @param [in] peer 结束代次
    @param [in] reason 0主动或负数错误，不能当作业务成功
    */
    virtual void onRawLocalClosed(uint64_t peer, int32_t reason) = 0;
};
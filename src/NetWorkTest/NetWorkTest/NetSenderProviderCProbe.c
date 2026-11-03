#include "NetSenderProviderCProbe.h"
#include "NetWork/NetSenderAPI.h"

/** 验证纯C消费NetWork时不会同时选择依赖NetPeer的NetSender别名
@return 0通过；非零表示公共入口参数或释放契约不符
*/
int netSenderProviderCProbe(void)
{
    NetSenderClientHandle handle = (NetSenderClientHandle)1;
    if (netSenderClientCreate(0, &handle) != NI_INVALID_ARGUMENT || handle != 0)
    {
        return 1;
    }
    return netSenderClientDestroy(0) == NI_OK ? 0 : 2;
}
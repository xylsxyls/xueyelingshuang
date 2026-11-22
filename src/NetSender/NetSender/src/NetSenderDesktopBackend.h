#pragma once
#include "NetSenderMacro.h"
#include <stdint.h>
#include "LockFreeHashMap/LockFreeHashMapAPI.h"

// 后端只借用独立客户端，不依赖上层NetSender门面
class NetSenderGatewayState;

// 桌面实际进程参数只放后端，不能放入三端共同业务接口
class NetSenderAPI NetSenderDesktopBackend
{
public:
    /** 配置已经准备且尚未连接的桌面目标
    @param [in,out] client 本桌面提供方实例
    @param [in] pid 非负，0按网关租约自动发现，正数仅用于受控目标
    @return OK或明确错误
    */
    static int32_t configureGateway(NetSenderGatewayState* client, int32_t pid);

    /** 特殊宿主在自己的ProcessWork.initReceive之前声明外部接收所有权
    宿主随后initReceive的首参至少16452；之后prepareGateway才检查实际声明容量；createClient只创建静止对象
    @return OK仅完成登记；已有未声明接收器为INVALID_STATE，一般业务使用默认自管接收器
    */
    static int32_t prepareExternalReceiver();

    /** 在任何接收租约或接收器启动之前，配置模块内收件箱借用槽的实际池
    @param [in] config 初始化槽数、最大槽数及增长策略；同一模块的所有客户端共用
    @return NI_OK或配置/资源/生命周期错误，失败保留原配置
    */
    static int32_t configureReceiverPool(const LockFreeHashMapPoolConfig& config);

    /** 外部宿主先释放全部Sender，再uninitReceive，最后调用本方法
    @return OK或INVALID_STATE；不停止外部资源
    */
    static int32_t externalReceiverStopped();

    /** 检查可最终释放，不驱动协议或业务回调
    @param [in] client 本提供方实例
    @return 协议及本地接纳均已收敛
    */
    static bool quiescent(const NetSenderGatewayState* client);
};
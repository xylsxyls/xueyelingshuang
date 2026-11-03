#pragma once

// 仅由Node.cpp默认关闭的单测main使用，不接触进程或真实网关。
class NetGatewayNodeLoadTest
{
public:
    /** 检查真实选择/OPEN实现的容量和快照预留边界
    @return 0全部通过，1失败
    */
    static int run();
};
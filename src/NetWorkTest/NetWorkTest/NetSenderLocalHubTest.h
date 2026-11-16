#pragma once
#include "NetSenderTestAccess.h"

// 白盒入口只在NetWorkTest编译，不修改正式类的可见性、ABI或导出。
class NetSenderLocalHubTest
{
public:
    /** 按独立目标随机代次验证投递与满载隔离
    @return 0通过，非零表示断言失败
    */
    static int run();
};
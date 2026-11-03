#pragma once
#include "NetSenderLocal.h"

// 测试入口仅在Inbox.cpp注释main启用时定义，不进入SDK导出
class NetSenderLocalHubTest
{
public:
    /** 按独立目标随机代次验证投递与满载隔离
    @return 0通过，非零表示断言失败
    */
    static int run();
};
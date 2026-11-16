#pragma once

// 协议请求记录及编号映射扩容测试；不依赖真实网络或TLS性能
class NetRequestPoolTest
{
public:
    /** 检查超过128条、期限堆任意摘除、真实池耗尽和双向映射复用
    */
    static void run();
};
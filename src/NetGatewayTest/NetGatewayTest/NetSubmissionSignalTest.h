#pragma once
#include "NetInterface/NetInterfaceAPI.h"
#include <atomic>

/** 黑盒记录单票据通知；不依赖等待线程是否已经登记系统事件
*/
class NetSubmissionSignalProbe : public NetSenderSubmissionSignal
{
public:
    /** 初始化零通知计数
    */
    NetSubmissionSignalProbe();

    /** 原子记录完成通知
    */
    virtual void notify();

public:
    // 不同执行线程可通知，测试线程在join后检查
    std::atomic<uint32_t> m_count;
};

/** 验证完成先到、登记先到、两者竞争，以及消费后的通知对象回收
*/
class NetSubmissionSignalTest
{
public:
    /** 按三种时序反复登记和完成，校验不漏通知、不重复交付、不泄露引用
    @return 全部通过为true
    */
    static bool check();

    /** 接入统一runner，失败抛出测试异常
    */
    static void run();
};
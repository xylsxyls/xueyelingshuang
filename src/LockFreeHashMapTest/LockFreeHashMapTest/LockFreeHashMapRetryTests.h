#pragma once

#include "LockFreeHashMapTests.h"

/** 增长回调的复制故障夹具，捕获区超过 v120 小对象优化范围
*/
struct LockFreeHashMapGrowthTestCallback
{
public:
    // 完整复制的确定性捕获内容
    unsigned char m_data[512];
    // 仅在无工作线程时设置的复制故障开关
    static bool s_throwCopy;
    // 已完整构造而尚未析构的目标数
    static int32_t s_live;
    // 复制尝试次数，包括失败尝试
    static int32_t s_copies;
public:
    /** 初始化捕获内容并计账
    */
    LockFreeHashMapGrowthTestCallback();

    /** 注入复制故障，失败构造不增加存活数
    @param [in] other 来源目标
    */
    LockFreeHashMapGrowthTestCallback(const LockFreeHashMapGrowthTestCallback& other);

    /** 归还目标存活计账
    */
    ~LockFreeHashMapGrowthTestCallback();

    /** 核对目标仍然完整
    @param [in] count 本次增长量
    @param [in] total 累计成功增长次数
    */
    void operator()(size_t count, uint64_t total) const;
};

/** 重试合同及增长通知的黑盒回归，所有注入均留在测试工程
*/
class LockFreeHashMapRetryTests
{
public:
    /** 验证普通失败、显式等待、回调所有权和异常回滚
    @return 全部独立预期通过为 true，失败由既有测试入口记录
    */
    static bool run();
private:
    /** 普通接口不等待真实容量；两项 try 开关分别生效
    */
    static void capacity();

    /** 用真实策略同步点确认等待者已重试，再释放槽
    @param [in] retryContention 是否同时开启竞争重试
    */
    static void waiting(bool retryContention);

    /** 检查条件版本及嵌套回调自等待防护
    */
    static void versions();

    /** 验证通知计账、回调复制异常及配置共享目标
    */
    static void observers();

    /** 相同下标赋值入口竞争，不能仅因 CAS 冲突抛异常
    */
    static void assignments();

    /** 验证并发逐次通知，计数快照不作为唯一事件编号
    */
    static void concurrentGrowth();
};

/** 主用例 11 的统一入口，单线程宿主跳过真实线程子场景
@return 全部适用子场景通过为 true
*/
bool LockFreeHashMapRetryTest();
#pragma once

// 最新ProcessWork返回值、真实共享发布及Hub生命周期回归，不向正式库嵌入测试。
class NetIpcAdapterTest
{
public:
    /** 真实两槽共享区满载返回CAPACITY，释放后可重试；检查字节、次数及错误分类
    */
    static void capacity();

    /** 发送器关闭不撤销共享发布，重开后继续完整发送，绝不重复正文
    */
    static void publication();

    /** Hub内外部所有权、重复attach、64租约和最后一项释放
    */
    static void lifecycle();

    /** 搬迁原生产源码中的关闭阶段测试，终态不能倒退或被后来的END覆盖
    */
    static void phase();

    /** 16个真实任务并发投递，每个256条；检查每生产者FIFO、进度及stop后清理
    */
    static void concurrentInbox();

    /** 生产者仍存活且四包窗口已占用时stop，随后join；旧代数据不得在重启后复活
    */
    static void concurrentStop();
};
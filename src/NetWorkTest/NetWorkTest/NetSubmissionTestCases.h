#pragma once

// 提交层的完整边界测试与独立模块基准，测试实现只存在于NetWorkTest。
class NetSubmissionTestCases
{
public:
    /** 验证预算、FIFO、借用、回执、重连及1024轮析构回收
    */
    static void boundary();

    /** 验证连续业务提交不越过控制屏障，同步票据仍等完整交付
    */
    static void orderedBatch();

    /** 同一128字节正文完成320000次请求/流帧交付；不代表网络吞吐
    */
    static void benchmark();
};
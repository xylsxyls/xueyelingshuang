#pragma once

// 提交层真实物理池回归；仅测试工程包含，生产库不嵌入检查钩子
class NetSubmissionPoolTest
{
public:
    /** 检查分级缓冲边界、超过旧条数限制、在途借用、耗尽恢复和析构排空
    */
    static void run();
};
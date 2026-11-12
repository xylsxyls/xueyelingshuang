#pragma once
#include <stdint.h>

/** 用例79：真实单槽共享通道与本地Post预算的两种重试策略
    不等待容量时必须立即返回容量不足且不提交；等待容量时须等接收方释放后提交一次。
    四种开关组合、普通接口等价性、停止等待、4096次之后持续1毫秒退避分别核对。
    使用真实共享缓冲和线程，库内不插入测试钩子；失败先停止接收器再回收等待线程。
*/
class ProcessWorkRetryTests
{
public:
    /** 运行容量释放及未提交边界用例
    */
    static void run();

private:
    /** 单槽发送容量释放后只提交一次
    @param [in] mode 两种bool组合0至3，4普通send，5等待中关闭
    */
    static void sendCapacity(int32_t mode);

    /** 本地Post在途预算满，后台发送释放预算后继续接纳
    @param [in] mode 两种bool组合0至3，4普通post
    */
    static void postCapacity(int32_t mode);
};
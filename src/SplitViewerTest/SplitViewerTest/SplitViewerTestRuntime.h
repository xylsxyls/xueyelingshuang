#pragma once

/** 测试进程最外层生命周期，在所有用例窗口结束后收敛本工程队列
*/
class SplitViewerTestRuntime
{
public:
    /** 建立未初始化的外层测试生命周期对象
    */
    SplitViewerTestRuntime();

    /** 正常启动阶段初始化日志及本工程的任务协调队列
    @return 初始化成功，失败由外层析构完成部分资源收尾
    */
    bool init();

    /** 外层受控回收队列后关闭日志，不调用UninitAll
    */
    ~SplitViewerTestRuntime();
};
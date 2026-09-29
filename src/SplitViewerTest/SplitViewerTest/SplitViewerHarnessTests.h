#pragma once
#include <QtCore/QString>

/** Test主界面真实控件和异步批次收尾回归 */
class SplitViewerHarnessTests
{
public:
    /** 点击主界面运行194，核对防重入、响应和关闭等待
    @param [in] directory 本项报告和截图目录
    @return 窗口、子进程及报告后置条件全部满足
    */
    static bool run(const QString& directory);
};
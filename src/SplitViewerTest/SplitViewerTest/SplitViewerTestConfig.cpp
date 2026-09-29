#include "SplitViewerTestConfig.h"

SplitViewerTestConfig& SplitViewerTestConfig::instance()
{
    static SplitViewerTestConfig config;
    return config;
}

SplitViewerTestConfig::SplitViewerTestConfig() :
m_windowSize(760, 520),
m_title(QStringLiteral("SplitViewer测试")),
m_coreRunText(QStringLiteral("运行Core测试")),
m_uiRunText(QStringLiteral("运行所选回归并保存报告")),
m_allCasesText(QStringLiteral("全部界面与兼容性回归")),
m_reportPrefix(QStringLiteral("报告目录：")),
m_failedText(QStringLiteral("本批次失败或报告不完整。")),
m_startErrorPrefix(QStringLiteral("测试进程启动失败：")),
m_closePendingText(QStringLiteral("本批次结束并恢复测试资源后将关闭窗口。")),
m_maximumOutputBlocks(5000),
m_resultFormat(L"[%ls] %ls%ls"),
m_summaryFormat(L"\n结果：%d/%d 项通过")
{

}
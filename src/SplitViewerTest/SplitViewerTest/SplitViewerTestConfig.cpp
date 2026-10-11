#include "SplitViewerTestConfig.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

SplitViewerTestConfig& SplitViewerTestConfig::instance()
{
    static SplitViewerTestConfig s_config;
    return s_config;
}

SplitViewerTestConfig::SplitViewerTestConfig() :
m_lifetimeProbeFileName(QStringLiteral("lifetime-probe.avi")),
m_logEnabled(true),
m_logConsoleEnabled(false),
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
m_instantText(QStringLiteral("全部瞬时测试")),
m_pressureText(QStringLiteral("全部压力测试")),
m_allText(QStringLiteral("全部测试")),
m_cancelText(QStringLiteral("取消批次")),
m_selectionHint(QStringLiteral("例如：101,103,207-210；空白时使用下拉选择")),
m_cancelPendingText(QStringLiteral("已请求取消，等待当前场景恢复资源后结束。")),
m_releaseProgramPath(),
m_debugProgramPath(),
m_releaseTestPath(),
m_debugTestPath(),
m_debugText(QStringLiteral("使用Debug构建（未提供时拒绝运行）")),
m_missingProgramText(QStringLiteral("被测主程序或对应测试入口不存在：")),
m_selectionMaximumLength(4096)
{
    init();
}

void SplitViewerTestConfig::init()
{
    const QDir directory(QCoreApplication::applicationDirPath());
    QString productDirectory = directory.absolutePath();
#ifdef Q_OS_WIN
    const QString suffix = QStringLiteral(".exe");
#else
    const QString suffix;
#endif
    if (!QFileInfo(directory.filePath(QStringLiteral("SplitViewer1.0") + suffix)).isFile())
    {
        productDirectory = directory.absoluteFilePath(QStringLiteral("../SplitViewer"));
    }
    m_releaseProgramPath = QDir(productDirectory).absoluteFilePath(QStringLiteral("SplitViewer1.0") + suffix);
    m_debugProgramPath = QDir(productDirectory).absoluteFilePath(QStringLiteral("SplitViewerd") + suffix);
    m_releaseTestPath = directory.absoluteFilePath(QStringLiteral("SplitViewerTest1.0") + suffix);
    m_debugTestPath = directory.absoluteFilePath(QStringLiteral("SplitViewerTestd") + suffix);
}
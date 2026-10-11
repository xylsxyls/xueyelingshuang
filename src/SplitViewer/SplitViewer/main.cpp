#include "SplitViewer.h"
#include "SplitViewerTaskLifecycleManager.h"
#include "SplitViewerDialogHelper.h"
#include "Config.h"
#include "SplitViewerPlatform.h"
#include "SplitViewerDialogSession.h"
#include "DialogManager/DialogManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <QtWidgets/QApplication>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtGui/QIcon>
#ifdef Q_OS_WIN
#include "CDump/CDumpAPI.h"
#endif

/** 创建桌面运行环境并按依赖顺序初始化，窗口和任务收敛后释放进程级资源
@param [in] argc 操作系统传入的命令行参数数量，交由QApplication处理
@param [in] argv 操作系统传入的参数数组，支持首个文件路径和诊断选项
@return 初始化失败返回1，否则返回Qt事件循环的实际退出码
*/
int main(int argc, char* argv[])
{
#ifdef Q_OS_WIN
    const bool dumpReady = CDump::declareDumpFile();
#else
    const bool dumpReady = false;
#endif
    QApplication app(argc, argv);
    Config::instance();
    if (!g_config.m_coreConfig.isValid() || !SplitViewerTaskLifecycleManager::instance().init())
    {
        return 1;
    }
    app.setWindowIcon(QIcon(g_config.m_iconPath));
    LogManagerConfig config;
    config.m_outputConsole = g_config.m_logConsoleEnabled;
    config.m_maxFileBytes = g_config.m_logMaximumBytes;
    config.m_maxFileCount = g_config.m_logMaximumCount;
    LogManager::instance().set(g_config.m_logEnabled, false);
    LogManager::instance().init(config);
    DialogManager::setLogCallback(SplitViewerDialogHelper::forwardLog);
    const QStringList arguments = app.arguments();
    const bool debug = arguments.contains(g_config.m_debugArgument, Qt::CaseInsensitive);
    app.setProperty("debug", debug);
    LOGINFO("SplitViewer 1.0 started; debug=%d dump=%d", debug, dumpReady);
    const bool association = SplitViewerRegisterSvFileAssociation(app.applicationFilePath());
    LOGINFO("File association registration=%d", association);
    int result = 0;
    {
        SplitViewerDialogSession dialogs;
        if (!dialogs.init())
        {
            DialogManager::instance().uninit();
            DialogManager::releaseInstance();
            DialogManager::setLogCallback(nullptr);
            SplitViewerTaskLifecycleManager::instance().finish();
            LogManager::instance().uninitAll();
            return 1;
        }
        SplitViewer window;
        for (int i = 1; i < arguments.size(); ++i)
        {
            if (arguments.at(i).compare(g_config.m_debugArgument, Qt::CaseInsensitive) != 0)
            {
                window.canvasFileDropped(arguments.at(i));
                break;
            }
        }
        window.show();
        result = app.exec();
    }
    SplitViewerTaskLifecycleManager::instance().finish();
    DialogManager::instance().uninit();
    DialogManager::releaseInstance();
    DialogManager::setLogCallback(nullptr);
    LOGINFO("SplitViewer stopped; result=%d", result);
    LogManager::instance().uninitAll();
    return result;
}
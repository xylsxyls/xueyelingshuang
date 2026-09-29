#include "SplitViewer.h"
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


int main(int argc, char* argv[])
{
#ifdef Q_OS_WIN
    const bool dumpReady=CDump::declareDumpFile();
#else
    const bool dumpReady=false;
#endif
    QApplication app(argc,argv);
    Config::instance();
    app.setWindowIcon(QIcon(g_config.m_iconPath));
    LogManagerConfig config;
    config.m_outputConsole=g_config.m_logConsoleEnabled;
    config.m_maxFileBytes=g_config.m_logMaximumBytes;
    config.m_maxFileCount=g_config.m_logMaximumCount;
    LogManager::instance().set(g_config.m_logEnabled,false);
    LogManager::instance().init(config);
    DialogManager::setLogCallback(SplitViewerDialogHelper::forwardLog);
    const QStringList arguments=app.arguments();
    const bool debug=arguments.contains(g_config.m_debugArgument,Qt::CaseInsensitive);
    app.setProperty("debug",debug);
    LOGINFO("SplitViewer 1.0 started; debug=%d dump=%d",debug,dumpReady);
    const bool association=SplitViewerRegisterSvFileAssociation(app.applicationFilePath());
    LOGINFO("File association registration=%d",association);
    int result=0;
    {
        SplitViewerDialogSession dialogs;
        SplitViewer window;
        for (int i=1;i<arguments.size();++i)
        {
            if (arguments.at(i).compare(g_config.m_debugArgument,Qt::CaseInsensitive)!=0)
            {
                window.canvasFileDropped(arguments.at(i));
                break;
            }
        }
        window.show();
        result=app.exec();
    }
    DialogManager::setLogCallback(nullptr);
    LOGINFO("SplitViewer stopped; result=%d",result);
    LogManager::instance().uninitAll();
    return result;
}
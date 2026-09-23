#include "SplitViewer.h"
#include "SplitViewerPlatform.h"
#include "SplitViewerDialogSession.h"
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
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/SplitViewer.png")));
    LogManagerConfig config;
    config.m_outputConsole=false;
    config.m_maxFileBytes=20LL*1024LL*1024LL;
    config.m_maxFileCount=5;
    LogManager::instance().set(true,false);
    LogManager::instance().init(config);
    const QStringList arguments=app.arguments();
    const bool debug=arguments.contains(QStringLiteral("debug"),Qt::CaseInsensitive);
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
            if (arguments.at(i).compare(QStringLiteral("debug"),Qt::CaseInsensitive)!=0)
            {
                window.canvasFileDropped(arguments.at(i));
                break;
            }
        }
        window.show();
        result=app.exec();
    }
    LOGINFO("SplitViewer stopped; result=%d",result);
    LogManager::instance().uninitAll();
    return result;
}
#include "PdfReaderTaskManager.h"
#include "PdfReader.h"
#include "Config.h"
#include "PdfReaderDialogRuntime.h"
#include "CDump/CDumpAPI.h"
#include "DialogManager/DialogManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <QApplication>
#include <QTimer>
#include <exception>

/** 将DialogManager日志交给PdfReader的LogManager
@param [in] level DialogManager日志级别
@param [in] message 完整日志消息
*/
static void ForwardDialogLog(DialogLogLevel level, const char* message)
{
    if (message == nullptr)
    {
        return;
    }
    switch (level)
    {
    case DIALOG_LOG_ERROR:
        LOGERROR("%s", message);
        break;
    case DIALOG_LOG_WARNING:
        LOGWARNING("%s", message);
        break;
    default:
        LOGINFO("%s", message);
        break;
    }
}

int main(int argc, char* argv[])
{
    const bool dumpEnabled = CDump::declareDumpFile();
    bool logInitialized = false;
    int result = -1;
    try
    {
        QApplication app(argc, argv);

        Config::instance();
        LogManager::instance().init(g_config.m_log);
        DialogManager::setLogCallback(ForwardDialogLog);
        logInitialized = true;
        LOGINFO("PdfReader startup, build=%s %s", __DATE__, __TIME__);
        LOGINFO("CDump registration result=%d", dumpEnabled ? 1 : 0);
        PdfReaderTaskManager::instance().init();
        PdfReaderDialogRuntime dialogs;
        PdfReader window(nullptr);
        window.show();
        if (app.arguments().size() > 1)
        {
            const QString startupPath = app.arguments().at(1);
            QTimer::singleShot(0, [&window, startupPath]() { window.openFile(startupPath); });
        }
        result = app.exec();
    }
    catch (const std::exception& exception)
    {
        LOGERROR("Unhandled std::exception in main: %s", exception.what());
        result = -2;
    }
    catch (...)
    {
        LOGERROR("Unhandled unknown exception in main");
        result = -3;
    }
    PdfReaderTaskManager::instance().finish();
    DialogManager::setLogCallback(nullptr);
    if (logInitialized)
    {
        LOGINFO("PdfReader log closing, exitCode=%d", result);
        LogManager::instance().uninit(0);
    }
    return result;
}
#include "PdfReader.h"
#include "Config.h"
#include "PdfReaderDialogRuntime.h"
#include "CDump/CDumpAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <QApplication>
#include <QTimer>
#include <exception>

int main(int argc, char* argv[])
{
    const bool dumpEnabled = CDump::declareDumpFile();
    bool logInitialized = false;
    int result = -1;
    try
    {
        QApplication app(argc, argv);
        Config config;
        LogManager::instance().init(config.log);
        logInitialized = true;
        LOGINFO("PdfReader startup, build=%s %s", __DATE__, __TIME__);
        LOGINFO("CDump registration result=%d", dumpEnabled ? 1 : 0);
        PdfReaderDialogRuntime dialogs;
        PdfReader window(nullptr, config);
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
    if (logInitialized)
    {
        LOGINFO("PdfReader log closing, exitCode=%d", result);
        LogManager::instance().uninit(0);
    }
    return result;
}
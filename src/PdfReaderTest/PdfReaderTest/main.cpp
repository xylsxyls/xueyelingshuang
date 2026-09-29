#include "../../PdfReader/PdfReader/PdfReaderLogHelper.h"
#include "PdfReaderTest.h"
#include "../../PdfReader/PdfReader/Config.h"
#include "PdfReaderRegression.h"
#include "../../PdfReader/PdfReader/PdfReaderDialogRuntime.h"
#include "../../PdfReader/PdfReader/PdfReaderTaskManager.h"
#include "DialogManager/DialogManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <QtWidgets/QApplication>
#include <exception>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    Config::instance();
    LogManager::instance().init(g_config.m_log);
    DialogManager::setLogCallback(PdfReaderLogHelper::forward);
    const int32_t initialThreads = CTaskThreadManager::Instance().Count();
    int result = 0;
    try
    {
        PdfReaderTaskManager::instance().init();
        PdfReaderDialogRuntime dialogs;
        const QStringList args = app.arguments();
        if (args.contains(QStringLiteral("--regression")))
        {
            const int at = args.indexOf(QStringLiteral("--cases"));
            const int report = args.indexOf(QStringLiteral("--reports"));
            result = PdfReaderRegression::run(at >= 0 ? args.value(at + 1) : QString(),
                report >= 0 ? args.value(report + 1) : QString());
        }
        else
        {
            PdfReaderTest window;
            window.show();
            result = app.exec();
        }
    }
    catch (...)
    {
        result = 4;
    }
    PdfReaderTaskManager::instance().finish();
    DialogManager::setLogCallback(nullptr);
    if (CTaskThreadManager::Instance().Count() != initialThreads)
    {
        LogManager::instance().uninit(0);
        return 5;
    }
    LogManager::instance().uninit(0);
    return result;
}
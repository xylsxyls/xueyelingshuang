#include "PdfReaderWindow.h"
#include "PdfReaderHelper.h"
#include "PdfReaderInstance.h"

#include "CDump/CDumpAPI.h"

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int showCommand)
{
    static bool s_declareDumpFile = CDump::declareDumpFile();
    (void)s_declareDumpFile;

    bool debugLogEnabled = PdfReaderHelper::HasDebugCommandLineArgument();
    PdfReaderInstance::instance().setDebugLogEnabled(debugLogEnabled);
    if (!s_declareDumpFile)
    {
        SetUnhandledExceptionFilter(PdfReaderInstance::unhandledExceptionFilter);
    }
    PdfReaderInstance::instance().logInfo("PdfReader process start, commandLine=%s",
                       PdfReaderHelper::WideToUtf8(GetCommandLineW()).c_str());
    PdfReaderInstance::instance().logInfo("CDump declare result=%d", s_declareDumpFile ? 1 : 0);

    HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(comResult))
    {
        PdfReaderInstance::instance().logWarning("CoInitializeEx failed, hr=0x%08X", static_cast<unsigned int>(comResult));
    }
    else
    {
        PdfReaderInstance::instance().logInfo("CoInitializeEx succeeded");
    }

    PdfReaderWindow window;
    if (!window.create(instance, showCommand))
    {
        PdfReaderInstance::instance().logError("Create PdfReader window failed, lastError=%lu", GetLastError());
        MessageBoxW(nullptr, L"创建PDF阅读器窗口失败。", kAppTitle, MB_OK | MB_ICONERROR);
        if (SUCCEEDED(comResult))
        {
            CoUninitialize();
        }
        PdfReaderInstance::instance().uninitDebugLog();
        return 1;
    }

    std::wstring startupFilePath = PdfReaderHelper::FirstCommandLineFilePath();
    if (!startupFilePath.empty())
    {
        PdfReaderInstance::instance().logInfo("Startup file argument found, path=%s",
                           PdfReaderHelper::WideToUtf8(startupFilePath).c_str());
        window.openPdfFile(startupFilePath);
    }
    else
    {
        PdfReaderInstance::instance().logInfo("No startup file argument");
    }

    int result = window.runMessageLoop();
    if (SUCCEEDED(comResult))
    {
        CoUninitialize();
        PdfReaderInstance::instance().logInfo("CoUninitialize called");
    }
    PdfReaderInstance::instance().logInfo("PdfReader process exit, code=%d", result);
    PdfReaderInstance::instance().uninitDebugLog();
    return result;
}

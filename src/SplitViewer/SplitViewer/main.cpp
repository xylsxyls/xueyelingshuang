#include "Logger.h"
#include "SplitViewerWindow.h"

#include "CDump/CDumpAPI.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE unusedInstance, LPWSTR commandLine, int cmdShow)
{
    UNREFERENCED_PARAMETER(unusedInstance);
    UNREFERENCED_PARAMETER(commandLine);

    bool debugLogging = false;
    std::wstring startupPath;

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv)
    {
        for (int i = 1; i < argc; ++i)
        {
            if (SplitViewerSameText(argv[i], L"debug"))
            {
                debugLogging = true;
            }
            else if (startupPath.empty())
            {
                startupPath = argv[i];
            }
        }
        LocalFree(argv);
    }

    static bool s_declareDumpFile = CDump::declareDumpFile();

    SplitViewerSetDebugLoggingEnabled(debugLogging);
    SplitViewerDebugLogFormat(L"SplitViewer started debug=%d dumpDeclared=%d startupPath=%s.",
        debugLogging ? 1 : 0,
        s_declareDumpFile ? 1 : 0,
        startupPath.c_str());

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        SplitViewerDebugLogFormat(L"COM initialization failed: 0x%08X.", static_cast<unsigned int>(hr));
        SplitViewerSetDebugLoggingEnabled(false);
        MessageBoxW(NULL, L"COM \u521D\u59CB\u5316\u5931\u8D25\u3002", kSplitViewerAppTitle, MB_ICONERROR | MB_OK);
        return 1;
    }

    Gdiplus::GdiplusStartupInput gdiplusInput;
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, NULL) != Gdiplus::Ok)
    {
        SplitViewerDebugLog(L"GDI+ initialization failed.");
        SplitViewerSetDebugLoggingEnabled(false);
        CoUninitialize();
        MessageBoxW(NULL, L"GDI+ \u521D\u59CB\u5316\u5931\u8D25\u3002", kSplitViewerAppTitle, MB_ICONERROR | MB_OK);
        return 1;
    }

    INITCOMMONCONTROLSEX icc = { 0 };
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);
    SplitViewerRegisterSvFileType();

    SplitViewerWindow window;
    if (!window.Create(hInstance, cmdShow))
    {
        SplitViewerDebugLog(L"Window creation failed.");
        Gdiplus::GdiplusShutdown(gdiplusToken);
        CoUninitialize();
        SplitViewerSetDebugLoggingEnabled(false);
        return 1;
    }

    if (!startupPath.empty())
    {
        SplitViewerDebugLogFormat(L"Loading startup path: %s", startupPath.c_str());
        window.LoadStartupPath(startupPath.c_str());
    }

    MSG msg = { 0 };
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    CoUninitialize();
    SplitViewerDebugLog(L"SplitViewer exiting.");
    SplitViewerSetDebugLoggingEnabled(false);
    return static_cast<int>(msg.wParam);
}
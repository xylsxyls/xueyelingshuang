#include "Logger.h"
#include "SplitViewerWindow.h"

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
            if (SplitViewer::SameText(argv[i], L"debug"))
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

    SplitViewer::SetDebugLoggingEnabled(debugLogging);
    SplitViewer::DebugLog(L"SplitViewer started.");

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        SplitViewer::DebugLogFormat(L"COM initialization failed: 0x%08X.", static_cast<unsigned int>(hr));
        SplitViewer::SetDebugLoggingEnabled(false);
        MessageBoxW(NULL, L"COM \u521D\u59CB\u5316\u5931\u8D25\u3002", SplitViewer::kAppTitle, MB_ICONERROR | MB_OK);
        return 1;
    }

    Gdiplus::GdiplusStartupInput gdiplusInput;
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, NULL) != Gdiplus::Ok)
    {
        SplitViewer::DebugLog(L"GDI+ initialization failed.");
        SplitViewer::SetDebugLoggingEnabled(false);
        CoUninitialize();
        MessageBoxW(NULL, L"GDI+ \u521D\u59CB\u5316\u5931\u8D25\u3002", SplitViewer::kAppTitle, MB_ICONERROR | MB_OK);
        return 1;
    }

    INITCOMMONCONTROLSEX icc = { 0 };
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);
    SplitViewer::RegisterSvFileType();

    SplitViewer::SplitViewerWindow window;
    if (!window.Create(hInstance, cmdShow))
    {
        SplitViewer::DebugLog(L"Window creation failed.");
        Gdiplus::GdiplusShutdown(gdiplusToken);
        CoUninitialize();
        SplitViewer::SetDebugLoggingEnabled(false);
        return 1;
    }

    if (!startupPath.empty())
    {
        SplitViewer::DebugLogFormat(L"Loading startup path: %s", startupPath.c_str());
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
    SplitViewer::DebugLog(L"SplitViewer exiting.");
    SplitViewer::SetDebugLoggingEnabled(false);
    return static_cast<int>(msg.wParam);
}

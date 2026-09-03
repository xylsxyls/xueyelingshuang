#include "LoopPlayerPlatform.h"
#include "LoopPlayerConstants.h"
#include "LoopPlayerUtils.h"
#include "PlayerWindow.h"
#include "CDump/CDumpAPI.h"

using namespace LoopPlayer;

/** 判断命令行参数是否表示启用debug日志
@param [in] arg 命令行参数
@return 是debug参数返回true，否则返回false
*/
static bool IsDebugArgument(const wchar_t* arg)
{
    if (arg == nullptr)
    {
        return false;
    }

    return lstrcmpiW(arg, L"debug") == 0 ||
           lstrcmpiW(arg, L"-debug") == 0 ||
           lstrcmpiW(arg, L"/debug") == 0 ||
           lstrcmpiW(arg, L"--debug") == 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int cmdShow)
{
    int argc = 0;
    bool debugLogEnabled = false;
    std::wstring initialFilePath;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv)
    {
        for (int i = 1; i < argc; ++i)
        {
            if (IsDebugArgument(argv[i]))
            {
                debugLogEnabled = true;
            }
            else if (initialFilePath.empty())
            {
                initialFilePath = argv[i];
            }
        }
        LocalFree(argv);
    }

    SetLoggingEnabled(debugLogEnabled);
    ResetLogFile();
    const bool dumpEnabled = CDump::declareDumpFile();
    Logf(L"================ LoopPlayer process start ================");
    Logf(L"Command line: %s", GetCommandLineW());
    Logf(L"Debug logging enabled: %d, log files: %s", debugLogEnabled ? 1 : 0, GetLogFilePath().c_str());
    Logf(L"CDump static library enabled: %d", dumpEnabled ? 1 : 0);

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        Logf(L"CoInitializeEx failed: 0x%08X", static_cast<unsigned int>(hr));
        MessageBoxW(nullptr, L"COM 初始化失败。", kAppTitle, MB_ICONERROR | MB_OK);
        ShutdownLog();
        return 1;
    }
    Logf(L"CoInitializeEx succeeded");

    hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
        Logf(L"MFStartup failed: 0x%08X", static_cast<unsigned int>(hr));
        CoUninitialize();
        MessageBoxW(nullptr, L"Media Foundation 初始化失败。", kAppTitle, MB_ICONERROR | MB_OK);
        ShutdownLog();
        return 1;
    }
    Logf(L"MFStartup succeeded");

    INITCOMMONCONTROLSEX icc = { 0 };
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    int exitCode = 0;
    {
        PlayerWindow window;
        if (!window.Create(hInstance, cmdShow))
        {
            MFShutdown();
            CoUninitialize();
            Logf(L"Process exit: create window failed");
            ShutdownLog();
            return 1;
        }

        if (!initialFilePath.empty())
        {
            window.LoadFile(initialFilePath.c_str());
        }

        MSG msg = { 0 };
        while (GetMessageW(&msg, nullptr, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        exitCode = static_cast<int>(msg.wParam);
        Logf(L"Message loop ended: code=%d", exitCode);
    }

    MFShutdown();
    CoUninitialize();
    Logf(L"Process exit: code=%d", exitCode);
    ShutdownLog();
    return exitCode;
}
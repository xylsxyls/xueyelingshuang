#include "LoopPlayerPlatform.h"
#include "LoopPlayerConstants.h"
#include "LoopPlayerUtils.h"
#include "PlayerWindow.h"

using namespace LoopPlayer;

static bool IsDebugArgument(const wchar_t* arg)
{
    if (!arg)
    {
        return false;
    }

    return lstrcmpiW(arg, L"debug") == 0 ||
           lstrcmpiW(arg, L"-debug") == 0 ||
           lstrcmpiW(arg, L"/debug") == 0 ||
           lstrcmpiW(arg, L"--debug") == 0;
}

// 进程异常兜底日志，便于定位未走正常退出流程的崩溃。
static LONG WINAPI LoopPlayerUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
    DWORD code = 0;
    void* address = NULL;
    if (exceptionInfo && exceptionInfo->ExceptionRecord)
    {
        code = exceptionInfo->ExceptionRecord->ExceptionCode;
        address = exceptionInfo->ExceptionRecord->ExceptionAddress;
    }

    Logf(L"Unhandled exception: code=0x%08X, address=%p", code, address);
    return EXCEPTION_EXECUTE_HANDLER;
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
    SetUnhandledExceptionFilter(LoopPlayerUnhandledExceptionFilter);
    ResetLogFile();
    Logf(L"================ LoopPlayer process start ================");
    Logf(L"Command line: %s", GetCommandLineW());

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        Logf(L"CoInitializeEx failed: 0x%08X", static_cast<unsigned int>(hr));
        MessageBoxW(NULL, L"COM 初始化失败。", kAppTitle, MB_ICONERROR | MB_OK);
        return 1;
    }
    Logf(L"CoInitializeEx succeeded");

    hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
        Logf(L"MFStartup failed: 0x%08X", static_cast<unsigned int>(hr));
        CoUninitialize();
        MessageBoxW(NULL, L"Media Foundation 初始化失败。", kAppTitle, MB_ICONERROR | MB_OK);
        return 1;
    }
    Logf(L"MFStartup succeeded");

    INITCOMMONCONTROLSEX icc = { 0 };
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    PlayerWindow window;
    if (!window.Create(hInstance, cmdShow))
    {
        MFShutdown();
        CoUninitialize();
        return 1;
    }

    if (!initialFilePath.empty())
    {
        window.LoadFile(initialFilePath.c_str());
    }

    MSG msg = { 0 };
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    MFShutdown();
    CoUninitialize();
    Logf(L"Process exit: code=%d", static_cast<int>(msg.wParam));
    return static_cast<int>(msg.wParam);
}

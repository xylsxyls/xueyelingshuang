#include "Logger.h"

#include <cstdarg>

namespace SplitViewer
{
    namespace
    {
        bool g_debugLoggingEnabled = false;
        HANDLE g_logFile = INVALID_HANDLE_VALUE;

        void CloseLogFile()
        {
            if (g_logFile != INVALID_HANDLE_VALUE)
            {
                CloseHandle(g_logFile);
                g_logFile = INVALID_HANDLE_VALUE;
            }
        }

        std::wstring GetLogPath()
        {
            wchar_t exePath[MAX_PATH] = { 0 };
            if (!GetModuleFileNameW(NULL, exePath, ARRAYSIZE(exePath)))
            {
                return L"SplitViewer.log";
            }

            PathRemoveFileSpecW(exePath);
            std::wstring path = exePath;
            if (!path.empty() && path[path.size() - 1] != L'\\')
            {
                path += L"\\";
            }
            path += L"SplitViewer.log";
            return path;
        }

        bool OpenLogFile()
        {
            if (g_logFile != INVALID_HANDLE_VALUE)
            {
                return true;
            }

            const std::wstring path = GetLogPath();
            g_logFile = CreateFileW(path.c_str(),
                                    FILE_APPEND_DATA,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
            if (g_logFile == INVALID_HANDLE_VALUE)
            {
                return false;
            }

            LARGE_INTEGER size = { 0 };
            if (GetFileSizeEx(g_logFile, &size) && size.QuadPart == 0)
            {
                const WORD bom = 0xFEFF;
                DWORD written = 0;
                WriteFile(g_logFile, &bom, sizeof(bom), &written, NULL);
            }
            return true;
        }

        void WriteLogLine(const std::wstring& line)
        {
            if (!g_debugLoggingEnabled || !OpenLogFile())
            {
                return;
            }

            DWORD written = 0;
            WriteFile(g_logFile,
                      line.c_str(),
                      static_cast<DWORD>(line.size() * sizeof(wchar_t)),
                      &written,
                      NULL);
            OutputDebugStringW(line.c_str());
        }
    }

    void SetDebugLoggingEnabled(bool enabled)
    {
        if (g_debugLoggingEnabled == enabled)
        {
            return;
        }

        g_debugLoggingEnabled = enabled;
        if (!g_debugLoggingEnabled)
        {
            CloseLogFile();
        }
    }

    bool IsDebugLoggingEnabled()
    {
        return g_debugLoggingEnabled;
    }

    void DebugLog(const wchar_t* message)
    {
        if (!g_debugLoggingEnabled || !message)
        {
            return;
        }

        SYSTEMTIME now = { 0 };
        GetLocalTime(&now);

        wchar_t prefix[96] = { 0 };
        StringCchPrintfW(prefix,
                         ARRAYSIZE(prefix),
                         L"[%04u-%02u-%02u %02u:%02u:%02u.%03u] ",
                         now.wYear,
                         now.wMonth,
                         now.wDay,
                         now.wHour,
                         now.wMinute,
                         now.wSecond,
                         now.wMilliseconds);

        std::wstring line = prefix;
        line += message;
        line += L"\r\n";
        WriteLogLine(line);
    }

    void DebugLogFormat(const wchar_t* format, ...)
    {
        if (!g_debugLoggingEnabled || !format)
        {
            return;
        }

        wchar_t buffer[1024] = { 0 };
        va_list args;
        va_start(args, format);
        StringCchVPrintfW(buffer, ARRAYSIZE(buffer), format, args);
        va_end(args);
        DebugLog(buffer);
    }
}

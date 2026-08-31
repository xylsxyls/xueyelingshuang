#include "LoopPlayerUtils.h"
#include "LoopPlayerConstants.h"

#include <stdarg.h>

namespace LoopPlayer
{
    static bool gLoggingEnabled = false;

    std::wstring FormatTime(REFERENCE_TIME value)
    {
        if (value < 0)
        {
            return L"--:--.---";
        }

        const LONGLONG totalMs = value / 10000;
        const int ms = static_cast<int>(totalMs % 1000);
        const LONGLONG totalSeconds = totalMs / 1000;
        const int seconds = static_cast<int>(totalSeconds % 60);
        const int minutes = static_cast<int>((totalSeconds / 60) % 60);
        const LONGLONG hours = totalSeconds / 3600;

        wchar_t buffer[64] = { 0 };
        if (hours > 0)
        {
            StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"%I64d:%02d:%02d.%03d", hours, minutes, seconds, ms);
        }
        else
        {
            StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"%02d:%02d.%03d", minutes, seconds, ms);
        }

        return buffer;
    }

    std::wstring FormatClockTime(REFERENCE_TIME value)
    {
        if (value < 0)
        {
            return L"--:--:--";
        }

        const LONGLONG totalSeconds = value / ONE_SECOND;
        const int seconds = static_cast<int>(totalSeconds % 60);
        const int minutes = static_cast<int>((totalSeconds / 60) % 60);
        const LONGLONG hours = totalSeconds / 3600;

        wchar_t buffer[64] = { 0 };
        StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"%02I64d:%02d:%02d", hours, minutes, seconds);
        return buffer;
    }

    std::wstring HResultText(HRESULT hr)
    {
        wchar_t buffer[256] = { 0 };
        StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"Error 0x%08X", static_cast<unsigned int>(hr));
        return buffer;
    }

    std::wstring GetLogFilePath()
    {
        wchar_t path[MAX_PATH] = { 0 };
        DWORD length = GetModuleFileNameW(NULL, path, ARRAYSIZE(path));
        if (length == 0 || length >= ARRAYSIZE(path))
        {
            return L"LoopPlayer.log";
        }

        PathRemoveFileSpecW(path);
        PathAppendW(path, L"LoopPlayer.log");
        return path;
    }

    void SetLoggingEnabled(bool enabled)
    {
        gLoggingEnabled = enabled;
    }

    bool IsLoggingEnabled()
    {
        return gLoggingEnabled;
    }

    void ResetLogFile()
    {
        if (!gLoggingEnabled)
        {
            return;
        }

        const std::wstring logPath = GetLogFilePath();
        HANDLE file = CreateFileW(logPath.c_str(),
                                  GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
        if (file != INVALID_HANDLE_VALUE)
        {
            CloseHandle(file);
        }
    }

    static void WriteUtf8Text(HANDLE file, const wchar_t* text)
    {
        if (!text)
        {
            return;
        }

        const int byteCount = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
        if (byteCount <= 1)
        {
            return;
        }

        std::string bytes;
        bytes.resize(byteCount - 1);
        WideCharToMultiByte(CP_UTF8, 0, text, -1, &bytes[0], byteCount, NULL, NULL);

        DWORD written = 0;
        WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, NULL);
    }

    void Logf(const wchar_t* format, ...)
    {
        if (!gLoggingEnabled)
        {
            return;
        }

        wchar_t message[2048] = { 0 };

        va_list args;
        va_start(args, format);
        StringCchVPrintfW(message, ARRAYSIZE(message), format, args);
        va_end(args);

        SYSTEMTIME now = { 0 };
        GetLocalTime(&now);

        wchar_t line[2300] = { 0 };
        StringCchPrintfW(line,
                         ARRAYSIZE(line),
                         L"[%04u-%02u-%02u %02u:%02u:%02u.%03u] %s\r\n",
                         now.wYear,
                         now.wMonth,
                         now.wDay,
                         now.wHour,
                         now.wMinute,
                         now.wSecond,
                         now.wMilliseconds,
                         message);

        const std::wstring logPath = GetLogFilePath();
        HANDLE file = CreateFileW(logPath.c_str(),
                                  FILE_APPEND_DATA,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
        if (file == INVALID_HANDLE_VALUE)
        {
            return;
        }

        WriteUtf8Text(file, line);
        CloseHandle(file);
    }

    const wchar_t* MediaPlayerStateName(MFP_MEDIAPLAYER_STATE state)
    {
        switch (state)
        {
        case MFP_MEDIAPLAYER_STATE_EMPTY:
            return L"EMPTY";
        case MFP_MEDIAPLAYER_STATE_STOPPED:
            return L"STOPPED";
        case MFP_MEDIAPLAYER_STATE_PLAYING:
            return L"PLAYING";
        case MFP_MEDIAPLAYER_STATE_PAUSED:
            return L"PAUSED";
        case MFP_MEDIAPLAYER_STATE_SHUTDOWN:
            return L"SHUTDOWN";
        default:
            return L"UNKNOWN";
        }
    }

    const wchar_t* MediaPlayerEventName(MFP_EVENT_TYPE type)
    {
        switch (type)
        {
        case MFP_EVENT_TYPE_PLAY:
            return L"PLAY";
        case MFP_EVENT_TYPE_PAUSE:
            return L"PAUSE";
        case MFP_EVENT_TYPE_STOP:
            return L"STOP";
        case MFP_EVENT_TYPE_POSITION_SET:
            return L"POSITION_SET";
        case MFP_EVENT_TYPE_RATE_SET:
            return L"RATE_SET";
        case MFP_EVENT_TYPE_MEDIAITEM_CREATED:
            return L"MEDIAITEM_CREATED";
        case MFP_EVENT_TYPE_MEDIAITEM_SET:
            return L"MEDIAITEM_SET";
        case MFP_EVENT_TYPE_FRAME_STEP:
            return L"FRAME_STEP";
        case MFP_EVENT_TYPE_MEDIAITEM_CLEARED:
            return L"MEDIAITEM_CLEARED";
        case MFP_EVENT_TYPE_MF:
            return L"MF";
        case MFP_EVENT_TYPE_ERROR:
            return L"ERROR";
        case MFP_EVENT_TYPE_PLAYBACK_ENDED:
            return L"PLAYBACK_ENDED";
        case MFP_EVENT_TYPE_ACQUIRE_USER_CREDENTIAL:
            return L"ACQUIRE_USER_CREDENTIAL";
        default:
            return L"UNKNOWN";
        }
    }

    void InitInt64PropVariant(PROPVARIANT& value, LONGLONG number)
    {
        PropVariantInit(&value);
        value.vt = VT_I8;
        value.hVal.QuadPart = number;
    }

    bool ReadInt64PropVariant(const PROPVARIANT& value, LONGLONG& number)
    {
        if (value.vt == VT_I8)
        {
            number = value.hVal.QuadPart;
            return true;
        }
        if (value.vt == VT_UI8)
        {
            number = static_cast<LONGLONG>(value.uhVal.QuadPart);
            return true;
        }
        return false;
    }
}

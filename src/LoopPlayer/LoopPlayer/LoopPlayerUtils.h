#pragma once

#include "LoopPlayerPlatform.h"

#include <string>

namespace LoopPlayer
{
    template <class T>
    void SafeRelease(T*& ptr)
    {
        if (ptr)
        {
            ptr->Release();
            ptr = NULL;
        }
    }

    std::wstring FormatTime(REFERENCE_TIME value);
    std::wstring FormatClockTime(REFERENCE_TIME value);
    std::wstring HResultText(HRESULT hr);
    std::wstring GetLogFilePath();
    void SetLoggingEnabled(bool enabled);
    bool IsLoggingEnabled();
    void ResetLogFile();
    void Logf(const wchar_t* format, ...);
    const wchar_t* MediaPlayerStateName(MFP_MEDIAPLAYER_STATE state);
    const wchar_t* MediaPlayerEventName(MFP_EVENT_TYPE type);
    void InitInt64PropVariant(PROPVARIANT& value, LONGLONG number);
    bool ReadInt64PropVariant(const PROPVARIANT& value, LONGLONG& number);
}

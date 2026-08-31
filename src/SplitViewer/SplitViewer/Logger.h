#pragma once

#include "SplitViewerCommon.h"

namespace SplitViewer
{
    void SetDebugLoggingEnabled(bool enabled);

    bool IsDebugLoggingEnabled();

    void DebugLog(const wchar_t* message);

    void DebugLogFormat(const wchar_t* format, ...);
}

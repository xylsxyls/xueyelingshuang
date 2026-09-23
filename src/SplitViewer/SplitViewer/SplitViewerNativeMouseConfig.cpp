#include "SplitViewerNativeMouseConfig.h"
#ifdef Q_OS_WIN
SplitViewerNativeMouseConfig::SplitViewerNativeMouseConfig() : clickGeneration(0), mouseHook(nullptr)
{
}
SplitViewerNativeMouseConfig::~SplitViewerNativeMouseConfig()
{
    if (mouseHook) UnhookWindowsHookEx(mouseHook);
}
SplitViewerNativeMouseConfig& SplitViewerNativeMouseConfig::instance()
{
    static SplitViewerNativeMouseConfig config;
    return config;
}
#endif
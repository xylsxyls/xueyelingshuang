#include "SplitViewerNativeMouseManager.h"
#ifdef Q_OS_WIN
SplitViewerNativeMouseManager::SplitViewerNativeMouseManager() : m_clickGeneration(0), m_mouseHook(nullptr)
{

}
SplitViewerNativeMouseManager::~SplitViewerNativeMouseManager()
{
    if (m_mouseHook)
    {
        UnhookWindowsHookEx(m_mouseHook);
    }
}
SplitViewerNativeMouseManager& SplitViewerNativeMouseManager::instance()
{
    static SplitViewerNativeMouseManager s_manager;
    return s_manager;
}
#endif
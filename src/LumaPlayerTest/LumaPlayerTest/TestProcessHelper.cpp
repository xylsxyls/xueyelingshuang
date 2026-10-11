#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "TestProcessHelper.h"
#ifdef _WIN32
#include <Windows.h>

/** 查找指定测试进程的可见窗口
@param [in] window 枚举得到的原生窗口句柄
@param [in] value TestWindowQuery指针转为LPARAM的值
@return 找到目标时返回FALSE停止枚举，否则返回TRUE
*/
static BOOL CALLBACK VisitTestWindow(HWND window, LPARAM value)
{
    TestWindowQuery* query = reinterpret_cast<TestWindowQuery*>(value);
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    if (static_cast<int64_t>(pid) == query->m_pid && IsWindowVisible(window) && GetWindow(window, GW_OWNER) == nullptr)
    {
        query->m_found = true;
        if (query->m_close)
        {
            PostMessage(window, WM_CLOSE, 0, 0);
        }
    }
    return TRUE;
}
#endif

bool TestProcessHelper::hasWindow(int64_t pid)
{
#ifdef _WIN32
    TestWindowQuery query = {pid, false, false};
    EnumWindows(VisitTestWindow, reinterpret_cast<LPARAM>(&query));
    return query.m_found;
#else
    (void)pid;
    return false;
#endif
}

bool TestProcessHelper::closeWindow(int64_t pid)
{
#ifdef _WIN32
    TestWindowQuery query = {pid, true, false};
    EnumWindows(VisitTestWindow, reinterpret_cast<LPARAM>(&query));
    return query.m_found;
#else
    (void)pid;
    return false;
#endif
}
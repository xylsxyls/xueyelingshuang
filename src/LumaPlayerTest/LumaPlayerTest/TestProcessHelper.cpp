#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "TestProcessHelper.h"
#ifdef _WIN32
#include <Windows.h>

/** 只匹配调用者已核实的测试进程窗口
@param [in] window 枚举窗口
@param [in] value 同步借用查询参数
@return TRUE继续枚举
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
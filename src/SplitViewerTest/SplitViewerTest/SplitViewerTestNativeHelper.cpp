#include "SplitViewerTestNativeHelper.h"
#include "../../SplitViewer/SplitViewer/SplitViewerPlatform.h"
#include <QtTest/QtTest>

#ifdef Q_OS_WIN
LRESULT CALLBACK SplitViewerTestNativeHelper::splitViewerNativeFixtureProc(
    HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND video = static_cast<HWND>(GetPropW(hwnd, L"SplitViewerVideoSurface"));
    if (message == WM_CREATE)
    {
        video = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"video surface", WS_CHILD | WS_VISIBLE | SS_CENTER, 0, 0,
            1, 1, hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        SetPropW(hwnd, L"SplitViewerVideoSurface", video);
        return 0;
    }
    if (message == WM_SIZE)
    {
        if (video)
        {
            MoveWindow(video, 0, 0, (std::max)(1, static_cast<int>(LOWORD(lParam))),
                (std::max)(1, static_cast<int>(HIWORD(lParam))), TRUE);
        }
        return 0;
    }
    if (message == WM_LBUTTONDBLCLK)
    {
        // 模拟LumaPlayer自身的双击全屏：若SplitViewer没有先拦截，
        // 这个窗口会脱离容器，回归用例即可捕获该回归。
        SetParent(hwnd, nullptr);
        ShowWindow(hwnd, SW_MAXIMIZE);
        return 0;
    }
    if (message == WM_NCDESTROY)
    {
        if (video)
        {
            RemovePropW(hwnd, L"SplitViewerVideoSurface");
            DestroyWindow(video);
        }
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool SplitViewerTestNativeHelper::splitViewerNativeVideoFills(HWND parent)
{
    const HWND video = static_cast<HWND>(GetPropW(parent, L"SplitViewerVideoSurface"));
    if (!IsWindow(parent) || !IsWindow(video))
    {
        return false;
    }
    RECT parentClient = {0};
    RECT videoRect = {0};
    if (!GetClientRect(parent, &parentClient) || !GetWindowRect(video, &videoRect))
    {
        return false;
    }
    POINT points[2] = {{videoRect.left, videoRect.top}, {videoRect.right, videoRect.bottom}};
    MapWindowPoints(nullptr, parent, points, 2);
    return points[0].x == parentClient.left && points[0].y == parentClient.top && points[1].x == parentClient.right &&
        points[1].y == parentClient.bottom;
}

bool SplitViewerTestNativeHelper::nativeDrag(const QPoint& from, const QPoint& to)
{
    SetCursorPos(from.x(), from.y());
    QTest::qWait(20);
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    const bool down = SendInput(1, &input, sizeof(input)) == 1;
    QTest::qWait(20);
    SetCursorPos(to.x(), to.y());
    QTest::qWait(20);
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    const bool up = SendInput(1, &input, sizeof(input)) == 1;
    QTest::qWait(80);
    return down && up;
}

bool SplitViewerTestNativeHelper::nativeDoubleClick(const QPoint& point)
{
    SetCursorPos(point.x(), point.y());
    for (int i = 0; i < 2; ++i)
    {
        INPUT input = {0};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        const bool down = SendInput(1, &input, sizeof(input)) == 1;
        QTest::qWait(70);
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        const bool up = SendInput(1, &input, sizeof(input)) == 1;
        if (!down || !up)
        {
            return false;
        }
        if (i == 0)
        {
            QTest::qWait(80);
        }
    }
    QTest::qWait(100);
    return true;
}

bool SplitViewerTestNativeHelper::nativePlayerClick(HWND player, const QPoint& point)
{
    const POINT screenPoint = {point.x(), point.y()};
    HWND target = WindowFromPoint(screenPoint);
    while (target && target != player)
    {
        target = GetParent(target);
    }
    if (target != player)
    {
        return false;
    }
    SetCursorPos(point.x(), point.y());
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    const bool down = SendInput(1, &input, sizeof(input)) == 1;
    QTest::qWait(30);
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    const bool up = SendInput(1, &input, sizeof(input)) == 1;
    // SendInput queues the release. Pump it before a caller teleports the
    // cursor with SetCursorPos, which can otherwise overtake that release
    // and make the low-level hook observe a drag instead of a single click.
    QTest::qWait(30);
    return up && down;
}
#endif
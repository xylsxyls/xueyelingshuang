#include "SplitViewerNativeMouseHelper.h"
#include "Config.h"
#include "SplitViewerNativeMouseManager.h"
#include "CSystem/CSystemAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <algorithm>
#include <cmath>
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>

#ifdef Q_OS_WIN
void SplitViewerNativeMouseHelper::replayClick(HWND embedded, bool downOnly)
{
    if (!SplitViewerNativeMouseManager::instance().m_pendingClicks.contains(embedded))
    {
        return;
    }
    const SplitViewerNativeClick click = SplitViewerNativeMouseManager::instance().m_pendingClicks.take(embedded);
    const HWND target = reinterpret_cast<HWND>(click.target);
    if (!SplitViewerNativeMouseManager::instance().m_embeddedWindows.contains(embedded) || !IsWindow(target))
    {
        return;
    }
    const LPARAM position = MAKELPARAM(click.clientPoint.x(),click.clientPoint.y());
    if (!downOnly)
    {
        // Keep a completed click in the target's input queue. A synchronous
        // press can establish capture before the release is sent and insert
        // a cursor-position move between the two deferred Qt mouse events.
        if (!PostMessageW(target, WM_LBUTTONDOWN, MK_LBUTTON|click.modifiers, position))
        {
            LOGERROR("Native click press queue failed; target=%p error=%lu", target, GetLastError());
            return;
        }
        if (!PostMessageW(target, WM_LBUTTONUP, click.modifiers, position))
        {
            LOGERROR("Native click release queue failed; target=%p error=%lu", target, GetLastError());
            DWORD_PTR result = 0;
            SendMessageTimeoutW(target, WM_LBUTTONUP, click.modifiers, position,
                SMTO_ABORTIFHUNG | SMTO_BLOCK, g_config.m_nativeClickTimeoutMs, &result);
        }
        return;
    }
    // An ongoing physical drag needs capture before its next native move.
    // Bound the wait so an unresponsive player cannot block the host UI.
    DWORD_PTR result = 0;
    const UINT flags = SMTO_ABORTIFHUNG | SMTO_BLOCK;
    SetLastError(ERROR_SUCCESS);
    if (!SendMessageTimeoutW(target, WM_LBUTTONDOWN, MK_LBUTTON|click.modifiers,
        position, flags, g_config.m_nativeClickTimeoutMs, &result))
    {
        LOGERROR("Native click press failed; target=%p error=%lu downOnly=%d", target, GetLastError(), downOnly);
    }
}

#endif
#ifdef Q_OS_WIN
HWND SplitViewerNativeMouseHelper::embeddedAncestor(HWND window)
{
    HWND current=window;
    while (current)
    {
        if (SplitViewerNativeMouseManager::instance().m_embeddedWindows.contains(current))
        {
            return current;
        }
        current=GetParent(current);
    }
    return nullptr;
}

#endif
#ifdef Q_OS_WIN
LRESULT CALLBACK SplitViewerNativeMouseHelper::mouseHook(int code,WPARAM message,LPARAM data)
{
    bool suppress=false;
    if (code==HC_ACTION && (message==WM_LBUTTONDOWN || message==WM_MOUSEMOVE || message==WM_LBUTTONUP))
    {
        const MSLLHOOKSTRUCT* mouse=reinterpret_cast<const MSLLHOOKSTRUCT*>(data);
        const QPoint point(mouse->pt.x,mouse->pt.y);
        const HWND sourceWindow=WindowFromPoint(mouse->pt);
        const HWND embedded=SplitViewerNativeMouseHelper::embeddedAncestor(sourceWindow);
        HWND window=sourceWindow;
        window=window ? GetAncestor(window,GA_ROOT) : nullptr;
        const WId id=reinterpret_cast<WId>(window);
        const int event=message==WM_LBUTTONDOWN ? 1 : message==WM_MOUSEMOVE ? 2 : 3;
        if (embedded && message==WM_LBUTTONDOWN)
        {
            const DWORD now = CSystem::GetTickCount();
            const DWORD lastTick=SplitViewerNativeMouseManager::instance().m_lastClickTicks.value(embedded,0);
            const QPoint lastPoint=SplitViewerNativeMouseManager::instance().m_lastClickPoints.value(embedded);
            const int doubleClickWidth=(std::max)(1,GetSystemMetrics(SM_CXDOUBLECLK));
            const int doubleClickHeight=(std::max)(1,GetSystemMetrics(SM_CYDOUBLECLK));
            if (lastTick!=0 && now-lastTick<=GetDoubleClickTime() &&
                std::abs(point.x()-lastPoint.x())<=doubleClickWidth &&
                std::abs(point.y()-lastPoint.y())<=doubleClickHeight)
            {
                SplitViewerNativeMouseManager::instance().m_pendingClicks.remove(embedded);
                SplitViewerNativeMouseManager::instance().m_suppressButtonUp.insert(embedded);
                suppress=true;
                SplitViewerNativeMouseManager::instance().m_lastClickTicks.remove(embedded);
                SplitViewerNativeMouseManager::instance().m_lastClickPoints.remove(embedded);
            }
            else
            {
                // A single click is delivered only after the double-click
                // interval. Otherwise a player's deferred play/pause action
                // can fire even though the second press was consumed here.
                SplitViewerNativeMouseHelper::replayClick(embedded,false);
                POINT local = mouse->pt;
                ScreenToClient(sourceWindow,&local);
                SplitViewerNativeClick click;
                click.target = reinterpret_cast<WId>(sourceWindow);
                click.screenPoint = point;
                click.clientPoint = QPoint(local.x,local.y);
                click.modifiers = ((GetKeyState(VK_SHIFT)&0x8000) ? MK_SHIFT : 0) |
                    ((GetKeyState(VK_CONTROL)&0x8000) ? MK_CONTROL : 0);
                click.released = false;
                click.generation = ++SplitViewerNativeMouseManager::instance().m_clickGeneration;
                SplitViewerNativeMouseManager::instance().m_pendingClicks.insert(embedded,click);
                suppress = true;
                SplitViewerNativeMouseManager::instance().m_lastClickTicks.insert(embedded,now);
                SplitViewerNativeMouseManager::instance().m_lastClickPoints.insert(embedded,point);
            }
        }
        else if (message==WM_LBUTTONUP && !SplitViewerNativeMouseManager::instance().m_suppressButtonUp.isEmpty())
        {
            SplitViewerNativeMouseManager::instance().m_suppressButtonUp.clear();
            suppress=true;
        }
        else if (message==WM_MOUSEMOVE || message==WM_LBUTTONUP)
        {
            const QList<HWND> pendingWindows = SplitViewerNativeMouseManager::instance().m_pendingClicks.keys();
            foreach (HWND pending, pendingWindows)
            {
                SplitViewerNativeClick& click = SplitViewerNativeMouseManager::instance().m_pendingClicks[pending];
                if (click.released)
                {
                    continue;
                }
                if (message==WM_MOUSEMOVE &&
                    (point-click.screenPoint).manhattanLength()>=QApplication::startDragDistance())
                {
                    SplitViewerNativeMouseHelper::replayClick(pending,true);
                    SplitViewerNativeMouseManager::instance().m_lastClickTicks.remove(pending);
                }
                else if (message==WM_LBUTTONUP && !SplitViewerNativeMouseManager::instance().m_mouseClients.isEmpty())
                {
                    click.released = true;
                    const quint64 generation = click.generation;
                    suppress = true;
                    // Detach removes the entry; generation rejects stale callbacks.
                    QTimer::singleShot(GetDoubleClickTime(),qApp,[pending,generation]()
                    {
                        if (SplitViewerNativeMouseManager::instance().m_pendingClicks.contains(pending) &&
                            SplitViewerNativeMouseManager::instance().m_pendingClicks.value(pending).generation==generation)
                        {
                            SplitViewerNativeMouseHelper::replayClick(pending,false);
                        }
                    });
                }
            }
        }
        // 不在系统钩子内嵌套改父窗口；复制通知，交回各接收者所属的GUI事件循环。
        for (auto it=SplitViewerNativeMouseManager::instance().m_mouseClients.constBegin();it!=SplitViewerNativeMouseManager::instance().m_mouseClients.constEnd();++it)
        {
            QObject* owner = it.key();
            const quint64 generation = SplitViewerNativeMouseManager::instance().m_clientGenerations.value(owner);
            // 上下文还活着也可能已经注销或重新订阅，投递后再次核对订阅身份。
            QTimer::singleShot(0, owner, [owner, generation, event, point, id]()
            {
                SplitViewerNativeMouseManager& manager = SplitViewerNativeMouseManager::instance();
                if (manager.m_clientGenerations.value(owner) == generation && manager.m_mouseClients.contains(owner))
                {
                    const std::function<void(int, const QPoint&, WId)> callback = manager.m_mouseClients.value(owner);
                    callback(event, point, id);
                }
            });
        }
    }
    if (suppress)
    {
        return 1;
    }
    return CallNextHookEx(SplitViewerNativeMouseManager::instance().m_mouseHook,code,message,data);
}

#endif
void SplitViewerNativeMouseHelper::watchNativeMouse(QObject* owner,const std::function<void(int,const QPoint&,WId)>& callback)
{
#ifdef Q_OS_WIN
    if (owner == nullptr || !callback)
    {
        return;
    }
    SplitViewerNativeMouseManager::instance().m_mouseClients.insert(owner,callback);
    SplitViewerNativeMouseManager::instance().m_clientGenerations.insert(owner,
        ++SplitViewerNativeMouseManager::instance().m_clientGeneration);
    if (!SplitViewerNativeMouseManager::instance().m_mouseHook)
    {
        SplitViewerNativeMouseManager::instance().m_mouseHook=SetWindowsHookExW(WH_MOUSE_LL,SplitViewerNativeMouseHelper::mouseHook,GetModuleHandleW(nullptr),0);
    }
#else
    Q_UNUSED(owner); Q_UNUSED(callback);
#endif
}

void SplitViewerNativeMouseHelper::unwatchNativeMouse(QObject* owner)
{
#ifdef Q_OS_WIN
    SplitViewerNativeMouseManager::instance().m_mouseClients.remove(owner);
    SplitViewerNativeMouseManager::instance().m_clientGenerations.remove(owner);
    if (SplitViewerNativeMouseManager::instance().m_mouseClients.isEmpty() && SplitViewerNativeMouseManager::instance().m_mouseHook)
    {
        UnhookWindowsHookEx(SplitViewerNativeMouseManager::instance().m_mouseHook);
        SplitViewerNativeMouseManager::instance().m_mouseHook=nullptr;
    }
#else
    Q_UNUSED(owner);
#endif
}
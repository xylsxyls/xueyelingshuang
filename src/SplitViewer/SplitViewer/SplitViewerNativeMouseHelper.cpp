#include "SplitViewerNativeMouseHelper.h"
#include "SplitViewerNativeMouseConfig.h"
#include <algorithm>
#include <cmath>
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>

#ifdef Q_OS_WIN
void SplitViewerNativeMouseHelper::replayClick(HWND embedded, bool downOnly)
{
    if (!SplitViewerNativeMouseConfig::instance().pendingClicks.contains(embedded))
    {
        return;
    }
    const SplitViewerNativeClick click = SplitViewerNativeMouseConfig::instance().pendingClicks.take(embedded);
    const HWND target = reinterpret_cast<HWND>(click.target);
    if (!SplitViewerNativeMouseConfig::instance().embeddedWindows.contains(embedded) || !IsWindow(target))
    {
        return;
    }
    const LPARAM position = MAKELPARAM(click.clientPoint.x(),click.clientPoint.y());
    // Complete the target's press handler before delivering release. Queuing
    // both messages allowed native input and deferred Qt input to interleave.
    // Bound cross-process waits so an unresponsive player cannot block the UI.
    DWORD_PTR result = 0;
    const UINT flags = SMTO_ABORTIFHUNG | SMTO_BLOCK;
    if (!SendMessageTimeoutW(target, WM_LBUTTONDOWN, MK_LBUTTON|click.modifiers,
        position, flags, 100, &result))
    {
        if (!downOnly) PostMessageW(target, WM_LBUTTONUP, click.modifiers, position);
        return;
    }
    if (!downOnly)
    {
        if (!SendMessageTimeoutW(target, WM_LBUTTONUP, click.modifiers, position, flags, 100, &result))
        {
            PostMessageW(target, WM_LBUTTONUP, click.modifiers, position);
        }
    }
}

#endif
#ifdef Q_OS_WIN
HWND SplitViewerNativeMouseHelper::embeddedAncestor(HWND window)
{
    HWND current=window;
    while (current)
    {
        if (SplitViewerNativeMouseConfig::instance().embeddedWindows.contains(current)) return current;
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
            const DWORD now=GetTickCount();
            const DWORD lastTick=SplitViewerNativeMouseConfig::instance().lastClickTicks.value(embedded,0);
            const QPoint lastPoint=SplitViewerNativeMouseConfig::instance().lastClickPoints.value(embedded);
            const int doubleClickWidth=(std::max)(1,GetSystemMetrics(SM_CXDOUBLECLK));
            const int doubleClickHeight=(std::max)(1,GetSystemMetrics(SM_CYDOUBLECLK));
            if (lastTick!=0 && now-lastTick<=GetDoubleClickTime() &&
                std::abs(point.x()-lastPoint.x())<=doubleClickWidth &&
                std::abs(point.y()-lastPoint.y())<=doubleClickHeight)
            {
                SplitViewerNativeMouseConfig::instance().pendingClicks.remove(embedded);
                SplitViewerNativeMouseConfig::instance().suppressButtonUp.insert(embedded);
                suppress=true;
                SplitViewerNativeMouseConfig::instance().lastClickTicks.remove(embedded);
                SplitViewerNativeMouseConfig::instance().lastClickPoints.remove(embedded);
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
                click.generation = ++SplitViewerNativeMouseConfig::instance().clickGeneration;
                SplitViewerNativeMouseConfig::instance().pendingClicks.insert(embedded,click);
                suppress = true;
                SplitViewerNativeMouseConfig::instance().lastClickTicks.insert(embedded,now);
                SplitViewerNativeMouseConfig::instance().lastClickPoints.insert(embedded,point);
            }
        }
        else if (message==WM_LBUTTONUP && !SplitViewerNativeMouseConfig::instance().suppressButtonUp.isEmpty())
        {
            SplitViewerNativeMouseConfig::instance().suppressButtonUp.clear();
            suppress=true;
        }
        else if (message==WM_MOUSEMOVE || message==WM_LBUTTONUP)
        {
            const QList<HWND> pendingWindows = SplitViewerNativeMouseConfig::instance().pendingClicks.keys();
            foreach (HWND pending, pendingWindows)
            {
                SplitViewerNativeClick& click = SplitViewerNativeMouseConfig::instance().pendingClicks[pending];
                if (click.released)
                {
                    continue;
                }
                if (message==WM_MOUSEMOVE &&
                    (point-click.screenPoint).manhattanLength()>=QApplication::startDragDistance())
                {
                    SplitViewerNativeMouseHelper::replayClick(pending,true);
                    SplitViewerNativeMouseConfig::instance().lastClickTicks.remove(pending);
                }
                else if (message==WM_LBUTTONUP && !SplitViewerNativeMouseConfig::instance().mouseClients.isEmpty())
                {
                    click.released = true;
                    const quint64 generation = click.generation;
                    suppress = true;
                    // Detach removes the entry; generation rejects stale callbacks.
                    QTimer::singleShot(GetDoubleClickTime(),qApp,[pending,generation]()
                    {
                        if (SplitViewerNativeMouseConfig::instance().pendingClicks.contains(pending) &&
                            SplitViewerNativeMouseConfig::instance().pendingClicks.value(pending).generation==generation)
                        {
                            SplitViewerNativeMouseHelper::replayClick(pending,false);
                        }
                    });
                }
            }
        }
        // 不在系统钩子内嵌套改父窗口；复制通知，交回各接收者所属的GUI事件循环。
        for (auto it=SplitViewerNativeMouseConfig::instance().mouseClients.constBegin();it!=SplitViewerNativeMouseConfig::instance().mouseClients.constEnd();++it)
        {
            const std::function<void(int,const QPoint&,WId)> callback=it.value();
            QTimer::singleShot(0,it.key(),[callback,event,point,id]() { callback(event,point,id); });
        }
    }
    if (suppress) return 1;
    return CallNextHookEx(SplitViewerNativeMouseConfig::instance().mouseHook,code,message,data);
}

#endif
void SplitViewerNativeMouseHelper::watchNativeMouse(QObject* owner,const std::function<void(int,const QPoint&,WId)>& callback)
{
#ifdef Q_OS_WIN
    SplitViewerNativeMouseConfig::instance().mouseClients.insert(owner,callback);
    if (!SplitViewerNativeMouseConfig::instance().mouseHook) SplitViewerNativeMouseConfig::instance().mouseHook=SetWindowsHookExW(WH_MOUSE_LL,SplitViewerNativeMouseHelper::mouseHook,GetModuleHandleW(nullptr),0);
#else
    Q_UNUSED(owner); Q_UNUSED(callback);
#endif
}

void SplitViewerNativeMouseHelper::unwatchNativeMouse(QObject* owner)
{
#ifdef Q_OS_WIN
    SplitViewerNativeMouseConfig::instance().mouseClients.remove(owner);
    if (SplitViewerNativeMouseConfig::instance().mouseClients.isEmpty() && SplitViewerNativeMouseConfig::instance().mouseHook)
    {
        UnhookWindowsHookEx(SplitViewerNativeMouseConfig::instance().mouseHook);
        SplitViewerNativeMouseConfig::instance().mouseHook=nullptr;
    }
#else
    Q_UNUSED(owner);
#endif
}
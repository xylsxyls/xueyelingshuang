#include "SplitViewerForeignWindowHelper.h"
#include "SplitViewerNativeMouseConfig.h"
#include "QtControls/Widget.h"
#include <algorithm>
#include <QtCore/QVariant>
#include <QtGui/QWindow>
#include <QtGui/QImage>
#include <QtGui/QScreen>
#include <QtGui/QGuiApplication>
#include <QtGui/QPixmap>
#include <QtWidgets/QWidget>

QWidget* SplitViewerForeignWindowHelper::embedForeignWindow(WId windowId, QWidget* parent)
{
    if (!windowId || !parent)
    {
        return NULL;
    }
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_MACX)
#ifdef Q_OS_WIN
    HWND hwnd=reinterpret_cast<HWND>(windowId);
    if (!IsWindow(hwnd) || hwnd==GetDesktopWindow() || hwnd==GetShellWindow() ||
        GetAncestor(hwnd,GA_ROOT)==reinterpret_cast<HWND>(parent->window()->winId())) return nullptr;
    RECT original;
    GetWindowRect(hwnd,&original);
    const LONG_PTR style=GetWindowLongPtrW(hwnd,GWL_STYLE);
    const LONG_PTR exStyle=GetWindowLongPtrW(hwnd,GWL_EXSTYLE);
    const HWND oldParent=GetParent(hwnd);
#endif
#ifdef Q_OS_WIN
    // A foreign QWindow and a native SetParent must not both own the HWND
    // geometry. Qt 5.5 otherwise reapplies the canvas offset inside the host.
    QWidget* container = new Widget(parent);
    container->setAttribute(Qt::WA_NativeWindow);
    container->setAttribute(Qt::WA_NoSystemBackground);
#else
    QWindow* foreignWindow = QWindow::fromWinId(windowId);
    if (!foreignWindow)
    {
        return NULL;
    }
    QWidget* container = QWidget::createWindowContainer(foreignWindow, parent);
#endif
    if (container)
    {
#ifndef Q_OS_WIN
        container->setProperty("foreignWindow",QVariant::fromValue<qulonglong>(reinterpret_cast<quintptr>(foreignWindow)));
#endif
        container->setProperty("foreignId",QVariant::fromValue<qulonglong>(windowId));
#ifdef Q_OS_WIN
        container->setProperty("oldParent",QVariant::fromValue<qulonglong>(reinterpret_cast<quintptr>(oldParent)));
        container->setProperty("oldStyle",QVariant::fromValue<qlonglong>(style));
        container->setProperty("oldExStyle",QVariant::fromValue<qlonglong>(exStyle));
        container->setProperty("oldRect",QRect(original.left,original.top,original.right-original.left,original.bottom-original.top));
        SetWindowLongPtrW(hwnd,GWL_STYLE,(style & ~(WS_POPUP|WS_CAPTION|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX))|WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);
        SetWindowLongPtrW(hwnd,GWL_EXSTYLE,exStyle & ~(WS_EX_APPWINDOW|WS_EX_TOOLWINDOW|WS_EX_TOPMOST|WS_EX_DLGMODALFRAME|WS_EX_WINDOWEDGE|WS_EX_CLIENTEDGE|WS_EX_STATICEDGE));
        SetLastError(ERROR_SUCCESS);
        const HWND previousParent = SetParent(hwnd,reinterpret_cast<HWND>(container->winId()));
        if (!previousParent && GetLastError() != ERROR_SUCCESS)
        {
            SetWindowLongPtrW(hwnd,GWL_STYLE,style);
            SetWindowLongPtrW(hwnd,GWL_EXSTYLE,exStyle);
            delete container;
            return nullptr;
        }
        // Maximize inside the dedicated child host, never on the desktop.
        // Qt players then use their fitted viewport rendering policy.
        ShowWindow(hwnd, SW_SHOWMAXIMIZED);
        SplitViewerNativeMouseConfig::instance().embeddedWindows.insert(hwnd);
#endif
        container->setFocusPolicy(Qt::StrongFocus);
        container->show();
    }
    return container;
#else
    Q_UNUSED(parent);
    return NULL;
#endif
}

void SplitViewerForeignWindowHelper::syncForeignWindow(QWidget* container, const QRect& geometry)
{
    if (!container)
    {
        return;
    }
    container->setGeometry(geometry);
    if (!geometry.isValid())
    {
        return;
    }
#ifdef Q_OS_WIN
    const HWND hwnd = reinterpret_cast<HWND>(static_cast<quintptr>(container->property("foreignId").toULongLong()));
    const HWND parent = reinterpret_cast<HWND>(container->winId());
    if (!IsWindow(hwnd) || !IsWindow(parent))
    {
        return;
    }
    // The native host owns child geometry; SetWindowPos delivers WM_SIZE
    // to the external application's event loop when its client size changes.
    if (GetParent(hwnd) != parent)
    {
        SetParent(hwnd, parent);
    }
    RECT client = {0};
    GetClientRect(parent, &client);
    const int width = (std::max)(1L, client.right);
    const int height = (std::max)(1L, client.bottom);
    RECT current = {0};
    GetWindowRect(hwnd, &current);
    MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&current), 2);
    if (current.left != 0 || current.top != 0 ||
        current.right != width || current.bottom != height || !IsWindowVisible(hwnd))
    {
        SetWindowPos(hwnd, HWND_TOP, 0, 0, width, height,
            SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    }
#else
    Q_UNUSED(container);
#endif
}

void SplitViewerForeignWindowHelper::detachForeignWindow(QWidget* container)
{
    if (!container) return;
    QWindow* foreign=reinterpret_cast<QWindow*>(static_cast<quintptr>(container->property("foreignWindow").toULongLong()));
#ifdef Q_OS_WIN
    HWND hwnd=reinterpret_cast<HWND>(static_cast<quintptr>(container->property("foreignId").toULongLong()));
    SplitViewerNativeMouseConfig::instance().embeddedWindows.remove(hwnd);
    SplitViewerNativeMouseConfig::instance().lastClickTicks.remove(hwnd);
    SplitViewerNativeMouseConfig::instance().lastClickPoints.remove(hwnd);
    SplitViewerNativeMouseConfig::instance().suppressButtonUp.remove(hwnd);
    SplitViewerNativeMouseConfig::instance().pendingClicks.remove(hwnd);
    const QRect rect=container->property("oldRect").toRect();
    const HWND parent=reinterpret_cast<HWND>(static_cast<quintptr>(container->property("oldParent").toULongLong()));
    const LONG_PTR style=static_cast<LONG_PTR>(container->property("oldStyle").toLongLong());
    const LONG_PTR exStyle=static_cast<LONG_PTR>(container->property("oldExStyle").toLongLong());
#endif
    if (foreign)
    {
        foreign->setParent(nullptr);
        container->setProperty("foreignWindow",QVariant());
        delete foreign;
    }
#ifdef Q_OS_WIN
    // Detach the borrowed HWND before destroying its native host, since
    // DestroyWindow also destroys any remaining native children.
    if (IsWindow(hwnd))
    {
        SetParent(hwnd,parent);
        SetWindowLongPtrW(hwnd,GWL_STYLE,style);
        SetWindowLongPtrW(hwnd,GWL_EXSTYLE,exStyle);
        SetWindowPos(hwnd,nullptr,rect.x(),rect.y(),rect.width(),rect.height(),SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
        ShowWindow(hwnd,(style & WS_VISIBLE) ? SW_SHOWNOACTIVATE : SW_HIDE);
    }
#endif
    delete container;
}

WId SplitViewerForeignWindowHelper::windowUnderCursor(WId ownWindowId)
{
#ifdef Q_OS_WIN
    POINT point;
    if (!GetCursorPos(&point))
    {
        return 0;
    }
    HWND window = WindowFromPoint(point);
    window = window ? GetAncestor(window, GA_ROOT) : NULL;
    if (!window || reinterpret_cast<WId>(window) == ownWindowId)
    {
        return 0;
    }
    return reinterpret_cast<WId>(window);
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_MACX)
    Q_UNUSED(ownWindowId);
    return 0;
#else
    Q_UNUSED(ownWindowId);
    return 0;
#endif
}

QImage SplitViewerForeignWindowHelper::foreignWindowSnapshot(QWidget* container)
{
    if (!container) return QImage();
    const WId id=static_cast<WId>(container->property("foreignId").toULongLong());
#ifdef Q_OS_WIN
    HWND hwnd=reinterpret_cast<HWND>(id);
    RECT rect;
    if (!IsWindow(hwnd) || !GetClientRect(hwnd,&rect) || rect.right<=0 || rect.bottom<=0) return QImage();
    QImage image(rect.right,rect.bottom,QImage::Format_RGB32);
    image.fill(QColor(128,128,128));
    HDC dc=GetDC(hwnd);
    HDC memory=CreateCompatibleDC(dc);
    HBITMAP bitmap=CreateCompatibleBitmap(dc,rect.right,rect.bottom);
    HGDIOBJ old=SelectObject(memory,bitmap);
    const BOOL ok=PrintWindow(hwnd,memory,PW_CLIENTONLY);
    SelectObject(memory,old);
    BITMAPINFO info={0};
    info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=rect.right;
    info.bmiHeader.biHeight=-rect.bottom;
    info.bmiHeader.biPlanes=1;
    info.bmiHeader.biBitCount=32;
    info.bmiHeader.biCompression=BI_RGB;
    if (ok) GetDIBits(memory,bitmap,0,rect.bottom,image.bits(),&info,DIB_RGB_COLORS);
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(hwnd,dc);
    return image;
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC) || defined(Q_OS_MACX)
    QScreen* screen=QGuiApplication::primaryScreen();
    return screen ? screen->grabWindow(id).toImage() : QImage();
#else
    return QImage();
#endif
}

bool SplitViewerForeignWindowHelper::isExternalWindow(WId id,WId own)
{
#ifdef Q_OS_WIN
    HWND hwnd=reinterpret_cast<HWND>(id);
    return hwnd && id!=own && IsWindow(hwnd) && IsWindowVisible(hwnd) && hwnd!=GetDesktopWindow() && hwnd!=GetShellWindow() && !(GetWindowLongPtrW(hwnd,GWL_STYLE)&WS_CHILD);
#else
    Q_UNUSED(id); Q_UNUSED(own);
    return false;
#endif
}

bool SplitViewerForeignWindowHelper::foreignWindowValid(QWidget* container)
{
    if (!container) return false;
#ifdef Q_OS_WIN
    return IsWindow(reinterpret_cast<HWND>(static_cast<quintptr>(container->property("foreignId").toULongLong())))!=FALSE;
#else
    return true;
#endif
}

void SplitViewerForeignWindowHelper::moveDetachedWindow(WId id,const QPoint& center)
{
#ifdef Q_OS_WIN
    HWND hwnd=reinterpret_cast<HWND>(id);
    RECT rect;
    if (IsWindow(hwnd) && GetWindowRect(hwnd,&rect))
    {
        SetWindowPos(hwnd,nullptr,center.x()-(rect.right-rect.left)/2,center.y()-(rect.bottom-rect.top)/2,0,0,SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
    }
#else
    Q_UNUSED(id); Q_UNUSED(center);
#endif
}
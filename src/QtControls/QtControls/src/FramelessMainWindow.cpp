#include "FramelessMainWindow.h"
#include "Widget.h"
#include <QAbstractButton>
#include <QMouseEvent>
#include <QShowEvent>
#include <algorithm>
#ifdef Q_OS_WIN
#include <Windows.h>
#include <windowsx.h>
#endif

FramelessMainWindow::FramelessMainWindow(QWidget* parent) :
MainWindow(parent),
m_resizeBorder(5),
m_resizeEdges(0)
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setMouseTracking(true);
}

void FramelessMainWindow::setTitleBar(Widget* titleBar, int resizeBorder)
{
    if (titleBar == nullptr)
    {
        return;
    }
    m_titleBar = titleBar;
    m_resizeBorder = (std::max)(1, (std::min)(16, resizeBorder));
    setMenuWidget(titleBar);
    setContentsMargins(m_resizeBorder, m_resizeBorder, m_resizeBorder, m_resizeBorder);
}

int FramelessMainWindow::edges(const QPoint& point) const
{
    if (isMaximized() || isFullScreen())
    {
        return 0;
    }
    return (point.x() < m_resizeBorder ? 1 : 0) | (point.y() < m_resizeBorder ? 2 : 0) |
        (point.x() >= width() - m_resizeBorder ? 4 : 0) | (point.y() >= height() - m_resizeBorder ? 8 : 0);
}

bool FramelessMainWindow::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
#ifdef Q_OS_WIN
    MSG* native = static_cast<MSG*>(message);
    if (native != nullptr && result != nullptr)
    {
        if (native->message == WM_NCCALCSIZE && native->wParam != 0)
        {
            *result = 0;
            return true;
        }
        if (native->message == WM_GETMINMAXINFO && !isFullScreen())
        {
            MONITORINFO info = {};
            info.cbSize = sizeof(info);
            if (GetMonitorInfo(MonitorFromWindow(native->hwnd, MONITOR_DEFAULTTONEAREST), &info))
            {
                MINMAXINFO* limits = reinterpret_cast<MINMAXINFO*>(native->lParam);
                limits->ptMaxPosition.x = info.rcWork.left - info.rcMonitor.left;
                limits->ptMaxPosition.y = info.rcWork.top - info.rcMonitor.top;
                limits->ptMaxSize.x = info.rcWork.right - info.rcWork.left;
                limits->ptMaxSize.y = info.rcWork.bottom - info.rcWork.top;
                const int ratio = devicePixelRatio();
                limits->ptMinTrackSize.x = minimumWidth() * ratio;
                limits->ptMinTrackSize.y = minimumHeight() * ratio;
                *result = 0;
                return true;
            }
        }
        if (native->message == WM_NCHITTEST && !isFullScreen())
        {
            const QPoint point = mapFromGlobal(QPoint(GET_X_LPARAM(native->lParam), GET_Y_LPARAM(native->lParam)));
            const int border = edges(point);
            if (border != 0)
            {
                *result = border == 3 ? HTTOPLEFT : border == 6 ? HTTOPRIGHT : border == 9 ? HTBOTTOMLEFT :
                    border == 12 ? HTBOTTOMRIGHT : border == 1 ? HTLEFT : border == 4 ? HTRIGHT :
                    border == 2 ? HTTOP : HTBOTTOM;
                return true;
            }
            if (m_titleBar && m_titleBar->rect().contains(m_titleBar->mapFrom(this, point)))
            {
                QWidget* hit = childAt(point);
                bool interactive = false;
                for (QWidget* child = hit; child != nullptr && child != m_titleBar.data(); child = child->parentWidget())
                {
                    if (qobject_cast<QAbstractButton*>(child) != nullptr || child->focusPolicy() != Qt::NoFocus)
                    {
                        interactive = true;
                        break;
                    }
                }
                if (!interactive)
                {
                    *result = HTCAPTION;
                    return true;
                }
            }
        }
    }
#endif
    return MainWindow::nativeEvent(eventType, message, result);
}

void FramelessMainWindow::showEvent(QShowEvent* event)
{
    MainWindow::showEvent(event);
#ifdef Q_OS_WIN
    if (!isFullScreen())
    {
        HWND handle = reinterpret_cast<HWND>(winId());
        const LONG_PTR style = GetWindowLongPtr(handle, GWL_STYLE);
        const LONG_PTR target = style | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        if (style != target)
        {
            SetWindowLongPtr(handle, GWL_STYLE, target);
            SetWindowPos(handle, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }
#endif
}

void FramelessMainWindow::changeEvent(QEvent* event)
{
    MainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange)
    {
        m_resizeEdges = 0;
        if (m_titleBar)
        {
            m_titleBar->setVisible(!isFullScreen());
        }
        const int margin = isFullScreen() || isMaximized() ? 0 : m_resizeBorder;
        setContentsMargins(margin, margin, margin, margin);
    }
}

void FramelessMainWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && (m_resizeEdges = edges(event->pos())) != 0)
    {
        m_resizeStart = event->globalPos();
        m_resizeGeometry = geometry();
        event->accept();
        return;
    }
    MainWindow::mousePressEvent(event);
}

void FramelessMainWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (m_resizeEdges != 0 && event->buttons().testFlag(Qt::LeftButton))
    {
        const QPoint delta = event->globalPos() - m_resizeStart;
        QRect target = m_resizeGeometry;
        if (m_resizeEdges & 1)
        {
            target.setLeft((std::min)(target.right() - minimumWidth() + 1, target.left() + delta.x()));
        }
        if (m_resizeEdges & 2)
        {
            target.setTop((std::min)(target.bottom() - minimumHeight() + 1, target.top() + delta.y()));
        }
        if (m_resizeEdges & 4)
        {
            target.setRight((std::max)(target.left() + minimumWidth() - 1, target.right() + delta.x()));
        }
        if (m_resizeEdges & 8)
        {
            target.setBottom((std::max)(target.top() + minimumHeight() - 1, target.bottom() + delta.y()));
        }
        setGeometry(target);
        event->accept();
        return;
    }
    const int border = edges(event->pos());
    setCursor(border == 3 || border == 12 ? Qt::SizeFDiagCursor : border == 6 || border == 9 ? Qt::SizeBDiagCursor :
        border & 5 ? Qt::SizeHorCursor : border & 10 ? Qt::SizeVerCursor : Qt::ArrowCursor);
    MainWindow::mouseMoveEvent(event);
}

void FramelessMainWindow::mouseReleaseEvent(QMouseEvent* event)
{
    m_resizeEdges = 0;
    MainWindow::mouseReleaseEvent(event);
}
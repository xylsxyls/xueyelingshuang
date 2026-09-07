#include "COriginalDialog.h"
#include "ControlStyleManager.h"
#include <QApplication>
#include <QCloseEvent>
#include <QCursor>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QWindow>
#include <algorithm>
#include <stdint.h>

#ifdef _MSC_VER
#include <Windows.h>
#endif

#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED        0x031E
#endif

COriginalDialog::COriginalDialog(QWidget* parent) :
QDialog(parent),
m_touchBorderWidth(0),
m_customerTitleBarHeight(0),
m_altF4Enable(true)
#ifndef _MSC_VER
,m_altF4Close(false)
,m_altCloseEve(nullptr)
,m_isLeftButtonPressed(false)
,m_isMovePressed(false)
,m_currentType(NORMAL)
,m_pressedType(NORMAL)
#endif
{
	setCustomerTitleBarHeight(0);
	setMouseTracking(true);
	setFont(defaultDialogFont());
}

COriginalDialog::~COriginalDialog()
{

}

void COriginalDialog::setDefaultDialogFont(const QFont& font)
{
	ControlStyleManager::instance().setDefaultFont(font);
}

QFont COriginalDialog::defaultDialogFont()
{
	return ControlStyleManager::instance().defaultFont();
}

void COriginalDialog::setDefaultDialogTextColor(const QColor& color)
{
	ControlStyleManager::instance().setDefaultTextColor(color);
}

QColor COriginalDialog::defaultDialogTextColor()
{
	return ControlStyleManager::instance().defaultTextColor();
}

#ifdef _MSC_VER
long COriginalDialog::onNcHitTest(const QPoint& pt)
{
	RECT windowRect;
	::GetWindowRect(HWND(winId()), &windowRect);
	QRect rcClient;
	rcClient.setLeft(windowRect.left);
	rcClient.setRight(windowRect.right);
	rcClient.setTop(windowRect.top);
	rcClient.setBottom(windowRect.bottom);

	bool fixdWidth = minimumWidth() == maximumWidth();
	bool fixdHeight = minimumHeight() == maximumHeight();

	if ((pt.x() < (rcClient.right() + m_touchBorderWidth)) &&
		(pt.x() > (rcClient.right() - m_touchBorderWidth)))
	{
		if ((pt.y() < (rcClient.top() + m_touchBorderWidth)) &&
			(pt.y() > (rcClient.top() - m_touchBorderWidth)))
		{
			if (fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			return HTTOPRIGHT;
		}

		if ((pt.y() < (rcClient.bottom() + m_touchBorderWidth)) &&
			(pt.y() > (rcClient.bottom() - m_touchBorderWidth)))
		{
			if (fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			return HTBOTTOMRIGHT;
		}

		if (fixdWidth)
		{
			return HTCLIENT;
		}
		return HTRIGHT;
	}

	if ((pt.x() < (rcClient.left() + m_touchBorderWidth)) &&
		(pt.x() > (rcClient.left() - m_touchBorderWidth)))
	{
		if ((pt.y() < (rcClient.top() + m_touchBorderWidth)) &&
			(pt.y() > (rcClient.top() - m_touchBorderWidth)))
		{
			if (fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			return HTTOPLEFT;
		}

		if ((pt.y() < (rcClient.bottom() + m_touchBorderWidth)) &&
			(pt.y() > (rcClient.bottom() - m_touchBorderWidth)))
		{
			if (fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			return HTBOTTOMLEFT;
		}

		if (fixdWidth)
		{
			return HTCLIENT;
		}
		return HTLEFT;
	}

	if ((pt.y() < (rcClient.top() + m_touchBorderWidth)) &&
		(pt.y() > (rcClient.top() - m_touchBorderWidth)))
	{
		if ((pt.x() < (rcClient.right() + m_touchBorderWidth)) &&
			(pt.x() > (rcClient.right() - m_touchBorderWidth)))
		{
			if (fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			return HTTOPRIGHT;
		}

		if ((pt.x() < (rcClient.left() + m_touchBorderWidth)) &&
			(pt.x() > (rcClient.left() - m_touchBorderWidth)))
		{
			if (fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			return HTTOPLEFT;
		}

		if (fixdHeight)
		{
			return HTCLIENT;
		}
		return HTTOP;
	}

	if ((pt.y() < (rcClient.bottom() + m_touchBorderWidth)) &&
		(pt.y() > (rcClient.bottom() - m_touchBorderWidth)))
	{
		if ((pt.x() < (rcClient.right() + m_touchBorderWidth)) &&
			(pt.x() > (rcClient.right() - m_touchBorderWidth)))
		{
			if (fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			return HTBOTTOMRIGHT;
		}

		if ((pt.x() < (rcClient.left() + m_touchBorderWidth)) &&
			(pt.x() > (rcClient.left() - m_touchBorderWidth)))
		{
			if (fixdWidth || fixdHeight)
			{
				return HTCLIENT;
			}
			return HTBOTTOMLEFT;
		}
		return HTBOTTOM;
	}

	if ((pt.y() - rcClient.top()) <= m_customerTitleBarRect.height())
	{
		bool hasChild = false;
		for (int32_t i = 0; i < children().count(); ++i)
		{
			QWidget* widget = qobject_cast<QWidget*>(children()[i]);
			if (widget == nullptr)
			{
				continue;
			}
			if (!widget->isVisible() || !widget->isEnabled())
			{
				continue;
			}

			QLabel* label = qobject_cast<QLabel*>(widget);
			if (label != nullptr)
			{
				continue;
			}

			QPoint mousePt = mapFromGlobal(QCursor::pos());
			if (widget->geometry().contains(mousePt))
			{
				hasChild = true;
				break;
			}
		}
		if (hasChild)
		{
			return HTCLIENT;
		}
		return HTCAPTION;
	}
	return HTCLIENT;
}
#endif

void COriginalDialog::resizeEvent(QResizeEvent* eve)
{
	QDialog::resizeEvent(eve);
	m_customerTitleBarRect = QRect(0, 0, width(), m_customerTitleBarHeight);
}

bool COriginalDialog::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
#ifdef _MSC_VER
	if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG")
	{
		MSG* msg = static_cast<MSG*>(message);
		switch (msg->message)
		{
		case WM_NCACTIVATE:
		{
			msg->lParam = -1;
			*result = DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
			emit ncActiveChanged(msg->wParam == 1);
			return true;
		}
		case WM_ACTIVATE:
		{
			emit activeChanged(msg->wParam);
			break;
		}
		case WM_SIZE:
		{
			break;
		}
		case WM_DWMCOMPOSITIONCHANGED:
		{
			break;
		}
		case WM_NCHITTEST:
		{
			int32_t xPos = static_cast<int16_t>(LOWORD(msg->lParam));
			int32_t yPos = static_cast<int16_t>(HIWORD(msg->lParam));
			*result = onNcHitTest(QPoint(xPos, yPos));
			if (HTERROR != *result)
			{
				return true;
			}
			break;
		}
		case WM_NCCALCSIZE:
		{
			break;
		}
		case WM_ERASEBKGND:
		{
			*result = -1;
			return true;
		}
		case WM_SYSKEYDOWN:
		{
			if (m_altF4Enable && (VK_F4 == msg->wParam) && (::GetKeyState(VK_MENU) & 0xF000))
			{
				altF4PressedEvent();
			}
			return true;
		}
		default:
		{
			break;
		}
		}
	}
#else
	Q_UNUSED(eventType);
	Q_UNUSED(message);
	Q_UNUSED(result);
#endif
	return QDialog::nativeEvent(eventType, message, result);
}

void COriginalDialog::altF4PressedEvent()
{
	if (!m_altF4Enable)
	{
		return;
	}
	close();
}

bool COriginalDialog::eventFilter(QObject* tar, QEvent* eve)
{
	bool res = QDialog::eventFilter(tar, eve);
	if (tar == nullptr || eve == nullptr)
	{
		return res;
	}

	switch (eve->type())
	{
	case QEvent::Hide:
	{
		if (tar == transientWindow())
		{
			close();
		}
		break;
	}
	default:
	{
		break;
	}
	}
	return res;
}

void COriginalDialog::keyPressEvent(QKeyEvent* eve)
{
	if (eve != nullptr && eve->key() == Qt::Key_F4 && eve->modifiers().testFlag(Qt::AltModifier))
	{
		altF4PressedEvent();
		eve->accept();
		return;
	}
	QDialog::keyPressEvent(eve);
}

#ifndef _MSC_VER
void COriginalDialog::mousePressEvent(QMouseEvent* eve)
{
	if (eve == nullptr)
	{
		return;
	}

	if (eve->button() != Qt::LeftButton)
	{
		QDialog::mousePressEvent(eve);
		return;
	}

	m_isLeftButtonPressed = true;
	m_pressGlobalPos = eve->globalPos();
	m_pressGeometry = geometry();
	m_pressedType = getTouchType(eve->pos());
	m_isMovePressed = (m_pressedType == NORMAL && isTitleDraggableArea(eve->pos()));

	if (m_pressedType != NORMAL || m_isMovePressed)
	{
		eve->accept();
		return;
	}

	QDialog::mousePressEvent(eve);
}

void COriginalDialog::mouseMoveEvent(QMouseEvent* eve)
{
	if (eve == nullptr)
	{
		return;
	}

	if (m_isLeftButtonPressed)
	{
		if (m_pressedType != NORMAL)
		{
			applyResizeByGlobalPos(eve->globalPos());
			eve->accept();
			return;
		}
		if (m_isMovePressed)
		{
			QPoint delta = eve->globalPos() - m_pressGlobalPos;
			move(m_pressGeometry.topLeft() + delta);
			eve->accept();
			return;
		}
	}

	m_currentType = getTouchType(eve->pos());
	updateCursorByTouchType(m_currentType);
	QDialog::mouseMoveEvent(eve);
}

void COriginalDialog::mouseReleaseEvent(QMouseEvent* eve)
{
	if (eve == nullptr)
	{
		return;
	}

	if (eve->button() == Qt::LeftButton)
	{
		m_isLeftButtonPressed = false;
		m_isMovePressed = false;
		m_pressedType = NORMAL;
		m_pressGeometry = QRect();
		updateCursorByTouchType(getTouchType(eve->pos()));
	}
	QDialog::mouseReleaseEvent(eve);
}

void COriginalDialog::closeEvent(QCloseEvent* eve)
{
	m_altCloseEve = eve;
	QDialog::closeEvent(eve);
	m_altCloseEve = nullptr;
}

void COriginalDialog::close()
{
	QDialog::close();
}

COriginalDialog::TouchType COriginalDialog::getTouchType(const QPoint& clientPos) const
{
	if (m_touchBorderWidth <= 0 || isMaximized())
	{
		return NORMAL;
	}

	QRect clientRect = rect();
	if (!clientRect.contains(clientPos))
	{
		return NORMAL;
	}

	bool fixdWidth = minimumWidth() == maximumWidth();
	bool fixdHeight = minimumHeight() == maximumHeight();
	bool left = clientPos.x() <= m_touchBorderWidth;
	bool right = clientPos.x() >= clientRect.width() - m_touchBorderWidth;
	bool top = clientPos.y() <= m_touchBorderWidth;
	bool bottom = clientPos.y() >= clientRect.height() - m_touchBorderWidth;

	if (top && left && !fixdWidth && !fixdHeight)
	{
		return NORTH_WEST;
	}
	if (top && right && !fixdWidth && !fixdHeight)
	{
		return NORTH_EAST;
	}
	if (bottom && left && !fixdWidth && !fixdHeight)
	{
		return SOUTH_WEST;
	}
	if (bottom && right && !fixdWidth && !fixdHeight)
	{
		return SOUTH_EAST;
	}
	if (top && !fixdHeight)
	{
		return NORTH;
	}
	if (bottom && !fixdHeight)
	{
		return SOUTH;
	}
	if (left && !fixdWidth)
	{
		return WEST;
	}
	if (right && !fixdWidth)
	{
		return EAST;
	}
	return NORMAL;
}

bool COriginalDialog::isTitleDraggableArea(const QPoint& clientPos) const
{
	if (m_customerTitleBarRect.height() <= 0 || !m_customerTitleBarRect.contains(clientPos))
	{
		return false;
	}

	QWidget* child = childAt(clientPos);
	if (child == nullptr)
	{
		return true;
	}
	if (qobject_cast<QLabel*>(child) != nullptr)
	{
		return true;
	}
	return false;
}

void COriginalDialog::updateCursorByTouchType(TouchType touchType)
{
	switch (touchType)
	{
	case EAST:
	case WEST:
	{
		setCursor(Qt::SizeHorCursor);
		break;
	}
	case NORTH:
	case SOUTH:
	{
		setCursor(Qt::SizeVerCursor);
		break;
	}
	case NORTH_EAST:
	case SOUTH_WEST:
	{
		setCursor(Qt::SizeBDiagCursor);
		break;
	}
	case NORTH_WEST:
	case SOUTH_EAST:
	{
		setCursor(Qt::SizeFDiagCursor);
		break;
	}
	case NORMAL:
	default:
	{
		unsetCursor();
		break;
	}
	}
}

void COriginalDialog::applyResizeByGlobalPos(const QPoint& globalPos)
{
	QPoint delta = globalPos - m_pressGlobalPos;
	QRect resizeRect = m_pressGeometry;

	switch (m_pressedType)
	{
	case EAST:
	{
		resizeRect.setRight(m_pressGeometry.right() + delta.x());
		break;
	}
	case WEST:
	{
		resizeRect.setLeft(m_pressGeometry.left() + delta.x());
		break;
	}
	case NORTH:
	{
		resizeRect.setTop(m_pressGeometry.top() + delta.y());
		break;
	}
	case SOUTH:
	{
		resizeRect.setBottom(m_pressGeometry.bottom() + delta.y());
		break;
	}
	case NORTH_EAST:
	{
		resizeRect.setTop(m_pressGeometry.top() + delta.y());
		resizeRect.setRight(m_pressGeometry.right() + delta.x());
		break;
	}
	case SOUTH_WEST:
	{
		resizeRect.setLeft(m_pressGeometry.left() + delta.x());
		resizeRect.setBottom(m_pressGeometry.bottom() + delta.y());
		break;
	}
	case NORTH_WEST:
	{
		resizeRect.setLeft(m_pressGeometry.left() + delta.x());
		resizeRect.setTop(m_pressGeometry.top() + delta.y());
		break;
	}
	case SOUTH_EAST:
	{
		resizeRect.setRight(m_pressGeometry.right() + delta.x());
		resizeRect.setBottom(m_pressGeometry.bottom() + delta.y());
		break;
	}
	case NORMAL:
	default:
	{
		return;
	}
	}

	int32_t minWidth = std::max(minimumWidth(), m_touchBorderWidth * 2);
	int32_t minHeight = std::max(minimumHeight(), m_touchBorderWidth * 2);
	int32_t maxWidth = maximumWidth();
	int32_t maxHeight = maximumHeight();

	if (resizeRect.width() < minWidth)
	{
		if (m_pressedType == WEST || m_pressedType == NORTH_WEST || m_pressedType == SOUTH_WEST)
		{
			resizeRect.setLeft(resizeRect.right() - minWidth + 1);
		}
		else
		{
			resizeRect.setRight(resizeRect.left() + minWidth - 1);
		}
	}
	if (resizeRect.height() < minHeight)
	{
		if (m_pressedType == NORTH || m_pressedType == NORTH_EAST || m_pressedType == NORTH_WEST)
		{
			resizeRect.setTop(resizeRect.bottom() - minHeight + 1);
		}
		else
		{
			resizeRect.setBottom(resizeRect.top() + minHeight - 1);
		}
	}
	if (resizeRect.width() > maxWidth)
	{
		if (m_pressedType == WEST || m_pressedType == NORTH_WEST || m_pressedType == SOUTH_WEST)
		{
			resizeRect.setLeft(resizeRect.right() - maxWidth + 1);
		}
		else
		{
			resizeRect.setRight(resizeRect.left() + maxWidth - 1);
		}
	}
	if (resizeRect.height() > maxHeight)
	{
		if (m_pressedType == NORTH || m_pressedType == NORTH_EAST || m_pressedType == NORTH_WEST)
		{
			resizeRect.setTop(resizeRect.bottom() - maxHeight + 1);
		}
		else
		{
			resizeRect.setBottom(resizeRect.top() + maxHeight - 1);
		}
	}

	setGeometry(resizeRect);
}
#endif

void COriginalDialog::setTouchBorderWidth(int n)
{
	m_touchBorderWidth = n;
}

int COriginalDialog::touchBorderWidth()
{
	return m_touchBorderWidth;
}

void COriginalDialog::setCustomerTitleBarHeight(int n)
{
	m_customerTitleBarHeight = n;
	m_customerTitleBarRect = QRect(0, 0, width(), n);
}

int COriginalDialog::customerTitleBarHeight()
{
	return m_customerTitleBarHeight;
}

QRect COriginalDialog::customerTitleBarRect()
{
	return m_customerTitleBarRect;
}

void COriginalDialog::setTransientWindow(QWindow* window)
{
	winId();
	QWindow* handle = windowHandle();
	if (handle == nullptr)
	{
		return;
	}

	QWindow* realTransientWindow = getAncestorHandle(window);
	if (realTransientWindow != nullptr)
	{
		realTransientWindow->installEventFilter(this);
	}
	handle->setTransientParent(realTransientWindow);
}

QWindow* COriginalDialog::getAncestorHandle(QWindow* window)
{
	if (window == nullptr)
	{
		return nullptr;
	}

	QWindow* realTransientWindow = nullptr;
#ifdef _MSC_VER
	WId ancetorId = (WId)::GetAncestor(HWND(window->winId()), GA_ROOT);
	QWidget* topLevelWidget = QWidget::find(ancetorId);
	if (topLevelWidget != nullptr)
	{
		realTransientWindow = topLevelWidget->windowHandle();
	}
	else
	{
		for (int32_t i = 0; i < qApp->allWindows().count(); ++i)
		{
			QWindow* tw = qApp->allWindows()[i];
			if (tw != nullptr && tw->winId() == ancetorId)
			{
				realTransientWindow = tw;
				break;
			}
		}
	}
	if (realTransientWindow == nullptr && ::IsWindow(HWND(ancetorId)) == TRUE)
	{
		realTransientWindow = QWindow::fromWinId(ancetorId);
	}
#else
	QWidget* sourceWidget = QWidget::find(window->winId());
	if (sourceWidget != nullptr && sourceWidget->window() != nullptr)
	{
		realTransientWindow = sourceWidget->window()->windowHandle();
	}
	if (realTransientWindow == nullptr)
	{
		realTransientWindow = window;
	}
#endif
	return realTransientWindow;
}

QWindow* COriginalDialog::transientWindow()
{
	QWindow* handle = windowHandle();
	if (handle == nullptr)
	{
		return nullptr;
	}
	return handle->transientParent();
}

bool COriginalDialog::altF4Enable()
{
	return m_altF4Enable;
}

void COriginalDialog::setAltF4Enable(bool enabled)
{
	m_altF4Enable = enabled;
}

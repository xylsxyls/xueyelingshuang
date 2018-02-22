#include "DialogShow.h"
#include "Label.h"
#include <QPainter>
#include <QMouseEvent>
#include "../COriginalButton.h"
#include <QWindow>
#include <Windows.h>
#include "CStringManager.h"
#include "../CGeneralStyle.h"
#include "NotifyDialogManager.h"

std::wstring DialogShow::s_countDownString = L"还有%d秒自动关闭";

DialogShow::DialogShow() :
m_cancelEscAltF4(false),
m_bPressed(false),
m_highLight(false),
m_isExec(false),
m_animation(this, "geometry"),
m_result(-2),
m_userType(-1),
m_inExit(false)
{
	
}

void DialogShow::initForExec(int32_t dialogWidth, int32_t dialogHeight)
{
	m_isExec = true;
	installEventFilter(this);
	setWindowFlags(windowFlags() | Qt::Tool);
	resize(dialogWidth, dialogHeight);//411, 165340, 165
	m_exit = addButton("", QRect(width() - 3 - 30, 3, 30, 30), 0);
	m_exit->setBkgMargins(0, 0);
	m_exit->setBorderRadius(0);
	m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/Dialog/PopupCloseButton.png");
	m_exit->installEventFilter(this);
	m_title = addLabel("title", QRect(17, 4, width() - 17 * 2, 27), QColor(163, 175, 191, 255));
	m_separator = addSeparator(QPoint(13, 33), width() - 13 * 2, true, QColor(16, 20, 31, 255), QColor(46, 52, 88, 255));
	m_time = addLabel("", QRect(width() - 125 - 10, 140, 125, 32), QColor("#abb3d3"));
	m_time->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_time->setFontSize(12);
	QObject::connect(this, &DialogBase::timeRest, this, &DialogShow::timeUpdate);
	setAttribute(Qt::WA_NativeWindow);
}

void DialogShow::initForShow(int32_t dialogWidth, int32_t dialogHeight, const std::string& typeName)
{
	m_isExec = false;

	setStyleSheet(("." + typeName + "{ background-color:rgba(44, 52, 74, 255); border-top-left-radius:2px; border-top-right-radius:2px; border:1px solid; border-color:rgba(78, 146, 212, 255); }").c_str());

	resize(dialogWidth, dialogHeight);
	m_titleBar = addLabel("", QRect(1, 1, width() - 2, 31), QColor(163, 175, 191, 255));
	m_titleBar->setBackgroundColor(QColor(67, 81, 117, 255));
	m_icon = addLabel("", QRect(6, 6, 13, 18), QColor(0, 0, 0, 0));
	m_icon->setParent(m_titleBar);
	m_icon->setBackgroundImage(CGeneralStyle::instance()->platformResourcePath() + "/Common/Image/NotificationView/11Logo.png", 1, 1, 1, 1);
	m_title = addLabel("title", QRect(23, 1, width() - 20 - 34, 28), QColor(163, 175, 191, 255));
	m_title->setParent(m_titleBar);
	m_title->setTextColor(QColor(221, 213, 198, 255));
	m_title->setFontFace(QString::fromStdWString(L"微软雅黑"));
	m_title->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	m_exit = addButton("", QRect(width() - 34, 1, 34, 31), 0);
	m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/Common/Image/NotificationView/CloseButton.png");
	m_exit->setBkgMargins(0, 0);
	m_exit->setBorderRadius(0);
	m_time = addLabel("", QRect(7, height() - 13, width() - 5, 13), QColor("#abb3d3"));
	m_time->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_time->setFontSize(12);

	RECT rect = { 0, 0, 0, 0 };
	::GetWindowRect(::FindWindow(L"Shell_TrayWnd", L""), &rect);
	RECT screenRect = { 0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN) };

	//分4种情况，任务栏可能出现在左侧上侧右侧底侧
	//右侧
	if (rect.left != screenRect.left)
	{
		m_beginRect.setRect(screenRect.right - (rect.right - rect.left) - width(), screenRect.bottom - height(), width(), height());
		m_endRect.setRect(screenRect.right - (rect.right - rect.left) - width(), screenRect.bottom, width(), height());
	}
	//底侧
	else if (rect.top != screenRect.top)
	{
		m_beginRect.setRect(screenRect.right - width(), screenRect.bottom - (rect.bottom - rect.top) - height(), width(), height());
		m_endRect.setRect(screenRect.right - width(), screenRect.bottom, width(), height());
	}
	//上侧左侧和其余情况一律从右下角弹出
	else
	{
		m_beginRect.setRect(screenRect.right - width(), screenRect.bottom - height(), width(), height());
		m_endRect.setRect(screenRect.right - width(), screenRect.bottom, width(), height());
	}

	//m_beginRect.setRect(screenRect.right - width(), screenRect.bottom - (rect.bottom - rect.top) - height(), width(), height());
	//m_endRect.setRect(screenRect.right - width(), screenRect.bottom, width(), height());
	QObject::connect(this, &DialogBase::timeRest, this, &DialogShow::timeUpdate);
	QObject::connect(&m_animation, &QPropertyAnimation::finished, this, &DialogShow::end);
	
}

void DialogShow::setExitVisible(bool visible)
{
	m_exit->setVisible(visible);
	m_cancelEscAltF4 = !visible;
}

void DialogShow::timeUpdate(int32_t timeOut)
{
	if (timeOut <= 0 || m_timeVisible == false)
	{
		m_time->setVisible(false);
	}
	m_time->setText(QString::fromStdWString(CStringManager::Format(s_countDownString.c_str(), timeOut)));
}

void DialogShow::paintEvent(QPaintEvent* eve)
{
	if (m_isExec)
	{
		QLinearGradient gradient(QPointF(0, 0), QPoint(0, height()));
		gradient.setColorAt(0.08, QColor(31, 37, 61, 255));
		gradient.setColorAt(0.50, QColor(29, 33, 50, 255));
		gradient.setColorAt(0.92, QColor(27, 29, 41, 255));

		gradient.setSpread(QGradient::PadSpread);

		QPainter painter(this);

		painter.fillRect(QRect(0, 0, width(), height()), gradient);
		if (m_highLight)
		{
			painter.setPen(QColor("#373e60"));
		}
		else
		{
			painter.setPen(QColor("#4b5586"));
		}
		painter.drawRect(0, 0, width() - 1, height() - 1);
	}

	QDialog::paintEvent(eve);
}

long DialogShow::onNcHitTest(QPoint pt)
{
	QRect rcClient = this->geometry();

	if ((pt.y() - rcClient.top()) <= 40 && m_isExec)
	{
		bool hasChild = false;
		for (int i = 0; i < this->children().count(); i++)
		{
			QWidget* w = qobject_cast<QWidget*>(this->children()[i]);
			if (w == NULL)
				continue;
	
			QPoint mousePt = this->mapFromGlobal(QCursor::pos());
			if (m_exit->geometry().contains(mousePt) && m_exit->isVisible())
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

void DialogShow::keyPressEvent(QKeyEvent* eve)
{
	if ((m_inExit == true) || (m_cancelEscAltF4 && (eve->key() == Qt::Key_Escape)))
	{
		return;
	}
	QDialog::keyPressEvent(eve);
}

void DialogShow::closeEvent(QCloseEvent* eve)
{
	if (m_cancelEscAltF4)
	{
		eve->ignore();
		return;
	}

	QDialog::closeEvent(eve);
}

bool DialogShow::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
	bool res = QDialog::nativeEvent(eventType, message, result);
	if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG")
	{
		MSG* msg = static_cast<MSG*>(message);
		switch (msg->message)
		{
		case WM_NCACTIVATE:
		{
			ncActiveChanged(msg->wParam);
		}
		break;
		case WM_NCHITTEST:
		{
			int xPos = (int)(short)LOWORD(msg->lParam);
			int yPos = (int)(short)HIWORD(msg->lParam);

			*result = onNcHitTest(QPoint(xPos, yPos));

			if (HTERROR != *result)
				return true;
		}
		break;
		default:
			break;
		}
	}
	return res;
}

void DialogShow::showEvent(QShowEvent* eve)
{
	DialogBase::showEvent(eve);
	if (m_isExec == false)
	{
		m_animation.setDuration(500);
		m_animation.setStartValue(m_endRect);
		m_animation.setEndValue(m_beginRect);
		m_animation.start();
	}
	else
	{
		activateWindow();
	}
}

void DialogShow::done(int result)
{
	if (m_isExec == false)
	{
		m_result = result;
		auto childs = children();
		for each (QWidget* var in childs)
		{
			var->setEnabled(false);
		}
		m_inExit = true;
		m_cancelEscAltF4 = true;

		if (m_timeId != -1)
		{
			killTimer(m_timeId);
			m_timeId = -1;
		}
		//DialogBase::done(m_result);

		if (m_result != DESTROY_ALL)
		{
			m_animation.setDuration(250);
			m_animation.setStartValue(m_beginRect);
			m_animation.setEndValue(m_endRect);
			m_animation.start();
		}
		else
		{
			DialogBase::done(m_result);
		}

		int32_t dialogId = NotifyDialogManager::instance().DialogId(this);
		if (dialogId != -1)
		{
			emit dialogDone(NotifyDialogManager::instance().DialogId(this), m_result, m_userType);
		}
		NotifyDialogManager::instance().removeDialog(this);
	}
	else
	{
		DialogManager::instance().removeDialog(this);
		DialogBase::done(result);
	}
}

void DialogShow::end()
{
	//判断-2是防止弹出的结束
	if (m_result != -2)
	{
		NotifyDialogManager::instance().removeDialog(this);
		DialogBase::done(m_result);
		if (m_result != DESTROY_ALL)
		{
			delete this;
		}
	}
}

void DialogShow::ncActiveChanged(int32_t wParam)
{
	m_highLight = !!wParam;
	QDialog::repaint();
}

bool DialogShow::eventFilter(QObject* tar, QEvent* eve)
{
	bool res = DialogBase::eventFilter(tar, eve);
	if (eve->type() == QEvent::KeyPress)
	{
		auto keyEvent = (QKeyEvent*)eve;
		if (keyEvent->key() == Qt::Key_Space || keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
		{
			emit keyboardAccept();
		}
	}
	return res;
}

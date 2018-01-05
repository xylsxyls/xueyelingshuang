#include "DialogShow.h"
#include "Label.h"
#include <QPainter>
#include <QMouseEvent>
#include "../COriginalButton.h"
#include <QWindow>
#include <Windows.h>
#include "CStringManager.h"
#include "../CGeneralStyle.h"
#include "ShowDialogManager.h"

DialogShow::DialogShow() :
m_cancelEscAltF4(false),
m_bPressed(false),
m_highLight(false),
m_isExec(false),
m_animation(this, "geometry"),
m_result(-1),
m_userType(-1)
{
	
}

void DialogShow::initForExec()
{
	m_isExec = true;
	resize(340, 165);
	m_exit = addButton("", QRect(width() - 3 - 30, 3, 30, 30), 0);
	m_exit->setBkgMargins(0, 0);
	m_exit->setBorderRadius(0);
	m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/Dialog/PopupCloseButton.png");
	m_title = addLabel("title", QRect(17, 4, 300, 27), QColor(163, 175, 191, 255));
	m_separator = addSeparator(QPoint(13, 33), 308, true);
	m_time = addLabel("", QRect(205, 140, 125, 32), QColor("#abb3d3"));
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

	m_beginRect.setRect(screenRect.right - width(), screenRect.bottom - (rect.bottom - rect.top) - height(), width(), height());
	m_endRect.setRect(screenRect.right - width(), screenRect.bottom, width(), height());
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
	m_time->setText(QString::fromStdWString(CStringManager::Format(L"还有%d秒自动关闭", timeOut)));
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

void DialogShow::mousePressEvent(QMouseEvent *eve)
{
	if (eve->button() == Qt::LeftButton && m_isExec)
	{
		m_bPressed = true;
		m_point = eve->pos();
	}
}

void DialogShow::mouseMoveEvent(QMouseEvent* eve)
{
	if (m_bPressed)
	{
		move(eve->pos() - m_point + pos());
	}
}

void DialogShow::mouseReleaseEvent(QMouseEvent* eve)
{
	Q_UNUSED(eve);
	m_bPressed = false;
}

void DialogShow::keyPressEvent(QKeyEvent* eve)
{
	if (m_cancelEscAltF4 && (eve->key() == Qt::Key_Escape))
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
		m_animation.setDuration(1000);
		m_animation.setStartValue(m_endRect);
		m_animation.setEndValue(m_beginRect);
		m_animation.start();
	}
}

void DialogShow::done(int result)
{
	if (m_isExec == false)
	{
		m_result = result;
		emit dialogDone(ShowDialogManager::instance().DialogId(this), m_result, m_userType);
		m_animation.setDuration(1000);
		m_animation.setStartValue(m_beginRect);
		m_animation.setEndValue(m_endRect);
		m_animation.start();
	}
	else
	{
		DialogManager::instance().removeDialog(this);
		DialogBase::done(result);
	}
}

void DialogShow::end()
{
	int x = 3;
	if (m_result != -1)
	{
		ShowDialogManager::instance().removeDialog(this);
		DialogBase::done(m_result);
		delete this;
	}
}

void DialogShow::ncActiveChanged(int32_t wParam)
{
	m_highLight = !!wParam;
	QDialog::repaint();
}
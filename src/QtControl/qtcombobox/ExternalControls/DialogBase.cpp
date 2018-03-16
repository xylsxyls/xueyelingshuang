#include "DialogBase.h"
#include "Label.h"
#include "../COriginalButton.h"
#include <QSplitter>
#include "LineEdit.h"
#include <QWindow>
#include "CPasswordInputBox.h"
#include "DialogManager.h"
#include "NotifyDialogManager.h"
#include "../CGeneralStyle.h"
#include <Windows.h>
#include <QApplication>

DialogBase::DialogBase():
m_timeVisible(false),
m_dialogEnum(-1)
{
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
}

int32_t DialogBase::exec(int32_t& dialogId, int32_t timeOut, bool isCountDownVisible)
{
	m_timeRest = timeOut;
	if (m_timeRest == 0)
	{
		return 0;
	}
	m_timeVisible = isCountDownVisible;

	auto oldModality = windowModality();

	QWindow* handle = windowHandle();
	if (handle != nullptr && handle->transientParent())
	{
		setWindowModality(Qt::WindowModal);
	}

	dialogId = DialogManager::instance().setDialog(this);
	int32_t result = QDialog::exec();
	//setWindowModality(oldModality);
	//DialogManager::instance().removeDialog(dialogId);
	return result;
}

void DialogBase::show(int32_t& dialogId, int32_t timeOut, bool isCountDownVisible)
{
	m_timeRest = timeOut;
	if (m_timeRest == 0)
	{
		return;
	}
	m_timeVisible = isCountDownVisible;
	dialogId = NotifyDialogManager::instance().setDialog(this);
	QDialog::show();
	return;
}

Label* DialogBase::addLabel(const QString& text, const QRect& rect, const QColor& textColor)
{
	Label* label = new Label(this);
	if (label == nullptr)
	{
		return nullptr;
	}
	label->setGeometry(rect);
	label->setText(text);
	label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	label->setTextColor(textColor);
	label->setFontFace(CGeneralStyle::instance()->font().family());
	label->setFontSize(14);
	return label;
}

Label* DialogBase::addTip(const QString& text, const QRect& rect, const QColor& textColor)
{
	Label* tip = addLabel(text, rect, textColor);
	if (tip == nullptr)
	{
		return nullptr;
	}
	tip->setWordWrap(true);
	return tip;
}

COriginalButton* DialogBase::addButton(const QString& text, const QRect& rect, int32_t result)
{
	COriginalButton* button = new COriginalButton(this);
	if (button == nullptr)
	{
		return nullptr;
	}
	button->setText(text);
	button->setGeometry(rect);
	button->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/Dialog/PopupButton.png");
	button->setBkgMargins(4, 0);
	button->setBorderRadius(4);
	button->setFontSize(13);
	button->setFontColor(QColor(201, 211, 252));
	m_mapResult[button] = result;
	QObject::connect(button, &COriginalButton::clicked, this, &DialogBase::endDialog);
	return button;
}

Label* DialogBase::addSeparator(const QPoint& point, int32_t length, bool isHorizon, const QColor upColor, const QColor downColor)
{
	Label* label = new Label(this);
	if (label == nullptr)
	{
		return nullptr;
	}

	label->setGeometry(point.x(), point.y(), isHorizon ? length : 2, isHorizon ? 2 : length);
	label->setBackgroundColor(downColor);
	Label* lab = new Label(label);
	if (lab == nullptr)
	{
		return nullptr;
	}
	lab->setGeometry(0, 0, isHorizon ? length : 1, isHorizon ? 1 : length);
	lab->setBackgroundColor(upColor);
	return label;
}

LineEdit* DialogBase::addLineEdit(const QRect& rect, bool isPassword, const QString& defaultText)
{
	LineEdit* lineEdit = nullptr;
	if (isPassword)
	{
		lineEdit = new CPasswordInputBox(this);
	}
	else
	{
		lineEdit = new LineEdit(this);
	}
	if (lineEdit == nullptr)
	{
		return nullptr;
	}
	lineEdit->setText(defaultText);
	lineEdit->setGeometry(rect);
	lineEdit->setBorderWidth(1);
	lineEdit->setBorderColor(QColor(31, 36, 51, 255));
	lineEdit->setBorderRadius(4);
	lineEdit->setFontSize(16);
	lineEdit->setFontFace(QString::fromStdWString(L"΢���ź�"));
	lineEdit->setAlignment(Qt::AlignVCenter);
	lineEdit->setTextOrigin(3);
	lineEdit->setBackgroundImage(CGeneralStyle::instance()->platformResourcePath() + "/Dialog/PopupInputBox.png", 1, 1, 1, 1);
	lineEdit->setTextColor(QColor(0, 0, 0, 255));
	return lineEdit;
}

CPasswordInputBox* DialogBase::addPasswordInputBox(const QRect& rect, const QString& defaultText)
{
	CPasswordInputBox* password = new CPasswordInputBox(this);
	if (password == nullptr)
	{
		return nullptr;
	}
	password->setText(defaultText);
	password->setGeometry(rect);
	return password;
}

void DialogBase::setParentWindow(QWindow* parent)
{
	if (parent != nullptr)
	{
		QWindow* window = windowHandle();
		if (window != nullptr)
		{
			QWindow* windowHandle = nullptr;
			WId topLevelHandle = (WId)::GetAncestor(HWND(parent->winId()), GA_ROOT);
			QWidget* widget = QWidget::find(topLevelHandle);
			if (widget != nullptr)
			{
				setAttribute(Qt::WA_NativeWindow);
				window->setTransientParent(widget->windowHandle());
			}
			else
			{
				foreach(QWindow* window, qApp->allWindows())
				{
					if (window->winId() == topLevelHandle)
					{
						windowHandle = window;
						break;
					}
				}
			}
			if (windowHandle != nullptr)
			{
				setAttribute(Qt::WA_NativeWindow);
				window->setTransientParent(windowHandle);
			}
			return;
		}
	}
	//ȥ��������
	//setAttribute(Qt::WA_ShowModal);
	setWindowModality(Qt::ApplicationModal);
}

void DialogBase::setDialogEnum(int32_t dialogEnum)
{
	m_dialogEnum = dialogEnum;
}

int32_t DialogBase::dialogEnum()
{
	return m_dialogEnum;
}

void DialogBase::endDialog()
{
	//childAt(mapFromGlobal(QWidget::cursor().pos()))
	auto itResult = m_mapResult.find(focusWidget());
	if (itResult != m_mapResult.end())
	{
		done(itResult->second);
	}
}

void DialogBase::showEvent(QShowEvent* eve)
{
	if (m_timeRest > 0)
	{
		m_timeId = startTimer(1000);
		emit timeRest(m_timeRest);
	}

	QDialog::showEvent(eve);
	raise();
}

void DialogBase::timerEvent(QTimerEvent* eve)
{
	--m_timeRest;
	emit timeRest(m_timeRest);
	if (m_timeRest == 0)
	{
		reject();
	}
	QDialog::timerEvent(eve);
}

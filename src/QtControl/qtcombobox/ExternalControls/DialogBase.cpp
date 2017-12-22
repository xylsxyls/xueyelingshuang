#include "DialogBase.h"
#include "Label.h"
#include "../COriginalButton.h"
#include <QSplitter>
#include "LineEdit.h"
#include <QWindow>
#include "CPasswordInputBox.h"

DialogBase::DialogBase()
{
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
}

int32_t DialogBase::exec(int32_t timeOut)
{
	m_timeRest = timeOut;
	if (m_timeRest == 0)
	{
		return 0;
	}
	return QDialog::exec();
}

Label* DialogBase::addLabel(const QString& text, const QRect& rect, const QColor& textColor)
{
	Label* label = new Label(this);
	if (label == nullptr)
	{
		return nullptr;
	}
	label->setGeometry(rect);
	label->setAttribute(Qt::WA_TranslucentBackground);
	label->setText(text);
	label->setAlignment(Qt::AlignVCenter);
	label->setTextColor(textColor);
	label->setFontFace(QString::fromStdWString(L"ºÚÌå"));
	label->setFontSize(14);
	return label;
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
	button->setBkgImage("E:/newClient/11UI/Resource/Image/PopupBox/PopupButton.png");
	button->setBkgMargins(4, 0);
	button->setBorderRadius(4);
	button->setFontSize(13);
	button->setFontColor(QColor(201, 211, 252));
	m_mapResult[button] = result;
	QObject::connect(button, &COriginalButton::clicked, this, &DialogBase::endDialog);
	return button;
}

Label* DialogBase::addSeparator(const QPoint& point, int32_t width, bool isHorizon)
{
	Label* label = new Label(this);
	if (label == nullptr)
	{
		return nullptr;
	}
	label->setGeometry(point.x(), point.y(), width, 2);
	label->setBackgroundColor(QColor(46, 52, 88, 255));
	Label* lab = new Label(label);
	if (lab == nullptr)
	{
		return nullptr;
	}
	lab->setGeometry(0, 0, width, 1);
	lab->setBackgroundColor(QColor(16, 20, 31, 255));
	return label;
}

LineEdit* DialogBase::addLineEdit(const QRect& rect, const QString& defaultText)
{
	LineEdit* lineEdit = new LineEdit(this);
	if (lineEdit == nullptr)
	{
		return nullptr;
	}
	lineEdit->setText(defaultText);
	lineEdit->setGeometry(rect);
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

void DialogBase::setResponseHighlightDialog(QDialog* parent)
{
	QWindow* handle = parent->windowHandle();
	if (handle != nullptr)
	{
		windowHandle()->setTransientParent(handle);
	}
}

void DialogBase::endDialog()
{
	auto itResult = m_mapResult.find(childAt(mapFromGlobal(QWidget::cursor().pos())));
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
}

void DialogBase::timerEvent(QTimerEvent* eve)
{
	--m_timeRest;
	emit timeRest(m_timeRest);
	if (m_timeRest == 0)
	{
		killTimer(m_timeId);
		reject();
	}
	QDialog::timerEvent(eve);
}
#include "CPasswordInputBox.h"
#include "CGeneralStyle.h"
#include "COriginalButton.h"
#include <QRegExpValidator>

CPasswordInputBox::CPasswordInputBox(QWidget *parent):
LineEdit(parent),
m_maskButton(nullptr),
m_rightOrigin(5)
{
	m_maskButton = new COriginalButton(this);
	init();
}

CPasswordInputBox::~CPasswordInputBox()
{

}

void CPasswordInputBox::setDefault()
{
	if (!check())
	{
		return;
	}

	QRegExp regx("[a-zA-Z0-9]+$");
	QValidator *validator = new QRegExpValidator(regx, this);
	if (validator == nullptr)
	{
		return;
	}
	setValidator(validator);

	QString maskPath = CGeneralStyle::instance()->war3lobbyResourcePath() + MASK_BUTTON_PNG;
	setMaskBackgroundImage(maskPath, 6, 4, 5, 6, 6, 1, 2, 3, 3);
	setMaskSize(18, 11);
	m_maskButton->setText(QString::fromStdWString(L""));
	m_maskButton->setCursor(Qt::ClosedHandCursor);
	m_maskButton->setCheckable(true);
	setEchoMode(QLineEdit::Password);
	setFont(CGeneralStyle::instance()->font());
	resize(146, 22);
	setBackgroundColor(QColor(39, 50, 83, 255), QColor(39, 50, 83, 255), QColor(39, 50, 83, 255));
	QColor textColor = CGeneralStyle::instance()->fontColor();
	setTextColor(textColor, textColor, textColor);
	setBorderWidth(1);
	setBorderColor(QColor(67, 81, 117, 255), QColor(67, 81, 117, 255), QColor(67, 81, 117, 255));
	setBorderRadius(5);
	setFontSize(14);
	setTextOrigin(4);
	setTextRightOrigin(25);
	setTextColor(QColor(255, 255, 255, 255), QColor(255, 255, 255, 255), QColor(82, 126, 185, 255));
	QObject::connect(this, &CPasswordInputBox::textChanged, this, &CPasswordInputBox::currentTextChanged);
}

void CPasswordInputBox::setMaskSize(qint32 width, qint32 height)
{
	if (!check())
	{
		return;
	}
	m_maskButton->resize(width, GetInt(height, width));
}

void CPasswordInputBox::setMaskRightOrigin(qint32 rightOrigin)
{
	if (!check())
	{
		return;
	}
	m_rightOrigin = rightOrigin;
	layoutControl();
}

void CPasswordInputBox::setMaskBackgroundImage(const QString& backgroundImgPath,
											   qint32 backgroundImgStateCount,
											   qint32 backgroundImgNormal,
											   qint32 backgroundImgHover,
											   qint32 backgroundImgPressed,
											   qint32 backgroundImgDisabled,
											   qint32 backgroundImgCkNormal,
											   qint32 backgroundImgCkHover,
											   qint32 backgroundImgCkPressed,
											   qint32 backgroundImgCkDisabled)
{
	if (!check())
	{
		return;
	}
	m_maskButton->setBkgImage(backgroundImgPath,
							  backgroundImgStateCount,
							  backgroundImgNormal,
							  backgroundImgHover,
							  backgroundImgPressed,
							  backgroundImgDisabled,
							  backgroundImgCkNormal,
							  backgroundImgCkHover,
							  backgroundImgCkPressed,
							  backgroundImgCkDisabled);
}

void CPasswordInputBox::setMaskVisible(bool enable)
{
	if (!check())
	{
		return;
	}
	m_maskButton->setVisible(enable);
}

bool CPasswordInputBox::check()
{
	return m_maskButton != nullptr;
}

void CPasswordInputBox::init()
{
	if (!check())
	{
		return;
	}
	QObject::connect(m_maskButton, &COriginalButton::clicked, this, &CPasswordInputBox::onMaskButtonClicked);
	setDefault();
}

void CPasswordInputBox::layoutControl()
{
	if (!check())
	{
		return;
	}
	m_maskButton->move(width() - m_maskButton->width() - m_rightOrigin, (height() - m_maskButton->height()) / 2);
}

void CPasswordInputBox::resizeEvent(QResizeEvent* eve)
{
    LineEdit::resizeEvent(eve);
    layoutControl();
}

void CPasswordInputBox::onMaskButtonClicked()
{
	if (!check())
	{
		return;
	}
	bool isChecked = m_maskButton->isChecked();
	setEchoMode(isChecked ? QLineEdit::Normal : QLineEdit::Password);
	setToolTip(isChecked ? text() : "");
}

void CPasswordInputBox::currentTextChanged(const QString& str)
{
	if (!check())
	{
		return;
	}
	setToolTip(m_maskButton->isChecked() ? str : "");
}
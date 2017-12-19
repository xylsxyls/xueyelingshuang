#include "CPasswordInputBox.h"
#include "../CGeneralStyle.h"
#include "../COriginalButton.h"

#define MASK_BUTTON_PNG QString::fromStdWString(L"/Image/Common/Setting/SettingPasswordIcon.png")

CPasswordInputBox::CPasswordInputBox(QWidget *parent):
LineEdit(parent),
m_maskButton(new COriginalButton(this)),
m_rightOrigin(5)
{
	INIT(L"");
	QObject::connect(m_maskButton, &COriginalButton::clicked, this, &CPasswordInputBox::onMaskButtonClicked);
	setDefault();
}

CPasswordInputBox::~CPasswordInputBox()
{

}

void CPasswordInputBox::setDefault()
{
	if (m_maskButton == nullptr)
	{
		return;
	}
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
}

void CPasswordInputBox::setMaskSize(int32_t width, int32_t height)
{
	if (m_maskButton == nullptr)
	{
		return;
	}
	m_maskButton->resize(width, GetInt(height, width));
}

void CPasswordInputBox::setMaskRightOrigin(int32_t rightOrigin)
{
	if (m_maskButton == nullptr)
	{
		return;
	}
	m_rightOrigin = rightOrigin;
	layoutControl();
}

void CPasswordInputBox::setMaskBackgroundImage(const QString& backgroundImgPath,
											   int32_t backgroundImgStateCount,
											   int32_t backgroundImgNormal,
											   int32_t backgroundImgHover,
											   int32_t backgroundImgPressed,
											   int32_t backgroundImgDisabled,
											   int32_t backgroundImgCkNormal,
											   int32_t backgroundImgCkHover,
											   int32_t backgroundImgCkPressed,
											   int32_t backgroundImgCkDisabled)
{
	if (m_maskButton == nullptr)
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

void CPasswordInputBox::layoutControl()
{
	if (m_maskButton == nullptr)
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
	if (m_maskButton == nullptr)
	{
		return;
	}
	setEchoMode((m_maskButton->isChecked()) ? QLineEdit::Normal : QLineEdit::Password);
}
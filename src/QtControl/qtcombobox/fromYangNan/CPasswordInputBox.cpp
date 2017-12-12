#include "CPasswordInputBox.h"
#include "../CGeneralStyle.h"
#include "../COriginalButton.h"
#include <QPainter>
#include <QStyle>

CPasswordInputBox::CPasswordInputBox(QWidget *parent):
LineEdit(parent),
m_maskButton(new COriginalButton(this))
{
    QObject::connect(m_maskButton, &COriginalButton::clicked, this, &CPasswordInputBox::onMaskButtonClicked);
    this->setStyleSheet(QString(".CPasswordInputBox{"
								"background-color:rgba(39, 50, 83, 255);"
								"color:%1;border:1px solid;border-color:rgba(67,81,117,255);"
								"border-radius:5px;"
								"font-size:14px;"
								"padding-left:4px;"
                                "padding-right: 25;"
                                "}").arg(CGeneralStyle::instance()->fontColor().name()));
	setDefault();
}

CPasswordInputBox::~CPasswordInputBox()
{

}

void CPasswordInputBox::setDefault()
{
	QString maskPath = CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/Common/Setting/SettingPasswordIcon.png";
	m_maskButton->setBkgImage(maskPath, 6, 4, 5, 6, 6, 1, 2, 3, 3);
	m_maskButton->resize(18, 11);
	m_maskButton->setCursor(Qt::ClosedHandCursor);
	m_maskButton->setCheckable(true);
	setEchoMode(QLineEdit::Password);
	setFont(CGeneralStyle::instance()->font());
	setMinimumSize(146, 22);
	setMaximumSize(146, 22);
}

void CPasswordInputBox::layoutControl()
{
    m_maskButton->move(width() - m_maskButton->width() - 5, (height() - m_maskButton->height()) / 2);
}

void CPasswordInputBox::resizeEvent(QResizeEvent* eve)
{
    LineEdit::resizeEvent(eve);
    layoutControl();
}

void CPasswordInputBox::onMaskButtonClicked()
{
	setEchoMode((m_maskButton->isChecked()) ? QLineEdit::Normal : QLineEdit::Password);
}


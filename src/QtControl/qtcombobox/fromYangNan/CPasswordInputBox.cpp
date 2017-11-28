#include "CPasswordInputBox.h"
#include "../CGeneralStyle.h"
#include <QPainter>
#include <QStyle>

CPasswordInputBox::CPasswordInputBox(QWidget *parent)
    :QLineEdit(parent)
    ,mMaskButton(new COriginalButton(this))
{
    mMaskButton->setText("");
	QString maskPath = CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/Common/Setting/SettingPasswordIcon.png";
	mMaskButton->setBkgImage(maskPath, 6, 4, 5, 6, 6, 1, 2, 3, 3);
    mMaskButton->resize(18,11);
    mMaskButton->setCursor(Qt::ClosedHandCursor);
    mMaskButton->setCheckable(true);
    QObject::connect(mMaskButton, &COriginalButton::clicked, this, &CPasswordInputBox::onMaskButtonClicked);

    this->setFont(CGeneralStyle::instance()->font());
    this->setStyleSheet(QString(".CPasswordInputBox{"
								"background-color:rgba(39, 50, 83, 255);"
								"color:%1;border:1px solid;border-color:rgba(67,81,117,255);"
								"border-radius:5px;"
								"font-size:14px;"
								"padding-left:4px;"
                                "padding-right: 25;"
                                "}").arg(CGeneralStyle::instance()->fontColor().name()));
    this->setEchoMode(QLineEdit::Password);
    this->setMinimumSize(146,22);
    this->setMaximumSize(146,22);
}

CPasswordInputBox::~CPasswordInputBox()
{

}

void CPasswordInputBox::layoutControl()
{
    //mask button
    mMaskButton->move(this->width() - mMaskButton->width() - 5, (this->height() - mMaskButton->height()) / 2);
}

void CPasswordInputBox::resizeEvent(QResizeEvent *e)
{
    QLineEdit::resizeEvent(e);

    this->layoutControl();
}

void CPasswordInputBox::onMaskButtonClicked()
{
    if(mMaskButton->isChecked())
    {
        this->setEchoMode(QLineEdit::Normal);
    }
    else
    {
        this->setEchoMode(QLineEdit::Password);
    }
}


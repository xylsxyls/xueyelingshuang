#include "CPasswordInputBox2.h"

#include "CGeneralStyle2.h"

#include <QPainter>
#include <QStyle>

CPasswordInputBox2::CPasswordInputBox2(QWidget *parent)
    :QLineEdit(parent)
    ,mMaskButton(new COriginalButton2(this))
{
    mMaskButton->setText("");
    mMaskButton->setBkgImage(QString::fromStdWString(L"D:/GameInfoWidgetPic/SettingPasswordIcon.png"),6,4,5,6,6,1,2,3,3);
    mMaskButton->resize(18,11);
    mMaskButton->setCursor(Qt::ClosedHandCursor);
    mMaskButton->setCheckable(true);
    QObject::connect(mMaskButton, &COriginalButton2::clicked, this, &CPasswordInputBox2::onMaskButtonClicked);

    this->setFont(CGeneralStyle2::instance()->font());
	this->setStyleSheet(QString(".CPasswordInputBox2{"
		"background-image:url(%1);"
		"color:%2;border:1px solid;border-color:rgba(67,81,117,255);"
		"border-radius:5px;"
		"font-size:14px;"
                                "padding-right: 25;"
                                "}").arg(CGeneralStyle2::instance()->war3lobbyResourcePath() + "/Image/Common/Setting/SettingBox.png")
                                                                                                     .arg(CGeneralStyle2::instance()->fontColor().name()));
    this->setEchoMode(QLineEdit::Password);
    this->setMinimumSize(146,22);
    this->setMaximumSize(146,22);
}

CPasswordInputBox2::~CPasswordInputBox2()
{

}

void CPasswordInputBox2::layoutControl()
{
    //mask button
    mMaskButton->move(this->width() - mMaskButton->width() - 5, (this->height() - mMaskButton->height()) / 2);
}

void CPasswordInputBox2::resizeEvent(QResizeEvent *e)
{
    QLineEdit::resizeEvent(e);

    this->layoutControl();
}

void CPasswordInputBox2::onMaskButtonClicked()
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


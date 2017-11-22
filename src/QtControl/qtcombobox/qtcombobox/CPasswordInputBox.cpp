#include "CPasswordInputBox.h"

#include "CGeneralStyle.h"

#include <QPainter>
#include <QStyle>

CPasswordInputBox::CPasswordInputBox(QWidget *parent)
    :QLineEdit(parent)
    ,mMaskButton(new COriginalButton(this))
{
    mMaskButton->setText("");
    mMaskButton->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/Common/Setting/SettingPasswordIcon.png",6,4,5,6,6,1,2,3,3);
    mMaskButton->resize(18,11);
    mMaskButton->setCursor(Qt::ClosedHandCursor);
    mMaskButton->setCheckable(true);
    connect(mMaskButton, &COriginalButton::clicked, this, &CPasswordInputBox::onMaskButtonClicked);

    this->setFont(CGeneralStyle::instance()->font());
    this->setStyleSheet(QString(".CPasswordInputBox{"
                                "background-image:url(%1);"
                                "color:%2;border:none;"
                                "padding-right: 25;"
                                "}").arg(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/Common/Setting/SettingBox.png")
                                                                                                     .arg(CGeneralStyle::instance()->fontColor().name()));
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


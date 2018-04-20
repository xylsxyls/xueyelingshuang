#include "DialogHelper.h"
#include "Label.h"
#include "CGeneralStyle.h"
#include "COriginalButton.h"
#include <map>
#include "DialogShow.h"
#include "LineEdit.h"
#include "CPasswordInputBox.h"
#include <QWindow>

void DialogHelper::setLabel(Label* label, const QString& text, const QColor& textColor, int32_t fontSize)
{
    if (label == nullptr)
    {
        return;
    }
    label->setText(text);
    label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    label->setTextColor(textColor);
    label->setFontFace(CGeneralStyle::instance()->font().family());
    label->setFontSize(fontSize);
}

void DialogHelper::setTip(Label* tip, const QString& text, const QColor& textColor, int32_t fontSize)
{
    setLabel(tip, text, textColor, fontSize);
    if (tip == nullptr)
    {
        return;
    }
    tip->setWordWrap(true);
}

void DialogHelper::setButton(DialogShow* dialog,
                             COriginalButton* button,
                             const QString& text,
                             const QColor& textColor,
                             const QString& backgroundImage,
                             int32_t result,
                             int32_t fontSize,
                             std::map<QWidget*, int32_t>* mapResult,
                             int32_t imageMargin)
{
    if (button == nullptr)
    {
        return;
    }
    button->setText(text);
    button->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/Dialog/PopupButton.png");
    button->setBkgMargins(imageMargin, 0);
    button->setBorderRadius(4);
    button->setFontSize(fontSize);
    button->setFontColor(textColor);
    if (mapResult != nullptr)
    {
        (*mapResult)[button] = result;
        QObject::connect(button, &COriginalButton::clicked, dialog, &DialogShow::endDialog);
    }
    
}

//Label* DialogHelper::addSeparator(const QPoint& point, int32_t length, bool isHorizon, const QColor upColor, const QColor downColor)
//{
//    Label* label = new Label(this);
//    if (label == nullptr)
//    {
//        return nullptr;
//    }
//
//    label->setGeometry(point.x(), point.y(), isHorizon ? length : 2, isHorizon ? 2 : length);
//    label->setBackgroundColor(downColor);
//    Label* lab = new Label(label);
//    if (lab == nullptr)
//    {
//        return nullptr;
//    }
//    lab->setGeometry(0, 0, isHorizon ? length : 1, isHorizon ? 1 : length);
//    lab->setBackgroundColor(upColor);
//    return label;
//}

LineEdit* DialogHelper::initLineEdit(DialogShow* dialog, bool isPassword, const QString& defaultText, int32_t maxLength)
{
    LineEdit* lineEdit = nullptr;
    if (isPassword)
    {
        lineEdit = new CPasswordInputBox(dialog);
    }
    else
    {
        lineEdit = new LineEdit(dialog);
    }
    if (lineEdit == nullptr)
    {
        return nullptr;
    }
    lineEdit->setText(defaultText);
    lineEdit->setBorderWidth(1);
    lineEdit->setBorderColor(QColor(31, 36, 51, 255));
    lineEdit->setBorderRadius(4);
    lineEdit->setFontSize(16);
    lineEdit->setFontFace(QString::fromStdWString(L"Î¢ÈíÑÅºÚ"));
    lineEdit->setAlignment(Qt::AlignVCenter);
    lineEdit->setTextOrigin(3);
    lineEdit->setBackgroundImage(CGeneralStyle::instance()->platformResourcePath() + "/Dialog/PopupInputBox.png", 1, 1, 1, 1);
    lineEdit->setTextColor(QColor(0, 0, 0, 255));
    if (maxLength > 0)
    {
        lineEdit->setMaxLength(maxLength);
    }
    return lineEdit;
}

//CPasswordInputBox* DialogHelper::addPasswordInputBox(const QRect& rect, const QString& defaultText)
//{
//    CPasswordInputBox* password = new CPasswordInputBox(this);
//    if (password == nullptr)
//    {
//        return nullptr;
//    }
//    password->setText(defaultText);
//    password->setGeometry(rect);
//    return password;
//}

void DialogHelper::activeWindow(QWindow* window)
{
    if (window == nullptr)
    {
        return;
    }

    QWindow* transParent = window->transientParent();
    if (transParent == nullptr)
    {
        return;
    }
    transParent->requestActivate();
    window->setTransientParent(nullptr);
}
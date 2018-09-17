#include "DialogHelper.h"
#include "Label.h"
#include "CGeneralStyle.h"
#include "COriginalButton.h"
#include "CPasswordInputBox.h"
#include <QWindow>
#include "Separator.h"
#include "PopDialog.h"
#include "NotifyDialog.h"

std::ofstream* g_11ClientLogFile = nullptr;
std::ofstream* g_war3LogFile = nullptr;

void DialogHelper::setLabel(Label* label, const QString& text, const QColor& textColor, qint32 fontSize)
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

void DialogHelper::setTip(Label* tip, const QString& text, const QColor& textColor, qint32 fontSize)
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
                             DialogResult result,
                             qint32 fontSize,
                             std::map<QWidget*, DialogResult>* mapResult,
                             qint32 imageMargin,
                             bool isPop)
{
    if (button == nullptr)
    {
        return;
    }
    button->setText(text);
    button->setBkgImage(backgroundImage);
    button->setBkgMargins(imageMargin, 0);
    button->setBorderRadius(4);
    button->setFontSize(fontSize);
    button->setFontColor(textColor);
    if (mapResult != nullptr)
    {
        (*mapResult)[button] = result;
        if (isPop)
        {
            QObject::connect(button, &COriginalButton::clicked, (PopDialog*)dialog, &PopDialog::endDialog);
        }
        else
        {
            QObject::connect(button, &COriginalButton::clicked, (NotifyDialog*)dialog, &NotifyDialog::prepareExit);
        }
    }
}

void DialogHelper::setSeparator(Separator* separator, bool isHorizon, const QColor& upColor, const QColor& downColor)
{
    if (separator == nullptr)
    {
        return;
    }

    separator->setDisplayMode(isHorizon);
    separator->setColor(upColor, downColor);
    return;
}

void DialogHelper::setLineEdit(LineEdit* lineEdit, const QString& defaultText, qint32 maxLength)
{
    if (lineEdit == nullptr)
    {
        return;
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
    return;
}

void DialogHelper::setPasswordInputBox(CPasswordInputBox* passwordInputBox, const QString& defaultText, qint32 maxLength)
{
    setLineEdit(passwordInputBox, defaultText, maxLength);
}

void DialogHelper::activeTransientParentWindow(QWindow* window)
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

void DialogHelper::setLogPathFrom11Client(const std::string& path)
{
	g_11ClientLogFile = new std::ofstream(path + "/11_controls.log", std::ios::app);
}

void DialogHelper::setLogPathFromWar3(const std::string& path)
{
	g_war3LogFile = new std::ofstream(path + "/11_controls.log", std::ios::app);
}

std::ofstream& DialogHelper::logFile()
{
	if (g_11ClientLogFile != nullptr)
	{
		return *g_11ClientLogFile;
	}
	else if (g_war3LogFile != nullptr)
	{
		return *g_war3LogFile;
	}
	static std::ofstream expFile("11_controls.log", std::ios::app);
	return expFile;
}
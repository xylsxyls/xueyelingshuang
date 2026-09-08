#include "DialogHelper.h"
#include "QtControls/Label.h"
#include "QtControls/ControlStyleManager.h"
#include "QtControls/PushButton.h"
#include "QtControls/CPasswordInputBox.h"
#include <QWindow>
#include "QtControls/Separator.h"
#include "PopDialog.h"
#include "NotifyDialog.h"
#include "CSystem/CSystemAPI.h"

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
    label->setFontFace(ControlStyleManager::instance().defaultFont().family());
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
                             PushButton* button,
                             const QString& text,
                             const QColor& textColor,
                             const QString& backgroundImage,
                             DialogResult result,
                             qint32 fontSize,
                             std::map<QWidget*, DialogResult>* mapResult,
                             qint32 imageMargin,
                             bool isPop)
{
    if (dialog == nullptr || button == nullptr)
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
			PopDialog* popDialog = qobject_cast<PopDialog*>(dialog);
			if (popDialog != nullptr)
			{
				QObject::connect(button, &PushButton::clicked, popDialog, &PopDialog::endDialog);
			}
        }
        else
        {
			NotifyDialog* notifyDialog = qobject_cast<NotifyDialog*>(dialog);
			if (notifyDialog != nullptr)
			{
				QObject::connect(button, &PushButton::clicked, notifyDialog, &NotifyDialog::prepareExit);
			}
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
    lineEdit->setFontFace(ControlStyleManager::instance().defaultFont().family());
    lineEdit->setAlignment(Qt::AlignVCenter);
    lineEdit->setTextOrigin(3);
    lineEdit->setBackgroundImage(ControlStyleManager::instance().resourcePath("res/Dialog/PopupInputBox.png"), 1, 1, 1, 1);
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
	delete g_11ClientLogFile;
	g_11ClientLogFile = nullptr;
	g_11ClientLogFile = new std::ofstream(path + "/11_controls.log", std::ios::app);
}

void DialogHelper::setLogPathFromWar3(const std::string& path)
{
	delete g_war3LogFile;
	g_war3LogFile = nullptr;
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

QRect DialogHelper::rectValid(const QRect& rect)
{
	return CSystem::qrectValid<QRect>(rect);
}
#include "PdfReaderControlHelper.h"
#include "QtControls/PushButton.h"
#include <QApplication>

void PdfReaderControlHelper::configureButton(PushButton* button, const Config& config)
{
    button->setMinimumSize(config.buttonSize);
    button->setBkgColor(config.buttonNormal, config.buttonHover, config.buttonPressed, config.buttonDisabled);
    button->setFontColor(config.buttonTextColor, config.buttonTextColor, config.buttonTextColor, config.buttonTextColor);
    button->setFontFace(QApplication::font().family());
    button->setBorderRadius(config.buttonRadius);
    button->setClickBreathTime(0);
    button->setAutoDefault(false);
}

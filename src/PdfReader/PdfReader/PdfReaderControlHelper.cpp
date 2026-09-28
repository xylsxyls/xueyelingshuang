#include "PdfReaderControlHelper.h"
#include "QtControls/PushButton.h"
#include <QApplication>

void PdfReaderControlHelper::configureButton(PushButton* button)
{
    button->setMinimumSize(g_config.m_buttonSize);
    button->setBkgColor(g_config.m_buttonNormal, g_config.m_buttonHover, g_config.m_buttonPressed, g_config.m_buttonDisabled);
    button->setFontColor(g_config.m_buttonTextColor, g_config.m_buttonTextColor, g_config.m_buttonTextColor, g_config.m_buttonTextColor);
    button->setFontFace(QApplication::font().family());
    button->setBorderRadius(g_config.m_buttonRadius);
    button->setClickBreathTime(0);
    button->setAutoDefault(false);
}
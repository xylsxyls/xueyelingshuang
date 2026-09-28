#include "PdfReaderDialogParam.h"

PdfReaderDialogParam::PdfReaderDialogParam() :
CustomDialogParam(static_cast<DialogType>(g_config.m_dialogType)),
m_mode(PdfReaderDialogMessage),
m_password(false),
m_titleClose(false),
m_about(false),
m_value(new QString)
{
    m_hasShadow = g_config.m_dialogShadowEnabled;
    m_shadowSize = g_config.m_dialogShadowSize;
    m_titleBarHeight = g_config.m_dialogTitleBarHeight;
}
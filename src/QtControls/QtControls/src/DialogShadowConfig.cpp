#include "DialogShadowConfig.h"

DialogShadowConfig::DialogShadowConfig() :
m_maximumSize(16),
m_broadSigmaFactor(4.0),
m_downwardOffsetFactor(2.0),
m_cutoffSigma(3.0),
m_contactOffset(1.0),
m_broadOpacity(0.28),
m_contactOpacity(0.12),
m_outlineWidth(2),
m_outlineColor(Qt::black),
m_color(20, 24, 32),
m_borderColor(151, 156, 166),
m_maxImagePixels(64000000)
{
}
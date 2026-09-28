#include "DialogShadowConfig.h"

const qint32 DialogShadowConfig::kMaximumSize = 16;
const qreal DialogShadowConfig::kBroadSigmaFactor = 4.0;
const qreal DialogShadowConfig::kDownwardOffsetFactor = 2.0;
const qreal DialogShadowConfig::kCutoffSigma = 3.0;
const qreal DialogShadowConfig::kContactOffset = 1.0;
const qreal DialogShadowConfig::kBroadOpacity = 0.28;
const qreal DialogShadowConfig::kContactOpacity = 0.12;
const qint32 DialogShadowConfig::kOutlineWidth = 2;

QColor DialogShadowConfig::color()
{
    return QColor(20, 24, 32);
}

QColor DialogShadowConfig::borderColor()
{
    return QColor(151, 156, 166);
}
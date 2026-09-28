#pragma once
#include <QtCore/QtGlobal>
#include <QtGui/QColor>

/** 弹窗投影样式；不可变默认值，不依赖应用或保存窗口状态 */
class DialogShadowConfig
{
public:
    // 阴影扩散级别上限
    static const qint32 kMaximumSize;
    // 主阴影标准差倍率
    static const qreal kBroadSigmaFactor;
    // 主阴影向下偏移倍率
    static const qreal kDownwardOffsetFactor;
    // 透明边缘距离的标准差倍数
    static const qreal kCutoffSigma;
    // 接触阴影下偏移逻辑像素
    static const qreal kContactOffset;
    // 主阴影不透明度
    static const qreal kBroadOpacity;
    // 接触阴影不透明度
    static const qreal kContactOpacity;
    // 移动目标框线宽
    static const qint32 kOutlineWidth;

    /** 返回中性灰投影色
    @return 不含渐变透明度的基础色
    */
    static QColor color();

    /** 返回启用阴影时的主体轮廓色
    @return 不透明轮廓颜色
    */
    static QColor borderColor();
};
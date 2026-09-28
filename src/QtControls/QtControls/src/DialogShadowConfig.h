#pragma once
#include <QtCore/QtGlobal>
#include <QtGui/QColor>

/** 弹窗投影默认配置；按需构造，不在 main 前初始化 Qt 资源
*/
class DialogShadowConfig
{
public:
    /** 构造当前阴影效果的默认参数，不创建窗口
    */
    DialogShadowConfig();
    // DialogShadow接收阴影扩散级别时的上限，超过此值按上限处理；传入0仍表示关闭阴影
    qint32 m_maximumSize;
    // 主阴影高斯标准差相对扩散级别的倍数，标准差用于计算四周模糊覆盖范围，单位为逻辑像素
    qreal m_broadSigmaFactor;
    // 主阴影向下偏移量相对扩散级别的倍数；计算得到的偏移使用逻辑像素
    qreal m_downwardOffsetFactor;
    // 生成主阴影缓存时保留的标准差倍数，与向下偏移一起计算主体四周所需透明边距
    qreal m_cutoffSigma;
    // 贴近弹窗的接触阴影相对主体向下移动的距离，逻辑像素，不随主阴影偏移倍率计算
    qreal m_contactOffset;
    // 主阴影高斯覆盖值合成到输出Alpha时的权重，0..1；数值越大远处阴影越深
    qreal m_broadOpacity;
    // 接触阴影高斯覆盖值合成到输出Alpha时的权重，0..1；与主阴影权重相加生成最终透明度
    qreal m_contactOpacity;
    // 系统关闭实时拖窗内容时，拖动预览矩形框的画笔宽度，逻辑像素
    qint32 m_outlineWidth;
    // 拖动预览矩形框的线条颜色，不是正常弹窗投影颜色
    QColor m_outlineColor;
    // 生成阴影缓存时采用的RGB基础色；输出Alpha由两层高斯覆盖和不透明度权重计算
    QColor m_color;
    // DialogBase启用阴影时绘制主体一像素轮廓的颜色，用于区分弹窗边缘与外部投影
    QColor m_borderColor;
    // 按屏幕像素倍率生成阴影缓存的最大像素总数，超限时不分配缓存并保留主体边框
    qint64 m_maxImagePixels;
};
#pragma once
#include "QtControlsMacro.h"
#include <QColor>

/** 可选桌面标题栏样式；旧MainWindow与旧弹窗不受影响。 */
struct QtControlsAPI WindowTitleBarConfig
{
    /** 初始化通用浅色标题样式。 */
    WindowTitleBarConfig();
    // 标题及操作按钮的逻辑像素尺寸。
    int m_height;
    int m_buttonSize;
    int m_iconSize;
    // 布局间距及横向留白，逻辑像素。
    int m_spacing;
    int m_margin;
    // 标题字体像素及边框缩放命中宽度。
    int m_fontSize;
    int m_resizeBorder;
    // 窗口控制图标逻辑尺寸。
    int m_controlIconSize;
    // 控制图标笔画宽度。
    double m_controlStroke;
    // 控制按钮的悬停圆角。
    int m_controlRadius;
    // 背景、图标与普通悬停/关闭悬停颜色。
    QColor m_background;
    QColor m_foreground;
    QColor m_hover;
    QColor m_closeHover;
};
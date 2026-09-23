#pragma once
#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

class SplitViewerIconHelper
{
public:
    /** 绘制蓝色问号关于图标。
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static QIcon aboutIcon();
};
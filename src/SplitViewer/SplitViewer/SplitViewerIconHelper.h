#pragma once
#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

/** 生成工具栏及标题关闭图标，不创建或管理窗口；产品图片来自PNG资源
*/
class SplitViewerIconHelper
{
public:
    /** 绘制蓝色问号关于图标
    @return 绘制蓝色问号关于图标
    */
    static QIcon aboutIcon();
};
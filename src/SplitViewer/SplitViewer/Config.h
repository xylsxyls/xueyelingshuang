#pragma once
#include <QtCore/QString>
#include <QtCore/QSize>
#include <stdint.h>

/** 桌面显示常量，不包含Core状态或平台窗口所有权 */
class Config
{
public:
    // DialogManager自定义文件选择内容类型，和其他产品的类型分离
    static const uint64_t kFileDialogType = 10732;

    /** 返回主窗口及子控件共用的QSS
    @return 主窗口样式文本
    */
    static QString windowStyle();

    /** 返回托管文件选择窗口的QSS
    @return 文件选择窗口样式文本
    */
    static QString fileDialogStyle();

    /** 返回文件选择窗口的默认尺寸
    @return 逻辑像素尺寸
    */
    static QSize fileDialogSize();
};
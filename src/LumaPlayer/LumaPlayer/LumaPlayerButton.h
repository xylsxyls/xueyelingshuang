#pragma once
#include "QtControls/PushButton.h"

/** 关于框按钮样式组合，通用状态绘制复用PushButton，仅关闭图标由产品绘制
*/
class LumaPlayerButton : public PushButton
{
public:
    /** 创建关闭按钮或普通蓝色确认按钮
    @param [in] closeButton true绘制红底关闭图标
    @param [in] parent Qt父控件
    */
    explicit LumaPlayerButton(bool closeButton, QWidget* parent);

protected:
    /** 复用基础按钮绘制，仅叠加产品关闭图标
    @param [in] event Qt绘制事件，借用到返回
    */
    virtual void paintEvent(QPaintEvent* event);

private:
    // 是否使用关闭按钮样式
    bool m_closeButton;
};
#pragma once
#include <QPushButton>

/** 应用自绘按钮，复用Config图标和按下颜色，Qt父子关系管理生命周期
*/
class LumaPlayerButton : public QPushButton
{
public:
    /** 创建关闭按钮或普通蓝色确认按钮
    @param [in] closeButton true绘制红底关闭图标
    @param [in] parent Qt父控件
    */
    explicit LumaPlayerButton(bool closeButton, QWidget* parent);

protected:
    /** 绘制统一底色、按下蒙层和图标或文字
    @param [in] event Qt绘制事件，借用到返回
    */
    virtual void paintEvent(QPaintEvent* event);

private:
    // 是否使用关闭按钮样式
    bool m_closeButton;
};
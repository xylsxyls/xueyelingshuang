#pragma once
#include "DialogManagerMacro.h"
#include "NotifyDialog.h"

/** 通知式提示框，展示提示文本并提供单个确认按钮
*/
class TipShowDialog : public NotifyDialog
{
	Q_OBJECT
public:
    /** 构造函数
    */
	TipShowDialog();

public:
    /** 设置提示
    @param [in] tip 提示内容
    */
    void setTip(const QString& tip);

    /** 设置确定按钮文字内容和按下的返回值
    @param [in] acceptText 按钮文字内容
    @param [in] acceptDone 按下的返回值
    */
    void setAcceptButton(const QString& acceptText, DialogResult acceptDone);

protected:
    /** 调整提示文本和确认按钮布局
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 检查内部控件是否已经创建完成
    @return 返回true表示控件有效
    */
    bool check();

private:
	// 提示文本
	Label* m_tip;

	// 确定按钮
	PushButton* m_accept;
};
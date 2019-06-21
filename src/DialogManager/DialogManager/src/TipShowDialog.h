#pragma once
#include "DialogManagerMacro.h"
#include "NotifyDialog.h"

/** 显示提示框
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
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	Label* m_tip;
	COriginalButton* m_accept;
};
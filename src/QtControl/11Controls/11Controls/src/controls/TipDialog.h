#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"
#include "PopDialog.h"

/** 提示弹出框
*/
class TipDialog : public PopDialog
{
	Q_OBJECT
public:
    /** 构造函数
    */
	TipDialog();

public:
    /** 设置提示
    @param [in] tip 提示内容
    */
    void setTip(const QString& tip);

    /** 设置确定按钮内容
    @param [in] acceptText 确定按钮文字内容
    @param [in] acceptDone 确定按钮按下返回值
    */
    void setAcceptButton(const QString& acceptText, DialogResult acceptDone);

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	Label* m_tip;
	COriginalButton* m_accept;
};
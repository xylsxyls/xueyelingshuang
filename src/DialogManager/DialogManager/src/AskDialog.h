#pragma once
#include "DialogManagerMacro.h"
#include "PopDialog.h"

/** 询问弹出框
*/
class AskDialog : public PopDialog
{
	Q_OBJECT
public:
    /** 构造函数
    */
    AskDialog();

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

    /** 设置取消按钮文字内容和按下的返回值
    @param [in] ignoreText 按钮文字内容
    @param [in] ignoreDone 按下的返回值
    */
    void setIgnoreButton(const QString& ignoreText, DialogResult ignoreDone);

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	Label* m_tip;
	COriginalButton* m_accept;
	COriginalButton* m_ignore;
};
#pragma once
#include "PopDialog.h"

class LineEdit;
class CPasswordInputBox;
/** 输入弹出框
*/
class InputDialog : public PopDialog
{
	Q_OBJECT
public:
    /** 构造函数
    */
	InputDialog();

public:
    /** 设置输入框内容
    @param [in] defaultText 输入框默认填入内容
    @param [out] editText 弹框关闭时通过该指针给出输入框内容
    @param [in] maxLength 输入框可输入的最大长度
    */
    void setLineEdit(const QString& defaultText, QString* editText, qint32 maxLength = -1);

    /** 设置密码框（与输入框不共存）
    @param [in] defaultText 输入框默认填入内容
    @param [out] editText 弹框关闭时通过该指针给出输入框内容
    @param [in] maxLength 输入框可输入的最大长度
    */
    void setPasswordInputBox(const QString& defaultText, QString* editText, qint32 maxLength = -1);

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
    void closeEvent(QCloseEvent* eve);
    bool check();

private:
	Label* m_editTip;
	COriginalButton* m_accept;
	LineEdit* m_edit;
    CPasswordInputBox* m_passwordInputBox;
	QString* m_editText;
	bool m_isPassword;
};
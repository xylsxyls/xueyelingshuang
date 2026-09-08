#pragma once
#include "PopDialog.h"
#include "DialogType.h"

class LineEdit;
class CPasswordInputBox;
/** 输入弹出框，支持普通输入、密码输入和多项扩展输入
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

	/** 设置扩展内容
	@param [in] inputEx 扩展内容
	*/
	void setInputEx(std::vector<InputEx>* inputEx);

protected:
    /** 调整提示、输入框和按钮布局
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 关闭窗口时将输入内容写回调用方提供的输出对象
    @param [in] eve 关闭事件
    */
    void closeEvent(QCloseEvent* eve);

    /** 检查内部控件是否已经创建完成
    @return 返回true表示控件有效
    */
    bool check();

private:
	// 输入提示文本
	Label* m_editTip;

	// 确定按钮
	PushButton* m_accept;

	// 普通文本输入框
	LineEdit* m_edit;

    // 密码输入框
    CPasswordInputBox* m_passwordInputBox;

	// 单项输入模式下的输出文本指针，不负责释放
	QString* m_editText;

	// 扩展输入配置列表，不负责释放
	std::vector<InputEx>* m_inputEx;

	// 扩展输入模式下创建的提示和输入控件
	std::vector<std::pair<Label*, LineEdit*>> m_inputExControls;

	// 当前是否处于密码输入模式
	bool m_isPassword;
};
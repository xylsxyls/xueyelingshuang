#pragma once
#include "BoxDialogBase.h"
#include "DialogManager.h"
#include "ControlsMacro.h"

class Label;
class ComboBox;
class LineEdit;
/** 添加账号框
*/
class ControlsAPI AccountDialog : public BoxDialogBase
{
	Q_OBJECT
public:
	/** 构造函数
	*/
	AccountDialog();

	/** 设置账号错误提示是否显示
	@param [in] visible 是否显示
	*/
	void setErrorVisible(bool visible);

	/** 清空输入框
	*/
	void clearAccountEdit();

	/** 获取输入框内容
	@return 返回输入框内容
	*/
	QString accountEditText();

    /** 设置确定按钮按下的返回值
    @param [in] result 确定按钮按下的返回值
    */
    void setAcceptDown(int32_t result);

    /** 设置取消按钮按下的返回值
    @param [in] result 取消按钮按下的返回值
    */
    void setIgnoreDown(int32_t result);

Q_SIGNALS:
    /** 错误提示是否显示
    @param [in] visible 是否显示
    */
	void errorVisible(bool visible);

protected:
	bool eventFilter(QObject* tar, QEvent* eve);
    void resizeEvent(QResizeEvent* eve);
    bool check();

private slots:
	void accountAccept(QObject* tar, Qt::Key key);

private:
	Label* m_sep;
	Label* m_registerAlt;
	LineEdit* m_account;
	Label* m_errorAccount;
	COriginalButton* m_accept;
	COriginalButton* m_ignore;
};
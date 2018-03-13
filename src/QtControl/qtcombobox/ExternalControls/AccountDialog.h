#pragma once
#include "BoxDialogBase.h"
#include "DialogManager.h"

class Label;
class ComboBox;
class AccountDialog : public BoxDialogBase
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口句柄
	@param [in] acceptDone 按下确定的返回值
	@param [in] ignoreDone 按下取消的返回值
	*/
	AccountDialog(QWindow* parent = nullptr, int32_t acceptDone = ACCEPT_BUTTON, int32_t ignoreDone = IGNORE_BUTTON);

	/** 设置账号错误提示是否显示
	@param [in] visible 是否显示
	*/
	void setErrorVisible(bool visible);

Q_SIGNALS:
	void errorVisible(bool visible);

protected:
	bool eventFilter(QObject* tar, QEvent* eve);

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
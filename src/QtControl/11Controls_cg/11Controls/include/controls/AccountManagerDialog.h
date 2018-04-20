#pragma once
#include "BoxDialogBase.h"
#include "ControlsMacro.h"

class AccountDialog;
class ClosureDialog;
class SubAccountPanel;
class ControlsAPI AccountManagerDialog : public BoxDialogBase
{
public:
	/** 构造函数
	*/
	AccountManagerDialog();

	/** 弹出注册账号窗口
	@return 返回输入的子账号名，如果点击取消则返回空字符串
	*/
	QString popAccountDialog();

	/** 弹出封号窗口
	*/
	void popClosureDialog();

private:
    void resizeEvent(QResizeEvent* eve);

private:
	AccountDialog* m_accountDialog;
	ClosureDialog* m_closureDialog;
	SubAccountPanel* m_subAccountPanel;
};
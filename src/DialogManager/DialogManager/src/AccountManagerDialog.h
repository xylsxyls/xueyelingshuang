#pragma once
#include "BoxDialogBase.h"
#include "DialogManagerMacro.h"

class AccountDialog;
class ClosureDialog;
class SubAccountPanel;
/** 账号管理框
*/
class DialogManagerAPI AccountManagerDialog : public BoxDialogBase
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

    /** 获取内部界面指针
    @return 返回内部界面指针
    */
    SubAccountPanel* subAccountPanelPtr();

    /** 获取账号框指针
    @return 返回账号框指针
    */
    AccountDialog* accountDialogPtr();

    /** 获取封号窗口指针
    @return 返回封号窗口指针
    */
    ClosureDialog* closureDialogPtr();

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	AccountDialog* m_accountDialog;
	ClosureDialog* m_closureDialog;
	SubAccountPanel* m_subAccountPanel;
};
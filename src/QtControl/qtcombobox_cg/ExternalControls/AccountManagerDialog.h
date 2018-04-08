#pragma once
#include "BoxDialogBase.h"

class AccountDialog;
class ClosureDialog;
class COriginalButton;
class SubAccountPanel;
class AccountManagerDialog : public BoxDialogBase
{
public:
	/** 构造函数
	*/
	AccountManagerDialog();

	/** 弹出注册账号窗口
	@return 返回输入的子账号名，如果点击取消则返回空字符串
	*/
	QString popAccountDialog();

	/** 获取注册账号窗口指针
	@return 返回注册账号窗口指针
	*/
	AccountDialog* accountDialogPtr();

	/** 弹出封号窗口
	*/
	void popClosureDialog();

	/** 获取封号窗口指针
	@return 返回封号窗口指针
	*/
	ClosureDialog* closureDialogPtr();

	//* 返回子账号列表
	SubAccountPanel* subAccountPanelPtr();

private slots:
	void onHide();

private:
	void layoutContorls();
	void resizeEvent(QResizeEvent *e);

private:
	AccountDialog* m_accountDialog;
	ClosureDialog* m_closureDialog;
	COriginalButton* m_hide;
	SubAccountPanel* m_subAccountPanel;
};
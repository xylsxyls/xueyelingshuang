#pragma once
#include "BoxDialogBase.h"
#include "DialogManagerMacro.h"

class AccountDialog;
class ClosureDialog;
class SubAccountPanel;
/** 账号管理弹框，组合子账号列表、注册子账号弹框和封号信息弹框
*/
class DialogManagerAPI AccountManagerDialog : public BoxDialogBase
{
public:
	/** 构造函数
	*/
	AccountManagerDialog();

	/** 析构函数，释放账号创建和封号信息弹窗
	*/
	~AccountManagerDialog();

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
    /** 调整子账号列表区域和标题栏按钮布局
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 检查内部弹框和面板是否已经创建完成
    @return 返回true表示控件有效
    */
    bool check();

private:
	// 注册子账号弹框
	AccountDialog* m_accountDialog;

	// 封号信息弹框
	ClosureDialog* m_closureDialog;

	// 子账号管理面板
	SubAccountPanel* m_subAccountPanel;
};
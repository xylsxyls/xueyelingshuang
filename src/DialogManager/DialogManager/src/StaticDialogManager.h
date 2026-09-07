#pragma once
#include <QObject>
#include "DialogType.h"
#include "ManagerBase/ManagerBaseAPI.h"
#include "DialogManagerMacro.h"

class SubAccountPanel;
class AccountDialog;
class ClosureDialog;
class AccountManagerDialog;
/** 静态窗口管理类
*/
class DialogManagerAPI StaticDialogManager :
    public QObject,
    public ManagerBase<StaticDialogManager>
{
    Q_OBJECT
public:
    /** 构造函数
    */
    StaticDialogManager();

	/** 析构函数
	*/
	~StaticDialogManager();

public:
    /** 弹出静态框
    @param [in] param 参数
    */
	void popStaticDialog(DialogParam& param);

	/** 操作窗口
	@param [in] param 操作参数
	*/
	void operateDialog(OperateParam& param);

Q_SIGNALS:
	/** 窗口发出信号
	@param [in] param 信号参数
	*/
	void dialogSignal(const SignalParam& param);

private slots:
    /** 处理静态弹框关闭信号
    @param [in] result 弹框关闭结果
    */
    void onClosedSignal(DialogResult* result);

	/** 处理静态弹框已经显示的情况
	*/
	void onAlreadyShown();

private:
	/** 关闭静态框
	@param [in] type 静态框类型
	*/
	void closeStaticDialog(DialogType type);

	/** 获取静态框ID
	@param [in] type 静态框类型
	@return 返回静态框ID
	*/
	quint64 staticDialogDialogId(DialogType type);

private:
	/** 获取账号管理弹框指针，不存在时创建
	@return 返回账号管理弹框指针，创建失败时返回nullptr
	*/
	AccountManagerDialog* accountManagerDialogPtr();

public:
    // 账号管理静态弹框ID
    quint64 m_accountManagerDialogId;
};
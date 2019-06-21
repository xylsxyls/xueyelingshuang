#pragma once
#include <QObject>
#include "DialogType.h"
#include "core/ManagerBase.h"
#include "DialogManagerMacro.h"

class SubAccountPanel;
class AccountDialog;
class ClosureDialog;
class AccountManagerDialog;
/** 静态窗口管理类
*/
class DialogManagerAPI StaticDialogManager :
    public QObject,
    public ManagerBase < StaticDialogManager >
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
    @param [in] type 静态框类型
    @param [in] param 参数
    */
	void popStaticDialog(DialogParam& param);

	/** 操作窗口
	@param [in] type 操作类型
	@param [in] param 操作参数
	*/
	void operateDialog(OperateParam& param);

Q_SIGNALS:
	/** 窗口发出信号
	@param [in] type 信号类型
	@param [in] param 信号参数
	*/
	void dialogSignal(const SignalParam& param);

private slots:
    void onClosedSignal(DialogResult* result);
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
	AccountManagerDialog* accountManagerDialogPtr();

public:
    quint64 m_accountManagerDialogId;
};
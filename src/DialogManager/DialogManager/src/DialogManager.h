#pragma once
#include <QObject>
#include "CustomDialogFactory.h"
#include "CustomViewFactory.h"
#include "DialogManagerMacro.h"
#include "DialogType.h"
#include "ManagerBase/ManagerBaseAPI.h"

/** 弹框统一对外入口，内部委托DialogRunTimeManager管理窗口运行期状态
*/
class DialogManagerAPI DialogManager :
	public QObject,
	public ManagerBase<DialogManager>
{
	Q_OBJECT
public:
	/** 构造函数
	*/
	DialogManager();

	/** 析构函数
	*/
	~DialogManager();

public:
	/** 注册业务自定义窗口工厂
	@param [in] dialogType 自定义窗口类型ID，必须大于等于CUSTOM_DIALOG_TYPE_BEGIN
	@param [in] factory 工厂指针，注册成功后由DialogManager托管
	@param [in] destroyFunction 工厂销毁函数，传nullptr时使用delete释放
	@return 返回true表示注册成功
	*/
	bool registerCustomDialogFactory(DialogType dialogType,
									 CustomDialogFactory* factory,
									 CustomDialogFactoryDestroy destroyFunction = nullptr);

	/** 注销业务自定义窗口工厂
	@param [in] dialogType 自定义窗口类型ID
	*/
	void unregisterCustomDialogFactory(DialogType dialogType);

	/** 注册业务自定义内容区工厂
	@param [in] dialogType 自定义窗口类型ID，必须大于等于CUSTOM_DIALOG_TYPE_BEGIN
	@param [in] factory 内容区工厂指针，注册成功后由DialogManager托管
	@param [in] showMode 展示模式，自定义内容区只支持模态和普通非模态两种模式
	@param [in] destroyFunction 工厂销毁函数，传nullptr时使用delete释放
	@return 返回true表示注册成功
	*/
	bool registerCustomViewFactory(DialogType dialogType,
								   CustomViewFactory* factory,
								   DialogShowMode showMode = POP_DIALOG_SHOW_MODE,
								   CustomViewFactoryDestroy destroyFunction = nullptr);

	/** 注销业务自定义内容区工厂
	@param [in] dialogType 自定义窗口类型ID
	*/
	void unregisterCustomViewFactory(DialogType dialogType);

	/** 创建或复用窗口；businessId/userId有效且已有窗口未关闭时会返回已有dialogId
	@param [in,out] param 窗口参数，创建或复用成功后会写入m_dialogId
	*/
	void makeDialog(DialogParam& param);

	/** 操作窗口
	@param [in,out] param 操作参数
	*/
	void operateDialog(OperateParam& param);

	/** 释放当前托管的全部窗口和内部资源
	*/
	void uninit();

Q_SIGNALS:
	/** 窗口发出信号
	@param [in] param 信号参数共享指针，queued connection下会保留实际派生字段
	*/
	void dialogSignal(const DialogSignalPtr& param);
};

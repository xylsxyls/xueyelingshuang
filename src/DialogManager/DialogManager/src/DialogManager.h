#pragma once
#include <QObject>
#include "CustomDialogFactory.h"
#include "CustomViewFactory.h"
#include "DialogManagerMacro.h"
#include "DialogType.h"
#include "ManagerBase/ManagerBaseAPI.h"

/** DialogManager向宿主转发日志时使用的严重级别
*/
enum DialogLogLevel
{
	DIALOG_LOG_INFO,
	DIALOG_LOG_WARNING,
	DIALOG_LOG_ERROR
};

/** 接收DialogManager完整日志消息的宿主回调
@param [in] level 日志级别
@param [in] message 完整消息，仅在回调期间有效
*/
typedef void (*DialogLogCallback)(DialogLogLevel level, const char* message);

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

	/** 设置宿主日志回调；传入nullptr时停止转发
	@param [in] callback 接收日志级别和完整消息的函数指针
	*/
	static void setLogCallback(DialogLogCallback callback);

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
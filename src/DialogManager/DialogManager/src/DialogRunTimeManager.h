#pragma once
#include <QObject>
#include <QMutex>
#include "CustomDialogFactory.h"
#include "CustomViewFactory.h"
#include "DialogManagerMacro.h"
#include "DialogType.h"
#include "ManagerBase/ManagerBaseAPI.h"

class COriginalDialog;

/** Dialog运行期管理器，负责窗口生命周期、业务ID复用、线程切换和子管理器信号汇总
*/
class DialogManagerAPI DialogRunTimeManager :
	public QObject,
	public ManagerBase<DialogRunTimeManager>
{
	Q_OBJECT
public:
	/** 构造函数，只初始化状态并登记Qt元类型
	*/
	DialogRunTimeManager();

	/** 析构函数，调用uninit释放托管窗口
	*/
	~DialogRunTimeManager();

public:
	/** 注册业务自定义窗口工厂
	@param [in] dialogType 自定义窗口类型ID，必须大于等于CUSTOM_DIALOG_TYPE_BEGIN
	@param [in] factory 工厂指针，注册成功后由DialogRunTimeManager托管
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
	@param [in] factory 内容区工厂指针，注册成功后由DialogRunTimeManager托管
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
	@param [in,out] param 操作参数，部分查询类操作会把结果写回参数对象
	*/
	void operateDialog(OperateParam& param);

	/** 释放当前托管的全部窗口和内部资源
	*/
	void uninit();

Q_SIGNALS:
	/** 子窗口发出的统一信号，使用共享指针避免queued connection切掉派生字段
	@param [in] param 信号参数共享指针，可能为空，接收方需要判空
	*/
	void dialogSignal(const DialogSignalPtr& param);

private slots:
	/** 在GUI线程创建窗口
	@param [in,out] param 窗口参数指针
	*/
	void makeDialogInGuiThread(DialogParam* param);

	/** 在GUI线程操作窗口
	@param [in,out] param 操作参数指针
	*/
	void operateDialogInGuiThread(OperateParam* param);

	/** 在GUI线程执行整体释放
	*/
	void uninitInGuiThread();

	/** 处理自定义窗口关闭信号
	@param [in] result 自定义窗口关闭结果
	*/
	void onCustomDialogClosed(DialogResult* result);

	/** 处理自定义窗口已经执行显示操作
	*/
	void onCustomDialogAlreadyShown();

private:
	/** 禁止拷贝构造，避免运行期窗口托管状态被复制
	@param [in] other 另一个运行期管理器
	*/
	DialogRunTimeManager(const DialogRunTimeManager& other);

	/** 禁止赋值，避免运行期窗口托管状态被复制
	@param [in] other 另一个运行期管理器
	@return 返回当前对象引用
	*/
	DialogRunTimeManager& operator=(const DialogRunTimeManager& other);

	/** 把管理器对象迁移到GUI线程，避免跨线程invoke落到错误线程
	*/
	void ensureObjectInGuiThread();

	/** 判断当前是否可以阻塞等待GUI线程执行请求
	@param [in] operationName 当前请求名称，用于日志定位
	@return 返回true表示可以使用BlockingQueuedConnection
	*/
	bool canBlockInvokeGuiThread(const char* operationName) const;

	/** 确保子管理器信号只连接一次
	*/
	void ensureManagersConnected();

	/** 复用已有业务窗口
	@param [in,out] param 创建参数，命中已有窗口时会写入已有窗口ID
	@return 返回true表示已有窗口已经被复用，本次不再创建新窗口
	*/
	bool reuseExistingUserDialog(DialogParam& param);

	/** 激活已存在的窗口，让用户能看到刚刚复用的业务窗口
	@param [in] dialogPtr 已经由AllocManager托管的窗口指针
	*/
	void activateDialog(COriginalDialog* dialogPtr);

	/** 创建内置窗口
	@param [in,out] param 窗口参数
	*/
	void makeBuiltInDialog(DialogParam& param);

	/** 创建业务自定义窗口
	@param [in,out] param 窗口参数
	*/
	void makeCustomDialog(DialogParam& param);

	/** 判断当前线程是否是Qt GUI线程
	@return 返回true表示当前线程可以直接操作QWidget
	*/
	bool isGuiThread() const;

	/** 关闭指定窗口
	@param [in] dialogId 窗口ID
	@param [in] result 窗口关闭结果
	*/
	void closeDialogByDialogId(quint64 dialogId, DialogResult result);

private:
	/** 保护初始化状态和释放状态的互斥锁
	*/
	QMutex m_mutex;

	/** 子管理器信号是否已经连接
	*/
	bool m_isManagersConnected;

	/** 运行期管理器是否已经进入释放流程
	*/
	bool m_isUninit;
};

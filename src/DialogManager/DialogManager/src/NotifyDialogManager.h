#pragma once
#include <QObject>
#include "DialogType.h"
#include "ManagerBase/ManagerBaseAPI.h"

/** 通知框的管理类
*/
class NotifyDialogManager :
    public QObject,
    public ManagerBase<NotifyDialogManager>
{
    Q_OBJECT
public:
    /** 显示窗口
    @param [in] param 弹窗参数
    */
	void showDialog(DialogParam& param);

Q_SIGNALS:
	/** 窗口发出信号
	@param [in] param 信号参数
	*/
	void dialogSignal(const SignalParam& param);

private slots:
    /** 处理通知框关闭信号
    @param [in] result 弹框关闭结果
    */
    void onClosedSignal(DialogResult* result);

	/** 处理同类通知框已经显示的情况
	*/
	void onAlreadyShown();
};
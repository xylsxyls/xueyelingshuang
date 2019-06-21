#pragma once
#include <QObject>
#include "DialogType.h"
#include "core/ManagerBase.h"

/** 通知框的管理类
*/
class NotifyDialogManager :
    public QObject,
    public ManagerBase < NotifyDialogManager >
{
    Q_OBJECT
public:
    /** 显示窗口
    @param [in] type 弹窗类型
    @param [in] param 弹窗参数
    */
	void showDialog(DialogParam& param);

Q_SIGNALS:
	/** 窗口发出信号
	@param [in] type 信号类型
	@param [in] param 信号参数
	*/
	void dialogSignal(const SignalParam& param);

private slots:
    void onClosedSignal(DialogResult* result);
	void onAlreadyShown();
};
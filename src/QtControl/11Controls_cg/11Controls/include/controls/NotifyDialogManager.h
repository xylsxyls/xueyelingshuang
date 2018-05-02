#pragma once
#include <QObject>
#include "DialogType.h"
#include "../core/ManagerBase.h"

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
    void showDialog(DialogType type, ParamBase* param);

Q_SIGNALS:
    /** 非模态窗关闭的时候发送信号
    @param [in] param 非模态窗关闭发送信号的参数结构体
    */
    void notifyDialogDone(const DialogDoneSignalParam& param);

private slots:
    void onClosedSignal(int result);
};
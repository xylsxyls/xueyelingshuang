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
    void showDialog(DialogType type, ParamBase* param);

Q_SIGNALS:
    void notifyDialogDone(const DialogDoneSignalParam& param);

private slots:
    void onClosedSignal(int result);
};
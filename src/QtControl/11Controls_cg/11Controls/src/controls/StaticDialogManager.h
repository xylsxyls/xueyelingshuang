#pragma once
#include <QObject>
#include "DialogType.h"
#include "../core/ManagerBase.h"
#include "ControlsMacro.h"

class AccountManagerDialog;
/** 静态窗口管理类
*/
class ControlsAPI StaticDialogManager :
    public QObject,
    public ManagerBase < StaticDialogManager >
{
    Q_OBJECT
public:
    void popStaticDialog(DialogType type, ParamBase* param);
    void deleteStaticDialog(DialogType type);

public:
    StaticDialogManager();

Q_SIGNALS:
    //静态窗关闭的时候发送信号
    void staticDialogDone(const DialogDoneSignalParam& param);

private slots:
    void onClosedSignal(int result);

public:
    int32_t m_accountManagerDialogId;
};
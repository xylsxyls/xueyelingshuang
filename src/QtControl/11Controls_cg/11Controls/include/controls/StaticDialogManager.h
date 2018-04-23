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
    void staticDialogDone(int32_t dialogId, int32_t userId, DialogType type, int32_t result, int32_t userParam);

private slots:
    void onDeleteDialog(DialogType type);
    void onFinished(int result);

public:
    AccountManagerDialog* m_accountManagerDialog;
};
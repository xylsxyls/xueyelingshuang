#pragma once
#include <QObject>
#include "DialogType.h"
#include "ManagerBase.h"

class QWindow;
class AccountManagerDialog;

class StaticDialogManager : public ManagerBase < StaticDialogManager >
{
    Q_OBJECT
public:
    void popStaticDialog(DialogType type, ParamBase* param);


    /** 弹出账号管理页面
    */
    void popAccountManagerDialog(QWindow* parent = nullptr);

    /** 销毁账号管理页面窗口
    */
    void destroyAccountManagerDialog();

private:
    AccountManagerDialog* m_accountManagerDialog;
};
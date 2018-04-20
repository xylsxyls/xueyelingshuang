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


    /** �����˺Ź���ҳ��
    */
    void popAccountManagerDialog(QWindow* parent = nullptr);

    /** �����˺Ź���ҳ�洰��
    */
    void destroyAccountManagerDialog();

private:
    AccountManagerDialog* m_accountManagerDialog;
};
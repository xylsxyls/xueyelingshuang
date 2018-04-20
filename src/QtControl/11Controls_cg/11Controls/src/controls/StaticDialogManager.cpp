#include "StaticDialogManager.h"
#include "PopDialog.h"
#include "AllocManager.h"
#include "AccountManagerDialog.h"

void StaticDialogManager::popStaticDialog(DialogType type, ParamBase* param)
{
    PopDialog* popDialogPtr = nullptr;
    switch (type)
    {
    case ACCOUNT_MANAGER_DIALOG:
    {
        AccountManagerDialog* accountManagerDialog = new AccountManagerDialog;
        param->m_dialogId = AllocManager::instance().add(accountManagerDialog, ACCOUNT_MANAGER_DIALOG, param->m_userId);
        AccountManagerDialogParam accountManagerDialogParam;
        if (param != nullptr)
        {
            accountManagerDialogParam = *((AccountManagerDialogParam*)param);
        }
        popDialogPtr = accountManagerDialog;
        break;
    }
    default:
        break;
    }

    ParamBase baseParam;
    if (param != nullptr)
    {
        baseParam = *param;
    }
    popDialogPtr->setWindowTiTle(baseParam.m_title);
    popDialogPtr->setTimeRest(param->m_timeOut);
    popDialogPtr->setTimeRestVisible(param->m_isCountDownVisible);
    popDialogPtr->setTopTransientWindow(param->m_parent);
    popDialogPtr->exec();
}

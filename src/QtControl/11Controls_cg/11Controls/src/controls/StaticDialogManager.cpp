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
        AccountManagerDialogParam* accountManagerDialogParam = (AccountManagerDialogParam*)param;
        int32_t dialogId = 0;
        AccountManagerDialog* accountManagerDialog = (AccountManagerDialog*)AllocManager::instance().createDialog(dialogId, accountManagerDialogParam->m_userId, ACCOUNT_MANAGER_DIALOG);
        if (accountManagerDialog == nullptr)
        {
            return;
        }
        param->m_dialogId = dialogId;
        m_accountManagerDialogId = param->m_dialogId;
        popDialogPtr = accountManagerDialog;
        break;
    }
    default:
        break;
    }
    
    popDialogPtr->setWindowTiTle(param->m_title);
    popDialogPtr->setUserParam(param->m_userParam);
    popDialogPtr->setTimeRest(param->m_timeOut);
    popDialogPtr->setTimeRestVisible(param->m_isCountDownVisible);
    popDialogPtr->setTopTransientWindow(param->m_parent);
    QObject::connect(popDialogPtr, &DialogShow::closedSignal, this, &StaticDialogManager::onClosedSignal);
    param->m_result = popDialogPtr->exec();
}

void StaticDialogManager::deleteStaticDialog(DialogType type)
{
    switch (type)
    {
    case ACCOUNT_MANAGER_DIALOG:
    {
        AllocManager::instance().removeByDialogId(m_accountManagerDialogId);
        m_accountManagerDialogId = -1;
        break;
    }
    default:
        break;
    }
}

StaticDialogManager::StaticDialogManager() :
m_accountManagerDialogId(-1)
{

}

void StaticDialogManager::onClosedSignal(int result)
{
    DialogShow* dialogPtr = (DialogShow*)sender();
    if (dialogPtr == nullptr)
    {
        return;
    }
    int32_t dialogId = AllocManager::instance().findDialogId(dialogPtr);
    int32_t userId = AllocManager::instance().findUserId(dialogId);
    DialogType type = AllocManager::instance().findDialogType(dialogId);
    int32_t userParam = dialogPtr->userParam();
    DialogDoneSignalParam param;
    param.m_dialogId = dialogId;
    param.m_userId = userId;
    param.m_type = type;
    param.m_result = (DialogResult)result;
    param.m_userParam = userParam;
    emit staticDialogDone(param);
}
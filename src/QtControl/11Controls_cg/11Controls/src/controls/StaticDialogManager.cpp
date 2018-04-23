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
        if (m_accountManagerDialog == nullptr)
        {
            AccountManagerDialogParam accountManagerDialogParam;
            if (param != nullptr)
            {
                accountManagerDialogParam = *((AccountManagerDialogParam*)param);
            }
            m_accountManagerDialog = new AccountManagerDialog;
            int32_t dialogId = AllocManager::instance().add(m_accountManagerDialog, ACCOUNT_MANAGER_DIALOG, accountManagerDialogParam.m_userId);
            if (param != nullptr)
            {
                param->m_dialogId = dialogId;
            }
        }

        popDialogPtr = m_accountManagerDialog;
        break;
    }
    default:
        break;
    }

    PopParamBase baseParam;
    if (param != nullptr)
    {
        baseParam = *(PopParamBase*)param;
    }
    popDialogPtr->setWindowTiTle(baseParam.m_title);
    popDialogPtr->setUserParam(baseParam.m_userParam);
    popDialogPtr->setTimeRest(baseParam.m_timeOut);
    popDialogPtr->setTimeRestVisible(baseParam.m_isCountDownVisible);
    popDialogPtr->setTopTransientWindow(baseParam.m_parent);
    QObject::connect(popDialogPtr, &COriginalDialog::finished, this, &StaticDialogManager::onFinished);
    DialogResult result = (DialogResult)popDialogPtr->exec();
    if (param != nullptr)
    {
        ((PopParamBase*)param)->m_result = result;
    }
}

void StaticDialogManager::deleteStaticDialog(DialogType type)
{
    switch (type)
    {
    case ACCOUNT_MANAGER_DIALOG:
    {
        AllocManager::instance().removeByDialogId(AllocManager::instance().findDialogId(m_accountManagerDialog));
        break;
    }
    default:
        break;
    }
}

StaticDialogManager::StaticDialogManager() :
m_accountManagerDialog(nullptr)
{
    QObject::connect(&AllocManager::instance(), &AllocManager::deleteDialog, this, &StaticDialogManager::onDeleteDialog);
}

void StaticDialogManager::onDeleteDialog(DialogType type)
{
    if (type == ACCOUNT_MANAGER_DIALOG)
    {
        m_accountManagerDialog = nullptr;
    }
}

void StaticDialogManager::onFinished(int result)
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
    emit staticDialogDone(dialogId, userId, type, result, userParam);
}
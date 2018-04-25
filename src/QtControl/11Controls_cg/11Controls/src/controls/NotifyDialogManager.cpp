#include "NotifyDialogManager.h"
#include "NotifyDialog.h"
#include "AskShowDialog.h"
#include "TipShowDialog.h"
#include "LoginShowDialog.h"
#include "AllocManager.h"
#include "DialogShow.h"

void NotifyDialogManager::showDialog(DialogType type, ParamBase* param)
{
    NotifyDialog* notifyDialogPtr = nullptr;
    switch (type)
    {
    case ASK_SHOW_DIALOG:
    {
        AskShowDialogParam* askShowDialogParam = (AskShowDialogParam*)param;
        int32_t dialogId = 0;
        AskShowDialog* askShowDialog = (AskShowDialog*)AllocManager::instance().createDialog(dialogId, askShowDialogParam->m_userId, ASK_SHOW_DIALOG);
        if (askShowDialog == nullptr)
        {
            return;
        }
        askShowDialog->setTip(askShowDialogParam->m_tip);
        askShowDialog->setAcceptButton(askShowDialogParam->m_acceptText, askShowDialogParam->m_acceptDone);
        askShowDialog->setIgnoreButton(askShowDialogParam->m_ignoreText, askShowDialogParam->m_ignoreDone);
        param->m_dialogId = dialogId;
        notifyDialogPtr = askShowDialog;
        break;
    }
    case TIP_SHOW_DIALOG:
    {
        TipShowDialogParam* tipShowDialogParam = (TipShowDialogParam*)param;
        int32_t dialogId = 0;
        TipShowDialog* tipShowDialog = (TipShowDialog*)AllocManager::instance().createDialog(dialogId, tipShowDialogParam->m_userId, TIP_SHOW_DIALOG);
        if (tipShowDialog == nullptr)
        {
            return;
        }
        tipShowDialog->setTip(tipShowDialogParam->m_tip);
        tipShowDialog->setAcceptButton(tipShowDialogParam->m_buttonText, tipShowDialogParam->m_done);
        param->m_dialogId = dialogId;
        notifyDialogPtr = tipShowDialog;
        break;
    }
    case LOGIN_SHOW_DIALOG:
    {
        LoginShowDialogParam* loginShowDialogParam = (LoginShowDialogParam*)param;
        int32_t dialogId = 0;
        LoginShowDialog* loginShowDialog = (LoginShowDialog*)AllocManager::instance().createDialog(dialogId, loginShowDialogParam->m_userId, LOGIN_SHOW_DIALOG);
        if (loginShowDialog == nullptr)
        {
            return;
        }
        loginShowDialog->setTip(loginShowDialogParam->m_tip);
        loginShowDialog->setGreeting(loginShowDialogParam->m_greeting);
        loginShowDialog->setMoreButton(loginShowDialogParam->m_urlButtonText, loginShowDialogParam->m_linkUrl, loginShowDialogParam->m_isUrlButtonVisible);
        param->m_dialogId = dialogId;
        notifyDialogPtr = loginShowDialog;
        break;
    }
    default:
        break;
    }

    if (notifyDialogPtr == nullptr)
    {
        return;
    }

    notifyDialogPtr->setWindowTiTle(param->m_title);
    notifyDialogPtr->setUserParam(param->m_userParam);
    notifyDialogPtr->setTimeRest(param->m_timeOut);
    notifyDialogPtr->setTimeRestVisible(param->m_isCountDownVisible);
    notifyDialogPtr->setTopTransientWindow(param->m_parent);

    QObject::connect(notifyDialogPtr, &DialogShow::closedSignal, this, &NotifyDialogManager::onClosedSignal);
    notifyDialogPtr->show();
}

void NotifyDialogManager::onClosedSignal(int result)
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
    emit notifyDialogDone(dialogId, userId, type, result, userParam);
    AllocManager::instance().removeByDialogId(dialogId);
}

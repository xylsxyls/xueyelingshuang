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
        AskShowDialogParam askShowDialogParam;
        if (param != nullptr)
        {
            askShowDialogParam = *((AskShowDialogParam*)param);
        }

        AskShowDialog* askShowDialog = new AskShowDialog;
        if (askShowDialog == nullptr)
        {
            break;
        }

        int32_t dialogId = AllocManager::instance().add(askShowDialog, ASK_SHOW_DIALOG, askShowDialogParam.m_userId);
        askShowDialog->setTip(askShowDialogParam.m_tip);
        askShowDialog->setAcceptButton(askShowDialogParam.m_acceptText, askShowDialogParam.m_acceptDone);
        askShowDialog->setIgnoreButton(askShowDialogParam.m_ignoreText, askShowDialogParam.m_ignoreDone);
        if (param != nullptr)
        {
            param->m_dialogId = dialogId;
        }

        notifyDialogPtr = askShowDialog;
        break;
    }
    case TIP_SHOW_DIALOG:
    {
        TipShowDialogParam tipShowDialogParam;
        if (param != nullptr)
        {
            tipShowDialogParam = *((TipShowDialogParam*)param);
        }

        TipShowDialog* tipShowDialog = new TipShowDialog;
        if (tipShowDialog == nullptr)
        {
            break;
        }

        int32_t dialogId = AllocManager::instance().add(tipShowDialog, TIP_SHOW_DIALOG, tipShowDialogParam.m_userId);
        tipShowDialog->setTip(tipShowDialogParam.m_tip);
        tipShowDialog->setAcceptButton(tipShowDialogParam.m_buttonText, tipShowDialogParam.m_done);
        if (param != nullptr)
        {
            param->m_dialogId = dialogId;
        }

        notifyDialogPtr = tipShowDialog;
        break;
    }
    case LOGIN_SHOW_DIALOG:
    {
        LoginShowDialogParam loginShowDialogParam;
        if (param != nullptr)
        {
            loginShowDialogParam = *((LoginShowDialogParam*)param);
        }

        LoginShowDialog* loginShowDialog = new LoginShowDialog;
        if (loginShowDialog == nullptr)
        {
            break;
        }

        int32_t dialogId = AllocManager::instance().add(loginShowDialog, LOGIN_SHOW_DIALOG, loginShowDialogParam.m_userId);
        loginShowDialog->setTip(loginShowDialogParam.m_tip);
        loginShowDialog->setGreeting(loginShowDialogParam.m_greeting);
        loginShowDialog->setMoreButton(loginShowDialogParam.m_urlButtonText, loginShowDialogParam.m_linkUrl, loginShowDialogParam.m_isUrlButtonVisible);
        if (param != nullptr)
        {
            param->m_dialogId = dialogId;
        }

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

    ParamBase baseParam;
    if (param != nullptr)
    {
        baseParam = *param;
    }
    notifyDialogPtr->setWindowTiTle(baseParam.m_title);
    notifyDialogPtr->setUserParam(baseParam.m_userParam);
    QObject::connect(notifyDialogPtr, &COriginalDialog::finished, this, &NotifyDialogManager::onFinished);
    notifyDialogPtr->show();
}

void NotifyDialogManager::onFinished(int result)
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
}

#include "NotifyDialogManager.h"
#include "NotifyDialog.h"
#include "AskShowDialog.h"
#include "TipShowDialog.h"
#include "LoginShowDialog.h"
#include "AllocManager.h"

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
        if (param != nullptr)
        {
            param->m_dialogId = dialogId;
        }
        
        askShowDialog->setTip(askShowDialogParam.m_tip);
        askShowDialog->setAcceptButton(askShowDialogParam.m_acceptText, askShowDialogParam.m_acceptDone);
        askShowDialog->setIgnoreButton(askShowDialogParam.m_ignoreText, askShowDialogParam.m_ignoreDone);
        notifyDialogPtr = askShowDialog;
        break;
    }
    case TIP_SHOW_DIALOG:
    {
        TipShowDialog* tipShowDialog = new TipShowDialog;
        param->m_dialogId = AllocManager::instance().add(tipShowDialog, ASK_SHOW_DIALOG, param->m_userId);
        TipShowDialogParam tipShowDialogParam;
        if (param != nullptr)
        {
            tipShowDialogParam = *((TipShowDialogParam*)param);
        }

        tipShowDialog->setTip(tipShowDialogParam.m_tip);
        tipShowDialog->setAcceptButton(tipShowDialogParam.m_buttonText, tipShowDialogParam.m_done);
        notifyDialogPtr = tipShowDialog;
        break;
    }
    case LOGIN_SHOW_DIALOG:
    {
        LoginShowDialog* loginShowDialog = new LoginShowDialog;
        param->m_dialogId = AllocManager::instance().add(loginShowDialog, LOGIN_SHOW_DIALOG, param->m_userId);
        LoginShowDialogParam loginShowDialogParam;
        if (param != nullptr)
        {
            loginShowDialogParam = *((LoginShowDialogParam*)param);
        }

        loginShowDialog->setTip(loginShowDialogParam.m_tip);
        loginShowDialog->setGreeting(loginShowDialogParam.m_greeting);
        loginShowDialog->setMoreButton(loginShowDialogParam.m_urlButtonText, loginShowDialogParam.m_linkUrl, loginShowDialogParam.m_isUrlButtonVisible);
        notifyDialogPtr = loginShowDialog;
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
    notifyDialogPtr->setUserParam(baseParam.m_userParam);
    notifyDialogPtr->setWindowTiTle(baseParam.m_title);
    notifyDialogPtr->setUserParam(baseParam.m_userParam);
    notifyDialogPtr->show();
}

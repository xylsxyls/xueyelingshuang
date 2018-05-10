#include "NotifyDialogManager.h"
#include "NotifyDialog.h"
#include "AskShowDialog.h"
#include "TipShowDialog.h"
#include "LoginShowDialog.h"
#include "AllocManager.h"
#include "DialogShow.h"
#include "AdvertShowDialog.h"

void NotifyDialogManager::showDialog(DialogType type, ParamBase* param)
{
    NotifyDialog* notifyDialogPtr = nullptr;
    switch (type)
    {
    case ASK_SHOW_DIALOG:
    {
        AskShowDialogParam* askShowDialogParam = (AskShowDialogParam*)param;
        quint64 dialogId = 0;
        AskShowDialog* askShowDialog = (AskShowDialog*)AllocManager::instance().createDialog(dialogId, askShowDialogParam->m_userId, ASK_SHOW_DIALOG);
        if (askShowDialog == nullptr)
        {
            return;
        }
        askShowDialog->setTip(askShowDialogParam->m_tip);
        askShowDialog->setAcceptButton(askShowDialogParam->m_acceptText, ACCEPT_BUTTON);
        askShowDialog->setIgnoreButton(askShowDialogParam->m_ignoreText, IGNORE_BUTTON);
		askShowDialog->setWindowTiTle(param->m_title);
        param->m_dialogId = dialogId;
        notifyDialogPtr = askShowDialog;
        break;
    }
    case TIP_SHOW_DIALOG:
    {
        TipShowDialogParam* tipShowDialogParam = (TipShowDialogParam*)param;
        quint64 dialogId = 0;
        TipShowDialog* tipShowDialog = (TipShowDialog*)AllocManager::instance().createDialog(dialogId, tipShowDialogParam->m_userId, TIP_SHOW_DIALOG);
        if (tipShowDialog == nullptr)
        {
            return;
        }
        tipShowDialog->setTip(tipShowDialogParam->m_tip);
        tipShowDialog->setAcceptButton(tipShowDialogParam->m_buttonText, ACCEPT_BUTTON);
		tipShowDialog->setWindowTiTle(param->m_title);
        param->m_dialogId = dialogId;
        notifyDialogPtr = tipShowDialog;
        break;
    }
    case LOGIN_SHOW_DIALOG:
    {
        LoginShowDialogParam* loginShowDialogParam = (LoginShowDialogParam*)param;
        quint64 dialogId = 0;
        LoginShowDialog* loginShowDialog = (LoginShowDialog*)AllocManager::instance().createDialog(dialogId, loginShowDialogParam->m_userId, LOGIN_SHOW_DIALOG);
        if (loginShowDialog == nullptr)
        {
            return;
        }
        loginShowDialog->setTip(loginShowDialogParam->m_tip);
        loginShowDialog->setGreeting(loginShowDialogParam->m_greeting);
        loginShowDialog->setMoreButton(loginShowDialogParam->m_urlButtonText, loginShowDialogParam->m_linkUrl, loginShowDialogParam->m_isUrlButtonVisible);
		loginShowDialog->setWindowTiTle(param->m_title);
        param->m_dialogId = dialogId;
        notifyDialogPtr = loginShowDialog;
        break;
    }
	case ADVERT_SHOW_DIALOG:
	{
		AdvertShowDialogParam* advertShowDialogParam = (AdvertShowDialogParam*)param;
		quint64 dialogId = 0;
		AdvertShowDialog* advertShowDialog = (AdvertShowDialog*)AllocManager::instance().createDialog(dialogId, advertShowDialogParam->m_userId, ADVERT_SHOW_DIALOG);
		if (advertShowDialog == nullptr)
		{
			return;
		}
		advertShowDialog->setAdvertUrl(advertShowDialogParam->m_advertUrl);
		advertShowDialog->setWindowTiTle(param->m_title, QColor(255, 255, 255, 255), 14);
		param->m_dialogId = dialogId;
		notifyDialogPtr = advertShowDialog;
		break;
	}
    default:
        break;
    }

    if (notifyDialogPtr == nullptr)
    {
        return;
    }

    notifyDialogPtr->setWindowResultAddr(new DialogResult);
    notifyDialogPtr->setUserParam(param->m_userParam);
    notifyDialogPtr->setTimeRest(param->m_timeOut);
    notifyDialogPtr->setTimeRestVisible(param->m_isCountDownVisible);
    notifyDialogPtr->setTransientWindow(param->m_parent);

    QObject::connect(notifyDialogPtr, &DialogShow::closedSignal, this, &NotifyDialogManager::onClosedSignal);
    notifyDialogPtr->show();
}

void NotifyDialogManager::onClosedSignal(DialogResult* result)
{
    DialogShow* dialogPtr = (DialogShow*)sender();
    if (dialogPtr == nullptr)
    {
        return;
    }
    int32_t dialogId = AllocManager::instance().findDialogId(dialogPtr);
    if (dialogId == 0)
    {
        return;
    }
    int32_t userId = AllocManager::instance().findUserId(dialogId);
    DialogType type = AllocManager::instance().findDialogType(dialogId);
    int32_t userParam = dialogPtr->userParam();

    NotifyDialogDoneSignalParam param;
    param.m_dialogId = dialogId;
    param.m_userId = userId;
    param.m_type = type;
    if (result != nullptr)
    {
        param.m_result = *result;
    }
    param.m_userParam = userParam;
	emit dialogSignal(NOTIFY_DIALOG_DONE, &param);
    AllocManager::instance().removeByDialogId(dialogId);
    delete result;
}

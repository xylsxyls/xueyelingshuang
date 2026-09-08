#include "NotifyDialogManager.h"
#include "AskShowDialog.h"
#include "TipShowDialog.h"
#include "LoginShowDialog.h"
#include "AllocManager.h"
#include "AdvertShowDialog.h"
#include "DialogHelper.h"
#include "QtControls/COriginalDialog.h"
#include "QtControls/DialogShow.h"

/** 将通知窗口创建参数转换为指定派生类型，避免传错参数导致未定义行为
@param [in] param 调用方传入的创建参数
@param [in] expectedType 期望的窗口类型
@return 返回转换后的参数指针，失败时返回nullptr
*/
template<typename ParamType>
static ParamType* CastNotifyDialogParam(DialogParam& param, DialogType expectedType)
{
	if (param.dialogType() != expectedType)
	{
		return nullptr;
	}
	ParamType* castParam = dynamic_cast<ParamType*>(&param);
	if (castParam == nullptr)
	{
		DialogHelper::logFile() << "NotifyDialogManager dialog param type mismatch, dialogType = "
			<< expectedType << std::endl;
	}
	return castParam;
}

void NotifyDialogManager::showDialog(DialogParam& param)
{
    NotifyDialog* notifyDialogPtr = nullptr;
    switch (param.dialogType())
    {
    case ASK_SHOW_DIALOG:
    {
		AskShowDialogParam* askShowDialogParam = CastNotifyDialogParam<AskShowDialogParam>(param, ASK_SHOW_DIALOG);
		if (askShowDialogParam == nullptr)
		{
			return;
		}
        quint64 dialogId = 0;
		AskShowDialog* askShowDialog = dynamic_cast<AskShowDialog*>(AllocManager::instance().createDialog(dialogId, *askShowDialogParam));
        if (askShowDialog == nullptr)
        {
            return;
        }
		askShowDialog->setTip(askShowDialogParam->m_tip);
		askShowDialog->setAcceptButton(askShowDialogParam->m_acceptText, ACCEPT_BUTTON);
		askShowDialog->setIgnoreButton(askShowDialogParam->m_ignoreText, IGNORE_BUTTON);
		askShowDialog->setWindowTitle(param.m_title);
		param.m_dialogId = dialogId;
        notifyDialogPtr = askShowDialog;
        break;
    }
    case TIP_SHOW_DIALOG:
    {
		TipShowDialogParam* tipShowDialogParam = CastNotifyDialogParam<TipShowDialogParam>(param, TIP_SHOW_DIALOG);
		if (tipShowDialogParam == nullptr)
		{
			return;
		}
        quint64 dialogId = 0;
		TipShowDialog* tipShowDialog = dynamic_cast<TipShowDialog*>(AllocManager::instance().createDialog(dialogId, *tipShowDialogParam));
        if (tipShowDialog == nullptr)
        {
            return;
        }
		tipShowDialog->setTip(tipShowDialogParam->m_tip);
		tipShowDialog->setAcceptButton(tipShowDialogParam->m_buttonText, ACCEPT_BUTTON);
		tipShowDialog->setWindowTitle(param.m_title);
		param.m_dialogId = dialogId;
        notifyDialogPtr = tipShowDialog;
        break;
    }
#if defined(QTCONTROLS_ENABLE_WEBKIT) && (QT_VERSION <= QT_VERSION_CHECK(5,5,1))
    case LOGIN_SHOW_DIALOG:
    {
		LoginShowDialogParam* loginShowDialogParam = CastNotifyDialogParam<LoginShowDialogParam>(param, LOGIN_SHOW_DIALOG);
		if (loginShowDialogParam == nullptr)
		{
			return;
		}
        quint64 dialogId = 0;
		LoginShowDialog* loginShowDialog = dynamic_cast<LoginShowDialog*>(AllocManager::instance().createDialog(dialogId, *loginShowDialogParam));
        if (loginShowDialog == nullptr)
        {
            return;
        }
		loginShowDialog->setTip(loginShowDialogParam->m_tip);
		loginShowDialog->setGreeting(loginShowDialogParam->m_greeting);
		loginShowDialog->setMoreButton(loginShowDialogParam->m_urlButtonText, loginShowDialogParam->m_linkUrl, loginShowDialogParam->m_isUrlButtonVisible);
		loginShowDialog->setPreLoginAddr(loginShowDialogParam->m_preLoginAddr);
		loginShowDialog->setPreLoginTime(loginShowDialogParam->m_preLoginTime);
		loginShowDialog->setWindowTitle(param.m_title);
		param.m_dialogId = dialogId;
        notifyDialogPtr = loginShowDialog;
        break;
    }
	case ADVERT_SHOW_DIALOG:
	{
		AdvertShowDialogParam* advertShowDialogParam = CastNotifyDialogParam<AdvertShowDialogParam>(param, ADVERT_SHOW_DIALOG);
		if (advertShowDialogParam == nullptr)
		{
			return;
		}
		quint64 dialogId = 0;
		AdvertShowDialog* advertShowDialog = dynamic_cast<AdvertShowDialog*>(AllocManager::instance().createDialog(dialogId, *advertShowDialogParam));
		if (advertShowDialog == nullptr)
		{
			return;
		}
		advertShowDialog->setAdvertUrl(advertShowDialogParam->m_advertUrl);
		advertShowDialog->setWindowTitle(param.m_title, QColor(255, 255, 255, 255), 14);
		param.m_dialogId = dialogId;
		notifyDialogPtr = advertShowDialog;
		break;
	}
#endif
    default:
        break;
    }

    if (notifyDialogPtr == nullptr)
    {
        return;
    }

	notifyDialogPtr->useInternalResultStorage(ERROR_RESULT, param.m_userResult);
	notifyDialogPtr->setTimeRest(param.m_timeOut);
	notifyDialogPtr->setTimeRestVisible(param.m_isCountDownVisible);
	notifyDialogPtr->setTransientWindow(param.m_parent);

    QObject::connect(notifyDialogPtr, &DialogShow::closedSignal, this, &NotifyDialogManager::onClosedSignal);
	QObject::connect(notifyDialogPtr, &DialogBase::alreadyShown, this, &NotifyDialogManager::onAlreadyShown, Qt::QueuedConnection);
    notifyDialogPtr->show();
}

void NotifyDialogManager::onClosedSignal(DialogResult* result)
{
    DialogShow* dialogPtr = qobject_cast<DialogShow*>(sender());
    if (dialogPtr == nullptr)
    {
        return;
    }
	quint64 dialogId = AllocManager::instance().findDialogId(dialogPtr);
    if (dialogId == 0)
    {
        return;
    }
	DialogUserKey currentUserKey = AllocManager::instance().findUserKey(dialogId);
    DialogType type = AllocManager::instance().findDialogType(dialogId);
    qint32 userResult = dialogPtr->userResult();

	DialogSignalPtr signalParam = CreateDialogSignalParam<NotifyDialogDoneSignalParam>();
	if (signalParam.isNull())
	{
		DialogHelper::logFile() << "NotifyDialogManager failed to create done signal" << std::endl;
		return;
	}
	NotifyDialogDoneSignalParam* param = static_cast<NotifyDialogDoneSignalParam*>(signalParam.data());
    param->m_dialogId = dialogId;
	param->m_businessId = currentUserKey.m_businessId;
    param->m_userId = currentUserKey.m_userId;
    param->m_dialogType = type;
    if (result != nullptr)
    {
        param->m_result = *result;
    }
    param->m_userResult = userResult;

	dialogPtr->clearResultStorage();
    AllocManager::instance().removeByDialogId(dialogId);
	emit dialogSignal(signalParam);
}

void NotifyDialogManager::onAlreadyShown()
{
	DialogSignalPtr signalParam = CreateDialogSignalParam<AlreadyShownSignalParam>();
	if (signalParam.isNull())
	{
		DialogHelper::logFile() << "NotifyDialogManager failed to create already shown signal" << std::endl;
		return;
	}
	AlreadyShownSignalParam* param = static_cast<AlreadyShownSignalParam*>(signalParam.data());
	param->m_dialog = AllocManager::instance().findDialogId(qobject_cast<COriginalDialog*>(sender()));
	DialogUserKey currentUserKey = AllocManager::instance().findUserKey(param->m_dialog);
	param->m_businessId = currentUserKey.m_businessId;
	param->m_userId = currentUserKey.m_userId;
	emit dialogSignal(signalParam);
}

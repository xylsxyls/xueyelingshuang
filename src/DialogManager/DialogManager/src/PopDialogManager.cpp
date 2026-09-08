#include "PopDialogManager.h"
#include "AskDialog.h"
#include "TipDialog.h"
#include "WaitDialog.h"
#include "AllocManager.h"
#include "AdvertAskDialog.h"
#include "InputDialog.h"
#include "DownloadOperateDialog.h"
#include "DialogType.h"
#include "DialogHelper.h"
#include "QtControls/COriginalDialog.h"
#include "QtControls/DialogShow.h"

/** 将创建参数转换为指定内置弹窗参数，避免调用方传错派生类型导致未定义行为
@param [in] param 调用方传入的创建参数
@param [in] expectedType 期望的窗口类型
@return 返回转换后的参数指针，失败时返回nullptr
*/
template<typename ParamType>
static ParamType* CastPopDialogParam(DialogParam& param, DialogType expectedType)
{
	if (param.dialogType() != expectedType)
	{
		return nullptr;
	}
	ParamType* castParam = dynamic_cast<ParamType*>(&param);
	if (castParam == nullptr)
	{
		DialogHelper::logFile() << "PopDialogManager dialog param type mismatch, dialogType = "
			<< expectedType << std::endl;
	}
	return castParam;
}

/** 将操作参数转换为指定操作类型，避免调用方传错派生类型导致未定义行为
@param [in] param 调用方传入的操作参数
@param [in] expectedType 期望的操作类型
@return 返回转换后的参数指针，失败时返回nullptr
*/
template<typename ParamType>
static ParamType* CastPopOperateParam(OperateParam& param, OperateType expectedType)
{
	if (param.operateType() != expectedType)
	{
		return nullptr;
	}
	ParamType* castParam = dynamic_cast<ParamType*>(&param);
	if (castParam == nullptr)
	{
		DialogHelper::logFile() << "PopDialogManager operate param type mismatch, operateType = "
			<< expectedType << std::endl;
	}
	return castParam;
}

void PopDialogManager::popDialog(DialogParam& param)
{
    PopDialog* popDialogPtr = nullptr;
	switch (param.dialogType())
    {
    case ASK_DIALOG:
    {
		AskDialogParam* askDialogParam = CastPopDialogParam<AskDialogParam>(param, ASK_DIALOG);
		if (askDialogParam == nullptr)
		{
			return;
		}
        quint64 dialogId = 0;
		AskDialog* askDialog = dynamic_cast<AskDialog*>(AllocManager::instance().createDialog(dialogId, *askDialogParam));
        if (askDialog == nullptr)
        {
            return;
        }
		askDialog->setTip(askDialogParam->m_tip);
		askDialog->setAcceptButton(askDialogParam->m_acceptText, ACCEPT_BUTTON);
		askDialog->setIgnoreButton(askDialogParam->m_ignoreText, IGNORE_BUTTON);
        param.m_dialogId = dialogId;
        popDialogPtr = askDialog;
        break;
    }
#if defined(QTCONTROLS_ENABLE_WEBKIT) && (QT_VERSION <= QT_VERSION_CHECK(5,5,1))
    case ADVERT_ASK_DIALOG:
    {
		AdvertAskDialogParam* advertAskDialogParam = CastPopDialogParam<AdvertAskDialogParam>(param, ADVERT_ASK_DIALOG);
		if (advertAskDialogParam == nullptr)
		{
			return;
		}
        quint64 dialogId = 0;
		AdvertAskDialog* advertAskDialog = dynamic_cast<AdvertAskDialog*>(AllocManager::instance().createDialog(dialogId, *advertAskDialogParam));
        if (advertAskDialog == nullptr)
        {
            return;
        }
		advertAskDialog->initAdvertUrl(advertAskDialogParam->m_advertUrl);
		advertAskDialog->setTip(advertAskDialogParam->m_tip);
		advertAskDialog->setAcceptButton(advertAskDialogParam->m_acceptText, ACCEPT_BUTTON);
		advertAskDialog->setIgnoreButton(advertAskDialogParam->m_ignoreText, IGNORE_BUTTON);
        param.m_dialogId = dialogId;
        popDialogPtr = advertAskDialog;
        break;
    }
#endif
    case INPUT_DIALOG:
    {
		InputDialogParam* inputDialogParam = CastPopDialogParam<InputDialogParam>(param, INPUT_DIALOG);
		if (inputDialogParam == nullptr)
		{
			return;
		}
        quint64 dialogId = 0;
		InputDialog* inputDialog = dynamic_cast<InputDialog*>(AllocManager::instance().createDialog(dialogId, *inputDialogParam));
        if (inputDialog == nullptr)
        {
            return;
        }
		inputDialog->setTip(inputDialogParam->m_editTip);
		inputDialog->setAcceptButton(inputDialogParam->m_buttonText, ACCEPT_BUTTON);
		if (inputDialogParam->m_vecInputEx.empty())
		{
			if (inputDialogParam->m_isPassword)
			{
				inputDialog->setPasswordInputBox(inputDialogParam->m_defaultText, &(inputDialogParam->m_editText), inputDialogParam->m_maxLength);
			}
			else
			{
				inputDialog->setLineEdit(inputDialogParam->m_defaultText, &(inputDialogParam->m_editText), inputDialogParam->m_maxLength);
			}
		}
		else
		{
			inputDialog->setInputEx(&inputDialogParam->m_vecInputEx);
		}
		inputDialog->setEscAltF4Enable(inputDialogParam->m_enableExit);
		inputDialog->setExitVisible(inputDialogParam->m_enableExit);
        param.m_dialogId = dialogId;
        popDialogPtr = inputDialog;
        break;
    }
    case TIP_DIALOG:
    {
		TipDialogParam* tipDialogParam = CastPopDialogParam<TipDialogParam>(param, TIP_DIALOG);
		if (tipDialogParam == nullptr)
		{
			return;
		}
        quint64 dialogId = 0;
		TipDialog* tipDialog = dynamic_cast<TipDialog*>(AllocManager::instance().createDialog(dialogId, *tipDialogParam));
        if (tipDialog == nullptr)
        {
            return;
        }
		tipDialog->setTip(tipDialogParam->m_tip);
		tipDialog->setAcceptButton(tipDialogParam->m_buttonText, ACCEPT_BUTTON);
        param.m_dialogId = dialogId;
        popDialogPtr = tipDialog;
        break;
    }
    case WAIT_DIALOG:
    {
		WaitDialogParam* waitDialogParam = CastPopDialogParam<WaitDialogParam>(param, WAIT_DIALOG);
		if (waitDialogParam == nullptr)
		{
			return;
		}
        quint64 dialogId = 0;
		WaitDialog* waitDialog = dynamic_cast<WaitDialog*>(AllocManager::instance().createDialog(dialogId, *waitDialogParam));
        if (waitDialog == nullptr)
        {
            return;
        }
		waitDialog->setTip(waitDialogParam->m_tip);
        param.m_dialogId = dialogId;
        popDialogPtr = waitDialog;
        break;
    }
    case DOWNLOAD_OPERATE_DIALOG:
    {
		DownloadOperateDialogParam* downloadOperateDialogParam = CastPopDialogParam<DownloadOperateDialogParam>(param, DOWNLOAD_OPERATE_DIALOG);
		if (downloadOperateDialogParam == nullptr)
		{
			return;
		}
        quint64 dialogId = 0;
		DownloadOperateDialog* downloadOperateDialog = dynamic_cast<DownloadOperateDialog*>(AllocManager::instance().createDialog(dialogId, *downloadOperateDialogParam));
        if (downloadOperateDialog == nullptr)
        {
            return;
        }
		downloadOperateDialog->setFileName(downloadOperateDialogParam->m_fileName);
		downloadOperateDialog->setDownloadSpeed(downloadOperateDialogParam->m_downloadSpeed);
		downloadOperateDialog->setDownloaded(downloadOperateDialogParam->m_hasDownloaded);
		downloadOperateDialog->setDownloadTime(downloadOperateDialogParam->m_downloadTime);
		downloadOperateDialog->setRate(downloadOperateDialogParam->m_rate);
		downloadOperateDialog->setBackEnable(downloadOperateDialogParam->m_backEnable);
		downloadOperateDialog->setEditDownloadAddr(downloadOperateDialogParam->m_downloadAddr);
		downloadOperateDialog->setEditPath(downloadOperateDialogParam->m_path);
        param.m_dialogId = dialogId;

        QObject::connect(downloadOperateDialog, &DownloadOperateDialog::changeToBack, this, &PopDialogManager::onChangeToBack);
        QObject::connect(downloadOperateDialog, &DownloadOperateDialog::downloadAgain, this, &PopDialogManager::onDownloadAgain);
        QObject::connect(downloadOperateDialog, &DownloadOperateDialog::cancelDownload, this, &PopDialogManager::onCancelDownload);
        QObject::connect(downloadOperateDialog, &DownloadOperateDialog::useOtherDownload, this, &PopDialogManager::onUseOtherDownload);
        QObject::connect(downloadOperateDialog, &DownloadOperateDialog::copyDownloadAddr, this, &PopDialogManager::onCopyDownloadAddr);
        QObject::connect(downloadOperateDialog, &DownloadOperateDialog::copyPath, this, &PopDialogManager::onCopyPath);

        popDialogPtr = downloadOperateDialog;
        break;
    }
    default:
        break;
    }

    if (popDialogPtr == nullptr)
    {
        return;
    }

    popDialogPtr->setWindowResultAddr(&(param.m_result));
    popDialogPtr->setWindowTitle(param.m_title);
	popDialogPtr->setUserResultPtr(&(param.m_userResult));
    popDialogPtr->setTimeRest(param.m_timeOut);
    popDialogPtr->setTimeRestVisible(param.m_isCountDownVisible);
    popDialogPtr->setTransientWindow(param.m_parent);
    QObject::connect(popDialogPtr, &DialogShow::closedSignal, this, &PopDialogManager::onClosedSignal);
	QObject::connect(popDialogPtr, &DialogBase::alreadyShown, this, &PopDialogManager::onAlreadyShown, Qt::QueuedConnection);

    popDialogPtr->exec();
}

void PopDialogManager::operateDialog(OperateParam& param)
{
	switch (param.operateType())
	{
	case SET_DOWNLOAD_SPEED_OPERATE:
	{
		SetDownloadSpeedOperateParam* operateParam = CastPopOperateParam<SetDownloadSpeedOperateParam>(param, SET_DOWNLOAD_SPEED_OPERATE);
		if (operateParam == nullptr)
		{
			return;
		}
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserKey(operateParam->userKey());
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setDownloadSpeed(operateParam->m_speed);
		break;
	}
	case SET_DOWNLOADED_OPERATE:
	{
		SetDownloadedOperateParam* operateParam = CastPopOperateParam<SetDownloadedOperateParam>(param, SET_DOWNLOADED_OPERATE);
		if (operateParam == nullptr)
		{
			return;
		}
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserKey(operateParam->userKey());
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setDownloaded(operateParam->m_downloaded);
		break;
	}
	case SET_DOWNLOAD_TIME_OPERATE:
	{
		SetDownloadTimeOperateParam* operateParam = CastPopOperateParam<SetDownloadTimeOperateParam>(param, SET_DOWNLOAD_TIME_OPERATE);
		if (operateParam == nullptr)
		{
			return;
		}
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserKey(operateParam->userKey());
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setDownloadTime(operateParam->m_time);
		break;
	}
	case SET_RATE_OPERATE:
	{
		SetRateOperateParam* operateParam = CastPopOperateParam<SetRateOperateParam>(param, SET_RATE_OPERATE);
		if (operateParam == nullptr)
		{
			return;
		}
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserKey(operateParam->userKey());
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setRate(operateParam->m_persent);
		break;
	}
	case SET_EDIT_DOWNLOAD_ADDR_OPERATE:
	{
		SetEditDownloadAddrOperateParam* operateParam = CastPopOperateParam<SetEditDownloadAddrOperateParam>(param, SET_EDIT_DOWNLOAD_ADDR_OPERATE);
		if (operateParam == nullptr)
		{
			return;
		}
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserKey(operateParam->userKey());
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setEditDownloadAddr(operateParam->m_addr);
		break;
	}
	case SET_EDIT_PATH_OPERATE:
	{
		SetEditPathOperateParam* operateParam = CastPopOperateParam<SetEditPathOperateParam>(param, SET_EDIT_PATH_OPERATE);
		if (operateParam == nullptr)
		{
			return;
		}
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserKey(operateParam->userKey());
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setEditPath(operateParam->m_path);
		break;
	}
	case SET_BACK_ENABLE_OPERATE:
	{
		SetBackEnableOperateParam* operateParam = CastPopOperateParam<SetBackEnableOperateParam>(param, SET_BACK_ENABLE_OPERATE);
		if (operateParam == nullptr)
		{
			return;
		}
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserKey(operateParam->userKey());
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setBackEnable(operateParam->m_enable);
		break;
	}
	case DOWNLOAD_ERROR_OPERATE:
	{
		DownloadErrorOperateParam* operateParam = CastPopOperateParam<DownloadErrorOperateParam>(param, DOWNLOAD_ERROR_OPERATE);
		if (operateParam == nullptr)
		{
			return;
		}
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserKey(operateParam->userKey());
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->downloadError();
		break;
	}
	case DOWNLOAD_NORMAL_OPERATE:
	{
		DownloadNormalOperateParam* operateParam = CastPopOperateParam<DownloadNormalOperateParam>(param, DOWNLOAD_NORMAL_OPERATE);
		if (operateParam == nullptr)
		{
			return;
		}
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserKey(operateParam->userKey());
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->downloadNormal();
		break;
	}
	case SET_ERROR_TYPE_OPERATE:
	{
		SetErrorTypeOperateParam* operateParam = CastPopOperateParam<SetErrorTypeOperateParam>(param, SET_ERROR_TYPE_OPERATE);
		if (operateParam == nullptr)
		{
			return;
		}
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserKey(operateParam->userKey());
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setErrorType(operateParam->m_errorText);
		break;
	}

	default:
		break;
	}
}

void PopDialogManager::onClosedSignal(DialogResult* result)
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

	DialogSignalPtr signalParam = CreateDialogSignalParam<PopDialogDoneSignalParam>();
	if (signalParam.isNull())
	{
		return;
	}
	PopDialogDoneSignalParam* param = static_cast<PopDialogDoneSignalParam*>(signalParam.data());
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

void PopDialogManager::onChangeToBack()
{
	DialogSignalPtr signalParam = CreateDialogSignalParam<ChangeToBackSignalParam>();
	if (signalParam.isNull())
	{
		return;
	}
	ChangeToBackSignalParam* param = static_cast<ChangeToBackSignalParam*>(signalParam.data());
	DialogUserKey currentUserKey = userKey();
	param->m_businessId = currentUserKey.m_businessId;
	param->m_userId = currentUserKey.m_userId;
	emit dialogSignal(signalParam);
}

void PopDialogManager::onDownloadAgain()
{
	DialogSignalPtr signalParam = CreateDialogSignalParam<DownloadAgainSignalParam>();
	if (signalParam.isNull())
	{
		return;
	}
	DownloadAgainSignalParam* param = static_cast<DownloadAgainSignalParam*>(signalParam.data());
	DialogUserKey currentUserKey = userKey();
	param->m_businessId = currentUserKey.m_businessId;
	param->m_userId = currentUserKey.m_userId;
	emit dialogSignal(signalParam);
}

void PopDialogManager::onCancelDownload()
{
	DialogSignalPtr signalParam = CreateDialogSignalParam<CancelDownloadSignalParam>();
	if (signalParam.isNull())
	{
		return;
	}
	CancelDownloadSignalParam* param = static_cast<CancelDownloadSignalParam*>(signalParam.data());
	DialogUserKey currentUserKey = userKey();
	param->m_businessId = currentUserKey.m_businessId;
	param->m_userId = currentUserKey.m_userId;
	emit dialogSignal(signalParam);
}

void PopDialogManager::onUseOtherDownload()
{
	DialogSignalPtr signalParam = CreateDialogSignalParam<UseOtherDownloadSignalParam>();
	if (signalParam.isNull())
	{
		return;
	}
	UseOtherDownloadSignalParam* param = static_cast<UseOtherDownloadSignalParam*>(signalParam.data());
	DialogUserKey currentUserKey = userKey();
	param->m_businessId = currentUserKey.m_businessId;
	param->m_userId = currentUserKey.m_userId;
	emit dialogSignal(signalParam);
}

void PopDialogManager::onCopyDownloadAddr(const QString& addr)
{
	DialogSignalPtr signalParam = CreateDialogSignalParam<CopyDownloadAddrSignalParam>();
	if (signalParam.isNull())
	{
		return;
	}
	CopyDownloadAddrSignalParam* param = static_cast<CopyDownloadAddrSignalParam*>(signalParam.data());
	DialogUserKey currentUserKey = userKey();
	param->m_businessId = currentUserKey.m_businessId;
	param->m_userId = currentUserKey.m_userId;
	param->m_addr = addr;
	emit dialogSignal(signalParam);
}

void PopDialogManager::onCopyPath(const QString& path)
{
	DialogSignalPtr signalParam = CreateDialogSignalParam<CopyPathSignalParam>();
	if (signalParam.isNull())
	{
		return;
	}
	CopyPathSignalParam* param = static_cast<CopyPathSignalParam*>(signalParam.data());
	DialogUserKey currentUserKey = userKey();
	param->m_businessId = currentUserKey.m_businessId;
	param->m_userId = currentUserKey.m_userId;
	param->m_path = path;
	emit dialogSignal(signalParam);
}

void PopDialogManager::onAlreadyShown()
{
	DialogSignalPtr signalParam = CreateDialogSignalParam<AlreadyShownSignalParam>();
	if (signalParam.isNull())
	{
		return;
	}
	AlreadyShownSignalParam* param = static_cast<AlreadyShownSignalParam*>(signalParam.data());
	param->m_dialog = AllocManager::instance().findDialogId(qobject_cast<COriginalDialog*>(sender()));
	DialogUserKey currentUserKey = userKey();
	param->m_businessId = currentUserKey.m_businessId;
	param->m_userId = currentUserKey.m_userId;
	emit dialogSignal(signalParam);
}

DialogUserKey PopDialogManager::userKey()
{
    quint64 dialogId = AllocManager::instance().findDialogId(qobject_cast<COriginalDialog*>(sender()));
    return AllocManager::instance().findUserKey(dialogId);
}

DownloadOperateDialog* PopDialogManager::downloadOperateDialogPtrByUserKey(const DialogUserKey& userKey)
{
	quint64 dialogId = AllocManager::instance().findDialogId(userKey);
	return dynamic_cast<DownloadOperateDialog*>(AllocManager::instance().findDialogPtr(dialogId));
}

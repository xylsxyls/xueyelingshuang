#include "PopDialogManager.h"
#include "PopDialog.h"
#include "AskDialog.h"
#include "TipDialog.h"
#include "WaitDialog.h"
#include "AllocManager.h"
#include "AdvertAskDialog.h"
#include "InputDialog.h"
#include "DownloadOperateDialog.h"
#include "DialogType.h"

void PopDialogManager::popDialog(DialogParam& param)
{
    PopDialog* popDialogPtr = nullptr;
	switch (param.dialogType())
    {
    case ASK_DIALOG:
    {
        AskDialogParam& askDialogParam = (AskDialogParam&)param;
        quint64 dialogId = 0;
        AskDialog* askDialog = (AskDialog*)AllocManager::instance().createDialog(dialogId, askDialogParam.m_userId, ASK_DIALOG);
        if (askDialog == nullptr)
        {
            return;
        }
        askDialog->setTip(askDialogParam.m_tip);
        askDialog->setAcceptButton(askDialogParam.m_acceptText, ACCEPT_BUTTON);
        askDialog->setIgnoreButton(askDialogParam.m_ignoreText, IGNORE_BUTTON);
        param.m_dialogId = dialogId;
        popDialogPtr = askDialog;
        break;
    }
    case ADVERT_ASK_DIALOG:
    {
        AdvertAskDialogParam& advertAskDialogParam = (AdvertAskDialogParam&)param;
        quint64 dialogId = 0;
        AdvertAskDialog* advertAskDialog = (AdvertAskDialog*)AllocManager::instance().createDialog(dialogId, advertAskDialogParam.m_userId, ADVERT_ASK_DIALOG);
        if (advertAskDialog == nullptr)
        {
            return;
        }
        advertAskDialog->initAdvertUrl(advertAskDialogParam.m_advertUrl);
        advertAskDialog->setTip(advertAskDialogParam.m_tip);
        advertAskDialog->setAcceptButton(advertAskDialogParam.m_acceptText, ACCEPT_BUTTON);
        advertAskDialog->setIgnoreButton(advertAskDialogParam.m_ignoreText, IGNORE_BUTTON);
        param.m_dialogId = dialogId;
        popDialogPtr = advertAskDialog;
        break;
    }
    case INPUT_DIALOG:
    {
        InputDialogParam& inputDialogParam = (InputDialogParam&)param;
        quint64 dialogId = 0;
        InputDialog* inputDialog = (InputDialog*)AllocManager::instance().createDialog(dialogId, inputDialogParam.m_userId, INPUT_DIALOG);
        if (inputDialog == nullptr)
        {
            return;
        }
        inputDialog->setTip(inputDialogParam.m_editTip);
        inputDialog->setAcceptButton(inputDialogParam.m_buttonText, ACCEPT_BUTTON);
        if (inputDialogParam.m_isPassword)
        {
            inputDialog->setPasswordInputBox(inputDialogParam.m_defaultText, &(inputDialogParam.m_editText), inputDialogParam.m_maxLength);
        }
        else
        {
            inputDialog->setLineEdit(inputDialogParam.m_defaultText, &(inputDialogParam.m_editText), inputDialogParam.m_maxLength);
        }
        param.m_dialogId = dialogId;
        popDialogPtr = inputDialog;
        break;
    }
    case TIP_DIALOG:
    {
        TipDialogParam& tipDialogParam = (TipDialogParam&)param;
        quint64 dialogId = 0;
        TipDialog* tipDialog = (TipDialog*)AllocManager::instance().createDialog(dialogId, tipDialogParam.m_userId, TIP_DIALOG);
        if (tipDialog == nullptr)
        {
            return;
        }
        tipDialog->setTip(tipDialogParam.m_tip);
        tipDialog->setAcceptButton(tipDialogParam.m_buttonText, ACCEPT_BUTTON);
        param.m_dialogId = dialogId;
        popDialogPtr = tipDialog;
        break;
    }
    case WAIT_DIALOG:
    {
        WaitDialogParam& waitDialogParam = (WaitDialogParam&)param;
        quint64 dialogId = 0;
        WaitDialog* waitDialog = (WaitDialog*)AllocManager::instance().createDialog(dialogId, waitDialogParam.m_userId, WAIT_DIALOG);
        if (waitDialog == nullptr)
        {
            return;
        }
        waitDialog->setTip(waitDialogParam.m_tip);
        param.m_dialogId = dialogId;
        popDialogPtr = waitDialog;
        break;
    }
    case DOWNLOAD_DIALOG:
    {
        break;
    }
    case DOWNLOAD_ERROR_DIALOG:
    {
        break;
    }
    case DOWNLOAD_OPERATE_DIALOG:
    {
        DownloadOperateDialogParam& downloadOperateDialogParam = (DownloadOperateDialogParam&)param;
        quint64 dialogId = 0;
        DownloadOperateDialog* downloadOperateDialog = (DownloadOperateDialog*)AllocManager::instance().createDialog(dialogId, downloadOperateDialogParam.m_userId, DOWNLOAD_OPERATE_DIALOG);
        if (downloadOperateDialog == nullptr)
        {
            return;
        }
        downloadOperateDialog->setFileName(downloadOperateDialogParam.m_fileName);
        downloadOperateDialog->setDownloadSpeed(downloadOperateDialogParam.m_downloadSpeed);
        downloadOperateDialog->setDownloaded(downloadOperateDialogParam.m_hasDownloaded);
        downloadOperateDialog->setDownloadTime(downloadOperateDialogParam.m_downloadTime);
        downloadOperateDialog->setRate(downloadOperateDialogParam.m_rate);
        downloadOperateDialog->setBackEnable(downloadOperateDialogParam.m_backEnable);
        downloadOperateDialog->setEditDownloadAddr(downloadOperateDialogParam.m_downloadAddr);
        downloadOperateDialog->setEditPath(downloadOperateDialogParam.m_path);
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
    popDialogPtr->setWindowTiTle(param.m_title);
    popDialogPtr->setUserParam(param.m_userParam);
    popDialogPtr->setTimeRest(param.m_timeOut);
    popDialogPtr->setTimeRestVisible(param.m_isCountDownVisible);
    popDialogPtr->setTransientWindow(param.m_parent);
    QObject::connect(popDialogPtr, &DialogShow::closedSignal, this, &PopDialogManager::onClosedSignal);
    popDialogPtr->exec();
}

void PopDialogManager::operateDialog(OperateParam& param)
{
	switch (param.operateType())
	{
	case SET_DOWNLOAD_SPEED_OPERATE:
	{
		SetDownloadSpeedOperateParam& operateParam = (SetDownloadSpeedOperateParam&)param;
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserId(operateParam.m_userId);
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setDownloadSpeed(operateParam.m_speed);
		break;
	}
	case SET_DOWNLOADED_OPERATE:
	{
		SetDownloadedOperateParam& operateParam = (SetDownloadedOperateParam&)param;
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserId(operateParam.m_userId);
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setDownloaded(operateParam.m_downloaded);
		break;
	}
	case SET_DOWNLOAD_TIME_OPERATE:
	{
		SetDownloadTimeOperateParam& operateParam = (SetDownloadTimeOperateParam&)param;
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserId(operateParam.m_userId);
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setDownloadTime(operateParam.m_time);
		break;
	}
	case SET_RATE_OPERATE:
	{
		SetRateOperateParam& operateParam = (SetRateOperateParam&)param;
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserId(operateParam.m_userId);
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setRate(operateParam.m_persent);
		break;
	}
	case SET_EDIT_DOWNLOAD_ADDR_OPERATE:
	{
		SetEditDownloadAddrOperateParam& operateParam = (SetEditDownloadAddrOperateParam&)param;
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserId(operateParam.m_userId);
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setEditDownloadAddr(operateParam.m_addr);
		break;
	}
	case SET_EDIT_PATH_OPERATE:
	{
		SetEditPathOperateParam& operateParam = (SetEditPathOperateParam&)param;
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserId(operateParam.m_userId);
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setEditPath(operateParam.m_path);
		break;
	}
	case SET_BACK_ENABLE_OPERATE:
	{
		SetBackEnableOperateParam& operateParam = (SetBackEnableOperateParam&)param;
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserId(operateParam.m_userId);
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setBackEnable(operateParam.m_enable);
		break;
	}
	case DOWNLOAD_ERROR_OPERATE:
	{
		DownloadErrorOperateParam& operateParam = (DownloadErrorOperateParam&)param;
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserId(operateParam.m_userId);
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->downloadError();
		break;
	}
	case DOWNLOAD_NORMAL_OPERATE:
	{
		DownloadNormalOperateParam& operateParam = (DownloadNormalOperateParam&)param;
		DownloadOperateDialog* dialogPtr = downloadOperateDialogPtrByUserId(operateParam.m_userId);
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->downloadNormal();
		break;
	}
	
	default:
		break;
	}
}

void PopDialogManager::onClosedSignal(DialogResult* result)
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

	PopDialogDoneSignalParam param;
    param.m_dialogId = dialogId;
    param.m_userId = userId;
    param.m_dialogType = type;
    if (result != nullptr)
    {
        param.m_result = *result;
    }
    param.m_userParam = userParam;

	emit dialogSignal(param);
    AllocManager::instance().removeByDialogId(dialogId);
}

void PopDialogManager::onChangeToBack()
{
	ChangeToBackSignalParam param;
	param.m_userId = userId();
	emit dialogSignal(param);
}

void PopDialogManager::onDownloadAgain()
{
	DownloadAgainSignalParam param;
	param.m_userId = userId();
	emit dialogSignal(param);
}

void PopDialogManager::onCancelDownload()
{
	CancelDownloadSignalParam param;
	param.m_userId = userId();
	emit dialogSignal(param);
}

void PopDialogManager::onUseOtherDownload()
{
	UseOtherDownloadSignalParam param;
	param.m_userId = userId();
	emit dialogSignal(param);
}

void PopDialogManager::onCopyDownloadAddr(const QString& addr)
{
	CopyDownloadAddrSignalParam param;
	param.m_userId = userId();
	param.m_addr = addr;
	emit dialogSignal(param);
}

void PopDialogManager::onCopyPath(const QString& path)
{
	CopyPathSignalParam param;
	param.m_userId = userId();
	param.m_path = path;
	emit dialogSignal(param);
}

quint64 PopDialogManager::userId()
{
    quint64 dialogId = AllocManager::instance().findDialogId((COriginalDialog*)sender());
    return AllocManager::instance().findUserId(dialogId);
}

DownloadOperateDialog* PopDialogManager::downloadOperateDialogPtrByUserId(quint64 userId)
{
	quint64 dialogId = AllocManager::instance().findDialogId(userId);
	return (DownloadOperateDialog*)AllocManager::instance().findDialogPtr(dialogId);
}

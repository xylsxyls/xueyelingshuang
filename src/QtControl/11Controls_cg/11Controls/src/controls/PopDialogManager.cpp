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

void PopDialogManager::popDialog(DialogType type, PopParamBase* param)
{
    PopDialog* popDialogPtr = nullptr;
    
    switch (type)
    {
    case ASK_DIALOG:
    {
        AskDialogParam askDialogParam;
        if (param != nullptr)
        {
            askDialogParam = *((AskDialogParam*)param);
        }

        AskDialog* askDialog = new AskDialog;
        if (askDialog == nullptr)
        {
            break;
        }

        int32_t dialogId = AllocManager::instance().add(askDialog, ASK_DIALOG, askDialogParam.m_userId);
        askDialog->setTip(askDialogParam.m_tip);
        askDialog->setAcceptButton(askDialogParam.m_acceptText, askDialogParam.m_acceptDone);
        askDialog->setIgnoreButton(askDialogParam.m_ignoreText, askDialogParam.m_ignoreDone);
        if (param != nullptr)
        {
            param->m_dialogId = dialogId;
        }

        popDialogPtr = askDialog;
        break;
    }
    case ADVERT_ASK_DIALOG:
    {
        AdvertAskDialogParam advertAskDialogParam;
        if (param != nullptr)
        {
            advertAskDialogParam = *((AdvertAskDialogParam*)param);
        }

        AdvertAskDialog* advertAskDialog = new AdvertAskDialog;
        if (advertAskDialog == nullptr)
        {
            break;
        }

        int32_t dialogId = AllocManager::instance().add(advertAskDialog, ADVERT_ASK_DIALOG, advertAskDialogParam.m_userId);
        advertAskDialog->initAdvertUrl(advertAskDialogParam.m_advertUrl);
        advertAskDialog->setTip(advertAskDialogParam.m_tip);
        advertAskDialog->setAcceptButton(advertAskDialogParam.m_acceptText, advertAskDialogParam.m_acceptDone);
        advertAskDialog->setIgnoreButton(advertAskDialogParam.m_ignoreText, advertAskDialogParam.m_ignoreDone);
        if (param != nullptr)
        {
            param->m_dialogId = dialogId;
        }

        popDialogPtr = advertAskDialog;
        break;
    }
    case INPUT_DIALOG:
    {
        InputDialogParam inputDialogParam;
        if (param != nullptr)
        {
            inputDialogParam = *((InputDialogParam*)param);
        }

        InputDialog* inputDialog = new InputDialog;
        if (inputDialog == nullptr)
        {
            break;
        }

        int32_t dialogId = AllocManager::instance().add(inputDialog, ADVERT_ASK_DIALOG, inputDialogParam.m_userId);
        inputDialog->setTip(inputDialogParam.m_editTip);
        inputDialog->setAcceptButton(inputDialogParam.m_buttonText, inputDialogParam.m_done);
        inputDialog->setLineEdit(inputDialogParam.m_defaultText, &inputDialogParam.m_editText, inputDialogParam.m_maxLength);
        if (param != nullptr)
        {
            param->m_dialogId = dialogId;
        }

        popDialogPtr = inputDialog;
        break;
    }
    case TIP_DIALOG:
    {
        TipDialogParam tipDialogParam;
        if (param != nullptr)
        {
            tipDialogParam = *((TipDialogParam*)param);
        }

        TipDialog* tipDialog = new TipDialog;
        if (tipDialog == nullptr)
        {
            break;
        }
        
        int32_t dialogId = AllocManager::instance().add(tipDialog, TIP_DIALOG, tipDialogParam.m_userId);
        tipDialog->setTip(tipDialogParam.m_tip);
        tipDialog->setAcceptButton(tipDialogParam.m_buttonText, tipDialogParam.m_done);
        if (param != nullptr)
        {
            param->m_dialogId = dialogId;
        }

        popDialogPtr = tipDialog;
        break;
    }
    case WAIT_DIALOG:
    {
        WaitDialogParam waitDialogParam;
        if (param != nullptr)
        {
            waitDialogParam = *((WaitDialogParam*)param);
        }

        WaitDialog* waitDialog = new WaitDialog;
        if (waitDialog == nullptr)
        {
            break;
        }

        int32_t dialogId = AllocManager::instance().add(waitDialog, WAIT_DIALOG, waitDialogParam.m_userId);
        waitDialog->setTip(waitDialogParam.m_tip);
        if (param != nullptr)
        {
            param->m_dialogId = dialogId;
        }

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
        DownloadOperateDialogParam downloadOperateDialogParam;
        if (param != nullptr)
        {
            downloadOperateDialogParam = *((DownloadOperateDialogParam*)param);
        }

        DownloadOperateDialog* downloadOperateDialog = new DownloadOperateDialog;
        if (downloadOperateDialog == nullptr)
        {
            break;
        }

        int32_t dialogId = AllocManager::instance().add(downloadOperateDialog, DOWNLOAD_OPERATE_DIALOG, downloadOperateDialogParam.m_userId);
        downloadOperateDialog->setFileName(downloadOperateDialogParam.m_fileName);
        downloadOperateDialog->setDownloadSpeed(downloadOperateDialogParam.m_downloadSpeed);
        downloadOperateDialog->setDownloaded(downloadOperateDialogParam.m_hasDownloaded);
        downloadOperateDialog->setDownloadTime(downloadOperateDialogParam.m_downloadTime);
        downloadOperateDialog->setRate(downloadOperateDialogParam.m_rate);
        downloadOperateDialog->setBackEnable(downloadOperateDialogParam.m_backEnable);
        downloadOperateDialog->setEditDownloadAddr(downloadOperateDialogParam.m_downloadAddr);
        downloadOperateDialog->setEditPath(downloadOperateDialogParam.m_path);
        if (param != nullptr)
        {
            param->m_dialogId = dialogId;
        }

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

    PopParamBase baseParam;
    if (param != nullptr)
    {
        baseParam = *param;
    }
    popDialogPtr->setWindowTiTle(baseParam.m_title);
    popDialogPtr->setUserParam(baseParam.m_userParam);
    popDialogPtr->setTimeRest(baseParam.m_timeOut);
    popDialogPtr->setTimeRestVisible(baseParam.m_isCountDownVisible);
    popDialogPtr->setTopTransientWindow(baseParam.m_parent);
    QObject::connect(popDialogPtr, &COriginalDialog::finished, this, &PopDialogManager::onFinished);
    DialogResult result = (DialogResult)popDialogPtr->exec();
    if (param != nullptr)
    {
        param->m_result = result;
    }
}

void PopDialogManager::onFinished(int result)
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
    emit popDialogDone(dialogId, userId, type, result, userParam);
}
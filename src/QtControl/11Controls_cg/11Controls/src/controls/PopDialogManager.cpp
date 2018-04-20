#include "PopDialogManager.h"
#include "PopDialog.h"
#include "AskDialog.h"
#include "TipDialog.h"
#include "WaitDialog.h"
#include "AllocManager.h"
#include "AdvertAskDialog.h"
#include "InputDialog.h"
#include "DownloadOperateDialog.h"

void PopDialogManager::popDialog(DialogType type, PopParamBase* param)
{
    PopDialog* popDialogPtr = nullptr;
    
    switch (type)
    {
    case ASK_DIALOG:
    {
        AskDialog* askDialog = new AskDialog;
        param->m_dialogId = AllocManager::instance().add(askDialog, ASK_DIALOG, param->m_userId);
        AskDialogParam askDialogParam;
        if (param != nullptr)
        {
            askDialogParam = *((AskDialogParam*)param);
        }
        askDialog->setTip(askDialogParam.m_tip);
        askDialog->setAcceptButton(askDialogParam.m_acceptText, askDialogParam.m_acceptDone);
        askDialog->setIgnoreButton(askDialogParam.m_ignoreText, askDialogParam.m_ignoreDone);
        popDialogPtr = askDialog;
        break;
    }
    case ADVERT_ASK_DIALOG:
    {
        AdvertAskDialog* advertAskDialog = new AdvertAskDialog;
        param->m_dialogId = AllocManager::instance().add(advertAskDialog, ADVERT_ASK_DIALOG, param->m_userId);
        AskAdvertDialogParam askAdvertDialogParam;
        if (param != nullptr)
        {
            askAdvertDialogParam = *((AskAdvertDialogParam*)param);
        }
        advertAskDialog->initAdvertUrl(askAdvertDialogParam.m_advertUrl);
        advertAskDialog->setTip(askAdvertDialogParam.m_tip);
        advertAskDialog->setAcceptButton(askAdvertDialogParam.m_acceptText, askAdvertDialogParam.m_acceptDone);
        advertAskDialog->setIgnoreButton(askAdvertDialogParam.m_ignoreText, askAdvertDialogParam.m_ignoreDone);
        popDialogPtr = advertAskDialog;
        break;
    }
    case INPUT_DIALOG:
    {
        InputDialog* inputDialog = new InputDialog;
        param->m_dialogId = AllocManager::instance().add(inputDialog, INPUT_DIALOG, param->m_userId);
        InputDialogParam inputDialogParam;
        if (param != nullptr)
        {
            inputDialogParam = *((InputDialogParam*)param);
        }
        inputDialog->setTip(inputDialogParam.m_editTip);
        inputDialog->setAcceptButton(inputDialogParam.m_buttonText, inputDialogParam.m_done);
        inputDialog->initLineEdit(inputDialogParam.m_defaultText, &inputDialogParam.m_editText, inputDialogParam.m_isPassword, inputDialogParam.m_maxLength);
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
        popDialogPtr = tipDialog;

        if (param != nullptr)
        {
            param->m_dialogId = dialogId;
        }
        break;
    }
    case WAIT_DIALOG:
    {
        WaitDialog* waitDialog = new WaitDialog;
        param->m_dialogId = AllocManager::instance().add(waitDialog, WAIT_DIALOG, param->m_userId);
        WaitDialogParam waitDialogParam;
        if (param != nullptr)
        {
            waitDialogParam = *((WaitDialogParam*)param);
        }
        waitDialog->setTip(waitDialogParam.m_tip);
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
        DownloadOperateDialog* downloadOperateDialog = new DownloadOperateDialog;
        param->m_dialogId = AllocManager::instance().add(downloadOperateDialog, DOWNLOAD_OPERATE_DIALOG, param->m_userId);
        DownloadOperateDialogParam downloadOperateDialogParam;
        if (param != nullptr)
        {
            downloadOperateDialogParam = *((DownloadOperateDialogParam*)param);
        }
        downloadOperateDialog->setFileName(downloadOperateDialogParam.m_fileName);
        downloadOperateDialog->setDownloadSpeed(downloadOperateDialogParam.m_downloadSpeed);
        downloadOperateDialog->setDownloaded(downloadOperateDialogParam.m_hasDownloaded);
        downloadOperateDialog->setDownloadTime(downloadOperateDialogParam.m_downloadTime);
        downloadOperateDialog->setRate(downloadOperateDialogParam.m_rate);
        downloadOperateDialog->setBackEnable(downloadOperateDialogParam.m_backEnable);
        downloadOperateDialog->setEditDownloadAddr(downloadOperateDialogParam.m_downloadAddr);
        downloadOperateDialog->setEditPath(downloadOperateDialogParam.m_path);
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
    popDialogPtr->setTimeRest(baseParam.m_timeOut);
    popDialogPtr->setTimeRestVisible(baseParam.m_isCountDownVisible);
    popDialogPtr->setTopTransientWindow(baseParam.m_parent);
    DialogResult result = (DialogResult)popDialogPtr->exec();
    if (param != nullptr)
    {
        param->m_result = result;
    }
}


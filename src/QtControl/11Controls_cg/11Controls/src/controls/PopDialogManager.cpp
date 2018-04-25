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

void PopDialogManager::popDialog(DialogType type, ParamBase* param)
{
    if (param == nullptr)
    {
        return;
    }

    PopDialog* popDialogPtr = nullptr;
    switch (type)
    {
    case ASK_DIALOG:
    {
        AskDialogParam* askDialogParam = (AskDialogParam*)param;
        int32_t dialogId = 0;
        AskDialog* askDialog = (AskDialog*)AllocManager::instance().createDialog(dialogId, askDialogParam->m_userId, ASK_DIALOG);
        if (askDialog == nullptr)
        {
            return;
        }
        askDialog->setTip(askDialogParam->m_tip);
        askDialog->setAcceptButton(askDialogParam->m_acceptText, askDialogParam->m_acceptDone);
        askDialog->setIgnoreButton(askDialogParam->m_ignoreText, askDialogParam->m_ignoreDone);
        param->m_dialogId = dialogId;
        popDialogPtr = askDialog;
        break;
    }
    case ADVERT_ASK_DIALOG:
    {
        AdvertAskDialogParam* advertAskDialogParam = (AdvertAskDialogParam*)param;
        int32_t dialogId = 0;
        AdvertAskDialog* advertAskDialog = (AdvertAskDialog*)AllocManager::instance().createDialog(dialogId, advertAskDialogParam->m_userId, ADVERT_ASK_DIALOG);
        if (advertAskDialog == nullptr)
        {
            return;
        }
        advertAskDialog->initAdvertUrl(advertAskDialogParam->m_advertUrl);
        advertAskDialog->setTip(advertAskDialogParam->m_tip);
        advertAskDialog->setAcceptButton(advertAskDialogParam->m_acceptText, advertAskDialogParam->m_acceptDone);
        advertAskDialog->setIgnoreButton(advertAskDialogParam->m_ignoreText, advertAskDialogParam->m_ignoreDone);
        param->m_dialogId = dialogId;
        popDialogPtr = advertAskDialog;
        break;
    }
    case INPUT_DIALOG:
    {
        InputDialogParam* inputDialogParam = (InputDialogParam*)param;
        int32_t dialogId = 0;
        InputDialog* inputDialog = (InputDialog*)AllocManager::instance().createDialog(dialogId, inputDialogParam->m_userId, INPUT_DIALOG);
        if (inputDialog == nullptr)
        {
            return;
        }
        inputDialog->setTip(inputDialogParam->m_editTip);
        inputDialog->setAcceptButton(inputDialogParam->m_buttonText, inputDialogParam->m_done);
        inputDialog->setLineEdit(inputDialogParam->m_defaultText, &(inputDialogParam->m_editText), inputDialogParam->m_maxLength);
        param->m_dialogId = dialogId;
        popDialogPtr = inputDialog;
        break;
    }
    case TIP_DIALOG:
    {
        TipDialogParam* tipDialogParam = (TipDialogParam*)param;
        int32_t dialogId = 0;
        TipDialog* tipDialog = (TipDialog*)AllocManager::instance().createDialog(dialogId, tipDialogParam->m_userId, TIP_DIALOG);
        if (tipDialog == nullptr)
        {
            return;
        }
        tipDialog->setTip(tipDialogParam->m_tip);
        tipDialog->setAcceptButton(tipDialogParam->m_buttonText, tipDialogParam->m_done);
        param->m_dialogId = dialogId;
        popDialogPtr = tipDialog;
        break;
    }
    case WAIT_DIALOG:
    {
        WaitDialogParam* waitDialogParam = (WaitDialogParam*)param;
        int32_t dialogId = 0;
        WaitDialog* waitDialog = (WaitDialog*)AllocManager::instance().createDialog(dialogId, waitDialogParam->m_userId, WAIT_DIALOG);
        if (waitDialog == nullptr)
        {
            return;
        }
        waitDialog->setTip(waitDialogParam->m_tip);
        param->m_dialogId = dialogId;
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
        DownloadOperateDialogParam* downloadOperateDialogParam = (DownloadOperateDialogParam*)param;
        int32_t dialogId = 0;
        DownloadOperateDialog* downloadOperateDialog = (DownloadOperateDialog*)AllocManager::instance().createDialog(dialogId, downloadOperateDialogParam->m_userId, DOWNLOAD_OPERATE_DIALOG);
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
        param->m_dialogId = dialogId;

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

    popDialogPtr->setWindowTiTle(param->m_title);
    popDialogPtr->setUserParam(param->m_userParam);
    popDialogPtr->setTimeRest(param->m_timeOut);
    popDialogPtr->setTimeRestVisible(param->m_isCountDownVisible);
    popDialogPtr->setTopTransientWindow(param->m_parent);
    QObject::connect(popDialogPtr, &DialogShow::closedSignal, this, &PopDialogManager::onClosedSignal);
    param->m_result = popDialogPtr->exec();
}

void PopDialogManager::onClosedSignal(int result)
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
    AllocManager::instance().removeByDialogId(dialogId);
}

void PopDialogManager::onChangeToBack()
{
    emit changeToBack(userId());
}

void PopDialogManager::onDownloadAgain()
{
    emit downloadAgain(userId());
}

void PopDialogManager::onCancelDownload()
{
    emit cancelDownload(userId());
}

void PopDialogManager::onUseOtherDownload()
{
    emit useOtherDownload(userId());
}

void PopDialogManager::onCopyDownloadAddr(const QString& addr)
{
    emit copyDownloadAddr(userId(), addr);
}

void PopDialogManager::onCopyPath(const QString& path)
{
    emit copyPath(userId(), path);
}

int32_t PopDialogManager::userId()
{
    int32_t dialogId = AllocManager::instance().findDialogId((COriginalDialog*)sender());
    return AllocManager::instance().findUserId(dialogId);
}

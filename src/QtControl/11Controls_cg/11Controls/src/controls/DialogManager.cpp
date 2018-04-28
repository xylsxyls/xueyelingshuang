#include "DialogManager.h"
#include "DialogBase.h"
#include "AskDialog.h"
#include "TipDialog.h"
#include "InputDialog.h"
#include "WaitDialog.h"
#include "TipShowDialog.h"
#include "AskShowDialog.h"
#include "LoginShowDialog.h"
#include "DownloadDialog.h"
#include "DownloadOperateDialog.h"
#include <vector>
#include <Windows.h>
#include "AccountManagerDialog.h"
#include <QWindow>
#include "AdvertAskDialog.h"
#include "PopDialogManager.h"
#include "NotifyDialogManager.h"
#include "StaticDialogManager.h"
#include "AllocManager.h"

DialogManager::DialogManager():
m_init(false)
{
    
}

void DialogManager::createDialog(DialogType type, ParamBase* param)
{
    if (m_init == false)
    {
        m_init = true;
        init();
    }

    switch (type)
    {
    case ERROR_TYPE:
    {
        break;
    }
    case ASK_DIALOG:
    case ADVERT_ASK_DIALOG:
    case INPUT_DIALOG:
    case TIP_DIALOG:
    case WAIT_DIALOG:
    case DOWNLOAD_DIALOG:
    case DOWNLOAD_ERROR_DIALOG:
    case DOWNLOAD_OPERATE_DIALOG:
    {
        PopDialogManager::instance().popDialog(type, (ParamBase*)param);
        break;
    }
    case ASK_SHOW_DIALOG:
    case TIP_SHOW_DIALOG:
    case LOGIN_SHOW_DIALOG:
    {
        NotifyDialogManager::instance().showDialog(type, (ParamBase*)param);
        break;
    }
    case ACCOUNT_MANAGER_DIALOG:
    {
        StaticDialogManager::instance().popStaticDialog(type, (ParamBase*)param);
        break;
    }
    default:
        break;
    }
}

void DialogManager::destroyDialog(int32_t dialogId)
{
    AllocManager::instance().removeByDialogId(dialogId);
}

void DialogManager::destroyLastDialog()
{
    AllocManager::instance().removeByDialogId(AllocManager::instance().findLastDialogId());
}

void DialogManager::destroyAll()
{
    AllocManager::instance().destroyAll();
}

int32_t DialogManager::dialogCount()
{
    return AllocManager::instance().dialogCount();
}

void DialogManager::setDownloadSpeed(int32_t userId, const QString& speed)
{
    PopDialogManager::instance().setDownloadSpeed(userId, speed);
}

void DialogManager::setDownloaded(int32_t userId, const QString& download)
{
    PopDialogManager::instance().setDownloaded(userId, download);
}

void DialogManager::setDownloadTime(int32_t userId, const QString& time)
{
    PopDialogManager::instance().setDownloadTime(userId, time);
}

void DialogManager::setRate(int32_t userId, int32_t persent)
{
    PopDialogManager::instance().setRate(userId, persent);
}

void DialogManager::setEditDownloadAddr(int32_t userId, const QString& addr)
{
    PopDialogManager::instance().setEditDownloadAddr(userId, addr);
}

void DialogManager::setEditPath(int32_t userId, const QString& path)
{
    PopDialogManager::instance().setEditPath(userId, path);
}

void DialogManager::setBackEnable(int32_t userId, bool enable)
{
    PopDialogManager::instance().setBackEnable(userId, enable);
}

void DialogManager::error(int32_t userId)
{
    PopDialogManager::instance().error(userId);
}

void DialogManager::normal(int32_t userId)
{
    PopDialogManager::instance().normal(userId);
}

void DialogManager::init()
{
    QObject::connect(&PopDialogManager::instance(), &PopDialogManager::changeToBack, &DialogManager::instance(), &DialogManager::changeToBack);
    QObject::connect(&PopDialogManager::instance(), &PopDialogManager::downloadAgain, &DialogManager::instance(), &DialogManager::downloadAgain);
    QObject::connect(&PopDialogManager::instance(), &PopDialogManager::cancelDownload, &DialogManager::instance(), &DialogManager::cancelDownload);
    QObject::connect(&PopDialogManager::instance(), &PopDialogManager::useOtherDownload, &DialogManager::instance(), &DialogManager::useOtherDownload);
    QObject::connect(&PopDialogManager::instance(), &PopDialogManager::copyDownloadAddr, &DialogManager::instance(), &DialogManager::copyDownloadAddr);
    QObject::connect(&PopDialogManager::instance(), &PopDialogManager::copyPath, &DialogManager::instance(), &DialogManager::copyPath);
    QObject::connect(&PopDialogManager::instance(), &PopDialogManager::popDialogDone, &DialogManager::instance(), &DialogManager::popDialogDone);

    QObject::connect(&NotifyDialogManager::instance(), &NotifyDialogManager::notifyDialogDone, &DialogManager::instance(), &DialogManager::notifyDialogDone);

    QObject::connect(&StaticDialogManager::instance(), &StaticDialogManager::staticDialogDone, &DialogManager::instance(), &DialogManager::staticDialogDone);
}

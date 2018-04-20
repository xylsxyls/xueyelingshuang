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

void DialogManager::createDialog(DialogType type, void* param)
{
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
        PopDialogManager::instance().popDialog(type, (PopParamBase*)param);
        break;
    }
    case ASK_SHOW_DIALOG:
    case TIP_SHOW_DIALOG:
    case LOGIN_SHOW_DIALOG:
    {
        NotifyDialogManager::instance().showDialog(type, (PopParamBase*)param);
        break;
    }
    case ACCOUNT_MANAGER_DIALOG:
    {
        StaticDialogManager::instance().popStaticDialog(type, (PopParamBase*)param);
        break;
    }
    default:
        break;
    }
}

void DialogManager::closeDialog(int32_t dialogId)
{
	
}

void DialogManager::closeLastDialog()
{
	
}

void DialogManager::destroyAll()
{
	
}

int32_t DialogManager::dialogCount()
{
    //m_mutex.lock();
    //int32_t size = m_mapDialog.size();
    //m_mutex.unlock();
	return 0;
}

DialogManager::DialogManager()
{

}
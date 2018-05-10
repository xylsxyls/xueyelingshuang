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

DialogManager::DialogManager()
{
	
}

void DialogManager::makeDialog(DialogType type, ParamBase* param)
{
	if (param == nullptr)
	{
		return;
	}
	static bool init = false;
	if (init == false)
	{
		init = true;
		QObject::connect(&PopDialogManager::instance(), &PopDialogManager::dialogSignal, &DialogManager::instance(), &DialogManager::dialogSignal);
		QObject::connect(&NotifyDialogManager::instance(), &NotifyDialogManager::dialogSignal, &DialogManager::instance(), &DialogManager::dialogSignal);
		QObject::connect(&StaticDialogManager::instance(), &StaticDialogManager::dialogSignal, &DialogManager::instance(), &DialogManager::dialogSignal);
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
	case ADVERT_SHOW_DIALOG:
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

void DialogManager::operateDialog(OperateType type, OperateParam* param)
{
	switch (type)
	{
	case SET_DOWNLOAD_SPEED:
	case SET_DOWNLOADED:
	case SET_DOWNLOAD_TIME:
	case SET_RATE:
	case SET_EDIT_DOWNLOAD_ADDR:
	case SET_EDIT_PATH:
	case SET_BACK_ENABLE:
	case DOWNLOAD_ERROR:
	case DOWNLOAD_NORMAL:
	{
		PopDialogManager::instance().operateDialog(type, param);
		break;
	}
	case STATIC_DIALOG_DIALOG_ID:
	case POP_ACCOUNT_DIALOG:
	case POP_CLOSURE_DIALOG:
	case SUB_ACCOUNT_PANEL_PTR:
	case ACCOUNT_DIALOG_PTR:
	case CLOSURE_DIALOG_PTR:
	case CLOSE_STATIC_DIALOG:
	{
		StaticDialogManager::instance().operateDialog(type, param);
		break;
	}
	case DIALOG_EXIST_BY_DIALOG_ID:
	{
		DialogExistByDialogIdOperateParam* operateParam = (DialogExistByDialogIdOperateParam*)param;
		operateParam->m_isExist = AllocManager::instance().findDialogPtr(operateParam->m_dialogId) != nullptr;
		break;
	}
	case DIALOG_EXIST_BY_USER_ID:
	{
		DialogExistByUserIdOperateParam* operateParam = (DialogExistByUserIdOperateParam*)param;

		DialogExistByDialogIdOperateParam dialogExistByDialogIdOperateParam;
		dialogExistByDialogIdOperateParam.m_dialogId = AllocManager::instance().findDialogId(operateParam->m_userId);
		operateDialog(DIALOG_EXIST_BY_DIALOG_ID, &dialogExistByDialogIdOperateParam);

		operateParam->m_isExist = dialogExistByDialogIdOperateParam.m_isExist;
		break;
	}
	case DESTROY_DIALOG:
	{
		DestroyDialogOperateParam* operateParam = (DestroyDialogOperateParam*)param;
		DialogShow* dialogPtr = (DialogShow*)AllocManager::instance().findDialogPtr(operateParam->m_dialogId);
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setWindowResult(CODE_DESTROY);
		dialogPtr->close();
		break;
	}
	case DESTROY_DIALOG_BY_USER_ID:
	{
		DestroyDialogByUserIdOperateParam* operateParam = (DestroyDialogByUserIdOperateParam*)param;
		DestroyDialogOperateParam destroyDialogOperateParam;
		destroyDialogOperateParam.m_dialogId = AllocManager::instance().findDialogId(operateParam->m_userId);
		operateDialog(DESTROY_DIALOG, &destroyDialogOperateParam);
		break;
	}
	case DESTROY_LAST_DIALOG:
	{
		DestroyLastDialogOperateParam* operateParam = (DestroyLastDialogOperateParam*)param;
		DestroyDialogOperateParam destroyDialogOperateParam;
		destroyDialogOperateParam.m_dialogId = AllocManager::instance().findLastDialogId();
		operateDialog(DESTROY_DIALOG, &destroyDialogOperateParam);
		break;
	}
	case DESTROY_ALL:
	{
		std::vector<quint64> vecAllDialogId = AllocManager::instance().allDialogId();
		for (auto itDialogId = vecAllDialogId.begin(); itDialogId != vecAllDialogId.end(); ++itDialogId)
		{
			DestroyDialogOperateParam destroyDialogOperateParam;
			destroyDialogOperateParam.m_dialogId = *itDialogId;
			operateDialog(DESTROY_DIALOG, &destroyDialogOperateParam);
		}
		break;
	}
	case DIALOG_HANDLE:
	{
		DialogHandleOperateParam* operateParam = (DialogHandleOperateParam*)param;
		auto dialogPtr = AllocManager::instance().findDialogPtr(operateParam->m_dialogId);
		if (dialogPtr == nullptr)
		{
			return;
		}
		operateParam->m_windowHandle = dialogPtr->windowHandle();
		break;
	}
	case DIALOG_COUNT:
	{
		DialogCountOperateParam* operateParam = (DialogCountOperateParam*)param;
		operateParam->m_count = AllocManager::instance().dialogCount();
		break;
	}
	case STATIC_DIALOG_HANDLE:
	{
		StaticDialogHandleOperateParam* operateParam = (StaticDialogHandleOperateParam*)param;
		StaticDialogDialogIdOperateParam staticDialogDialogIdOperateParam;
		staticDialogDialogIdOperateParam.m_type = operateParam->m_type;
		DialogManager::instance().operateDialog(STATIC_DIALOG_DIALOG_ID, &staticDialogDialogIdOperateParam);
		DialogHandleOperateParam dialogHandleOperateParam;
		dialogHandleOperateParam.m_dialogId = staticDialogDialogIdOperateParam.m_dialogId;
		DialogManager::instance().operateDialog(DIALOG_HANDLE, &dialogHandleOperateParam);
		operateParam->m_windowHandle = dialogHandleOperateParam.m_windowHandle;
		break;
	}
	default:
		break;
	}
}
#include "DialogManager.h"
#include "DialogShow.h"
#include "PopDialogManager.h"
#include "NotifyDialogManager.h"
#include "StaticDialogManager.h"
#include "AllocManager.h"

DialogManager::DialogManager()
{
	
}

void DialogManager::makeDialog(DialogParam& param)
{
	static bool init = false;
	if (init == false)
	{
		init = true;
		QObject::connect(&PopDialogManager::instance(), &PopDialogManager::dialogSignal, &DialogManager::instance(), &DialogManager::dialogSignal);
		QObject::connect(&NotifyDialogManager::instance(), &NotifyDialogManager::dialogSignal, &DialogManager::instance(), &DialogManager::dialogSignal);
		QObject::connect(&StaticDialogManager::instance(), &StaticDialogManager::dialogSignal, &DialogManager::instance(), &DialogManager::dialogSignal);
	}
	switch (param.dialogType())
    {
    case ERROR_DIALOG_TYPE:
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
        PopDialogManager::instance().popDialog(param);
        break;
    }
    case ASK_SHOW_DIALOG:
    case TIP_SHOW_DIALOG:
    case LOGIN_SHOW_DIALOG:
	case ADVERT_SHOW_DIALOG:
    {
        NotifyDialogManager::instance().showDialog(param);
        break;
    }
    case ACCOUNT_MANAGER_DIALOG:
    {
        StaticDialogManager::instance().popStaticDialog(param);
        break;
    }
    default:
        break;
    }
}

void DialogManager::operateDialog(OperateParam& param)
{
	switch (param.operateType())
	{
	case SET_DOWNLOAD_SPEED_OPERATE:
	case SET_DOWNLOADED_OPERATE:
	case SET_DOWNLOAD_TIME_OPERATE:
	case SET_RATE_OPERATE:
	case SET_EDIT_DOWNLOAD_ADDR_OPERATE:
	case SET_EDIT_PATH_OPERATE:
	case SET_BACK_ENABLE_OPERATE:
	case DOWNLOAD_ERROR_OPERATE:
	case DOWNLOAD_NORMAL_OPERATE:
	{
		PopDialogManager::instance().operateDialog(param);
		break;
	}
	case STATIC_DIALOG_DIALOG_ID_OPERATE:
	case POP_ACCOUNT_DIALOG_OPERATE:
	case POP_CLOSURE_DIALOG_OPERATE:
	case SUB_ACCOUNT_PANEL_PTR_OPERATE:
	case ACCOUNT_DIALOG_PTR_OPERATE:
	case CLOSURE_DIALOG_PTR_OPERATE:
	case CLOSE_STATIC_DIALOG_OPERATE:
	{
		StaticDialogManager::instance().operateDialog(param);
		break;
	}
	case DIALOG_EXIST_BY_DIALOG_ID_OPERATE:
	{
		DialogExistByDialogIdOperateParam& operateParam = (DialogExistByDialogIdOperateParam&)param;
		operateParam.m_isExist = AllocManager::instance().findDialogPtr(operateParam.m_dialogId) != nullptr;
		break;
	}
	case DIALOG_EXIST_BY_USER_ID_OPERATE:
	{
		DialogExistByUserIdOperateParam& operateParam = (DialogExistByUserIdOperateParam&)param;

		DialogExistByDialogIdOperateParam dialogExistByDialogIdOperateParam;
		dialogExistByDialogIdOperateParam.m_dialogId = AllocManager::instance().findDialogId(operateParam.m_userId);
		operateDialog(dialogExistByDialogIdOperateParam);

		operateParam.m_isExist = dialogExistByDialogIdOperateParam.m_isExist;
		break;
	}
	case DESTROY_DIALOG_BY_DIALOG_ID_OPERATE:
	{
		DestroyDialogOperateParam& operateParam = (DestroyDialogOperateParam&)param;
		DialogShow* dialogPtr = (DialogShow*)AllocManager::instance().findDialogPtr(operateParam.m_dialogId);
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->setWindowResult(CODE_DESTROY);
		dialogPtr->close();
		break;
	}
	case DESTROY_DIALOG_BY_USER_ID_OPERATE:
	{
		DestroyDialogByUserIdOperateParam& operateParam = (DestroyDialogByUserIdOperateParam&)param;
		DestroyDialogOperateParam destroyDialogOperateParam;
		destroyDialogOperateParam.m_dialogId = AllocManager::instance().findDialogId(operateParam.m_userId);
		operateDialog(destroyDialogOperateParam);
		break;
	}
	case DESTROY_LAST_DIALOG_OPERATE:
	{
		DestroyLastDialogOperateParam& operateParam = (DestroyLastDialogOperateParam&)param;
		DestroyDialogOperateParam destroyDialogOperateParam;
		destroyDialogOperateParam.m_dialogId = AllocManager::instance().findLastDialogId();
		operateDialog(destroyDialogOperateParam);
		break;
	}
	case DESTROY_ALL_OPERATE:
	{
		std::vector<quint64> vecAllDialogId = AllocManager::instance().allDialogId();
		for (auto itDialogId = vecAllDialogId.begin(); itDialogId != vecAllDialogId.end(); ++itDialogId)
		{
			DestroyDialogOperateParam destroyDialogOperateParam;
			destroyDialogOperateParam.m_dialogId = *itDialogId;
			operateDialog(destroyDialogOperateParam);
		}
		break;
	}
	case DIALOG_HANDLE_OPERATE:
	{
		DialogHandleOperateParam& operateParam = (DialogHandleOperateParam&)param;
		auto dialogPtr = AllocManager::instance().findDialogPtr(operateParam.m_dialogId);
		if (dialogPtr == nullptr)
		{
			return;
		}
		operateParam.m_windowHandle = dialogPtr->windowHandle();
		break;
	}
	case DIALOG_COUNT_OPERATE:
	{
		DialogCountOperateParam& operateParam = (DialogCountOperateParam&)param;
		operateParam.m_count = AllocManager::instance().dialogCount();
		break;
	}
	case STATIC_DIALOG_HANDLE_OPERATE:
	{
		StaticDialogHandleOperateParam& operateParam = (StaticDialogHandleOperateParam&)param;
		StaticDialogDialogIdOperateParam staticDialogDialogIdOperateParam;
		staticDialogDialogIdOperateParam.m_dialogType = operateParam.m_dialogType;
		DialogManager::instance().operateDialog(staticDialogDialogIdOperateParam);
		DialogHandleOperateParam dialogHandleOperateParam;
		dialogHandleOperateParam.m_dialogId = staticDialogDialogIdOperateParam.m_dialogId;
		DialogManager::instance().operateDialog(dialogHandleOperateParam);
		operateParam.m_windowHandle = dialogHandleOperateParam.m_windowHandle;
		break;
	}
	default:
		break;
	}
}
#include "StaticDialogManager.h"
#include "PopDialog.h"
#include "AllocManager.h"
#include "AccountManagerDialog.h"

StaticDialogManager::StaticDialogManager() :
m_accountManagerDialogId(0)
{

}

void StaticDialogManager::popStaticDialog(DialogParam& param)
{
    PopDialog* popDialogPtr = nullptr;
	switch (param.dialogType())
    {
    case ACCOUNT_MANAGER_DIALOG:
    {
        AccountManagerDialogParam& accountManagerDialogParam = (AccountManagerDialogParam&)param;
        quint64 dialogId = 0;
        AccountManagerDialog* accountManagerDialog = (AccountManagerDialog*)AllocManager::instance().createDialog(dialogId, accountManagerDialogParam.m_userId, ACCOUNT_MANAGER_DIALOG);
        if (accountManagerDialog == nullptr)
        {
            return;
        }
        param.m_dialogId = dialogId;
        m_accountManagerDialogId = param.m_dialogId;
        popDialogPtr = accountManagerDialog;
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
    QObject::connect(popDialogPtr, &DialogShow::closedSignal, this, &StaticDialogManager::onClosedSignal);
    popDialogPtr->exec();
}

void StaticDialogManager::operateDialog(OperateParam& param)
{
	switch (param.operateType())
	{
	case STATIC_DIALOG_DIALOG_ID_OPERATE:
	{
		StaticDialogDialogIdOperateParam& operateParam = (StaticDialogDialogIdOperateParam&)param;
		operateParam.m_dialogId = staticDialogDialogId(operateParam.m_dialogType);
		break;
	}
	case POP_ACCOUNT_DIALOG_OPERATE:
	{
		PopAccountDialogOperateParam& operateParam = (PopAccountDialogOperateParam&)param;
		AccountManagerDialog* dialogPtr = accountManagerDialogPtr();
		if (dialogPtr == nullptr)
		{
			return;
		}
		operateParam.m_accountName = dialogPtr->popAccountDialog();
		break;
	}
	case POP_CLOSURE_DIALOG_OPERATE:
	{
		PopClosureDialogOperateParam& operateParam = (PopClosureDialogOperateParam&)param;
		AccountManagerDialog* dialogPtr = accountManagerDialogPtr();
		if (dialogPtr == nullptr)
		{
			return;
		}
		dialogPtr->popClosureDialog();
		break;
	}
	case SUB_ACCOUNT_PANEL_PTR_OPERATE:
	{
		SubAccountPanelPtrOperateParam& operateParam = (SubAccountPanelPtrOperateParam&)param;
		AccountManagerDialog* dialogPtr = accountManagerDialogPtr();
		if (dialogPtr == nullptr)
		{
			return;
		}
		operateParam.m_subAccountPanel = dialogPtr->subAccountPanelPtr();
		break;
	}
	case ACCOUNT_DIALOG_PTR_OPERATE:
	{
		AccountDialogPtrOperateParam& operateParam = (AccountDialogPtrOperateParam&)param;
		AccountManagerDialog* dialogPtr = accountManagerDialogPtr();
		if (dialogPtr == nullptr)
		{
			return;
		}
		operateParam.m_accountDialog = dialogPtr->accountDialogPtr();
		break;
	}
	case CLOSURE_DIALOG_PTR_OPERATE:
	{
		ClosureDialogPtrOperateParam& operateParam = (ClosureDialogPtrOperateParam&)param;
		AccountManagerDialog* dialogPtr = accountManagerDialogPtr();
		if (dialogPtr == nullptr)
		{
			return;
		}
		operateParam.m_closureDialog = dialogPtr->closureDialogPtr();
		break;
	}
	case CLOSE_STATIC_DIALOG_OPERATE:
	{
		CloseStaticDialogOperateParam& operateParam = (CloseStaticDialogOperateParam&)param;
		closeStaticDialog(operateParam.m_dialogType);
		break;
	}
	default:
		break;
	}
}

void StaticDialogManager::closeStaticDialog(DialogType type)
{
    switch (type)
    {
    case ACCOUNT_MANAGER_DIALOG:
    {
        COriginalDialog* dialogPtr = AllocManager::instance().findDialogPtr(m_accountManagerDialogId);
        if (dialogPtr != nullptr)
        {
            dialogPtr->close();
        }
        break;
    }
    default:
        break;
    }
}

quint64 StaticDialogManager::staticDialogDialogId(DialogType type)
{
    switch (type)
    {
    case ACCOUNT_MANAGER_DIALOG:
    {
        return m_accountManagerDialogId;
        break;
    }
    default:
        break;
    }
    return 0;
}

void StaticDialogManager::onClosedSignal(DialogResult* result)
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
    StaticDialogDoneSignalParam param;
    param.m_dialogId = dialogId;
    param.m_userId = userId;
    param.m_dialogType = type;
    if (result != nullptr)
    {
        param.m_result = *result;
    }
    param.m_userParam = userParam;
	emit dialogSignal(param);
}

AccountManagerDialog* StaticDialogManager::accountManagerDialogPtr()
{
	return (AccountManagerDialog*)AllocManager::instance().findDialogPtr(m_accountManagerDialogId);
}

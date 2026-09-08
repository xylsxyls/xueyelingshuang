#include "StaticDialogManager.h"
#include "AllocManager.h"
#include "AccountManagerDialog.h"
#include "DialogHelper.h"

/** 将静态窗口创建参数转换为指定派生类型，避免传错参数导致未定义行为
@param [in] param 调用方传入的创建参数
@param [in] expectedType 期望的窗口类型
@return 返回转换后的参数指针，失败时返回nullptr
*/
template<typename ParamType>
static ParamType* CastStaticDialogParam(DialogParam& param, DialogType expectedType)
{
	if (param.dialogType() != expectedType)
	{
		return nullptr;
	}
	ParamType* castParam = dynamic_cast<ParamType*>(&param);
	if (castParam == nullptr)
	{
		DialogHelper::logFile() << "StaticDialogManager dialog param type mismatch, dialogType = "
			<< expectedType << std::endl;
	}
	return castParam;
}

/** 将静态窗口操作参数转换为指定派生类型，避免传错参数导致未定义行为
@param [in] param 调用方传入的操作参数
@param [in] expectedType 期望的操作类型
@return 返回转换后的参数指针，失败时返回nullptr
*/
template<typename ParamType>
static ParamType* CastStaticOperateParam(OperateParam& param, OperateType expectedType)
{
	if (param.operateType() != expectedType)
	{
		return nullptr;
	}
	ParamType* castParam = dynamic_cast<ParamType*>(&param);
	if (castParam == nullptr)
	{
		DialogHelper::logFile() << "StaticDialogManager operate param type mismatch, operateType = "
			<< expectedType << std::endl;
	}
	return castParam;
}

StaticDialogManager::StaticDialogManager() :
m_accountManagerDialogId(0)
{

}

StaticDialogManager::~StaticDialogManager()
{
	AllocManager::instance().removeByDialogId(m_accountManagerDialogId);
}

void StaticDialogManager::popStaticDialog(DialogParam& param)
{
    PopDialog* popDialogPtr = nullptr;
	switch (param.dialogType())
    {
    case ACCOUNT_MANAGER_DIALOG:
    {
		AccountManagerDialogParam* accountManagerDialogParam =
			CastStaticDialogParam<AccountManagerDialogParam>(param, ACCOUNT_MANAGER_DIALOG);
		if (accountManagerDialogParam == nullptr)
		{
			return;
		}
		bool bExist = false;
		auto dialogPtr = AllocManager::instance().findDialogPtrByType(ACCOUNT_MANAGER_DIALOG);
		if (dialogPtr != nullptr)
		{
			bExist = true;
		}

        quint64 dialogId = 0;
		AccountManagerDialog* accountManagerDialog =
			dynamic_cast<AccountManagerDialog*>(AllocManager::instance().createDialog(dialogId, *accountManagerDialogParam));
        if (accountManagerDialog == nullptr)
        {
            return;
        }
        param.m_dialogId = dialogId;
        m_accountManagerDialogId = param.m_dialogId;
        popDialogPtr = accountManagerDialog;

		if (bExist == false)
		{
			QObject::connect(popDialogPtr, &DialogShow::closedSignal, this, &StaticDialogManager::onClosedSignal);
			QObject::connect(popDialogPtr, &DialogBase::alreadyShown, this, &StaticDialogManager::onAlreadyShown, Qt::QueuedConnection);
		}
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
    popDialogPtr->exec();
	popDialogPtr->setWindowResultAddr(nullptr);
	popDialogPtr->setUserResultPtr(nullptr);
}

void StaticDialogManager::operateDialog(OperateParam& param)
{
	switch (param.operateType())
	{
	case STATIC_DIALOG_DIALOG_ID_OPERATE:
	{
		StaticDialogDialogIdOperateParam* operateParam =
			CastStaticOperateParam<StaticDialogDialogIdOperateParam>(param, STATIC_DIALOG_DIALOG_ID_OPERATE);
		if (operateParam != nullptr)
		{
			operateParam->m_dialogId = staticDialogDialogId(operateParam->m_dialogType);
		}
		break;
	}
	case POP_ACCOUNT_DIALOG_OPERATE:
	{
		PopAccountDialogOperateParam* operateParam =
			CastStaticOperateParam<PopAccountDialogOperateParam>(param, POP_ACCOUNT_DIALOG_OPERATE);
		if (operateParam == nullptr)
		{
			break;
		}
		AccountManagerDialog* dialogPtr = accountManagerDialogPtr();
		if (dialogPtr == nullptr)
		{
			return;
		}
		operateParam->m_accountName = dialogPtr->popAccountDialog();
		break;
	}
	case POP_CLOSURE_DIALOG_OPERATE:
	{
		PopClosureDialogOperateParam* operateParam =
			CastStaticOperateParam<PopClosureDialogOperateParam>(param, POP_CLOSURE_DIALOG_OPERATE);
		if (operateParam == nullptr)
		{
			break;
		}
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
		SubAccountPanelPtrOperateParam* operateParam =
			CastStaticOperateParam<SubAccountPanelPtrOperateParam>(param, SUB_ACCOUNT_PANEL_PTR_OPERATE);
		if (operateParam == nullptr)
		{
			break;
		}
		AccountManagerDialog* dialogPtr = accountManagerDialogPtr();
		if (dialogPtr == nullptr)
		{
			return;
		}
		operateParam->m_subAccountPanel = dialogPtr->subAccountPanelPtr();
		break;
	}
	case ACCOUNT_DIALOG_PTR_OPERATE:
	{
		AccountDialogPtrOperateParam* operateParam =
			CastStaticOperateParam<AccountDialogPtrOperateParam>(param, ACCOUNT_DIALOG_PTR_OPERATE);
		if (operateParam == nullptr)
		{
			break;
		}
		AccountManagerDialog* dialogPtr = accountManagerDialogPtr();
		if (dialogPtr == nullptr)
		{
			return;
		}
		operateParam->m_accountDialog = dialogPtr->accountDialogPtr();
		break;
	}
	case CLOSURE_DIALOG_PTR_OPERATE:
	{
		ClosureDialogPtrOperateParam* operateParam =
			CastStaticOperateParam<ClosureDialogPtrOperateParam>(param, CLOSURE_DIALOG_PTR_OPERATE);
		if (operateParam == nullptr)
		{
			break;
		}
		AccountManagerDialog* dialogPtr = accountManagerDialogPtr();
		if (dialogPtr == nullptr)
		{
			return;
		}
		operateParam->m_closureDialog = dialogPtr->closureDialogPtr();
		break;
	}
	case CLOSE_STATIC_DIALOG_OPERATE:
	{
		CloseStaticDialogOperateParam* operateParam =
			CastStaticOperateParam<CloseStaticDialogOperateParam>(param, CLOSE_STATIC_DIALOG_OPERATE);
		if (operateParam != nullptr)
		{
			closeStaticDialog(operateParam->m_dialogType);
		}
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
	DialogSignalPtr signalParam = CreateDialogSignalParam<StaticDialogDoneSignalParam>();
	if (signalParam.isNull())
	{
		DialogHelper::logFile() << "StaticDialogManager failed to create done signal" << std::endl;
		return;
	}
	StaticDialogDoneSignalParam* param = static_cast<StaticDialogDoneSignalParam*>(signalParam.data());
    param->m_dialogId = dialogId;
	param->m_businessId = currentUserKey.m_businessId;
    param->m_userId = currentUserKey.m_userId;
    param->m_dialogType = type;
    if (result != nullptr)
    {
        param->m_result = *result;
    }
    param->m_userResult = userResult;
	emit dialogSignal(signalParam);
}

void StaticDialogManager::onAlreadyShown()
{
	DialogSignalPtr signalParam = CreateDialogSignalParam<AlreadyShownSignalParam>();
	if (signalParam.isNull())
	{
		DialogHelper::logFile() << "StaticDialogManager failed to create already shown signal" << std::endl;
		return;
	}
	AlreadyShownSignalParam* param = static_cast<AlreadyShownSignalParam*>(signalParam.data());
	param->m_dialog = AllocManager::instance().findDialogId(qobject_cast<COriginalDialog*>(sender()));
	DialogUserKey currentUserKey = AllocManager::instance().findUserKey(param->m_dialog);
	param->m_businessId = currentUserKey.m_businessId;
	param->m_userId = currentUserKey.m_userId;
	emit dialogSignal(signalParam);
}

AccountManagerDialog* StaticDialogManager::accountManagerDialogPtr()
{
	return dynamic_cast<AccountManagerDialog*>(AllocManager::instance().findDialogPtr(m_accountManagerDialogId));
}

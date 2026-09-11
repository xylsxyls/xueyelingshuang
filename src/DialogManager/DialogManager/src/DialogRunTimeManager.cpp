#include "DialogRunTimeManager.h"
#include "AllocManager.h"
#include "CustomDialog.h"
#include "DialogFactoryManager.h"
#include "DialogHelper.h"
#include "NotifyDialogManager.h"
#include "PopDialogManager.h"
#include "StaticDialogManager.h"
#include "QtControls/COriginalDialog.h"
#include "QtControls/DialogBase.h"
#include "QtControls/DialogShow.h"
#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>

/** 将运行期操作参数转换为指定派生类型，避免传错参数导致越界读写
@param [in] param 调用方传入的操作参数
@param [in] expectedType 期望的操作类型
@return 返回转换后的参数指针，失败时返回nullptr
*/
template<typename ParamType>
static ParamType* CastRunTimeOperateParam(OperateParam* param, OperateType expectedType)
{
	if (param == nullptr || param->operateType() != expectedType)
	{
		return nullptr;
	}
	ParamType* castParam = dynamic_cast<ParamType*>(param);
	if (castParam == nullptr)
	{
		DialogHelper::logFile() << "DialogRunTimeManager operate param type mismatch, operateType = "
			<< expectedType << std::endl;
	}
	return castParam;
}

DialogRunTimeManager::DialogRunTimeManager() :
m_isManagersConnected(false),
m_isUninit(false)
{
	qRegisterMetaType<DialogParam*>("DialogParam*");
	qRegisterMetaType<OperateParam*>("OperateParam*");
	qRegisterMetaType<DialogUserKey>("DialogUserKey");
	qRegisterMetaType<DialogSignalPtr>("DialogSignalPtr");
	ensureObjectInGuiThread();
}

DialogRunTimeManager::~DialogRunTimeManager()
{
	uninit();
}

bool DialogRunTimeManager::registerCustomDialogFactory(DialogType dialogType,
													   CustomDialogFactory* factory,
													   CustomDialogFactoryDestroy destroyFunction)
{
	return DialogFactoryManager::instance().registerCustomDialogFactory(dialogType, factory, destroyFunction);
}

void DialogRunTimeManager::unregisterCustomDialogFactory(DialogType dialogType)
{
	DialogFactoryManager::instance().unregisterCustomDialogFactory(dialogType);
}

bool DialogRunTimeManager::registerCustomViewFactory(DialogType dialogType,
													 CustomViewFactory* factory,
													 DialogShowMode showMode,
													 CustomViewFactoryDestroy destroyFunction)
{
	return DialogFactoryManager::instance().registerCustomViewFactory(dialogType, factory, showMode, destroyFunction);
}

void DialogRunTimeManager::unregisterCustomViewFactory(DialogType dialogType)
{
	DialogFactoryManager::instance().unregisterCustomViewFactory(dialogType);
}

void DialogRunTimeManager::makeDialog(DialogParam& param)
{
	ensureObjectInGuiThread();
	if (isGuiThread())
	{
		makeDialogInGuiThread(&param);
		return;
	}
	if (!canBlockInvokeGuiThread("makeDialog"))
	{
		return;
	}
	bool invokeOk = false;
	try
	{
		invokeOk = QMetaObject::invokeMethod(this,
											 "makeDialogInGuiThread",
											 Qt::BlockingQueuedConnection,
											 Q_ARG(DialogParam*, &param));
	}
	catch (...)
	{
		DialogHelper::logFile() << "DialogRunTimeManager makeDialog invokeMethod caught exception, type = "
			<< param.dialogType() << std::endl;
		return;
	}
	if (!invokeOk)
	{
		DialogHelper::logFile() << "DialogRunTimeManager makeDialog invokeMethod failed, type = "
			<< param.dialogType() << std::endl;
	}
}

void DialogRunTimeManager::operateDialog(OperateParam& param)
{
	ensureObjectInGuiThread();
	if (isGuiThread())
	{
		operateDialogInGuiThread(&param);
		return;
	}
	if (!canBlockInvokeGuiThread("operateDialog"))
	{
		return;
	}
	bool invokeOk = false;
	try
	{
		invokeOk = QMetaObject::invokeMethod(this,
											 "operateDialogInGuiThread",
											 Qt::BlockingQueuedConnection,
											 Q_ARG(OperateParam*, &param));
	}
	catch (...)
	{
		DialogHelper::logFile() << "DialogRunTimeManager operateDialog invokeMethod caught exception, operateType = "
			<< param.operateType() << std::endl;
		return;
	}
	if (!invokeOk)
	{
		DialogHelper::logFile() << "DialogRunTimeManager operateDialog invokeMethod failed, operateType = "
			<< param.operateType() << std::endl;
	}
}

void DialogRunTimeManager::uninit()
{
	{
		QMutexLocker locker(&m_mutex);
		if (m_isUninit)
		{
			return;
		}
		m_isUninit = true;
	}
	ensureObjectInGuiThread();
	if (isGuiThread())
	{
		uninitInGuiThread();
		return;
	}
	if (!canBlockInvokeGuiThread("uninit"))
	{
		DialogHelper::logFile() << "DialogRunTimeManager uninit falls back to AllocManager::uninit" << std::endl;
		AllocManager::instance().uninit();
		return;
	}
	bool invokeOk = false;
	try
	{
		invokeOk = QMetaObject::invokeMethod(this, "uninitInGuiThread", Qt::BlockingQueuedConnection);
	}
	catch (...)
	{
		DialogHelper::logFile() << "DialogRunTimeManager uninit invokeMethod caught exception" << std::endl;
		return;
	}
	if (!invokeOk)
	{
		DialogHelper::logFile() << "DialogRunTimeManager uninit invokeMethod failed" << std::endl;
	}
}

void DialogRunTimeManager::makeDialogInGuiThread(DialogParam* param)
{
	if (param == nullptr)
	{
		return;
	}
	{
		QMutexLocker locker(&m_mutex);
		if (m_isUninit)
		{
			DialogHelper::logFile() << "DialogRunTimeManager makeDialog ignored during uninit, type = "
				<< param->dialogType() << std::endl;
			return;
		}
	}
	ensureManagersConnected();
	if (reuseExistingUserDialog(*param))
	{
		return;
	}
	if (DialogFactoryManager::instance().isCustomDialogType(param->dialogType()))
	{
		makeCustomDialog(*param);
		return;
	}
	makeBuiltInDialog(*param);
}

void DialogRunTimeManager::operateDialogInGuiThread(OperateParam* param)
{
	if (param == nullptr)
	{
		return;
	}
	switch (param->operateType())
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
	case SET_ERROR_TYPE_OPERATE:
	{
		PopDialogManager::instance().operateDialog(*param);
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
		StaticDialogManager::instance().operateDialog(*param);
		break;
	}
	case DIALOG_EXIST_BY_DIALOG_ID_OPERATE:
	{
		DialogExistByDialogIdOperateParam* operateParam =
			CastRunTimeOperateParam<DialogExistByDialogIdOperateParam>(param, DIALOG_EXIST_BY_DIALOG_ID_OPERATE);
		if (operateParam != nullptr)
		{
			operateParam->m_isExist = AllocManager::instance().findDialogPtr(operateParam->m_dialogId) != nullptr;
		}
		break;
	}
	case DIALOG_EXIST_BY_USER_ID_OPERATE:
	{
		DialogExistByUserIdOperateParam* operateParam =
			CastRunTimeOperateParam<DialogExistByUserIdOperateParam>(param, DIALOG_EXIST_BY_USER_ID_OPERATE);
		if (operateParam != nullptr)
		{
			quint64 dialogId = AllocManager::instance().findDialogId(operateParam->userKey());
			operateParam->m_isExist = AllocManager::instance().findDialogPtr(dialogId) != nullptr;
		}
		break;
	}
	case DIALOG_ID_BY_USER_ID_OPERATE:
	{
		DialogIdByUserIdOperateParam* operateParam =
			CastRunTimeOperateParam<DialogIdByUserIdOperateParam>(param, DIALOG_ID_BY_USER_ID_OPERATE);
		if (operateParam != nullptr)
		{
			operateParam->m_dialogId = AllocManager::instance().findDialogId(operateParam->userKey());
		}
		break;
	}
	case CHANGE_USER_RESULT_BY_DIALOG_ID_OPERATE:
	{
		ChangeUserResultByDialogIdOperateParam* operateParam =
			CastRunTimeOperateParam<ChangeUserResultByDialogIdOperateParam>(param, CHANGE_USER_RESULT_BY_DIALOG_ID_OPERATE);
		if (operateParam == nullptr)
		{
			break;
		}
		DialogShow* dialogPtr = qobject_cast<DialogShow*>(AllocManager::instance().findDialogPtr(operateParam->m_dialogId));
		if (dialogPtr != nullptr)
		{
			dialogPtr->setUserResult(operateParam->m_userResult);
		}
		break;
	}
	case CHANGE_USER_RESULT_BY_USER_ID_OPERATE:
	{
		ChangeUserResultByUserIdOperateParam* operateParam =
			CastRunTimeOperateParam<ChangeUserResultByUserIdOperateParam>(param, CHANGE_USER_RESULT_BY_USER_ID_OPERATE);
		if (operateParam == nullptr)
		{
			break;
		}
		ChangeUserResultByDialogIdOperateParam changeUserResultByDialogIdOperateParam;
		changeUserResultByDialogIdOperateParam.m_dialogId = AllocManager::instance().findDialogId(operateParam->userKey());
		changeUserResultByDialogIdOperateParam.m_userResult = operateParam->m_userResult;
		operateDialogInGuiThread(&changeUserResultByDialogIdOperateParam);
		break;
	}
	case DESTROY_DIALOG_BY_DIALOG_ID_OPERATE:
	{
		DestroyDialogByDialogIdOperateParam* operateParam =
			CastRunTimeOperateParam<DestroyDialogByDialogIdOperateParam>(param, DESTROY_DIALOG_BY_DIALOG_ID_OPERATE);
		if (operateParam != nullptr)
		{
			closeDialogByDialogId(operateParam->m_dialogId, CODE_DESTROY);
		}
		break;
	}
	case DESTROY_DIALOG_BY_USER_ID_OPERATE:
	{
		DestroyDialogByUserIdOperateParam* operateParam =
			CastRunTimeOperateParam<DestroyDialogByUserIdOperateParam>(param, DESTROY_DIALOG_BY_USER_ID_OPERATE);
		if (operateParam != nullptr)
		{
			closeDialogByDialogId(AllocManager::instance().findDialogId(operateParam->userKey()), CODE_DESTROY);
		}
		break;
	}
	case DESTROY_LAST_DIALOG_OPERATE:
	{
		closeDialogByDialogId(AllocManager::instance().findLastDialogId(), CODE_DESTROY);
		break;
	}
	case DESTROY_ALL_OPERATE:
	{
		std::vector<quint64> vecAllDialogId = AllocManager::instance().allDialogId();
		for (auto itDialogId = vecAllDialogId.begin(); itDialogId != vecAllDialogId.end(); ++itDialogId)
		{
			closeDialogByDialogId(*itDialogId, CODE_DESTROY);
		}
		break;
	}
	case DIALOG_HANDLE_OPERATE:
	{
		DialogHandleOperateParam* operateParam =
			CastRunTimeOperateParam<DialogHandleOperateParam>(param, DIALOG_HANDLE_OPERATE);
		if (operateParam == nullptr)
		{
			break;
		}
		COriginalDialog* dialogPtr = AllocManager::instance().findDialogPtr(operateParam->m_dialogId);
		if (dialogPtr != nullptr)
		{
			operateParam->m_windowHandle = dialogPtr->windowHandle();
		}
		break;
	}
	case DIALOG_COUNT_OPERATE:
	{
		DialogCountOperateParam* dialogCountOperateParam =
			CastRunTimeOperateParam<DialogCountOperateParam>(param, DIALOG_COUNT_OPERATE);
		if (dialogCountOperateParam == nullptr)
		{
			break;
		}
		dialogCountOperateParam->m_count = 0;
		if (dialogCountOperateParam->m_vecOperateType.empty())
		{
			dialogCountOperateParam->m_count = AllocManager::instance().dialogCount();
			break;
		}
		for (auto itType = dialogCountOperateParam->m_vecOperateType.begin();
			itType != dialogCountOperateParam->m_vecOperateType.end();
			++itType)
		{
			dialogCountOperateParam->m_count += AllocManager::instance().dialogTypeCount(*itType);
		}
		break;
	}
	case STATIC_DIALOG_HANDLE_OPERATE:
	{
		StaticDialogHandleOperateParam* operateParam =
			CastRunTimeOperateParam<StaticDialogHandleOperateParam>(param, STATIC_DIALOG_HANDLE_OPERATE);
		if (operateParam == nullptr)
		{
			break;
		}
		StaticDialogDialogIdOperateParam staticDialogDialogIdOperateParam;
		staticDialogDialogIdOperateParam.m_dialogType = operateParam->m_dialogType;
		operateDialogInGuiThread(&staticDialogDialogIdOperateParam);
		DialogHandleOperateParam dialogHandleOperateParam;
		dialogHandleOperateParam.m_dialogId = staticDialogDialogIdOperateParam.m_dialogId;
		operateDialogInGuiThread(&dialogHandleOperateParam);
		operateParam->m_windowHandle = dialogHandleOperateParam.m_windowHandle;
		break;
	}
	default:
		DialogHelper::logFile() << "DialogRunTimeManager unsupported operateType = "
			<< param->operateType() << std::endl;
		break;
	}
}

void DialogRunTimeManager::uninitInGuiThread()
{
	std::vector<quint64> vecAllDialogId = AllocManager::instance().allDialogId();
	for (auto itDialogId = vecAllDialogId.begin(); itDialogId != vecAllDialogId.end(); ++itDialogId)
	{
		closeDialogByDialogId(*itDialogId, CODE_DESTROY);
	}
	AllocManager::instance().uninit();
}

void DialogRunTimeManager::onCustomDialogClosed(DialogResult* result)
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
	DialogSignalPtr signalParam = CreateDialogSignalParam<CustomDialogDoneSignalParam>();
	if (signalParam.isNull())
	{
		DialogHelper::logFile() << "DialogRunTimeManager failed to create custom done signal" << std::endl;
		return;
	}
	CustomDialogDoneSignalParam* param = static_cast<CustomDialogDoneSignalParam*>(signalParam.data());
	param->m_dialogId = dialogId;
	DialogUserKey currentUserKey = AllocManager::instance().findUserKey(dialogId);
	param->m_businessId = currentUserKey.m_businessId;
	param->m_userId = currentUserKey.m_userId;
	param->m_dialogType = AllocManager::instance().findDialogType(dialogId);
	if (result != nullptr)
	{
		param->m_result = *result;
	}
	param->m_userResult = dialogPtr->userResult();

	dialogPtr->clearResultStorage();
	AllocManager::instance().removeByDialogId(dialogId);
	emit dialogSignal(signalParam);
}

void DialogRunTimeManager::onCustomDialogAlreadyShown()
{
	COriginalDialog* senderDialog = qobject_cast<COriginalDialog*>(sender());
	quint64 dialogId = AllocManager::instance().findDialogId(senderDialog);
	if (dialogId == 0)
	{
		return;
	}
	DialogSignalPtr signalParam = CreateDialogSignalParam<AlreadyShownSignalParam>();
	if (signalParam.isNull())
	{
		DialogHelper::logFile() << "DialogRunTimeManager failed to create already shown signal" << std::endl;
		return;
	}
	AlreadyShownSignalParam* param = static_cast<AlreadyShownSignalParam*>(signalParam.data());
	param->m_dialog = dialogId;
	DialogUserKey currentUserKey = AllocManager::instance().findUserKey(dialogId);
	param->m_businessId = currentUserKey.m_businessId;
	param->m_userId = currentUserKey.m_userId;
	emit dialogSignal(signalParam);
}

void DialogRunTimeManager::ensureObjectInGuiThread()
{
	QCoreApplication* application = QCoreApplication::instance();
	if (application == nullptr)
	{
		return;
	}
	if (thread() == application->thread())
	{
		return;
	}
	if (QThread::currentThread() == thread())
	{
		moveToThread(application->thread());
		return;
	}
	DialogHelper::logFile() << "DialogRunTimeManager cannot move object to GUI thread from current thread" << std::endl;
}

bool DialogRunTimeManager::canBlockInvokeGuiThread(const char* operationName) const
{
	QCoreApplication* application = QCoreApplication::instance();
	if (application == nullptr)
	{
		DialogHelper::logFile() << "DialogRunTimeManager " << operationName
			<< " failed because QCoreApplication is null" << std::endl;
		return false;
	}
	if (QCoreApplication::closingDown())
	{
		DialogHelper::logFile() << "DialogRunTimeManager " << operationName
			<< " ignored because QCoreApplication is closing down" << std::endl;
		return false;
	}
	if (thread() != application->thread())
	{
		DialogHelper::logFile() << "DialogRunTimeManager " << operationName
			<< " failed because manager object is not in GUI thread" << std::endl;
		return false;
	}
	return true;
}

void DialogRunTimeManager::ensureManagersConnected()
{
	QMutexLocker locker(&m_mutex);
	if (m_isManagersConnected)
	{
		return;
	}
	QObject::connect(&PopDialogManager::instance(),
					 &PopDialogManager::dialogSignal,
					 this,
					 &DialogRunTimeManager::dialogSignal,
					 Qt::UniqueConnection);
	QObject::connect(&NotifyDialogManager::instance(),
					 &NotifyDialogManager::dialogSignal,
					 this,
					 &DialogRunTimeManager::dialogSignal,
					 Qt::UniqueConnection);
	QObject::connect(&StaticDialogManager::instance(),
					 &StaticDialogManager::dialogSignal,
					 this,
					 &DialogRunTimeManager::dialogSignal,
					 Qt::UniqueConnection);
	m_isManagersConnected = true;
}

bool DialogRunTimeManager::reuseExistingUserDialog(DialogParam& param)
{
	DialogUserKey userKey = param.userKey();
	if (!userKey.isValid())
	{
		return false;
	}
	quint64 dialogId = AllocManager::instance().findDialogId(userKey);
	if (dialogId == 0)
	{
		return false;
	}
	COriginalDialog* dialogPtr = AllocManager::instance().findDialogPtr(dialogId);
	if (dialogPtr == nullptr)
	{
		AllocManager::instance().removeByUserKey(userKey);
		return false;
	}
	param.m_dialogId = dialogId;
	DialogType oldType = AllocManager::instance().findDialogType(dialogId);
	if (oldType != param.dialogType())
	{
		DialogHelper::logFile() << "DialogRunTimeManager reuse existing dialog with different type, businessId = "
			<< userKey.m_businessId << ", userId = " << userKey.m_userId
			<< ", oldType = " << oldType << ", newType = " << param.dialogType() << std::endl;
	}
	CustomDialog* customDialog = dynamic_cast<CustomDialog*>(dialogPtr);
	if (customDialog != nullptr)
	{
		customDialog->updateCustomView(param);
	}
	if (param.m_isActivateWhenReuse)
	{
		activateDialog(dialogPtr);
	}
	return true;
}

void DialogRunTimeManager::activateDialog(COriginalDialog* dialogPtr)
{
	if (dialogPtr == nullptr)
	{
		return;
	}
	if (!dialogPtr->isVisible())
	{
		dialogPtr->show();
	}
	if (dialogPtr->isMinimized())
	{
		dialogPtr->showNormal();
	}
	dialogPtr->raise();
	dialogPtr->activateWindow();
}

void DialogRunTimeManager::makeBuiltInDialog(DialogParam& param)
{
	switch (DialogFactoryManager::instance().showMode(param.dialogType()))
	{
	case POP_DIALOG_SHOW_MODE:
		PopDialogManager::instance().popDialog(param);
		break;
	case NOTIFY_DIALOG_SHOW_MODE:
		NotifyDialogManager::instance().showDialog(param);
		break;
	case STATIC_DIALOG_SHOW_MODE:
		StaticDialogManager::instance().popStaticDialog(param);
		break;
	default:
		DialogHelper::logFile() << "DialogRunTimeManager unsupported built-in dialogType = "
			<< param.dialogType() << std::endl;
		break;
	}
}

void DialogRunTimeManager::makeCustomDialog(DialogParam& param)
{
	quint64 dialogId = 0;
	COriginalDialog* baseDialogPtr = AllocManager::instance().createDialog(dialogId, param);
	CustomDialog* dialogPtr = dynamic_cast<CustomDialog*>(baseDialogPtr);
	if (dialogPtr == nullptr)
	{
		if (dialogId != 0)
		{
			AllocManager::instance().removeByDialogId(dialogId);
		}
		return;
	}
	param.m_dialogId = dialogId;
	if (!dialogPtr->initDialog(param))
	{
		AllocManager::instance().removeByDialogId(dialogId);
		param.m_dialogId = 0;
		return;
	}
	QObject::connect(dialogPtr,
					 &DialogShow::closedSignal,
					 this,
					 &DialogRunTimeManager::onCustomDialogClosed,
					 Qt::UniqueConnection);
	QObject::connect(dialogPtr,
					 &DialogBase::alreadyShown,
					 this,
					 &DialogRunTimeManager::onCustomDialogAlreadyShown,
					 Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

	if (dialogPtr->isModalMode())
	{
		dialogPtr->setWindowResultAddr(&(param.m_result));
		dialogPtr->setUserResultPtr(&(param.m_userResult));
		dialogPtr->exec();
		return;
	}
	dialogPtr->useInternalResultStorage(ERROR_RESULT, param.m_userResult);
	dialogPtr->show();
}

bool DialogRunTimeManager::isGuiThread() const
{
	QCoreApplication* application = QCoreApplication::instance();
	if (application == nullptr)
	{
		return true;
	}
	return QThread::currentThread() == application->thread();
}

void DialogRunTimeManager::closeDialogByDialogId(quint64 dialogId, DialogResult result)
{
	if (dialogId == 0)
	{
		return;
	}
	DialogShow* dialogPtr = qobject_cast<DialogShow*>(AllocManager::instance().findDialogPtr(dialogId));
	if (dialogPtr == nullptr)
	{
		AllocManager::instance().removeByDialogId(dialogId);
		return;
	}
	dialogPtr->setWindowResult(result);
	dialogPtr->close();
}

#include "DialogFactoryManager.h"
#include "CustomDialog.h"
#include "CustomViewBase.h"
#include "DialogHelper.h"
#include "DialogTemplate.h"
#include "QtControls/COriginalDialog.h"
#include <new>

/** 释放自定义窗口工厂
@param [in] factory 待释放的工厂指针
*/
static void DestroyCustomDialogFactory(CustomDialogFactory* factory)
{
	delete factory;
}

/** 释放自定义内容区工厂
@param [in] factory 待释放的工厂指针
*/
static void DestroyCustomViewFactory(CustomViewFactory* factory)
{
	delete factory;
}

DialogFactoryManager::CustomDialogFactoryRecord::CustomDialogFactoryRecord() :
m_factory(nullptr),
m_destroyFunction(nullptr),
m_isOwned(false)
{

}

DialogFactoryManager::CustomDialogFactoryRecord::~CustomDialogFactoryRecord()
{
	if (m_isOwned && m_factory != nullptr && m_destroyFunction != nullptr)
	{
		m_destroyFunction(m_factory);
	}
}

DialogFactoryManager::CustomViewFactoryRecord::CustomViewFactoryRecord() :
m_factory(nullptr),
m_showMode(POP_DIALOG_SHOW_MODE),
m_destroyFunction(nullptr),
m_isOwned(false)
{

}

DialogFactoryManager::CustomViewFactoryRecord::~CustomViewFactoryRecord()
{
	if (m_isOwned && m_factory != nullptr && m_destroyFunction != nullptr)
	{
		m_destroyFunction(m_factory);
	}
}

DialogFactoryManager& DialogFactoryManager::instance()
{
	static DialogFactoryManager s_manager;
	return s_manager;
}

bool DialogFactoryManager::registerCustomDialogFactory(DialogType dialogType,
													   CustomDialogFactory* factory,
													   CustomDialogFactoryDestroy destroyFunction)
{
	if (!isCustomDialogType(dialogType) || factory == nullptr)
	{
		return false;
	}
	if (destroyFunction == nullptr)
	{
		destroyFunction = DestroyCustomDialogFactory;
	}
	std::shared_ptr<CustomDialogFactoryRecord> factoryRecord;
	try
	{
		factoryRecord.reset(new (std::nothrow) CustomDialogFactoryRecord);
		if (!factoryRecord)
		{
			return false;
		}
		factoryRecord->m_factory = factory;
		factoryRecord->m_destroyFunction = destroyFunction;

		QMutexLocker locker(&m_mutex);
		if (m_mapCustomDialogFactory.find(dialogType) != m_mapCustomDialogFactory.end())
		{
			return false;
		}
		if (m_mapCustomViewFactory.find(dialogType) != m_mapCustomViewFactory.end())
		{
			return false;
		}
		m_mapCustomDialogFactory[dialogType] = factoryRecord;
		factoryRecord->m_isOwned = true;
	}
	catch (...)
	{
		DialogHelper::logFile() << "DialogFactoryManager register custom dialog factory failed, type = "
			<< dialogType << std::endl;
		return false;
	}
	return true;
}

void DialogFactoryManager::unregisterCustomDialogFactory(DialogType dialogType)
{
	std::shared_ptr<CustomDialogFactoryRecord> factoryRecord;
	{
		QMutexLocker locker(&m_mutex);
		auto itFactory = m_mapCustomDialogFactory.find(dialogType);
		if (itFactory == m_mapCustomDialogFactory.end())
		{
			return;
		}
		factoryRecord = itFactory->second;
		m_mapCustomDialogFactory.erase(itFactory);
	}
}

bool DialogFactoryManager::registerCustomViewFactory(DialogType dialogType,
													 CustomViewFactory* factory,
													 DialogShowMode showMode,
													 CustomViewFactoryDestroy destroyFunction)
{
	if (!isCustomDialogType(dialogType) || factory == nullptr)
	{
		return false;
	}
	if (destroyFunction == nullptr)
	{
		destroyFunction = DestroyCustomViewFactory;
	}
	std::shared_ptr<CustomViewFactoryRecord> factoryRecord;
	try
	{
		factoryRecord.reset(new (std::nothrow) CustomViewFactoryRecord);
		if (!factoryRecord)
		{
			return false;
		}
		factoryRecord->m_factory = factory;
		factoryRecord->m_showMode = normalizeCustomShowMode(showMode);
		factoryRecord->m_destroyFunction = destroyFunction;

		QMutexLocker locker(&m_mutex);
		if (m_mapCustomDialogFactory.find(dialogType) != m_mapCustomDialogFactory.end())
		{
			return false;
		}
		if (m_mapCustomViewFactory.find(dialogType) != m_mapCustomViewFactory.end())
		{
			return false;
		}
		m_mapCustomViewFactory[dialogType] = factoryRecord;
		factoryRecord->m_isOwned = true;
	}
	catch (...)
	{
		DialogHelper::logFile() << "DialogFactoryManager register custom view factory failed, type = "
			<< dialogType << std::endl;
		return false;
	}
	return true;
}

void DialogFactoryManager::unregisterCustomViewFactory(DialogType dialogType)
{
	std::shared_ptr<CustomViewFactoryRecord> factoryRecord;
	{
		QMutexLocker locker(&m_mutex);
		auto itFactory = m_mapCustomViewFactory.find(dialogType);
		if (itFactory == m_mapCustomViewFactory.end())
		{
			return;
		}
		factoryRecord = itFactory->second;
		m_mapCustomViewFactory.erase(itFactory);
	}
}

COriginalDialog* DialogFactoryManager::createDialog(const DialogParam& param)
{
	if (isCustomDialogType(param.dialogType()))
	{
		return createCustomDialog(param);
	}
	return DialogTemplate::createDialog(param.dialogType());
}

DialogShowMode DialogFactoryManager::showMode(DialogType dialogType)
{
	DialogShowMode builtInShowMode = DialogTemplate::showMode(dialogType);
	if (builtInShowMode != ERROR_DIALOG_SHOW_MODE)
	{
		return builtInShowMode;
	}
	if (!isCustomDialogType(dialogType))
	{
		return ERROR_DIALOG_SHOW_MODE;
	}
	QMutexLocker locker(&m_mutex);
	auto itDialogFactory = m_mapCustomDialogFactory.find(dialogType);
	if (itDialogFactory != m_mapCustomDialogFactory.end())
	{
		return POP_DIALOG_SHOW_MODE;
	}
	auto itViewFactory = m_mapCustomViewFactory.find(dialogType);
	if (itViewFactory != m_mapCustomViewFactory.end())
	{
		return itViewFactory->second->m_showMode;
	}
	return ERROR_DIALOG_SHOW_MODE;
}

bool DialogFactoryManager::isBuiltInDialogType(DialogType dialogType) const
{
	return DialogTemplate::isBuiltInDialogType(dialogType);
}

bool DialogFactoryManager::isCustomDialogType(DialogType dialogType) const
{
	return dialogType >= CUSTOM_DIALOG_TYPE_BEGIN;
}

DialogFactoryManager::DialogFactoryManager()
{

}

DialogFactoryManager::~DialogFactoryManager()
{
	std::map<DialogType, std::shared_ptr<CustomDialogFactoryRecord> > mapCustomDialogFactory;
	std::map<DialogType, std::shared_ptr<CustomViewFactoryRecord> > mapCustomViewFactory;
	{
		QMutexLocker locker(&m_mutex);
		mapCustomDialogFactory.swap(m_mapCustomDialogFactory);
		mapCustomViewFactory.swap(m_mapCustomViewFactory);
	}
}

COriginalDialog* DialogFactoryManager::createCustomDialog(const DialogParam& param)
{
	std::shared_ptr<CustomDialogFactoryRecord> dialogFactoryRecord;
	std::shared_ptr<CustomViewFactoryRecord> viewFactoryRecord;
	{
		QMutexLocker locker(&m_mutex);
		auto itDialogFactory = m_mapCustomDialogFactory.find(param.dialogType());
		if (itDialogFactory != m_mapCustomDialogFactory.end())
		{
			dialogFactoryRecord = itDialogFactory->second;
		}
		auto itViewFactory = m_mapCustomViewFactory.find(param.dialogType());
		if (itViewFactory != m_mapCustomViewFactory.end())
		{
			viewFactoryRecord = itViewFactory->second;
		}
	}
	if (dialogFactoryRecord && dialogFactoryRecord->m_factory != nullptr)
	{
		try
		{
			CustomDialog* dialog = dialogFactoryRecord->m_factory->createDialog(param);
			if (dialog != nullptr)
			{
				dialog->setShowMode(normalizeCustomShowMode(dialogFactoryRecord->m_factory->showMode(param)));
			}
			return dialog;
		}
		catch (...)
		{
			return nullptr;
		}
	}
	if (!viewFactoryRecord || viewFactoryRecord->m_factory == nullptr)
	{
		return nullptr;
	}

	CustomDialog* dialog = nullptr;
	CustomViewBase* view = nullptr;
	try
	{
		dialog = new (std::nothrow) CustomDialog;
		if (dialog == nullptr)
		{
			return nullptr;
		}
		view = viewFactoryRecord->m_factory->createView(param);
		if (view == nullptr)
		{
			delete dialog;
			return nullptr;
		}
		if (!dialog->setView(view))
		{
			delete view;
			delete dialog;
			return nullptr;
		}
		dialog->setShowMode(viewFactoryRecord->m_showMode);
	}
	catch (...)
	{
		delete view;
		delete dialog;
		return nullptr;
	}
	return dialog;
}

DialogShowMode DialogFactoryManager::normalizeCustomShowMode(DialogShowMode showMode) const
{
	if (showMode == MODELESS_DIALOG_SHOW_MODE)
	{
		return MODELESS_DIALOG_SHOW_MODE;
	}
	return POP_DIALOG_SHOW_MODE;
}

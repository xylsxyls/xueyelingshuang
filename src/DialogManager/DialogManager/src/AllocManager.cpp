#include "AllocManager.h"
#include "AccountManagerDialog.h"
#include "DialogFactoryManager.h"
#include "DialogHelper.h"
#include "QtControls/COriginalDialog.h"
#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>

AllocManager::AllocManager() :
m_accountManagerDialog(nullptr),
m_nextDialogId(0)
{

}

AllocManager::~AllocManager()
{
	uninit();
}

COriginalDialog* AllocManager::createDialog(quint64& dialogId, quint64 userId, DialogType type)
{
	return createDialog(dialogId, 0, userId, type);
}

COriginalDialog* AllocManager::createDialog(quint64& dialogId, quint64 businessId, quint64 userId, DialogType type)
{
	DialogParam param;
	param.setDialogType(type);
	param.m_businessId = businessId;
	param.m_userId = userId;
	return createDialog(dialogId, param);
}

COriginalDialog* AllocManager::createDialog(quint64& dialogId, DialogParam& param)
{
	dialogId = 0;
	if (param.dialogType() == ACCOUNT_MANAGER_DIALOG)
	{
		QMutexLocker locker(&m_mutex);
		if (!m_accountManagerDialog.isNull())
		{
			auto itDialogId = m_mapDialogPtrToDialogId.find(m_accountManagerDialog.data());
			if (itDialogId != m_mapDialogPtrToDialogId.end())
			{
				dialogId = itDialogId->second;
				return m_accountManagerDialog.data();
			}
		}
	}

	COriginalDialog* dialogPtr = DialogFactoryManager::instance().createDialog(param);
	if (dialogPtr == nullptr)
	{
		DialogHelper::logFile() << "AllocManager createDialog failed, type = " << param.dialogType()
			<< ", businessId = " << param.m_businessId
			<< ", userId = " << param.m_userId << std::endl;
		return nullptr;
	}

	dialogId = add(dialogPtr, param.dialogType(), param.userKey());
	if (dialogId == 0)
	{
		DialogHelper::logFile() << "AllocManager add dialog failed, type = " << param.dialogType()
			<< ", businessId = " << param.m_businessId
			<< ", userId = " << param.m_userId << std::endl;
		releaseDialog(dialogPtr, true);
		return nullptr;
	}

	if (param.dialogType() == ACCOUNT_MANAGER_DIALOG)
	{
		AccountManagerDialog* accountManagerDialog = dynamic_cast<AccountManagerDialog*>(dialogPtr);
		if (accountManagerDialog == nullptr)
		{
			removeByDialogId(dialogId);
			dialogId = 0;
			return nullptr;
		}
		QMutexLocker locker(&m_mutex);
		m_accountManagerDialog = accountManagerDialog;
	}

	return dialogPtr;
}

void AllocManager::uninit()
{
	std::vector<COriginalDialog*> vecDialogPtr;
	{
		QMutexLocker locker(&m_mutex);
		for (auto itDialog = m_mapDialogIdToDialogPtr.begin(); itDialog != m_mapDialogIdToDialogPtr.end(); ++itDialog)
		{
			COriginalDialog* dialogPtr = itDialog->second.data();
			if (dialogPtr != nullptr)
			{
				vecDialogPtr.push_back(dialogPtr);
			}
		}
		m_mapObjectToDialogId.clear();
		m_mapObjectToDialogPtr.clear();
		m_mapDialogPtrToDialogId.clear();
		m_mapDialogIdToDialogPtr.clear();
		m_mapUserKeyToDialogId.clear();
		m_mapDialogIdToUserKey.clear();
		m_mapDialogIdToDialogType.clear();
		m_accountManagerDialog = nullptr;
	}

	for (auto itDialog = vecDialogPtr.begin(); itDialog != vecDialogPtr.end(); ++itDialog)
	{
		releaseDialog(*itDialog, true);
	}
}

void AllocManager::removeByDialogId(quint64 dialogId)
{
	QPointer<COriginalDialog> dialogPtr;
	{
		QMutexLocker locker(&m_mutex);
		auto itDialog = m_mapDialogIdToDialogPtr.find(dialogId);
		if (itDialog == m_mapDialogIdToDialogPtr.end())
		{
			return;
		}
		dialogPtr = itDialog->second;
		COriginalDialog* rawDialogPtr = dialogPtr.data();
		auto itUserKey = m_mapDialogIdToUserKey.find(dialogId);
		if (itUserKey != m_mapDialogIdToUserKey.end())
		{
			m_mapUserKeyToDialogId.erase(itUserKey->second);
			m_mapDialogIdToUserKey.erase(itUserKey);
		}
		m_mapDialogIdToDialogPtr.erase(itDialog);
		if (rawDialogPtr != nullptr)
		{
			QObject* objectPtr = static_cast<QObject*>(rawDialogPtr);
			m_mapObjectToDialogId.erase(objectPtr);
			m_mapObjectToDialogPtr.erase(objectPtr);
			m_mapDialogPtrToDialogId.erase(rawDialogPtr);
		}
		m_mapDialogIdToDialogType.erase(dialogId);
		if (rawDialogPtr == m_accountManagerDialog.data())
		{
			m_accountManagerDialog = nullptr;
		}
	}
	releaseDialog(dialogPtr.data(), false);
}

void AllocManager::removeByUserId(quint64 userId)
{
	removeByUserKey(DialogUserKey(0, userId));
}

void AllocManager::removeByUserKey(const DialogUserKey& userKey)
{
	removeByDialogId(findDialogId(userKey));
}

quint64 AllocManager::findDialogId(COriginalDialog* dialogPtr)
{
	QMutexLocker locker(&m_mutex);
	auto itDialog = m_mapDialogPtrToDialogId.find(dialogPtr);
	if (itDialog == m_mapDialogPtrToDialogId.end())
	{
		return 0;
	}
	return itDialog->second;
}

COriginalDialog* AllocManager::findDialogPtr(quint64 dialogId)
{
	QCoreApplication* application = QCoreApplication::instance();
	if (application != nullptr && QThread::currentThread() != application->thread())
	{
		DialogHelper::logFile() << "AllocManager findDialogPtr rejected outside GUI thread, dialogId = "
			<< dialogId << std::endl;
		return nullptr;
	}
	QMutexLocker locker(&m_mutex);
	auto itDialog = m_mapDialogIdToDialogPtr.find(dialogId);
	if (itDialog == m_mapDialogIdToDialogPtr.end())
	{
		return nullptr;
	}
	COriginalDialog* dialogPtr = itDialog->second.data();
	if (dialogPtr == nullptr)
	{
		auto itUserKey = m_mapDialogIdToUserKey.find(dialogId);
		if (itUserKey != m_mapDialogIdToUserKey.end())
		{
			m_mapUserKeyToDialogId.erase(itUserKey->second);
			m_mapDialogIdToUserKey.erase(itUserKey);
		}
		m_mapDialogIdToDialogPtr.erase(itDialog);
		m_mapDialogIdToDialogType.erase(dialogId);
		return nullptr;
	}
	return dialogPtr;
}

COriginalDialog* AllocManager::findDialogPtrByType(DialogType type)
{
	QCoreApplication* application = QCoreApplication::instance();
	if (application != nullptr && QThread::currentThread() != application->thread())
	{
		DialogHelper::logFile() << "AllocManager findDialogPtrByType rejected outside GUI thread, type = "
			<< type << std::endl;
		return nullptr;
	}
	QMutexLocker locker(&m_mutex);
	if (type == ACCOUNT_MANAGER_DIALOG)
	{
		return m_accountManagerDialog.data();
	}
	return nullptr;
}

quint64 AllocManager::findDialogId(quint64 userId)
{
	return findDialogId(DialogUserKey(0, userId));
}

quint64 AllocManager::findDialogId(const DialogUserKey& userKey)
{
	QMutexLocker locker(&m_mutex);
	if (!userKey.isValid())
	{
		return 0;
	}
	auto itDialog = m_mapUserKeyToDialogId.find(userKey);
	if (itDialog == m_mapUserKeyToDialogId.end())
	{
		return 0;
	}
	return itDialog->second;
}

quint64 AllocManager::findBusinessId(quint64 dialogId)
{
	QMutexLocker locker(&m_mutex);
	auto itDialog = m_mapDialogIdToUserKey.find(dialogId);
	if (itDialog == m_mapDialogIdToUserKey.end())
	{
		return 0;
	}
	return itDialog->second.m_businessId;
}

quint64 AllocManager::findUserId(quint64 dialogId)
{
	QMutexLocker locker(&m_mutex);
	auto itDialog = m_mapDialogIdToUserKey.find(dialogId);
	if (itDialog == m_mapDialogIdToUserKey.end())
	{
		return 0;
	}
	return itDialog->second.m_userId;
}

DialogUserKey AllocManager::findUserKey(quint64 dialogId)
{
	QMutexLocker locker(&m_mutex);
	auto itDialog = m_mapDialogIdToUserKey.find(dialogId);
	if (itDialog == m_mapDialogIdToUserKey.end())
	{
		return DialogUserKey();
	}
	return itDialog->second;
}

DialogType AllocManager::findDialogType(quint64 dialogId)
{
	QMutexLocker locker(&m_mutex);
	auto itDialog = m_mapDialogIdToDialogType.find(dialogId);
	if (itDialog == m_mapDialogIdToDialogType.end())
	{
		return ERROR_DIALOG_TYPE;
	}
	return itDialog->second;
}

quint64 AllocManager::findLastDialogId()
{
	QMutexLocker locker(&m_mutex);
	if (m_mapDialogIdToDialogPtr.empty())
	{
		return 0;
	}
	return m_mapDialogIdToDialogPtr.rbegin()->first;
}

bool AllocManager::isStatic(quint64 dialogId)
{
	return DialogFactoryManager::instance().showMode(findDialogType(dialogId)) == STATIC_DIALOG_SHOW_MODE;
}

quint64 AllocManager::dialogCount()
{
	QMutexLocker locker(&m_mutex);
	return m_mapDialogIdToDialogPtr.size();
}

quint64 AllocManager::dialogTypeCount(DialogType type)
{
	QMutexLocker locker(&m_mutex);
	quint64 count = 0;
	for (auto itDialog = m_mapDialogIdToDialogType.begin(); itDialog != m_mapDialogIdToDialogType.end(); ++itDialog)
	{
		if (itDialog->second == type)
		{
			++count;
		}
	}
	return count;
}

std::vector<quint64> AllocManager::allDialogId()
{
	QMutexLocker locker(&m_mutex);
	std::vector<quint64> vecAllDialogId;
	for (auto itDialogId = m_mapDialogIdToDialogPtr.begin(); itDialogId != m_mapDialogIdToDialogPtr.end(); ++itDialogId)
	{
		vecAllDialogId.push_back(itDialogId->first);
	}
	return vecAllDialogId;
}

quint64 AllocManager::add(COriginalDialog* base, DialogType type, const DialogUserKey& userKey)
{
	if (base == nullptr)
	{
		return 0;
	}
	QMutexLocker locker(&m_mutex);
	if (userKey.isValid() && m_mapUserKeyToDialogId.find(userKey) != m_mapUserKeyToDialogId.end())
	{
		return 0;
	}
	if (m_mapDialogPtrToDialogId.find(base) != m_mapDialogPtrToDialogId.end())
	{
		return 0;
	}
	quint64 dialogId = getDialogId();
	m_mapDialogIdToDialogPtr[dialogId] = base;
	m_mapObjectToDialogId[static_cast<QObject*>(base)] = dialogId;
	m_mapObjectToDialogPtr[static_cast<QObject*>(base)] = base;
	m_mapDialogPtrToDialogId[base] = dialogId;
	m_mapDialogIdToDialogType[dialogId] = type;
	QObject::connect(base, &QObject::destroyed, this, &AllocManager::onDialogDestroyed, Qt::DirectConnection);
	if (userKey.isValid())
	{
		m_mapDialogIdToUserKey[dialogId] = userKey;
		m_mapUserKeyToDialogId[userKey] = dialogId;
	}
	return dialogId;
}

quint64 AllocManager::getDialogId()
{
	do
	{
		++m_nextDialogId;
		if (m_nextDialogId == 0)
		{
			++m_nextDialogId;
		}
	}
	while (m_mapDialogIdToDialogPtr.find(m_nextDialogId) != m_mapDialogIdToDialogPtr.end());
	return m_nextDialogId;
}

void AllocManager::releaseDialog(COriginalDialog* dialogPtr, bool deleteNow)
{
	if (dialogPtr == nullptr)
	{
		return;
	}
	if (QCoreApplication::instance() == nullptr)
	{
		delete dialogPtr;
		return;
	}
	if (deleteNow || QCoreApplication::closingDown())
	{
		if (QThread::currentThread() == dialogPtr->thread())
		{
			delete dialogPtr;
			return;
		}
		if (QCoreApplication::closingDown())
		{
			DialogHelper::logFile() << "AllocManager cannot delete dialog during QCoreApplication closingDown from another thread"
				<< std::endl;
			return;
		}
	}
	bool invokeOk = QMetaObject::invokeMethod(dialogPtr, "deleteLater", Qt::QueuedConnection);
	if (!invokeOk)
	{
		DialogHelper::logFile() << "AllocManager deleteLater invokeMethod failed" << std::endl;
	}
}

void AllocManager::onDialogDestroyed(QObject* object)
{
	quint64 dialogId = 0;
	{
		QMutexLocker locker(&m_mutex);
		auto itObjectDialogId = m_mapObjectToDialogId.find(object);
		if (itObjectDialogId == m_mapObjectToDialogId.end())
		{
			return;
		}
		dialogId = itObjectDialogId->second;
		m_mapObjectToDialogId.erase(itObjectDialogId);
		COriginalDialog* dialogPtr = nullptr;
		auto itObjectDialogPtr = m_mapObjectToDialogPtr.find(object);
		if (itObjectDialogPtr != m_mapObjectToDialogPtr.end())
		{
			dialogPtr = itObjectDialogPtr->second;
			m_mapObjectToDialogPtr.erase(itObjectDialogPtr);
		}
		if (dialogPtr != nullptr)
		{
			m_mapDialogPtrToDialogId.erase(dialogPtr);
		}
		m_mapDialogIdToDialogPtr.erase(dialogId);
		auto itUserKey = m_mapDialogIdToUserKey.find(dialogId);
		if (itUserKey != m_mapDialogIdToUserKey.end())
		{
			m_mapUserKeyToDialogId.erase(itUserKey->second);
			m_mapDialogIdToUserKey.erase(itUserKey);
		}
		m_mapDialogIdToDialogType.erase(dialogId);
		if (dialogPtr == m_accountManagerDialog.data())
		{
			m_accountManagerDialog = nullptr;
		}
	}
	DialogHelper::logFile() << "AllocManager cleaned destroyed dialog, dialogId = "
		<< dialogId << std::endl;
}

#include "AllocManager.h"
#include "AdvertAskDialog.h"
#include "AskDialog.h"
#include "DownloadOperateDialog.h"
#include "InputDialog.h"
#include "TipDialog.h"
#include "WaitDialog.h"
#include "AskShowDialog.h"
#include "LoginShowDialog.h"
#include "TipShowDialog.h"
#include "AccountManagerDialog.h"
#include "AdvertShowDialog.h"
#include "DialogHelper.h"

quint64 AllocManager::add(COriginalDialog* base, DialogType type, quint64 userId)
{
    if (base == nullptr)
    {
        return 0;
    }
    QMutexLocker locker(&m_mutex);
    quint64 dialogId = getDialogId();
    if (userId != 0)
    {
        if (m_mapUserIdToDialogId.find(userId) != m_mapUserIdToDialogId.end())
        {
            return 0;
        }
    }
    if (m_mapDialogPtrToDialogId.find(base) != m_mapDialogPtrToDialogId.end())
    {
        return 0;
    }
    m_mapDialogIdToDialogPtr[dialogId] = base;
    m_mapDialogPtrToDialogId[base] = dialogId;
    m_mapDialogIdToDialogType[dialogId] = type;
    if (userId != 0)
    {
        m_mapDialogIdToUserId[dialogId] = userId;
        m_mapUserIdToDialogId[userId] = dialogId;
    }

    return dialogId;
}

void AllocManager::removeByDialogId(quint64 dialogId)
{
    COriginalDialog* dialogPtr = findDialogPtr(dialogId);
    if (dialogPtr == nullptr)
    {
        return;
    }
    
    quint64 userId = findUserId(dialogId);
    {
        QMutexLocker locker(&m_mutex);
        if (userId != 0)
        {
            m_mapDialogIdToUserId.erase(m_mapDialogIdToUserId.find(dialogId));
            m_mapUserIdToDialogId.erase(m_mapUserIdToDialogId.find(userId));
        }
        m_mapDialogIdToDialogPtr.erase(m_mapDialogIdToDialogPtr.find(dialogId));
        m_mapDialogPtrToDialogId.erase(m_mapDialogPtrToDialogId.find(dialogPtr));
        m_mapDialogIdToDialogType.erase(m_mapDialogIdToDialogType.find(dialogId));
    }
    //delete dialogPtr;
	dialogPtr->deleteLater();
}

void AllocManager::removeByUserId(quint64 userId)
{
    removeByDialogId(findDialogId(userId));
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
    QMutexLocker locker(&m_mutex);
    auto itDialog = m_mapDialogIdToDialogPtr.find(dialogId);
    if (itDialog == m_mapDialogIdToDialogPtr.end())
    {
        return nullptr;
    }
    return itDialog->second;
}

COriginalDialog* AllocManager::findDialogPtr(DialogType type)
{
	switch (type)
	{
	case ACCOUNT_MANAGER_DIALOG:
	{
		return m_accountManagerDialog;
	}
	default:
		break;
	}
	return nullptr;
}

quint64 AllocManager::findDialogId(quint64 userId)
{
    QMutexLocker locker(&m_mutex);
    auto itDialog = m_mapUserIdToDialogId.find(userId);
    if (itDialog == m_mapUserIdToDialogId.end())
    {
        return 0;
    }
    return itDialog->second;
}

quint64 AllocManager::findUserId(quint64 dialogId)
{
    QMutexLocker locker(&m_mutex);
    auto itDialog = m_mapDialogIdToUserId.find(dialogId);
    if (itDialog == m_mapDialogIdToUserId.end())
    {
        return 0;
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
    //�������
    return m_mapDialogIdToDialogPtr.rbegin()->first;
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
    std::vector<quint64> vecAllDialogId;
	DialogHelper::logFile() << "m_mapDialogIdToDialogPtr.size = " << m_mapDialogIdToDialogPtr.size() << std::endl;
    for (auto itDialogId = m_mapDialogIdToDialogPtr.begin(); itDialogId != m_mapDialogIdToDialogPtr.end(); ++itDialogId)
    {
        vecAllDialogId.push_back(itDialogId->first);
    }
    return vecAllDialogId;
}

AllocManager::AllocManager() :
m_accountManagerDialog(nullptr)
{

}

AllocManager::~AllocManager()
{

}

COriginalDialog* AllocManager::createDialog(quint64& dialogId, quint64 userId, DialogType type)
{
    COriginalDialog* dialogPtr = nullptr;
    switch (type)
    {
    case ERROR_DIALOG_TYPE:
        break;
    case ASK_DIALOG:
    {
        dialogPtr = new AskDialog;
        break;
    }
    case INPUT_DIALOG:
    {
        dialogPtr = new InputDialog;
        break;
    }
    case TIP_DIALOG:
    {
        dialogPtr = new TipDialog;
        break;
    }
    case WAIT_DIALOG:
    {
        dialogPtr = new WaitDialog;
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
        dialogPtr = new DownloadOperateDialog;
        break;
    }
    case ASK_SHOW_DIALOG:
    {
        dialogPtr = new AskShowDialog;
        break;
    }
    case TIP_SHOW_DIALOG:
    {
        dialogPtr = new TipShowDialog;
        break;
    }
#if (QT_VERSION <= QT_VERSION_CHECK(5,5,1))
    case LOGIN_SHOW_DIALOG:
    {
        dialogPtr = new LoginShowDialog;
        break;
    }
	case ADVERT_ASK_DIALOG:
	{
		dialogPtr = new AdvertAskDialog;
		break;
	}
	case ADVERT_SHOW_DIALOG:
	{
		dialogPtr = new AdvertShowDialog;
		break;
	}
#endif
    default:
        break;
    }
    dialogId = add(dialogPtr, type, userId);

    if (type == ACCOUNT_MANAGER_DIALOG)
    {
        if (m_accountManagerDialog == nullptr)
        {
            m_accountManagerDialog = new AccountManagerDialog;
            dialogId = add(m_accountManagerDialog, type, userId);
        }
        else
        {
            dialogId = findDialogId(m_accountManagerDialog);
        }
        dialogPtr = m_accountManagerDialog;
    }

    return dialogPtr;
}

quint64 AllocManager::getDialogId()
{
    static quint64 dialogId = 0;
    return ++dialogId;
}

bool AllocManager::isStatic(quint64 dialogId)
{
    if (findDialogType(dialogId) == ACCOUNT_MANAGER_DIALOG)
    {
        return true;
    }
    return false;
}
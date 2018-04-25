#include "AllocManager.h"
#include "COriginalDialog.h"
#include "PopDialog.h"
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

int32_t AllocManager::add(COriginalDialog* base, DialogType type, int32_t userId)
{
    if (base == nullptr)
    {
        return -1;
    }
    QMutexLocker locker(&m_mutex);
    int32_t dialogId = getDialogId();
    if (userId != -1)
    {
        if (m_mapUserIdToDialogId.find(userId) != m_mapUserIdToDialogId.end())
        {
            return -1;
        }
    }
    if (m_mapDialogPtrToDialogId.find(base) != m_mapDialogPtrToDialogId.end())
    {
        return -1;
    }
    m_mapDialogIdToDialogPtr[dialogId] = base;
    m_mapDialogPtrToDialogId[base] = dialogId;
    m_mapDialogIdToDialogType[dialogId] = type;
    if (userId != -1)
    {
        m_mapDialogIdToUserId[dialogId] = userId;
        m_mapUserIdToDialogId[userId] = dialogId;
    }
    //QObject::connect((DialogShow*)base, &DialogShow::closedSignal, this, &AllocManager::onClosedSignal);
    return dialogId;
}

void AllocManager::removeByDialogId(int32_t dialogId)
{
    COriginalDialog* dialogPtr = findDialogPtr(dialogId);
    if (dialogPtr == nullptr)
    {
        return;
    }
    
    emit deleteDialog(findDialogType(dialogId));
    int32_t userId = findUserId(dialogId);
    {
        QMutexLocker locker(&m_mutex);
        if (userId != -1)
        {
            m_mapDialogIdToUserId.erase(m_mapDialogIdToUserId.find(dialogId));
            m_mapUserIdToDialogId.erase(m_mapUserIdToDialogId.find(userId));
        }
        m_mapDialogIdToDialogPtr.erase(m_mapDialogIdToDialogPtr.find(dialogId));
        m_mapDialogPtrToDialogId.erase(m_mapDialogPtrToDialogId.find(dialogPtr));
        m_mapDialogIdToDialogType.erase(m_mapDialogIdToDialogType.find(dialogId));
    }
    delete dialogPtr;
}

void AllocManager::removeByUserId(int32_t userId)
{
    removeByDialogId(findDialogId(userId));
}

int32_t AllocManager::findDialogId(COriginalDialog* dialogPtr)
{
    QMutexLocker locker(&m_mutex);
    auto itDialog = m_mapDialogPtrToDialogId.find(dialogPtr);
    if (itDialog == m_mapDialogPtrToDialogId.end())
    {
        return -1;
    }
    return itDialog->second;
}

COriginalDialog* AllocManager::findDialogPtr(int32_t dialogId)
{
    QMutexLocker locker(&m_mutex);
    auto itDialog = m_mapDialogIdToDialogPtr.find(dialogId);
    if (itDialog == m_mapDialogIdToDialogPtr.end())
    {
        return nullptr;
    }
    return itDialog->second;
}

int32_t AllocManager::findDialogId(int32_t userId)
{
    QMutexLocker locker(&m_mutex);
    auto itDialog = m_mapUserIdToDialogId.find(userId);
    if (itDialog == m_mapUserIdToDialogId.end())
    {
        return -1;
    }
    return itDialog->second;
}

int32_t AllocManager::findUserId(int32_t dialogId)
{
    QMutexLocker locker(&m_mutex);
    auto itDialog = m_mapDialogIdToUserId.find(dialogId);
    if (itDialog == m_mapDialogIdToUserId.end())
    {
        return -1;
    }
    return itDialog->second;
}

DialogType AllocManager::findDialogType(int32_t dialogId)
{
    QMutexLocker locker(&m_mutex);
    auto itDialog = m_mapDialogIdToDialogType.find(dialogId);
    if (itDialog == m_mapDialogIdToDialogType.end())
    {
        abort();
    }
    return itDialog->second;
}

int32_t AllocManager::findLastDialogId()
{
    QMutexLocker locker(&m_mutex);
    if (m_mapDialogIdToDialogPtr.empty())
    {
        return -1;
    }
    //·´Ïò±éÀú
    return m_mapDialogIdToDialogPtr.rbegin()->first;
}

int32_t AllocManager::dialogCount()
{
    QMutexLocker locker(&m_mutex);
    return m_mapDialogIdToDialogPtr.size();
}

AllocManager::~AllocManager()
{
    destroyAll();
}

COriginalDialog* AllocManager::createDialog(int32_t& dialogId, int32_t userId, DialogType type)
{
    COriginalDialog* dialogPtr = nullptr;
    switch (type)
    {
    case ERROR_TYPE:
        break;
    case ASK_DIALOG:
    {
        dialogPtr = new AskDialog;
        break;
    }
    case ADVERT_ASK_DIALOG:
    {
        dialogPtr = new AdvertAskDialog;
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
    case LOGIN_SHOW_DIALOG:
    {
        dialogPtr = new LoginShowDialog;
        break;
    }
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
        dialogPtr = m_accountManagerDialog;
    }

    return dialogPtr;
}

void AllocManager::onClosedSignal(int result)
{
    int32_t dialogId = findDialogId((COriginalDialog*)sender());
    if (isStatic(dialogId))
    {
        return;
    }
    removeByDialogId(dialogId);
}

int32_t AllocManager::getDialogId()
{
    static int32_t dialogId = 0;
    return ++dialogId;
}

bool AllocManager::isStatic(int32_t dialogId)
{
    if (findDialogType(dialogId) == ACCOUNT_MANAGER_DIALOG)
    {
        return true;
    }
    return false;
}

void AllocManager::destroyAll()
{
    int32_t count = dialogCount();
    while (count-- != 0)
    {
        removeByDialogId(findLastDialogId());
    }
    
    QMutexLocker locker(&m_mutex);
    if (!(m_mapDialogPtrToDialogId.empty() &&
        m_mapDialogIdToDialogPtr.empty() &&
        m_mapUserIdToDialogId.empty() &&
        m_mapDialogIdToUserId.empty()))
    {
        abort();
    }
}


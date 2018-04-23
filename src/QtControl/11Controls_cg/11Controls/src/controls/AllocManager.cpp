#include "AllocManager.h"
#include "COriginalDialog.h"
#include "PopDialog.h"

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
    QObject::connect(base, &COriginalDialog::dialogFinished, this, &AllocManager::onDialogFinished);
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

AllocManager::~AllocManager()
{
    destroyAll();
}

void AllocManager::onDialogFinished(int result)
{
    int32_t dialogId = findDialogId((COriginalDialog*)sender());
    if (findDialogType(dialogId) == ACCOUNT_MANAGER_DIALOG)
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

void AllocManager::destroyAll()
{
    int32_t count = 0;
    {
        QMutexLocker locker(&m_mutex);
        count = m_mapDialogIdToDialogPtr.size();
    }
    
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


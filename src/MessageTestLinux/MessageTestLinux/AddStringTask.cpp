#include "AddStringTask.h"
#include "MessageTestLinux.h"
#include "CSystem/CSystemAPI.h"
#include "TypeManager.h"

AddStringTask::AddStringTask():
    m_client(nullptr),
    m_exit(false)
{

}

void AddStringTask::DoTask()
{
    std::shared_ptr<TextMessage> lineMessage;
    while (!m_exit)
    {
        while (!m_client->m_listReceiveStr.empty())
        {
            if (m_exit)
            {
                return;
            }
            
            m_client->m_listReceiveStr.pop(&lineMessage);
            if (lineMessage->m_peopleId == -1)
            {
                TypeManager::instance().sendLostPid(lineMessage->m_pid);
                continue;
            }
            lineMessage->m_peopleId = *((int32_t*)&(lineMessage->m_text[0]));
            lineMessage->m_threadId = *((int32_t*)&(lineMessage->m_text[4]));
            lineMessage->m_text = lineMessage->m_text.substr(8, lineMessage->m_text.size() - 8);
            lineMessage->m_text.push_back('\n');
            {
                WriteLock lock(m_client->m_paramMutex);
                if (m_client->m_isClear)
                {
                    m_client->m_isClear = false;
                    m_client->m_areaCount.fetch_sub(1);
                    continue;
                }
                if (m_client->m_allPeopleId.find(lineMessage->m_peopleId) == m_client->m_allPeopleId.end())
                {
                    m_client->m_allPeopleId.insert(lineMessage->m_peopleId);
                    TypeManager::instance().sendPeopleId(lineMessage->m_peopleId);
                }
                auto itThreadIdSet = m_client->m_pidThreadIdReceiveMap.find(lineMessage->m_pid);
                if (itThreadIdSet == m_client->m_pidThreadIdReceiveMap.end())
                {
                    TypeManager::instance().sendPid(lineMessage->m_pid);
                    m_client->m_pidThreadIdReceiveMap[lineMessage->m_pid];
                    itThreadIdSet = m_client->m_pidThreadIdReceiveMap.find(lineMessage->m_pid);
                }
                auto& threadIdSet = itThreadIdSet->second;
                if (threadIdSet.find(lineMessage->m_threadId) == threadIdSet.end())
                {
                    threadIdSet.insert(lineMessage->m_threadId);
                    TypeManager::instance().sendPidThreadId(lineMessage->m_pid, lineMessage->m_threadId);
                }
            }
            {
                WriteLock lock(m_client->m_showMutex);
                if (m_client->m_isClear)
                {
                    m_client->m_isClear = false;
                    m_client->m_areaCount.fetch_sub(1);
                    continue;
                }
                if (m_client->textAvailable(lineMessage))
                {
                    m_client->m_showStr.push_front(lineMessage);
                    ++m_client->m_screenCount;
                }
            }
            {
                WriteLock lock(m_client->m_allStringMutex);
                if (m_client->m_isClear)
                {
                    m_client->m_isClear = false;
                    m_client->m_areaCount.fetch_sub(1);
                    continue;
                }
                if (m_client->m_isChangeState)
                {
                    if (m_client->m_showStr.empty() ||
                        (m_client->m_showStr.front()->m_id != lineMessage->m_id))
                    {
                        if (m_client->textAvailable(lineMessage))
                        {
                            m_client->m_showStr.push_front(lineMessage);
                            ++m_client->m_screenCount;
                        }
                    }
                }
                m_client->m_textStorage.push(lineMessage);
                ++m_client->m_allCount;
            }
            m_client->m_update = true;
        }
        m_client->m_isClear = false;
        CSystem::Sleep(10);
    }
}

void AddStringTask::StopTask()
{
    m_exit = true;
}

void AddStringTask::setParam(MessageTestLinux* client)
{
    m_client = client;
}
#include "MessageRecoverTask.h"
#include "MessageTestLinux.h"
#include "CSystem/CSystemAPI.h"

MessageRecoverTask::MessageRecoverTask():
    m_client(nullptr),
    m_exit(false)
{

}

void MessageRecoverTask::DoTask()
{
    while (!m_exit)
    {
        CSystem::Sleep(100);
        WriteLock lock(m_client->m_pidMutex);
        for (auto it = m_client->m_pid.begin(); it != m_client->m_pid.end();)
        {
            if (CSystem::processName(*it).empty())
            {
                printf("end pid = %d\n", (int32_t)*it);
                CTaskThreadManager::Instance().Uninit(m_client->m_pidThreadId[*it]);
                m_client->m_pidThreadId.erase(m_client->m_pidThreadId.find(*it));
                it = m_client->m_pid.erase(it);
                continue;
            }
            ++it;
        }
    }
}

void MessageRecoverTask::StopTask()
{
    m_exit = true;
}

void MessageRecoverTask::setParam(MessageTestLinux* client)
{
    m_client = client;
}
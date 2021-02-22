#include "AddStringTask.h"
#include "MessageTestLinux.h"
#include "CSystem/CSystemAPI.h"

AddStringTask::AddStringTask():
    m_client(nullptr),
    m_exit(false)
{

}

void AddStringTask::DoTask()
{
    std::string line;
    while (!m_exit)
    {
        while (!m_client->m_listReceiveStr.empty())
        {
            if (m_exit)
            {
                return;
            }
            m_client->m_listReceiveStr.pop(&line);
            {
                std::unique_lock<std::mutex> lock(m_client->m_showMutex);
                m_client->m_showStr.push_front(line);
            }
            {
                std::unique_lock<std::mutex> lock(m_client->m_addStringMutex);
                m_client->m_listAllStr.front().insert(0, QString::fromStdString(line + "\n"));
                if (m_client->m_listAllStr.front().size() > 20000)
                {
                    m_client->m_listAllStr.push_front(QString());
                }
                ++m_client->m_screenCount;
            }
            m_client->m_update = true;
        }
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
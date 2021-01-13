#include "MessageReceiveTask.h"
#include "MessageTestLinux.h"
#include "MsgLinux/MsgLinuxAPI.h"
#include "CSystem/CSystemAPI.h"
#include "MessagePidTask.h"
#include "AddStringTask.h"

MessageReceiveTask::MessageReceiveTask():
m_client(nullptr),
m_exit(false)
{

}

void MessageReceiveTask::DoTask()
{
    m_client->m_msg = new MsgLinux("/tmp/MessageTestLinux.file", true);
    m_client->m_addStringThreadId = CTaskThreadManager::Instance().Init();
    std::shared_ptr<AddStringTask> spAddStringTask(new AddStringTask);
    spAddStringTask->setParam(m_client);
    CTaskThreadManager::Instance().GetThreadInterface(m_client->m_addStringThreadId)->PostTask(spAddStringTask);

    std::string pid;
    while(!m_exit)
    {
        bool result = m_client->m_msg->recv(pid, 1);
        if (pid == "-1")
        {
            break;
        }
        if (!result)
        {
            delete m_client->m_msg;
            m_client->m_msg = new MsgLinux("/tmp/MessageTestLinux.file", true);
            continue;
        }
        printf("pid begin = %s\n", pid.c_str());
        uint32_t newPid = (uint32_t)atoi(pid.c_str());
        {
            std::unique_lock<std::mutex> lock(m_client->m_pidMutex);
            if (m_client->m_pid.find(newPid) != m_client->m_pid.end())
            {
                continue;
            }
            uint32_t threadId = CTaskThreadManager::Instance().Init();
            m_client->m_pid.insert(newPid);
            m_client->m_pidThreadId[newPid] = threadId;
            std::shared_ptr<MessagePidTask> spMessagePidTask(new MessagePidTask);
            spMessagePidTask->setParam(newPid, m_client);
            CTaskThreadManager::Instance().GetThreadInterface(threadId)->PostTask(spMessagePidTask);
        }
    }
}

void MessageReceiveTask::StopTask()
{
    m_exit = true;
    m_client->m_msg->send("-1", 1);
}

void MessageReceiveTask::setParam(MessageTestLinux* client)
{
    m_client = client;
}
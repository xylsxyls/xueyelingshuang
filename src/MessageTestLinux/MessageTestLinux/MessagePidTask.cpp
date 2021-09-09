#include "MessagePidTask.h"
#include "MessageTestLinux.h"
#include "MsgLinux/MsgLinuxAPI.h"
#include "AddStringTask.h"

MessagePidTask::MessagePidTask():
    m_pid(0),
    m_client(nullptr),
    m_exit(false)
{

}

void MessagePidTask::DoTask()
{
    std::string message;
    while (!m_exit)
    {
        m_client->m_msg->recv(message, m_pid);
        if (message == "-1" && m_exit)
        {
            printf("exit pid = %d\n", m_pid);
            break;
        }
        m_client->m_listReceiveStr.push(message);
    }
}

void MessagePidTask::StopTask()
{
    m_exit = true;
    m_client->m_msg->send("-1", m_pid);
}

void MessagePidTask::setParam(int32_t pid, MessageTestLinux* client)
{
    m_pid = pid;
    m_client = client;
}
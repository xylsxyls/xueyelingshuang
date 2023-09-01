#include "MessagePidTask.h"
#include "MessageTestLinux.h"
#include "MsgLinux/MsgLinuxAPI.h"
#include "AddStringTask.h"
#include "TextMessage.h"
#include "TypeManager.h"

MessagePidTask::MessagePidTask():
    m_pid(0),
    m_client(nullptr),
    m_exit(false)
{

}

void MessagePidTask::DoTask()
{
    while (!m_exit)
    {
        std::shared_ptr<TextMessage> lineMessage(new TextMessage);
        m_client->m_msg->recv(lineMessage->m_text, m_pid);
        if (lineMessage->m_text == "-1" && m_exit)
        {
            printf("exit pid = %d\n", m_pid);
            lineMessage->m_pid = m_pid;
            lineMessage->m_peopleId = -1;
            m_client->m_listReceiveStr.push(lineMessage);
            break;
        }
        lineMessage->m_pid = m_pid;
        lineMessage->m_id = ++m_client->m_id;
        ++m_client->m_areaCount;
        m_client->m_listReceiveStr.push(lineMessage);
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
#include "HeadTask.h"
#include "NetServer.h"

HeadTask::HeadTask():
m_time(0),
m_netServer(nullptr),
m_exit(false)
{

}

void HeadTask::DoTask()
{
	while (!m_exit)
	{
		if (m_netServer->m_isListen)
		{
			int32_t headNum = m_netServer->headNumber();
			int32_t index = -1;
			while (index++ != m_netServer->m_allClient.size() - 1)
			{
				uv_tcp_t* client = m_netServer->m_allClient[index];
				//如果没有firstMessage则不会有缓冲区
				auto itClient = m_netServer->m_receiveAreaMap.find(client);
				if (itClient != m_netServer->m_receiveAreaMap.end() && itClient->second.m_isFirstPass)
				{
					m_netServer->send(client, (const char*)&headNum, 4, MessageType::HEAD);
				}
			}
			m_netServer->m_head[1] = m_netServer->m_head[0];
			m_netServer->m_head[0] = headNum;
		}
		m_sleep.wait(m_time);
	}
}

void HeadTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void HeadTask::setParam(int32_t time, NetServer* netServer)
{
	m_time = time;
	m_netServer = netServer;
}
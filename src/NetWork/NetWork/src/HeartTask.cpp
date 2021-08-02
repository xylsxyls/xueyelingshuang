#include "HeartTask.h"
#include "NetClient.h"

HeartTask::HeartTask():
m_time(0),
m_netClient(nullptr),
m_exit(false)
{

}

void HeartTask::DoTask()
{
	while (!m_exit)
	{
		m_sleep.wait(m_time);
		if (m_exit)
		{
			return;
		}
		if (m_netClient->m_isConnected)
		{
			m_netClient->send(nullptr, 0, MessageType::HEART);
		}
	}
}

void HeartTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void HeartTask::setParam(int32_t time, NetClient* netClient)
{
	m_time = time;
	m_netClient = netClient;
}
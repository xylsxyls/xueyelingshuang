#include "HeartTask.h"
#include "NetClient.h"

HeartTask::HeartTask():
m_time(0),
m_netClient(nullptr),
m_stop(false)
{

}

void HeartTask::DoTask()
{
	while (!m_stop)
	{
		m_netClient->send("", 0, MessageType::HEART);
		std::this_thread::sleep_for(std::chrono::milliseconds(m_time));
	}
}

void HeartTask::StopTask()
{
	m_stop = true;
}

void HeartTask::setParam(int32_t time, NetClient* netClient)
{
	m_time = time;
	m_netClient = netClient;
}
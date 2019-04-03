#include "HeartTask.h"
#include "NetClient.h"
#include "D:\\SendToMessageTest.h"

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
		m_netClient->send("", 0, CorrespondParam::HEART);
		Sleep(m_time);
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
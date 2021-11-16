#include "stdafx.h"
#include "CSleepTask.h"

CSleepTask::CSleepTask():
m_sleepTime(0)
{

}

void CSleepTask::DoTask()
{
	if (m_sleepTime != 0)
	{
		m_sleep.wait(m_sleepTime);
	}
}

void CSleepTask::StopTask()
{
	m_sleep.signal();
}

bool CSleepTask::ReExecute()
{
	return true;
}

void CSleepTask::setParam(int32_t sleepTime)
{
	m_sleepTime = sleepTime;
}
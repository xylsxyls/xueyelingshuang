#include "stdafx.h"
#include "SleepTask.h"

SleepTask::SleepTask():
m_sleepTime(0)
{

}

void SleepTask::DoTask()
{
	if (m_sleepTime != 0)
	{
		Sleep(m_sleepTime);
	}
}

void SleepTask::setParam(int32_t sleepTime)
{
	m_sleepTime = sleepTime;
}
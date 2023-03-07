#include "SleepTask.h"

SleepTask::SleepTask():
m_time(0)
{

}

void SleepTask::DoTask()
{
	Sleep(m_time);
}

void SleepTask::setParam(int32_t time)
{
	m_time = time;
}
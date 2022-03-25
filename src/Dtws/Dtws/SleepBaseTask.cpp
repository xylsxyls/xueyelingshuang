#include "SleepBaseTask.h"

SleepBaseTask::SleepBaseTask():
m_exit(false)
{

}

void SleepBaseTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void SleepBaseTask::sleep(int32_t time)
{
	if (m_exit)
	{
		return;
	}
	m_sleep.wait(time);
}
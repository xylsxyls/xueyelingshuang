#include "stdafx.h"
#include "CqTask.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"

CqTask::CqTask():
m_times(0),
m_sleepTime(0),
m_exit(false)
{

}

void CqTask::DoTask()
{
	if (m_sleepTime != 0)
	{
		if (Sleep(m_sleepTime))
		{
			return;
		}
	}
	int32_t count = m_times;
	while (count-- != 0)
	{
		if (m_exit)
		{
			return;
		}
		CKeyboard::KeyPress('Q', 0);
		Sleep(10);
	}
}

void CqTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void CqTask::setParam(int32_t times, int32_t sleepTime)
{
	m_times = times;
	m_sleepTime = sleepTime;
}

bool CqTask::Sleep(int32_t time)
{
	return m_sleep.wait(time);
}
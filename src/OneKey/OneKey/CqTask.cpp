#include "stdafx.h"
#include "CqTask.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"

CqTask::CqTask():
m_exit(false)
{

}

void CqTask::DoTask()
{
	RCSend("w");
	int32_t count = 7;
	while (count-- != 0)
	{
		if (m_exit)
		{
			return;
		}
		CKeyboard::InputString("q", 0);
		Sleep(50);
	}
}

void CqTask::StopTask()
{
	m_exit = true;
}
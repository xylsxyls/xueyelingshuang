#include "stdafx.h"
#include "CWeqTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

CWeqTask::CWeqTask():
m_sleepTime(0)
{

}

void CWeqTask::DoTask()
{
	CKeyboard::KeyDown('W');
	CKeyboard::KeyDown('E');
	
	CKeyboard::KeyUp('W');
	CKeyboard::KeyUp('E');
	if (m_sleepTime != 0)
	{
		Sleep(m_sleepTime);
	}
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('Q');
}

void CWeqTask::setParam(int32_t sleepTime)
{
	m_sleepTime = sleepTime;
}
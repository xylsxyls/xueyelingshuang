#include "stdafx.h"
#include "CKeyTask.h"
#include "CKeyBoard/CKeyboardAPI.h"

CKeyTask::CKeyTask():
m_vkCode(0),
m_isDown(false)
{

}

void CKeyTask::DoTask()
{
	if (m_isDown)
	{
		CKeyboard::KeyDown(m_vkCode);
	}
	else
	{
		CKeyboard::KeyUp(m_vkCode);
	}
}

void CKeyTask::setParam(int32_t vkCode, bool isDown)
{
	m_vkCode = vkCode;
	m_isDown = isDown;
}
#include "stdafx.h"
#include "CKeyTask.h"
#include "CKeyBoard/CKeyboardAPI.h"

CKeyTask::CKeyTask():
m_vkCode(0),
m_type(0)
{

}

void CKeyTask::DoTask()
{
	switch (m_type)
	{
	case 0:
	{
		CKeyboard::KeyUp(m_vkCode);
	}
	break;
	case 1:
	{
		CKeyboard::KeyDown(m_vkCode);
	}
	break;
	case 2:
	{
		CKeyboard::KeyPress(m_vkCode, 0);
	}
	break;
	default:
		break;
	}
}

void CKeyTask::setParam(int32_t vkCode, bool isDown)
{
	m_vkCode = vkCode;
	m_type = (int32_t)isDown;
}

void CKeyTask::setParam(int32_t vkCode)
{
	m_vkCode = vkCode;
	m_type = 2;
}

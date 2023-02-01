#include "stdafx.h"
#include "HeroTask.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"

extern int32_t g_codetimes;
extern bool isMain;

HeroTask::HeroTask():
m_code1(0),
m_code2(0)
{

}

void HeroTask::DoTask()
{
	if (m_code1 != 0)
	{
		++g_codetimes;
		if (g_codetimes % 3 == 0 && m_code1 == 'G')
		{
			CKeyboard::KeyPress(m_code1, 0);
		}
	}
	if (m_code2 != 0)
	{
		CKeyboard::KeyPress(m_code2, 0);
		if (isMain)
		{
			CKeyboard::KeyPress('C', 0);
		}
	}
}

void HeroTask::setParam(int32_t code1, int32_t code2)
{
	m_code1 = code1;
	m_code2 = code2;
}
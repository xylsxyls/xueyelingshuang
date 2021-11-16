#include "stdafx.h"
#include "HeroTask.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"

extern HWND g_editWnd;

HeroTask::HeroTask():
m_code1(0),
m_code2(0)
{

}

void HeroTask::DoTask()
{
	Sleep(20);
	if (m_code1 != 0)
	{
		if (m_code1 == '?')
		{
			char editText[1024] = {};
			::GetWindowTextA(g_editWnd, editText, 1024);
			std::string str = editText;
			int32_t index = -1;
			while (index++ != str.size() - 1)
			{
				CKeyboard::KeyPress(str[index] + 48, 0);
			}
		}
		else
		{
			CKeyboard::KeyPress(m_code1, 0);
		}
	}
	if (m_code2 != 0)
	{
		CKeyboard::KeyPress(m_code2, 0);
	}
}

void HeroTask::setParam(int32_t code1, int32_t code2)
{
	m_code1 = code1;
	m_code2 = code2;
}
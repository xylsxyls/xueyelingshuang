#include "stdafx.h"
#include "CqswrTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "Config.h"
#include "CMouse/CMouseAPI.h"

CqswrTask::CqswrTask() :
m_editIndex(0),
m_exit(false)
{

}

void CqswrTask::DoTask()
{
	V5();
}

void CqswrTask::V5()
{
	char str[1024] = {};
	::GetWindowTextA(g_config.m_editWnd, str, 1024);

	std::string text = str;

	if (text.size() <= m_editIndex)
	{
		KeyPressE();
		if (Sleep(80))
		{
			return;
		}

		CKeyboard::KeyPress('J', 0);
		if (Sleep(150))
		{
			return;
		}
		KeyPressE();
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('L', 0);
		if (Sleep(10))
		{
			return;
		}
		CKeyboard::KeyPress('X', 0);
	}
	else
	{
		char e = text[m_editIndex];
		if (e == '8')
		{
			e = 'C' - 48;
		}
		CKeyboard::KeyPress(e + 48, 0);
		if (g_config.m_isMain && text[m_editIndex] != '8')
		{
			if (Sleep(20))
			{
				return;
			}
			CKeyboard::KeyPress('C', 0);
		}
		//if (Sleep(10))
		//{
		//	return;
		//}
		//CMouse::RightUp();
		if (Sleep(50))
		{
			return;
		}
		CMouse::RightUp();
		if (Sleep(110))
		{
			return;
		}
		for (int index = 0; index < 2; ++index)
		{
			if (Sleep(10))
			{
				return;
			}
			CKeyboard::KeyPress('J', 0);
		}
		if (Sleep(180))
		{
			return;
		}
		for (int index = 0; index < 5; ++index)
		{
			if (Sleep(10))
			{
				return;
			}
			CKeyboard::KeyPress(e + 48, 0);
		}
		if (g_config.m_isMain && text[m_editIndex] != '8')
		{
			if (Sleep(20))
			{
				return;
			}
			CKeyboard::KeyPress('C', 0);
		}
		//if (Sleep(10))
		//{
		//	return;
		//}
		for (int index = 0; index < 1; ++index)
		{
			//if (Sleep(10))
			//{
			//	return;
			//}
			CKeyboard::KeyPress('L', 0);
		}
		if (Sleep(40))
		{
			return;
		}
		CKeyboard::KeyDown('N');
		if (Sleep(150))
		{
			return;
		}
		CKeyboard::KeyUp('N');
	}
}

void CqswrTask::Q5()
{
	char str[1024] = {};
	::GetWindowTextA(g_config.m_editWnd, str, 1024);

	std::string text = str;

	if (text.size() <= m_editIndex)
	{
		KeyPressE();
		if (Sleep(80))
		{
			return;
		}

		CKeyboard::KeyPress('J', 0);
		if (Sleep(150))
		{
			return;
		}
		KeyPressE();
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('L', 0);
		if (Sleep(10))
		{
			return;
		}
		CKeyboard::KeyPress('X', 0);
	}
	else
	{
		char e = text[m_editIndex];
		if (e == '8')
		{
			e = 'C' - 48;
		}
		CKeyboard::KeyPress(e + 48, 0);
		if (g_config.m_isMain && text[m_editIndex] != '8')
		{
			if (Sleep(20))
			{
				return;
			}
			CKeyboard::KeyPress('C', 0);
		}
		//if (Sleep(10))
		//{
		//	return;
		//}
		//CMouse::RightUp();
		if (Sleep(50))
		{
			return;
		}
		CMouse::RightUp();
		if (Sleep(110))
		{
			return;
		}
		for (int index = 0; index < 2; ++index)
		{
			if (Sleep(10))
			{
				return;
			}
			CKeyboard::KeyPress('J', 0);
		}
		if (Sleep(180))
		{
			return;
		}
		for (int index = 0; index < 5; ++index)
		{
			if (Sleep(10))
			{
				return;
			}
			CKeyboard::KeyPress(e + 48, 0);
		}
		if (g_config.m_isMain && text[m_editIndex] != '8')
		{
			if (Sleep(20))
			{
				return;
			}
			CKeyboard::KeyPress('C', 0);
		}
		//if (Sleep(10))
		//{
		//	return;
		//}
		for (int index = 0; index < 1; ++index)
		{
			//if (Sleep(10))
			//{
			//	return;
			//}
			CKeyboard::KeyPress('L', 0);
		}
		if (Sleep(60))
		{
			return;
		}
		CKeyboard::KeyDown('N');
		if (Sleep(170))
		{
			return;
		}
		CKeyboard::KeyUp('N');
	}
}

void CqswrTask::V4()
{
	char str[1024] = {};
	::GetWindowTextA(g_config.m_editWnd, str, 1024);

	std::string text = str;

	if (text.size() <= m_editIndex)
	{
		KeyPressE();
		if (Sleep(80))
		{
			return;
		}
		CKeyboard::KeyPress('Z', 0);
		if (Sleep(100))
		{
			return;
		}

		CKeyboard::KeyPress('J', 0);
		if (Sleep(150))
		{
			return;
		}
		KeyPressE();
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('L', 0);
		if (Sleep(10))
		{
			return;
		}
		CKeyboard::KeyPress('X', 0);
	}
	else
	{
		char e = text[m_editIndex];
		if (e == '8')
		{
			e = 'C' - 48;
		}
		CKeyboard::KeyPress(e + 48, 0);
		if (g_config.m_isMain && text[m_editIndex] != '8')
		{
			if (Sleep(50))
			{
				return;
			}
			CKeyboard::KeyPress('C', 0);
		}
		if (Sleep(130))
		{
			return;
		}
		CMouse::RightUp();
		for (int index = 0; index < 3; ++index)
		{
			if (Sleep(10))
			{
				return;
			}
			CKeyboard::KeyPress('Z', 0);
		}
		for (int index = 0; index < 9; ++index)
		{
			if (Sleep(10))
			{
				return;
			}
			CKeyboard::KeyPress('J', 0);
		}
		if (Sleep(100))
		{
			return;
		}
		for (int index = 0; index < 3; ++index)
		{
			if (Sleep(10))
			{
				return;
			}
			CKeyboard::KeyPress(e + 48, 0);
		}
		if (g_config.m_isMain && text[m_editIndex] != '8')
		{
			if (Sleep(50))
			{
				return;
			}
			CKeyboard::KeyPress('C', 0);
		}
		if (Sleep(50))
		{
			return;
		}
		for (int index = 0; index < 5; ++index)
		{
			if (Sleep(10))
			{
				return;
			}
			CKeyboard::KeyPress('L', 0);
		}
		if (Sleep(30))
		{
			return;
		}
		CKeyboard::KeyDown('N');
		if (Sleep(120))
		{
			return;
		}
		CKeyboard::KeyUp('N');
	}
}

void CqswrTask::V3()
{
	char str[1024] = {};
	::GetWindowTextA(g_config.m_editWnd, str, 1024);

	std::string text = str;

	if (text.size() <= m_editIndex)
	{
		KeyPressE();
		if (Sleep(80))
		{
			return;
		}
		CKeyboard::KeyPress('Z', 0);
		if (Sleep(100))
		{
			return;
		}

		CKeyboard::KeyPress('J', 0);
		if (Sleep(150))
		{
			return;
		}
		KeyPressE();
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('L', 0);
		if (Sleep(10))
		{
			return;
		}
		CKeyboard::KeyPress('X', 0);
	}
	else
	{
		char e = text[m_editIndex];
		if (e == '8')
		{
			e = 'C' - 48;
			//e = 'H' - 48;
		}
		CKeyboard::KeyPress(e + 48, 0);
		if (g_config.m_isMain && text[m_editIndex] != '8')
		{
			if (Sleep(50))
			{
				return;
			}
			CKeyboard::KeyPress('C', 0);
		}
		if (Sleep(140))
		{
			return;
		}
		CKeyboard::KeyPress('J', 0);
		if (Sleep(10))
		{
			return;
		}
		CKeyboard::KeyPress('J', 0);
		for (int index = 0; index < 10; ++index)
		{
			if (Sleep(10))
			{
				return;
			}
			CKeyboard::KeyPress('Z', 0);
		}
		for (int index = 0; index < 2; ++index)
		{
			if (Sleep(10))
			{
				return;
			}
			CKeyboard::KeyPress('L', 0);
		}
		for (int index = 0; index < 6; ++index)
		{
			if (Sleep(10))
			{
				return;
			}
			CKeyboard::KeyPress(e + 48, 0);
		}
		if (g_config.m_isMain && text[m_editIndex] != '8')
		{
			if (Sleep(50))
			{
				return;
			}
			CKeyboard::KeyPress('C', 0);
		}
		if (Sleep(100))
		{
			return;
		}

		CKeyboard::KeyDown('N');
		if (Sleep(120))
		{
			return;
		}
		CKeyboard::KeyUp('N');
	}
}


void CqswrTask::V2()
{
	char str[1024] = {};
	::GetWindowTextA(g_config.m_editWnd, str, 1024);

	std::string text = str;

	if (text.size() <= m_editIndex)
	{
		KeyPressE();
		if (Sleep(80))
		{
			return;
		}
		CKeyboard::KeyPress('Z', 0);
		if (Sleep(100))
		{
			return;
		}

		CKeyboard::KeyPress('J', 0);
		if (Sleep(150))
		{
			return;
		}
		KeyPressE();
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('L', 0);
		if (Sleep(10))
		{
			return;
		}
		CKeyboard::KeyPress('X', 0);
	}
	else
	{
		char e = text[m_editIndex];
		if (e == '8')
		{
			e = 'C' - 48;
			//e = 'H' - 48;
		}
		CKeyboard::KeyPress(e + 48, 0);
		if (g_config.m_isMain && text[m_editIndex] != '8')
		{
			if (Sleep(50))
			{
				return;
			}
			CKeyboard::KeyPress('C', 0);
		}
		if (Sleep(100))
		{
			return;
		}
		CKeyboard::KeyPress('Z', 0);
		if (Sleep(100))
		{
			return;
		}
		CKeyboard::KeyPress('J', 0);
		if (Sleep(100))
		{
			return;
		}
		CKeyboard::KeyPress(e + 48, 0);
		if (Sleep(10))
		{
			return;
		}
		CKeyboard::KeyPress(e + 48, 0);
		if (Sleep(10))
		{
			return;
		}
		CKeyboard::KeyPress(e + 48, 0);
		if (Sleep(10))
		{
			return;
		}
		CKeyboard::KeyPress(e + 48, 0);
		if (Sleep(10))
		{
			return;
		}
		CKeyboard::KeyPress(e + 48, 0);
		if (Sleep(10))
		{
			return;
		}
		CKeyboard::KeyPress(e + 48, 0);
		if (g_config.m_isMain && text[m_editIndex] != '8')
		{
			if (Sleep(50))
			{
				return;
			}
			CKeyboard::KeyPress('C', 0);
		}
		if (Sleep(100))
		{
			return;
		}
		CKeyboard::KeyPress('L', 0);
		if (Sleep(30))
		{
			return;
		}
		CKeyboard::KeyDown('N');
		if (Sleep(100))
		{
			return;
		}
		CKeyboard::KeyUp('N');
	}
}

void CqswrTask::V1()
{
	char str[1024] = {};
	::GetWindowTextA(g_config.m_editWnd, str, 1024);

	std::string text = str;

	if (text.size() <= m_editIndex)
	{
		KeyPressE();
		if (Sleep(80))
		{
			return;
		}
		CKeyboard::KeyPress('Z', 0);
		if (Sleep(100))
		{
			return;
		}

		CKeyboard::KeyPress('J', 0);
		if (Sleep(150))
		{
			return;
		}
		KeyPressE();
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('L', 0);
		if (Sleep(10))
		{
			return;
		}
		CKeyboard::KeyPress('X', 0);
	}
	else
	{
		char e = text[m_editIndex];
		if (e == '8')
		{
			e = 'C' - 48;
		}
		CKeyboard::KeyPress(e + 48, 0);
		if (g_config.m_isMain)
		{
			if (Sleep(10))
			{
				return;
			}
			CKeyboard::KeyPress('C', 0);
		}
		if (Sleep(150))
		{
			return;
		}
		CKeyboard::KeyPress('Z', 0);
		if (Sleep(150))
		{
			return;
		}
		CKeyboard::KeyPress('J', 0);
		if (Sleep(150))
		{
			return;
		}
		CKeyboard::KeyPress(e + 48, 0);
		if (g_config.m_isMain)
		{
			if (Sleep(10))
			{
				return;
			}
			CKeyboard::KeyPress('C', 0);
		}
		if (Sleep(200))
		{
			return;
		}
		CKeyboard::KeyPress('L', 0);
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('N', 0);
	}
}

void CqswrTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

bool CqswrTask::Sleep(int32_t sleepTime)
{
	std::this_thread::sleep_for(std::chrono::microseconds(sleepTime * 1000));
	return m_exit;
}

void CqswrTask::KeyPressE()
{
	if (g_config.m_code1 != 0)
	{
		//CKeyboard::KeyPress(code1, 0);
	}
	if (g_config.m_code2 != 0)
	{
		CKeyboard::KeyPress('C', 0);
	}
}

void CqswrTask::KeyPressF()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(g_config.m_fCenterPoint, 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void CqswrTask::ClickHero(char heroNum)
{
	if (heroNum < '1' || heroNum > '5')
	{
		KeyPressE();
		return;
	}
	xyls::Rect point = g_config.m_heroHeadPoint[heroNum - 49] + xyls::Point(g_config.m_side / 2, g_config.m_side / 2);
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(point, 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void CqswrTask::setParam(uint32_t editIndex)
{
	m_editIndex = editIndex;
}
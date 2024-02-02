#include "stdafx.h"
#include "CWeaTask.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

CWeaTask::CWeaTask():
m_exit(false)
{

}

void CWeaTask::DoTask()
{
	std::string path = CSystem::GetCurrentExePath();
	if (m_vkCode == 'W')
	{
		while (!m_exit && ScreenScript::FindPic(path + "res\\yellow.png", xyls::Rect(788, 938, 860, 1010)).center().empty())
		{
			continue;
		}
		CKeyboard::InputString("www", 0);
	}
	else if (m_vkCode == 'E')
	{
		CKeyboard::InputString("w", 0);
		while (!m_exit && ScreenScript::FindPic(path + "res\\blue.png", xyls::Rect(788, 938, 860, 1010)).center().empty())
		{
			continue;
		}
		CKeyboard::InputString("www", 0);
	}
	else if (m_vkCode == 'A')
	{
		CKeyboard::InputString("w", 0);
		while (!m_exit && ScreenScript::FindPic(path + "res\\red.png", xyls::Rect(788, 938, 860, 1010)).center().empty())
		{
			continue;
		}
		CKeyboard::InputString("www", 0);
	}
	else if (m_vkCode == 'R')
	{
		CKeyboard::InputString("w", 0);
		while (!m_exit && ScreenScript::FindPic(path + "res\\yellow.png", xyls::Rect(788, 938, 860, 1010)).center().empty())
		{
			continue;
		}
		CKeyboard::InputString("www", 0);
	}
}

void CWeaTask::StopTask()
{
	m_exit = true;
}

void CWeaTask::setParam(int32_t vkCode)
{
	m_vkCode = vkCode;
}
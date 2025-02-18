#include "stdafx.h"
#include "SkillTask.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CScreen/CScreenAPI.h"
#include "Config.h"
#include "D:\\SendToMessageTest.h"

SkillTask::SkillTask():
m_skill1(0),
m_skill2(0),
m_skill3(0)
{

}

void SkillTask::DoTask()
{
	xyls::Rect shopPoint = ScreenScript::FindPic(CSystem::GetCurrentExePath() + "res\\shop_" + std::to_string(g_config.m_screenWidth) + ".png",
		xyls::Rect(0, 0, g_config.m_screenWidth / 5, g_config.m_screenHeight), 0.6);
	if (shopPoint.empty())
	{
		RCSend("not find shop");
		return;
	}

	xyls::Rect addPoint = ScreenScript::FindPic(CSystem::GetCurrentExePath() + "res\\add_" + std::to_string(g_config.m_screenWidth) + ".png",
		xyls::Rect(g_config.m_screenWidth / 2, g_config.m_screenHeight / 2, g_config.m_screenWidth, g_config.m_screenHeight), 0.9);
	xyls::Rect addPoint3 = ScreenScript::FindPic(CSystem::GetCurrentExePath() + "res\\add3_" + std::to_string(g_config.m_screenWidth) + ".png",
		xyls::Rect(g_config.m_screenWidth / 2, g_config.m_screenHeight / 2, g_config.m_screenWidth, g_config.m_screenHeight), 0.9);
	if (addPoint.empty() && addPoint3.empty())
	{
		RCSend("not find add");
		return;
	}
	CKeyboard::KeyPress(m_skill1, 0);
	Sleep(10);
	CKeyboard::KeyPress(m_skill2, 0);
	Sleep(10);
	CKeyboard::KeyPress(m_skill3, 0);
}

void SkillTask::setParam(char skill1, char skill2, char skill3)
{
	m_skill1 = skill1;
	m_skill2 = skill2;
	m_skill3 = skill3;
}

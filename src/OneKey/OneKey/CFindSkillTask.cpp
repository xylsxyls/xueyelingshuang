#include "stdafx.h"
#include "CFindSkillTask.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CSystem/CSystemAPI.h"
//#include "D:\\SendToMessageTest.h"
#include "Config.h"

CFindSkillTask::CFindSkillTask()
{

}

void CFindSkillTask::DoTask()
{
	xyls::Rect fPoint = ScreenScript::FindPic(CSystem::GetCurrentExePath() + "res\\chengjie_" + std::to_string(g_config.m_screenWidth) + ".png",
		xyls::Rect(g_config.m_screenWidth / 2, 0, g_config.m_screenWidth, g_config.m_screenHeight), 0.7);
	xyls::Rect sPoint = ScreenScript::FindPic(CSystem::GetCurrentExePath() + "res\\bingjia_" + std::to_string(g_config.m_screenWidth) + ".png",
		xyls::Rect(g_config.m_screenWidth / 2, 0, g_config.m_screenWidth, g_config.m_screenHeight), 0.7);

	g_config.m_hasF = !fPoint.empty();
	g_config.m_hasS = !sPoint.empty();
}
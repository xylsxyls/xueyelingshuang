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
	xyls::Point fPoint = ScreenScript::FindPic(CSystem::GetCurrentExePath() + "res\\chengjie.png",
		xyls::Rect(1174, 891, 1920, 1080), 0.7);
	xyls::Point sPoint = ScreenScript::FindPic(CSystem::GetCurrentExePath() + "res\\bingjia.png",
		xyls::Rect(1656, 340, 1818, 497), 0.7);

	g_config.m_hasF = !fPoint.empty();
	g_config.m_hasS = !sPoint.empty();
}
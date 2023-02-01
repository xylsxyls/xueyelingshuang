#include "stdafx.h"
#include "SkillTask.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"
#include "CScreen/CScreenAPI.h"

SkillTask::SkillTask():
m_skill1(0),
m_skill2(0),
m_skill3(0)
{

}

void SkillTask::DoTask()
{
	xyls::Point addPoint = ScreenScript::FindPic(CSystem::GetCurrentExePath() + "res\\add.png",
		xyls::Rect(1920 / 2, 1080 / 2, 1920, 1080), 0.9);
	xyls::Point addPoint3 = ScreenScript::FindPic(CSystem::GetCurrentExePath() + "res\\add3.png",
		xyls::Rect(1920 / 2, 1080 / 2, 1920, 1080), 0.9);
	if (addPoint.empty() && addPoint3.empty())
	{
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

#include "SkillTask.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

SkillTask::SkillTask():
m_exit(false)
{

}

void SkillTask::DoTask()
{
	std::string currentExePath = CSystem::GetCurrentExePath();
	while (!m_exit)
	{
		CKeyboard::InputString("\t");
		Sleep(200);
		ScreenScript::FindClick(currentExePath + "res\\shoufu.png", true, false, xyls::Rect(xyls::Point(904, 557), 96, 80));
		Sleep(1000);
	}
}

void SkillTask::StopTask()
{
	m_exit = true;
}
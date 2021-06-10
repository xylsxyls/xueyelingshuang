#include "SkillTask.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

SkillTask::SkillTask():
m_exit(false)
{

}

void SkillTask::DoTask()
{
	xyls::Point lastPos;
	std::string currentExePath = CSystem::GetCurrentExePath();
	int32_t index = -1;
	while (!m_exit)
	{
		xyls::Point currentPos = CMouse::GetCurrentPos();
		if (lastPos.x() == currentPos.x() && lastPos.y() == currentPos.y())
		{
			CKeyboard::InputString("\t");
			Sleep(200);
		}
		ScreenScript::FindClick(currentExePath + "res\\shoufu.png", true, false, xyls::Rect(xyls::Point(904, 557), 96, 80));
		Sleep(1000);
		if (++index % 5 == 0)
		{
			if (lastPos.x() == currentPos.x() && lastPos.y() == currentPos.y())
			{
				CMouse::MoveAbsolute(xyls::Point(817, 13), 0);
				CMouse::LeftClick();
				CKeyboard::InputString("`");
			}
			lastPos = currentPos;
		}
	}
}

void SkillTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void SkillTask::Sleep(int32_t time)
{
	if (m_exit)
	{
		return;
	}
	m_sleep.wait(time);
}
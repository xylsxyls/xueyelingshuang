#include "CheckTask.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

CheckTask::CheckTask():
m_exit(false)
{

}

void CheckTask::DoTask()
{
	RCSend("step = %d", m_step);
	std::string currentPath = CSystem::GetCurrentExePath();

	while (!m_exit)
	{
		CKeyboard::InputString("`");
		if (ScreenScript::FindClick(currentPath + "res/dtws_confirm.png"))
		{
			Sleep(3000);
		}
		if (ScreenScript::FindClick(currentPath + "res/dtws_close.png", true, false, xyls::Rect(981, 309, 1350, 538)))
		{
			Sleep(3000);
		}
		if (ScreenScript::FindClick(currentPath + "res/dtws_recommand_close.png"))
		{
			Sleep(3000);
		}
		if (ScreenScript::FindPic(currentPath + "res/dtws_other1.png"))
		{
			CKeyboard::KeyPress(CKeyboard::Esc);
			Sleep(3000);
		}
		if (ScreenScript::FindPic(currentPath + "res/dtws_other2.png"))
		{
			CKeyboard::KeyPress(CKeyboard::Esc);
			Sleep(3000);
		}
		if (ScreenScript::FindPic(currentPath + "res/dtws_other3.png"))
		{
			CKeyboard::KeyPress(CKeyboard::Esc);
			Sleep(3000);
		}
		if (ScreenScript::FindPic(currentPath + "res/dtws_other4.png"))
		{
			CKeyboard::KeyPress(CKeyboard::Esc);
			Sleep(3000);
		}
		Sleep(2000);
	}
}

void CheckTask::StopTask()
{
	m_exit = true;
}

bool CheckTask::Sleep(int32_t sleepTime)
{
	int32_t count = sleepTime / 100;
	while (count-- != 0)
	{
		if (m_exit)
		{
			return false;
		}
		::Sleep(100);
	}
	return true;
}
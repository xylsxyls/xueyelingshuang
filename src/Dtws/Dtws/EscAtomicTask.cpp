#include "EscAtomicTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "Config.h"

bool EscAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	Sleep(g_config.m_goFirstSleepTime);
	CKeyboard::KeyDown(CKeyboard::Esc);
	CKeyboard::KeyUp(CKeyboard::Esc);
	return false;
}
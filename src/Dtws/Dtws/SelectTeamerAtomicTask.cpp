#include "SelectTeamerAtomicTask.h"
#include "CKeyboard/CKeyboardAPI.h"

bool SelectTeamerAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	if (!vecPic.empty())
	{
		CKeyboard::KeyPress(CKeyboard::Esc, 0);
	}
	CKeyboard::KeyPress(CKeyboard::F2, 0);
	return false;
}

bool SelectTeamerAtomicTask::CheckFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	if (vecPic.empty())
	{
		return true;
	}
	CKeyboard::InputString("4", 0);
	Sleep(10);
	CKeyboard::InputString("4", 0);
	Sleep(10);
	CKeyboard::InputString("4", 0);
	return false;
}
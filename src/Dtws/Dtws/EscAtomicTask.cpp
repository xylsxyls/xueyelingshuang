#include "EscAtomicTask.h"
#include "CKeyboard/CKeyboardAPI.h"

bool EscAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	CKeyboard::KeyDown(CKeyboard::Esc);
	CKeyboard::KeyUp(CKeyboard::Esc);
	return false;
}
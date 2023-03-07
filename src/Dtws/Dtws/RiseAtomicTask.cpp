#include "RiseAtomicTask.h"
#include "CKeyboard/CKeyboardAPI.h"

bool RiseAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	CKeyboard::KeyDown(CKeyboard::Alt);
	CKeyboard::KeyDown('2');
	CKeyboard::KeyUp(CKeyboard::Alt);
	CKeyboard::KeyUp('2');
	return false;
}
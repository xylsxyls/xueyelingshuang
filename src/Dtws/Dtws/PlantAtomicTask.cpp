#include "PlantAtomicTask.h"
#include "CKeyboard/CKeyboardAPI.h"

bool PlantAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	CKeyboard::KeyDown(CKeyboard::Alt);
	CKeyboard::KeyDown('3');
	CKeyboard::KeyUp(CKeyboard::Alt);
	CKeyboard::KeyUp('3');
	return false;
}
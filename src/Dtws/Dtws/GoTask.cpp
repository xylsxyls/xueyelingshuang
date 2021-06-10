#include "GoTask.h"
#include "CMouse/CMouseAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

GoTask::GoTask():
m_index(0)
{

}

void GoTask::DoTask()
{
	ScreenScript::FindClick("go.png");
	ScreenScript::FindClick("xun.png");
	CMouse::MoveOpposite(xyls::Point(0, 0));
	CSystem::setClipboardData(nullptr, m_placeName);
	CKeyboard::KeyDown(CKeyboard::Ctrl);
	CKeyboard::KeyDown('V');
	CKeyboard::KeyUp(CKeyboard::Ctrl);
	CKeyboard::KeyUp('V');
	ScreenScript::FindClick("xun.png");
	CMouse::MoveOpposite(xyls::Point(0, 0 + m_index * 50));
	CMouse::LeftDoubleClick();
}

void GoTask::setParam(const std::string& placeName, int32_t index)
{
	m_placeName = placeName;
	m_index = index;
}
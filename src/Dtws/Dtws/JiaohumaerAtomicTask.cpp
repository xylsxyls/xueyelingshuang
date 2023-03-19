#include "JiaohumaerAtomicTask.h"
#include "CKeyboard/CKeyboardAPI.h"

JiaohumaerAtomicTask::JiaohumaerAtomicTask():
m_isFind(false)
{

}

bool JiaohumaerAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	if (vecPic.empty())
	{
		RCSend("222");
		m_isFind = false;
		return false;
	}
	RCSend("111");
	CKeyboard::KeyPress(CKeyboard::Right);
	CKeyboard::KeyPress(CKeyboard::Left);
	CKeyboard::KeyPress(96);
	CKeyboard::KeyPress(96);
	CKeyboard::KeyPress(96);
	CKeyboard::KeyPress(96);
	CKeyboard::KeyPress(96);
	CKeyboard::KeyPress(96);
	CKeyboard::KeyPress(96);
	CKeyboard::KeyPress(96);
	m_isFind = true;
	return false;
}

bool JiaohumaerAtomicTask::isFind()
{
	return m_isFind;
}
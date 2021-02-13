#include "SayTask.h"
#include "CKeyboard/CKeyboardAPI.h"

void SayTask::DoTask()
{
	CKeyboard::InputString("\n");
	CKeyboard::InputString(m_say + " ");
	CKeyboard::InputString("\n");
}

void SayTask::setParam(const std::string& say)
{
	m_say = say;
}
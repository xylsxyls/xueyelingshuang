#include "SaTask.h"
#include "CKeyboard/CKeyboardAPI.h"

void SaTask::DoTask()
{
	while (m_exit)
	{
		CKeyboard::InputString("`s`a", 100);
		CKeyboard::InputString("`", 100);
	}
}

void SaTask::StopTask()
{
	m_exit = true;
}
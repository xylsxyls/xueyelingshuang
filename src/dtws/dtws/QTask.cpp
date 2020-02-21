#include "QTask.h"
#include "CKeyboard/CKeyboardAPI.h"

QTask::QTask():
m_exit(false)
{

}

void QTask::DoTask()
{
	while (!m_exit)
	{
		CKeyboard::InputString("qqqqqrqqqqqrq", 100);
		CKeyboard::InputString("rrrrrrr", 100);
		CKeyboard::InputString("`", 500);
	}
}

void QTask::StopTask()
{
	m_exit = true;
}
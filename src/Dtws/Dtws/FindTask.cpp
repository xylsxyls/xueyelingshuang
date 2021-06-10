#include "FindTask.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CMouse/CMouseAPI.h"

FindTask::FindTask():
m_isFind(nullptr),
m_findEnd(nullptr),
m_exit(false)
{

}

void FindTask::DoTask()
{
	if (m_exit)
	{
		return;
	}
	*m_isFind = false;
	CMouse::MoveAbsolute(m_click);
	Sleep(200);
	if (m_exit)
	{
		return;
	}
	m_clone->m_exit = true;
	int32_t count = 3;
	while (count-- != 0)
	{
		if (ScreenScript::FindPic(m_findPic))
		{
			*m_isFind = true;
			break;
		}
	}
	m_findEnd->signal();
}

void FindTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

bool FindTask::ReExecute()
{
	return true;
}

CTask* FindTask::Clone()
{
	m_clone = new FindTask;
	m_clone->setParam(m_click, m_findPic, m_isFind, m_findEnd);
	return m_clone;
}

void FindTask::setParam(const xyls::Point& click, const std::string& findPic, bool* isFind, Semaphore* findEnd)
{
	m_click = click;
	m_findPic = findPic;
	m_isFind = isFind;
	m_findEnd = findEnd;
}

void FindTask::Sleep(int32_t time)
{
	if (m_exit)
	{
		return;
	}
	m_sleep.wait(time);
}
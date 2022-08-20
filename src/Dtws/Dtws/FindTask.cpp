#include "FindTask.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CMouse/CMouseAPI.h"

extern int32_t g_accountCount;
extern xyls::Point g_clickTop[3];

FindTask::FindTask():
m_isFind(nullptr),
m_findEnd(nullptr),
m_exit(false)
{

}

FindTask::~FindTask()
{
	m_findEnd->signal();
}

void FindTask::DoTask()
{
	if (m_exit)
	{
		return;
	}
	*m_isFind = false;
	if (g_accountCount != 1)
	{
		CMouse::MoveAbsolute(m_click, 0);
		CMouse::MiddleClick();
		Sleep(200);
	}
	if (m_exit)
	{
		return;
	}
	m_clone->m_exit = true;
	int32_t count = 3;
	while (count-- != 0)
	{
		if (!ScreenScript::FindPic(m_findPic, m_findPicRect).empty())
		{
			*m_isFind = true;
			break;
		}
	}
}

void FindTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
	m_findEnd->signal();
}

bool FindTask::ReExecute()
{
	return true;
}

CTask* FindTask::Clone()
{
	m_clone = new FindTask;
	m_clone->setParam(m_click, m_findPicRect, m_findPic, m_isFind, m_findEnd);
	return m_clone;
}

void FindTask::setParam(const xyls::Point& click, const xyls::Rect& findPicRect, const std::string& findPic, bool* isFind, Semaphore* findEnd)
{
	m_click = click;
	m_findPicRect = findPicRect;
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
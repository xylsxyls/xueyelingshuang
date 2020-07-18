#include "stdafx.h"
#include "IntoGameTask.h"
#include "CMouse/CMouseAPI.h"

IntoGameTask::IntoGameTask():
m_exit(false),
m_time(0)
{

}

void IntoGameTask::DoTask()
{
	int32_t count = 40 - m_time / 2 - 4;
	Sleep(3000);
	while (!m_exit)
	{
		CMouse::MoveAbsolute(xyls::Point(858, 843));
		CMouse::LeftClick();
		Sleep(3000);
		if (m_exit)
		{
			return;
		}
		while (count-- > 0)
		{
			if (m_exit)
			{
				return;
			}
			CMouse::MoveAbsolute(xyls::Point(952, 716));
			CMouse::LeftClick();
			Sleep(3000);
		}
		CMouse::MoveAbsolute(xyls::Point(764, 847));
		CMouse::LeftClick();
		Sleep(3000);
		count = 40;
	}
}

void IntoGameTask::StopTask()
{
	m_exit = true;
}

void IntoGameTask::setParam(int32_t time)
{
	m_time = time;
}
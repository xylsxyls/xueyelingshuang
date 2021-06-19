#include "SubmitTask.h"
#include "CMouse/CMouseAPI.h"

extern int32_t g_accountCount;
extern xyls::Point g_clickTop[3];
extern xyls::Point g_taskPoint[3];

SubmitTask::SubmitTask():
m_sleepTime(0)
{

}

void SubmitTask::DoTask()
{
	if (m_sleepTime != 0)
	{
		Sleep(m_sleepTime);
	}

	int32_t accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		CMouse::MoveAbsolute(g_accountCount == 1 ? xyls::Point(149, 9) : g_clickTop[accountIndex], 0);
		CMouse::MiddleClick();
		Sleep(50);
		CMouse::MoveAbsolute(g_accountCount == 1 ? xyls::Point(1713, 287) : g_taskPoint[accountIndex], 0);
		CMouse::LeftClick(0);
		xyls::Point downPoint = g_taskPoint[accountIndex];
		downPoint.SetY(downPoint.y() + 15);
		CMouse::MoveAbsolute(g_accountCount == 1 ? xyls::Point(1713, 302) : downPoint, 0);
		CMouse::LeftClick(0);
	}
}

void SubmitTask::setParam(int32_t sleepTime)
{
	m_sleepTime = sleepTime;
}
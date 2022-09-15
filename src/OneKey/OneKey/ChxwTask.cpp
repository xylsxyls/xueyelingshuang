#include "stdafx.h"
#include "ChxwTask.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

ChxwTask::ChxwTask()
{

}

extern xyls::Point g_heroCenterPoint;
xyls::Point g_wCenterPoint = { 1542, 749 };
int32_t g_wWidth = 222;
int32_t g_wUpHeight = 134;
int32_t g_wHeight = 190;

void ChxwTask::DoTask()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	if (currentPos.x() < (1920 / 2) && currentPos.y() > 618)
	{
		g_wWidth = 162;
		g_wUpHeight = 134;
		g_wHeight = 190;
	}
	else if (currentPos.x() < (1920 / 2) && currentPos.y() <= 618)
	{
		g_wWidth = 245;
		g_wUpHeight = 118;
		g_wHeight = 190;
	}
	else if (currentPos.x() >= (1920 / 2) && currentPos.y() <= 618)
	{
		g_wWidth = 245;
		g_wUpHeight = 118;
		g_wHeight = 190;
	}
	else if (currentPos.x() >= (1920 / 2) && currentPos.y() > 618)
	{
		g_wWidth = 162;
		g_wUpHeight = 134;
		g_wHeight = 190;
	}

	int32_t x = (int32_t)(g_wCenterPoint.x() - (currentPos.x() - 1920 / 2) / (1920 / 2.0) * (g_wWidth / 2));
	int32_t y = 0;
	if (currentPos.y() < g_heroCenterPoint.y())
	{
		y = (int32_t)(g_wCenterPoint.y() + (g_heroCenterPoint.y() - currentPos.y()) / (double)g_heroCenterPoint.y() * g_wUpHeight);
	}
	else
	{
		y = (int32_t)(g_wCenterPoint.y() - (currentPos.y() - g_heroCenterPoint.y()) / (double)(1080 - g_heroCenterPoint.y()) * (g_wHeight - g_wUpHeight));
	}

	xyls::Point movePoint = { x, y };

	CMouse::RightClick(5);

	CMouse::MoveAbsolute(g_wCenterPoint, 0);
	CMouse::LeftDown();
	Sleep(10);
	CMouse::MoveAbsolute(xyls::Point(1542, 700), 0);
	Sleep(10);
	CMouse::MoveAbsolute(movePoint, 1);
	Sleep(20);
	CMouse::LeftUp();
	CMouse::MoveAbsolute(currentPos, 0);
}
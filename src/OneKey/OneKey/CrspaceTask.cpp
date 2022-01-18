#include "stdafx.h"
#include "CrspaceTask.h"
#include "CMouse/CMouseAPI.h"
#include "D:\\SendToMessageTest.h"

xyls::Point g_heroCenterPoint = { 1920 / 2, 618 };
xyls::Point g_rCenterPoint = { 1742, 634 };
int32_t g_rWidth = 222;
int32_t g_rUpHeight = 134;
int32_t g_rHeight = 190;

void CrspaceTask::DoTask()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	if (currentPos.x() < (1920 / 2) && currentPos.y() > 618)
	{
		g_rWidth = 162;
		g_rUpHeight = 134;
		g_rHeight = 190;
	}
	else if (currentPos.x() < (1920 / 2) && currentPos.y() <= 618)
	{
		g_rWidth = 245;
		g_rUpHeight = 118;
		g_rHeight = 190;
	}
	else if (currentPos.x() >= (1920 / 2) && currentPos.y() <= 618)
	{
		g_rWidth = 245;
		g_rUpHeight = 118;
		g_rHeight = 190;
	}
	else if (currentPos.x() >= (1920 / 2) && currentPos.y() > 618)
	{
		g_rWidth = 162;
		g_rUpHeight = 134;
		g_rHeight = 190;
	}
	
	int32_t x = (int32_t)((currentPos.x() - 1920 / 2) / (1920 / 2.0) * (g_rWidth / 2) + g_rCenterPoint.x());
	int32_t y = 0;
	if (currentPos.y() < g_heroCenterPoint.y())
	{
		y = (int32_t)(g_rCenterPoint.y() - (g_heroCenterPoint.y() - currentPos.y()) / (double)g_heroCenterPoint.y() * g_rUpHeight);
	}
	else
	{
		y = (int32_t)(g_rCenterPoint.y() + (currentPos.y() - g_heroCenterPoint.y()) / (double)(1080 - g_heroCenterPoint.y()) * (g_rHeight - g_rUpHeight));
	}
	
	xyls::Point movePoint = { x, y }; 

	CMouse::MoveAbsolute(g_rCenterPoint, 0);
	CMouse::LeftDown();
	Sleep(10);
	CMouse::MoveAbsolute(xyls::Point(1742, 700), 0);
	Sleep(10);
	CMouse::MoveAbsolute(movePoint, 1);
	Sleep(20);
	CMouse::LeftUp();
	CMouse::MoveAbsolute(currentPos, 0);
}
#include "stdafx.h"
#include "CRMoveTask.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

void CRMoveTask::DoTask()
{
	int32_t rWidth = 0;
	int32_t rHeight = 0;
	int32_t rUpHeight = 0;

	xyls::Point currentPos = CMouse::GetCurrentPos();
	if (currentPos.x() < g_config.m_heroCenterPoint.x() && currentPos.y() > g_config.m_heroCenterPoint.y())
	{
		if (g_config.m_screenWidth == 1920)
		{
			rWidth = 162;
			rHeight = 190;
			rUpHeight = 134;
		}
		else if (g_config.m_screenWidth == 3840)
		{
			rWidth = 315;
			rHeight = 296;
			rUpHeight = 180;
		}
	}
	else if (currentPos.x() < g_config.m_heroCenterPoint.x() && currentPos.y() <= g_config.m_heroCenterPoint.y())
	{
		if (g_config.m_screenWidth == 1920)
		{
			rWidth = 245;
			rHeight = 190;
			rUpHeight = 118;
		}
		else if (g_config.m_screenWidth == 3840)
		{
			rWidth = 476;
			rHeight = 296;
			rUpHeight = 210;
		}
	}
	else if (currentPos.x() >= g_config.m_heroCenterPoint.x() && currentPos.y() <= g_config.m_heroCenterPoint.y())
	{
		if (g_config.m_screenWidth == 1920)
		{
			rWidth = 245;
			rHeight = 190;
			rUpHeight = 118;
		}
		else if (g_config.m_screenWidth == 3840)
		{
			rWidth = 476;
			rHeight = 296;
			rUpHeight = 210;
		}
	}
	else if (currentPos.x() >= g_config.m_heroCenterPoint.x() && currentPos.y() > g_config.m_heroCenterPoint.y())
	{
		if (g_config.m_screenWidth == 1920)
		{
			rWidth = 162;
			rHeight = 190;
			rUpHeight = 134;
		}
		else if (g_config.m_screenWidth == 3840)
		{
			rWidth = 315;
			rHeight = 296;
			rUpHeight = 180;
		}
	}

	int32_t x = (int32_t)((currentPos.x() - g_config.m_heroCenterPoint.x()) / (double)(g_config.m_heroCenterPoint.x()) * (rWidth / 2) + g_config.m_rCenterPoint.x());
	int32_t y = 0;
	if (currentPos.y() < g_config.m_heroCenterPoint.y())
	{
		y = (int32_t)(g_config.m_rCenterPoint.y() - (g_config.m_heroCenterPoint.y() - currentPos.y()) / (double)g_config.m_heroCenterPoint.y() * rUpHeight);
	}
	else
	{
		y = (int32_t)(g_config.m_rCenterPoint.y() + (currentPos.y() - g_config.m_heroCenterPoint.y()) / (double)(g_config.m_screenHeight - g_config.m_heroCenterPoint.y()) * (rHeight - rUpHeight));
	}

	xyls::Point movePoint = { x, y };

	CMouse::MoveAbsolute(g_config.m_rCenterPoint, 0);
	CMouse::LeftDown();
	Sleep(10);
	CMouse::MoveAbsolute(g_config.m_rCenterPoint + xyls::Point(0, 100), 0);
	Sleep(10);
	CMouse::MoveAbsolute(movePoint, 1);
	Sleep(20);
	CMouse::LeftUp();
	CMouse::MoveAbsolute(currentPos, 0);
}
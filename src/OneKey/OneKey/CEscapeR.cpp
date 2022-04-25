#include "stdafx.h"
#include "CEscapeR.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyBoard/CKeyboardAPI.h"

static xyls::Point s_heroCenterPoint = { 1920 / 2, 618 };
static xyls::Point s_rCenterPoint = { 1742, 634 };
static int32_t s_rWidth = 222;
static int32_t s_rUpHeight = 134;
static int32_t s_rHeight = 190;

void CEscapeR::DoTask()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	if (currentPos.x() < (1920 / 2) && currentPos.y() > 618)
	{
		s_rWidth = 162 * 5;
		s_rUpHeight = 134 * 5;
		s_rHeight = 190 * 5;
	}
	else if (currentPos.x() < (1920 / 2) && currentPos.y() <= 618)
	{
		s_rWidth = 245 * 5;
		s_rUpHeight = 118 * 5;
		s_rHeight = 190 * 5;
	}
	else if (currentPos.x() >= (1920 / 2) && currentPos.y() <= 618)
	{
		s_rWidth = 245 * 5;
		s_rUpHeight = 118 * 5;
		s_rHeight = 190 * 5;
	}
	else if (currentPos.x() >= (1920 / 2) && currentPos.y() > 618)
	{
		s_rWidth = 162 * 5;
		s_rUpHeight = 134 * 5;
		s_rHeight = 190 * 5;
	}

	int32_t x = (int32_t)((currentPos.x() - 1920 / 2) / (1920 / 2.0) * (s_rWidth / 2) + s_rCenterPoint.x());
	int32_t y = 0;
	if (currentPos.y() < s_heroCenterPoint.y())
	{
		y = (int32_t)(s_rCenterPoint.y() - (s_heroCenterPoint.y() - currentPos.y()) / (double)s_heroCenterPoint.y() * s_rUpHeight);
	}
	else
	{
		y = (int32_t)(s_rCenterPoint.y() + (currentPos.y() - s_heroCenterPoint.y()) / (double)(1080 - s_heroCenterPoint.y()) * (s_rHeight - s_rUpHeight));
	}

	xyls::Point movePoint = { x, y };

	CMouse::MoveAbsolute(s_rCenterPoint, 0);
	CMouse::LeftDown();
	Sleep(10);
	CMouse::MoveAbsolute(xyls::Point(1742, 700), 0);
	Sleep(10);
	CMouse::MoveAbsolute(movePoint, 1);
	Sleep(20);
	CMouse::LeftUp();
	CMouse::MoveAbsolute(currentPos, 0);
}
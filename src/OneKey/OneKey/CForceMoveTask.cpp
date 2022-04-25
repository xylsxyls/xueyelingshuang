#include "stdafx.h"
#include "CForceMoveTask.h"
#include "CMouse/CMouseAPI.h"

void CForceMoveTask::disableMouse()
{
	RECT rect{ 1, 1, 3, 3 };
	::ClipCursor(&rect);
}

void CForceMoveTask::enableMouse()
{
	::ClipCursor(nullptr);
}

void CForceMoveTask::moveAbsolute(const xyls::Point& point, int32_t sleepTime)
{
	RECT rect;
	rect.left = point.x() - 1;
	rect.top = point.y() - 1;
	rect.right = point.x() + 1;
	rect.bottom = point.y() + 1;
	::ClipCursor(&rect);
	CMouse::MoveAbsolute(point, sleepTime);
}
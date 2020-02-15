#include "MinimizeTask.h"
#include "CMouse/CMouseAPI.h"

void MinimizeTask::DoTask()
{
	CMouse::MoveAbsolute(xyls::Point(1836, 9), 0);
	CMouse::LeftClick();
}
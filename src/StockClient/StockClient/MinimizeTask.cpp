#include "MinimizeTask.h"
#include "CMouse/CMouseAPI.h"

void MinimizeTask::DoTask()
{
	CMouse::MoveClick(xyls::Point(1836, 9));
}
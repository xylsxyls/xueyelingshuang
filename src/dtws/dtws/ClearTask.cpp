#include "ClearTask.h"
#include "CMouse/CMouseAPI.h"

void ClearTask::DoTask()
{
	RCSend("step = %d", m_step);
	CMouse::MoveClick(xyls::Point(941, 547), 10);
}
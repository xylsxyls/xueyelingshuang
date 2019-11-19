#include "MainTask.h"
#include "CMouse/CMouseAPI.h"

void MainTask::DoTask()
{
	RCSend("step = %d", m_step);
	int32_t count = 10;
	while (count-- != 0)
	{
		CMouse::MoveClick(xyls::Point(785, 613), 30);
		Sleep(800);
	}
}
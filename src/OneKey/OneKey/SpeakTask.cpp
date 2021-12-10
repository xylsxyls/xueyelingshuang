#include "stdafx.h"
#include "SpeakTask.h"
#include "CMouse/CMouseAPI.h"

void SpeakTask::DoTask()
{
	CMouse::MoveClick(xyls::Point(1856, 410), 0);
	Sleep(50);
	CMouse::MoveClick(xyls::Point(1585, 834), 0);
}
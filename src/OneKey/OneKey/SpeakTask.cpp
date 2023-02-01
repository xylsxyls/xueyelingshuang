#include "stdafx.h"
#include "SpeakTask.h"
#include "CMouse/CMouseAPI.h"

void SpeakTask::DoTask()
{
	CMouse::MoveClick(xyls::Point(1742, 447), 0);
	Sleep(50);
	CMouse::MoveClick(xyls::Point(1523, 765), 0);
}
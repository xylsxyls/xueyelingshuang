#include "stdafx.h"
#include "SpeakTask.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

void SpeakTask::DoTask()
{
	CMouse::MoveClick(g_config.m_speakBeginPoint, 0);
	Sleep(50);
	CMouse::MoveClick(g_config.m_speakEndPoint, 0);
}
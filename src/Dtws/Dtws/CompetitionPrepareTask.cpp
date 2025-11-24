#include "CompetitionPrepareTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "NetSender/NetSenderAPI.h"

CompetitionPrepareTask::CompetitionPrepareTask() :
m_exit(false)
{

}

void CompetitionPrepareTask::DoTask()
{
	if (!ScreenScript::WaitClickPic(g_config.m_currentExePath + g_config.m_competitionPath,
		true, false, g_config.m_competitionRect, 0.7, 600000))
	{
		return;
	}
	NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_STOP));
	NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_COMPETITION));
}

void CompetitionPrepareTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void CompetitionPrepareTask::Sleep(int32_t time)
{
	if (m_exit)
	{
		return;
	}
	m_sleep.wait(time);
}
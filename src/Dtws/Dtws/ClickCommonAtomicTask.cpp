#include "ClickCommonAtomicTask.h"
#include "Config.h"
#include "CSystem/CSystemAPI.h"
#include "DtwsParam.h"

void ClickCommonAtomicTask::heroBlood(bool isStart, bool isFind)
{
	pic(isStart, isFind, g_config.m_currentExePath + g_screen.m_bloodPath, g_screen.m_bloodRect[m_accountIndex], 0.7);
}

void ClickCommonAtomicTask::go(bool isStart, bool isFind)
{
	pic(isStart, isFind, g_config.m_currentExePath + g_screen.m_goWindowHeadPath,
		g_screen.m_talkWindowHeadRect[m_accountIndex], 0.7);
}

void ClickCommonAtomicTask::accept(bool isStart, bool isFind)
{
	pic(isStart, isFind, g_config.m_currentExePath + g_screen.m_talkWindowHeadPath,
		g_screen.m_talkWindowHeadRect[m_accountIndex], 0.7);
}

void ClickCommonAtomicTask::jiaohumaer(bool isStart, bool isFind)
{
	pic(isStart, isFind, g_config.m_currentExePath + g_screen.m_chatPath,
		g_screen.m_chatRect[m_accountIndex], 0.7);
}

void ClickCommonAtomicTask::lowSleep(int32_t sleepTime)
{
	if (CSystem::getComputerName() != FIRST_COMPUTER)
	{
		Sleep(sleepTime);
	}
}

void ClickCommonAtomicTask::pic(bool isStart, bool isFind, const std::string& path, const xyls::Rect& rect, double sim)
{
	if (isStart)
	{
		if (isFind)
		{
			ClickAtomicTask::startWithFindPic(
				std::vector < std::pair<std::string, std::pair<xyls::Rect, double>> > {{ path, { rect, sim } }});
		}
		else
		{
			ClickAtomicTask::startWithNotFindPic(
				std::vector < std::pair<std::string, std::pair<xyls::Rect, double>> > {{ path, { rect, sim } }});
		}
	}
	else
	{
		if (isFind)
		{
			ClickAtomicTask::endWithFindPic(
				std::vector < std::pair<std::string, std::pair<xyls::Rect, double>> > {{ path, { rect, sim } }});
		}
		else
		{
			ClickAtomicTask::endWithNotFindPic(
				std::vector < std::pair<std::string, std::pair<xyls::Rect, double>> > {{ path, { rect, sim } }});
		}
	}
}
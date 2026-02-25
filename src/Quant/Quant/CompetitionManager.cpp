#include "CompetitionManager.h"
#include "StrategyTask.h"
#include "QuantStrategyManager.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include "CompetitionTask.h"
#include "Cini/CiniAPI.h"
#include "Util.h"

CompetitionManager::CompetitionManager() :
m_competitionThreadId(0),
m_isShowResult(false),
m_isShowTradeLog(false),
m_spCompetitionTask(nullptr),
m_isInit(false)
{

}

CompetitionManager& CompetitionManager::instance()
{
	static CompetitionManager s_competitionManager;
	return s_competitionManager;
}

CompetitionManager::~CompetitionManager()
{
	uninit();
}

void CompetitionManager::init()
{
	if (m_isInit)
	{
		return;
	}
	m_competitionThreadId = CTaskThreadManager::Instance().Init();
}

void CompetitionManager::uninit()
{
	if (m_competitionThreadId != 0)
	{
		stopCompetition();
		CTaskThreadManager::Instance().Uninit(m_competitionThreadId);
		m_competitionThreadId = 0;
		RCSend("CompetitionManager uninitialized");
	}
}

void CompetitionManager::addCompetition(StrategyMode strategyMode, const CompetitionConfig& config)
{
	m_competitionConfigMap[(int32_t)strategyMode] = config;
}

void CompetitionManager::setParam(bool isShowResult, bool isShowTradeLog)
{
	m_isShowResult = isShowResult;
	m_isShowTradeLog = isShowTradeLog;
}

bool CompetitionManager::startCompetition()
{
	if (m_competitionThreadId == 0)
	{
		m_competitionThreadId = CTaskThreadManager::Instance().Init();
	}
	int32_t allStrategyCount = 0;
	m_spCompetitionTask.reset(new CompetitionTask);
	for (auto it = m_competitionConfigMap.begin(); it != m_competitionConfigMap.end(); ++it)
	{
		if (!isCompetitionParamValid((StrategyMode)it->first, it->second))
		{
			RCSend("StrategyMode %d error", it->first);
			return false;
		}
		m_spCompetitionTask->addParam((StrategyMode)it->first, it->second);
		++allStrategyCount;
	}
	m_spCompetitionTask->setParam(m_isShowResult, m_isShowTradeLog);
	CTaskThreadManager::Instance().GetThreadInterface(m_competitionThreadId)->PostTask(m_spCompetitionTask);

	RCSend("Competition started with %d strategies", allStrategyCount);
	return true;
}

void CompetitionManager::stopCompetition()
{
	CTaskThreadManager::Instance().GetThreadInterface(m_competitionThreadId)->StopAllTask();
}

std::shared_ptr<CompetitionTask> CompetitionManager::waitForEnd()
{
	CTaskThreadManager::Instance().GetThreadInterface(m_competitionThreadId)->WaitForEnd();
	m_competitionThreadId = 0;
	return m_spCompetitionTask;
}

bool CompetitionManager::isCompetitionParamValid(StrategyMode strategyMode, const CompetitionConfig& config)
{
	if (config.allParam.empty())
	{
		RCSend("No strategy parameters provided");
		return false;
	}
	if (!config.marketData)
	{
		RCSend("Market data is not provided in config");
		return false;
	}
	if (config.stocks.empty())
	{
		RCSend("No stocks provided in config");
		return false;
	}
	return true;
}
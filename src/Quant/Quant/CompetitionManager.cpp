#include "CompetitionManager.h"
#include "StrategyTask.h"
#include "QuantStrategyManager.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include "CompetitionTask.h"

CompetitionManager::CompetitionManager() :
m_competitionThreadId(0),
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
	stopCompetition();
	CTaskThreadManager::Instance().Uninit(m_competitionThreadId);
	RCSend("CompetitionManager uninitialized");
}

void CompetitionManager::addCompetition(StrategyMode strategyMode, const CompetitionConfig& config)
{
	m_competitionConfigMap[(int32_t)strategyMode] = config;
}

bool CompetitionManager::startCompetition()
{
	int32_t allStrategyCount = 0;
	std::shared_ptr<CompetitionTask> spCompetitionTask(new CompetitionTask);
	for (auto it = m_competitionConfigMap.begin(); it != m_competitionConfigMap.end(); ++it)
	{
		if (!isCompetitionParamValid((StrategyMode)it->first, it->second))
		{
			RCSend("StrategyMode %d error", it->first);
			return false;
		}
		spCompetitionTask->addParam((StrategyMode)it->first, it->second);
		++allStrategyCount;
	}
	CTaskThreadManager::Instance().GetThreadInterface(m_competitionThreadId)->PostTask(spCompetitionTask);
	

	RCSend("Competition started with %d strategies", allStrategyCount);
	return true;
}

void CompetitionManager::stopCompetition()
{
	CTaskThreadManager::Instance().GetThreadInterface(m_competitionThreadId)->StopAllTask();
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
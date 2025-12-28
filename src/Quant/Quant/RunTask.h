#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CompetitionManager.h"

class Quant;

class RunTask : public CTask
{
public:
	RunTask();

public:
	void DoTask();

	void setParam(Quant* client, uint32_t beginTime, uint32_t endTime, uint32_t historyTimes, uint32_t futureTimes,
		StrategyMode mode, const CompetitionConfig& config);

private:
	Quant* m_client;
	uint32_t m_beginTime;
	uint32_t m_endTime;
	uint32_t m_historyTimes;
	uint32_t m_futureTimes;
	StrategyMode m_mode;
	CompetitionConfig m_config;
};
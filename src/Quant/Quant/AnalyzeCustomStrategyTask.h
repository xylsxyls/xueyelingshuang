#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <string>

class AnalyzeCustomStrategyTask : public CTask
{
public:
	AnalyzeCustomStrategyTask();

public:
	void DoTask();

	void StopTask();

	void setParam(const std::string& strategyName);

private:
	std::atomic<bool> m_exit;
	std::string m_strategyName;
};

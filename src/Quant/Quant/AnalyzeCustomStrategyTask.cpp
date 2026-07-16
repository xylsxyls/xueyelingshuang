#include "AnalyzeCustomStrategyTask.h"
#include "CustomStrategyManager.h"

AnalyzeCustomStrategyTask::AnalyzeCustomStrategyTask():
m_exit(false)
{

}

void AnalyzeCustomStrategyTask::DoTask()
{
	if (m_exit || m_strategyName.empty())
	{
		return;
	}
	CustomStrategyManager::instance().saveStrategyStocksToRedis(m_strategyName);
}

void AnalyzeCustomStrategyTask::StopTask()
{
	m_exit = true;
}

void AnalyzeCustomStrategyTask::setParam(const std::string& strategyName)
{
	m_strategyName = strategyName;
}

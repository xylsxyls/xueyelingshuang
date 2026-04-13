#include "RunTask.h"
#include "ScoreManager.h"
#include "StrategyTask.h"
#include "QuantStrategyManager.h"
#include "LockFreeQueue/LockFreeQueue.inl"
#include "Semaphore/SemaphoreAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Util.h"

RunTask::RunTask()
{

}

void RunTask::DoTask()
{
	// 创建市场数据
	auto marketData = std::make_shared<Market>();
	marketData->init(g_config.m_allBeginTime, g_config.m_allEndTime);
	for (size_t index = 0; index < m_stocks.size(); ++index)
	{
		marketData->addStock(m_stocks[index]);
	}

	// 设置账户
	std::shared_ptr<Fund> spFund(new Fund);
	spFund->init(m_initialFund);
	spFund->setMarket(marketData);

	ImportParam import;
	LockFreeQueue<std::shared_ptr<StrategyResult>> resultQueue;

	uint32_t currentTime = m_beginTime;

	while (true)
	{
		std::pair<StrategyMode, std::vector<int32_t>> best = ScoreManager::instance().bestStrategy(currentTime);

		uint32_t endTime = Util::calcFutureTime(currentTime, g_config.m_runDayCount / 2);
		if (endTime > m_endTime)
		{
			endTime = m_endTime;
		}

		if (!best.second.empty())
		{
			// 创建策略实例
			std::shared_ptr<Strategy> spStrategy = QuantStrategyManager::instance().createStrategy(best.first);
			if (spStrategy == nullptr)
			{
				RCSend("Failed to create strategy for parameters");
				return;
			}

			RCSend("%u-%u best use %s", currentTime, endTime, spStrategy->describeParam(best.second).c_str());

			uint32_t threadId = CTaskThreadManager::Instance().Init();

			// 设置导入参数
			spStrategy->setImportParam(import);

			// 设置市场数据
			spStrategy->setMarket(marketData);

			// 添加股票
			for (const auto& stock : m_stocks)
			{
				spStrategy->addStock(stock);
			}

			// 设置账户
			spStrategy->setFund(spFund);

			// 设置策略参数
			spStrategy->setStrategyParam(best.second);

			// 填入参数并检查是否合理
			if (!spStrategy->fillCheckParam())
			{
				continue;
			}

			Semaphore resultSemaphore;

			std::shared_ptr<StrategyTask> spStrategyTask(new StrategyTask);
			spStrategyTask->setParam(currentTime, endTime, m_stocks, spStrategy, spFund, endTime == m_endTime,
				&resultQueue, &resultSemaphore);

			CTaskThreadManager::Instance().GetThreadInterface(threadId)->PostTask(spStrategyTask);

			CTaskThreadManager::Instance().WaitForEnd(threadId);

			import = spStrategy->getImportParam();
		}
		else
		{
			const std::vector<int32_t>& dayInfo = marketData->getStockData(m_stocks[0], currentTime);
			spFund->buyAll(m_stocks[0], dayInfo[(int32_t)Overall::CLOSE], currentTime, ObserveTime::COUNT);
		}

		if (endTime == m_endTime)
		{
			break;
		}
		currentTime = endTime;
	}

	std::shared_ptr<StrategyResult> spStrategyResult;
	while (!resultQueue.empty())
	{
		resultQueue.pop(&spStrategyResult);
	}
	for (size_t lineIndex = 0; lineIndex < spStrategyResult->m_tradeLog.size(); ++lineIndex)
	{
		RCSend("%s", spStrategyResult->m_tradeLog[lineIndex].c_str());
	}

	RCSend("all = %s", (BigNumber(spFund->getAvailableFund()) / 100).toPrec(2).toString().c_str());
}

void RunTask::setParam(uint32_t beginTime, uint32_t endTime, std::vector<std::string> stocks, int32_t initialFund)
{
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_stocks = stocks;
	m_initialFund = initialFund;
}
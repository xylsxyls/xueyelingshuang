#include <stdio.h>
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockMysql/StockMysqlAPI.h"
#include "CSystem/CSystemAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "CStopWatch/CStopWatchAPI.h"

class LoadTask : public CTask
{
public:
	LoadTask()
	{

	}

	void DoTask()
	{
		m_spStockMarket->load();
		m_spWr->load();
		//auto sss = m_spWr->m_indicator;
		m_spRsi->load();
		//auto sss2 = m_spRsi->m_indicator;
	}

	void setParam(const std::shared_ptr<StockMarket>& spStockMarket,
		const std::shared_ptr<StockWrIndicator>& spWr,
		const std::shared_ptr<StockRsiIndicator>& spRsi)
	{
		m_spStockMarket = spStockMarket;
		m_spWr = spWr;
		m_spRsi = spRsi;
	}

	std::shared_ptr<StockMarket> m_spStockMarket;
	std::shared_ptr<StockWrIndicator> m_spWr;
	std::shared_ptr<StockRsiIndicator> m_spRsi;
};

int main()
{
	IntDateTime beginTime = "2014-01-01";
	IntDateTime endTime = "2019-01-01";
	CStopWatch watch;
	int32_t begin = ::GetTickCount();
	int32_t coreCount = CSystem::GetCPUCoreCount() * 2;

	std::vector<int32_t> vecThreadId;
	int32_t index = -1;
	while (index++ != coreCount - 1)
	{
		vecThreadId.push_back(CTaskThreadManager::Instance().Init());
	}

	auto allStock = StockMysql::instance().allStock();

	index = -1;
	while (index++ != allStock.size() - 1)
	{
		const std::string& stock = allStock[index];
		
		StockIndicator::instance().loadFromRedis(stock, beginTime, endTime);
		
		auto spWr = StockIndicator::instance().wr();
		auto spRsi = StockIndicator::instance().rsi();

		std::shared_ptr<LoadTask> spLoadTask(new LoadTask);
		std::shared_ptr<StockMarket> spStockMarket(new StockMarket);
		//watch.Run();
		spStockMarket->loadFromDb(stock, beginTime, endTime);
		//watch.Stop();
		spLoadTask->setParam(spStockMarket, spWr, spRsi);
		CTaskThreadManager::Instance().GetThreadInterface(vecThreadId[index % coreCount])->PostTask(spLoadTask);
	}
	
	index = -1;
	while (index++ != coreCount - 1)
	{
		CTaskThreadManager::Instance().WaitForEnd(vecThreadId[index]);
	}
	RCSend("time = %d, watch = %d", ::GetTickCount() - begin, watch.GetWatchTime());
	return 0;
}
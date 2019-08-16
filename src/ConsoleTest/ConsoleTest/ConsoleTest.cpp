#include <stdio.h>
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockMysql/StockMysqlAPI.h"
#include "CSystem/CSystemAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"

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
		m_spRsi->load();
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
	int32_t begin = ::GetTickCount();
	int32_t coreCount = CSystem::GetCPUCoreCount();

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
		StockIndicator::instance().loadFromRedis(stock);
		auto spWr = StockIndicator::instance().wr(stock);
		auto spRsi = StockIndicator::instance().rsi(stock);

		std::shared_ptr<LoadTask> spLoadTask(new LoadTask);
		std::shared_ptr<StockMarket> spStockMarket(new StockMarket);
		spStockMarket->loadFromDb(stock);
		spLoadTask->setParam(spStockMarket, spWr, spRsi);
		CTaskThreadManager::Instance().GetThreadInterface(vecThreadId[index % coreCount])->PostTask(spLoadTask);
	}
	
	index = -1;
	while (index++ != coreCount - 1)
	{
		CTaskThreadManager::Instance().WaitForEnd(vecThreadId[index]);
	}
	RCSend("time = %d", ::GetTickCount() - begin);
	return 0;
}
#include "SyntheticalTestTask.h"
#include "StockRetest/StockRetestAPI.h"
#include "StockStorage/StockStorageAPI.h"
#include "CStringManager/CStringManagerAPI.h"

SyntheticalTestTask::SyntheticalTestTask() :
m_beginTime(0, 0),
m_endTime(0, 0),
m_stockClient(nullptr)
{

}

void SyntheticalTestTask::DoTask()
{
	StockStorage::instance().init(m_beginTime, m_endTime, 1, m_allStock);
	StockStorage::instance().loadMarket();
	StockStorage::instance().load();
	StockMarket runMarket = *StockStorage::instance().runMarket();

	BigNumber profit = 1;
	BigNumber profitValue = m_beginValue;
	runMarket.setNewDate(m_beginTime);
	runMarket.previous();
	while (runMarket.next())
	{
		IntDateTime currentTime = runMarket.date();
		BigNumber num = 0;
		BigNumber allChg = 0;
		int32_t index = -1;
		while (index++ != m_allStock.size() - 1)
		{
			const std::string& stock = m_allStock[index];
			if (CStringManager::Left(stock, 3) == "688")
			{
				continue;
			}
			std::shared_ptr<StockMarket> spMarket = StockStorage::instance().market(stock);
			if (spMarket == nullptr)
			{
				continue;
			}
			if (!spMarket->setDate(currentTime))
			{
				continue;
			}
			BigNumber close = spMarket->day()->close();
			//if (close > 10)
			//{
			//	continue;
			//}
			allChg = allChg + (close / spMarket->day()->preClose().toPrec(16));
			++num;
		}
		BigNumber currentChg = allChg / num.toPrec(16);
		profit = profit * currentChg;
		profitValue = profitValue * currentChg;
		RCSend("date = %s, chg = %s%%, profit = %s%%, profitValue = %s",
			currentTime.dateToString().c_str(),
			((currentChg - 1) * 100).toPrec(2).toString().c_str(),
			((profit - 1) * 100).toPrec(2).toString().c_str(),
			profitValue.toPrec(2).toString().c_str());
	}
	StockStorage::instance().clear();
}

void SyntheticalTestTask::setParam(const BigNumber& beginValue,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	const std::vector<std::string>& allStock,
	StockClient* stockClient)
{
	m_beginValue = beginValue;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_allStock = allStock;
	m_stockClient = stockClient;
}
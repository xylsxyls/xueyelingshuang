#include "TodayMarketFileToMemoryTask.h"
#include "StockClientLogicManager.h"
#include "CSystem/CSystemAPI.h"
#include "CMouse/CMouseAPI.h"
#include "StockClient.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "StockMysql/StockMysqlAPI.h"

TodayMarketFileToMemoryTask::TodayMarketFileToMemoryTask() :
m_stockClient(nullptr)
{

}

bool sortFun(const std::vector<std::string>& x, const std::vector<std::string>& y)
{
	return x[0] < y[0];
}

void TodayMarketFileToMemoryTask::DoTask()
{
	m_stockClient->m_todayMarket.clear();

	std::string allStockPath = CSystem::GetCurrentExePath() + "all_stock.txt";
	Ctxt txt(allStockPath);
	txt.LoadTxt(Ctxt::SPLIT, "\t");
	int32_t index = 0;
	while (index++ != txt.m_vectxt.size() - 2)
	{
		std::vector<std::string>& vecLine = txt.m_vectxt[index];
		m_stockClient->m_todayMarket.push_back(std::vector<std::string>());
		std::string stock = vecLine[0];
		stock = CStringManager::Mid(stock, 2, 6);
		m_stockClient->m_todayMarket.back().push_back(stock);
		std::string& name = vecLine[1];
		m_stockClient->m_todayMarket.back().push_back(name);
		m_stockClient->m_todayMarket.back().push_back((CStringManager::Replace(name, "ST", "ST") != 0) ? "1" : "0");
		const std::string& stockClose = vecLine[vecLine.size() == 6 ? 3 : 2];
		m_stockClient->m_todayMarket.back().push_back(stockClose);
	}
	std::sort(m_stockClient->m_todayMarket.begin(), m_stockClient->m_todayMarket.end(), sortFun);
}

void TodayMarketFileToMemoryTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}
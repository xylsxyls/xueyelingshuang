#include "CheckAllStockTask.h"
#include "Ctxt/CtxtAPI.h"
#include "CSystem/CSystemAPI.h"
#include "StockClient.h"
#include "CStringManager/CStringManagerAPI.h"
#include "StockClientLogicManager.h"
#include "SaveGroupMarketTask.h"
#include "Win7SaveGroupMarketTask.h"

CheckAllStockTask::CheckAllStockTask()
{

}

void CheckAllStockTask::DoTask()
{
	std::string marketFolder = CSystem::GetCurrentExePath() + "CurrentAllMarketFile\\";
	int32_t index = -1;
	while (index++ != m_stockClient->m_todayMarket.size() - 1)
	{
		RCSend("check = %d", index + 1);
		const std::vector<std::string>& vecLine = m_stockClient->m_todayMarket[index];
		const std::string& stock = vecLine[0];
		const std::string& stockClose = vecLine[6];
		std::string marketPath = marketFolder + stock + ".txt";
		if (!CSystem::DirOrFileExist(marketPath))
		{
			m_vecErrorStock.push_back(stock);
			continue;
		}
		Ctxt marketTxt(marketPath);
		marketTxt.LoadTxt(Ctxt::SPLIT, "\t");
		if (marketTxt.m_vectxt.size() <= 5)
		{
			m_vecErrorStock.push_back(stock);
			continue;
		}
		const std::string& marketClose = marketTxt.m_vectxt[marketTxt.m_vectxt.size() - 2][7];
		if (stockClose != marketClose)
		{
			m_vecErrorStock.push_back(stock);
			continue;
		}
	}

	std::string allErrorStock;
	index = -1;
	while (index++ != m_vecErrorStock.size() - 2)
	{
		const std::string& stock = m_vecErrorStock[index];
		allErrorStock.append(stock + ",");
	}
	allErrorStock.append(m_vecErrorStock[index]);
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("´íÎóµÄgupiao´úÂë£º%1").arg(allErrorStock.c_str()));

	if (CSystem::GetSystemVersionNum() < 655360)
	{
		std::shared_ptr<Win7SaveGroupMarketTask> spWin7SaveAllMarketTask(new Win7SaveGroupMarketTask);
		spWin7SaveAllMarketTask->setParam(0, 0, m_stockClient, m_vecErrorStock);
		CTaskThreadManager::Instance().GetThreadInterface(m_stockClient->m_sendTaskThreadId)->PostTask(spWin7SaveAllMarketTask);
		return;
	}
	std::shared_ptr<SaveGroupMarketTask> spSaveAllMarketTask(new SaveGroupMarketTask);
	spSaveAllMarketTask->setParam(0, 0, m_stockClient, m_vecErrorStock);
	CTaskThreadManager::Instance().GetThreadInterface(m_stockClient->m_sendTaskThreadId)->PostTask(spSaveAllMarketTask);
}

void CheckAllStockTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}
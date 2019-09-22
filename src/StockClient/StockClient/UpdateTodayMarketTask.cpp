#include "UpdateTodayMarketTask.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockClient.h"
#include "StockClientLogicManager.h"
#include "CSystem/CSystemAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "SaveGroupMarketTask.h"

UpdateTodayMarketTask::UpdateTodayMarketTask():
m_stockClient(nullptr)
{

}

void UpdateTodayMarketTask::DoTask()
{
	if (!m_stockClient->m_hasRisestFile)
	{
		std::shared_ptr<SaveGroupMarketTask> spSaveGroupMarketTask(new SaveGroupMarketTask);
		std::vector<std::string> groupStock;
		groupStock.push_back(m_stockClient->m_risestStock);
		spSaveGroupMarketTask->setParam(0, 1, m_stockClient, groupStock);
		m_stockClient->m_hasRisestFile = true;
		CTaskThreadManager::Instance().GetThreadInterface(m_stockClient->m_sendTaskThreadId)->PostTask(spSaveGroupMarketTask, 2);
		return;
	}

	std::string risestStockMarketFile = CSystem::GetCurrentExePath() + "CurrentAllMarketFile\\" + m_stockClient->m_risestStock + ".txt";
	Ctxt risestFile(risestStockMarketFile);
	risestFile.LoadTxt(Ctxt::SPLIT, "\t");
	m_stockClient->m_today = CStringManager::split(risestFile.m_vectxt[risestFile.m_vectxt.size() - 2][0], ",")[0];
	std::vector<std::string> todayMarket;
	todayMarket.push_back("");
	todayMarket.push_back("");
	todayMarket.push_back("");
	todayMarket.push_back("");
	int32_t index = -1;
	while (index++ != m_stockClient->m_todayMarket.size() - 1)
	{
		RCSend("update = %d", index + 1);
		const std::string& stock = m_stockClient->m_todayMarket[index][0];
		const std::string& open = m_stockClient->m_todayMarket[index][3];
		const std::string& high = m_stockClient->m_todayMarket[index][4];
		const std::string& low = m_stockClient->m_todayMarket[index][5];
		const std::string& close = m_stockClient->m_todayMarket[index][6];
		if (open == "0.00" && high == "0.00" && low == "0.00" && close == "0.00")
		{
			continue;
		}
		todayMarket[0] = open;
		todayMarket[1] = high;
		todayMarket[2] = low;
		todayMarket[3] = close;
		StockMarketHelper::updateDateMarketToMysql(stock, m_stockClient->m_today, todayMarket);
	}
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("hangqing更新完成"));
}

void UpdateTodayMarketTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}

bool UpdateTodayMarketTask::ReExecute()
{
	return true;
}

CTask* UpdateTodayMarketTask::Clone()
{
	return new UpdateTodayMarketTask(*this);
}
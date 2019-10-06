#include "UpdateTodayToMemoryTask.h"
#include "CSystem/CSystemAPI.h"
#include "StockClient.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"

UpdateTodayToMemoryTask::UpdateTodayToMemoryTask():
m_stockClient(nullptr)
{

}

void UpdateTodayToMemoryTask::DoTask()
{
	std::string risestStockMarketFile = CSystem::GetCurrentExePath() + "CurrentAllMarketFile\\" + m_stockClient->m_risestStock + ".txt";
	Ctxt risestFile(risestStockMarketFile);
	risestFile.LoadTxt(Ctxt::SPLIT, "\t");
	m_stockClient->m_today = CStringManager::split(risestFile.m_vectxt[risestFile.m_vectxt.size() - 2][0], ",")[0];
}

void UpdateTodayToMemoryTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}
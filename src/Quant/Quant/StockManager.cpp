#include "StockManager.h"
#include "Cini/CiniAPI.h"
#include "Config.h"
#include "Util.h"

StockManager::StockManager()
{

}

StockManager& StockManager::instance()
{
	static StockManager s_stockManager;
	return s_stockManager;
}

void StockManager::init()
{
	getAllTradingDays();
}

std::vector<int32_t> StockManager::getAllTradingDays()
{
	if (!m_tradingDays.empty())
	{
		return m_tradingDays;
	}
	Cini ini(g_config.m_currentExePath + g_config.m_stock + ".ini", true);
	std::vector<std::string> allSection = ini.getAllSection();
	m_tradingDays = Util::groupToInt(allSection, 4);
	return m_tradingDays;
}
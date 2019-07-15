#include "StockMarketHelper.h"
#include "StockMysql/StockMysqlAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"

void StockMarketHelper::save(const std::string& stock, const std::string& file)
{
	std::vector<std::vector<std::string>> vecMarket;
	Ctxt txt(file);
	txt.LoadTxt(Ctxt::SPLIT, "\t\t");
	int32_t lineIndex = 0;
	while (lineIndex++ != txt.m_vectxt.size() - 1)
	{
		std::vector<std::string> vec = CStringManager::split(txt.m_vectxt[lineIndex][0], ",");
		if (vec[0].empty())
		{
			break;
		}
		std::string& date = vec[0];
		std::vector<std::string> price = CStringManager::split(vec[1], "\t");
		std::string& open = price[1];
		std::string& high = txt.m_vectxt[lineIndex][1];
		std::string& low = txt.m_vectxt[lineIndex][2];
		std::string& close = txt.m_vectxt[lineIndex][3];
		vecMarket.push_back(std::vector<std::string>());
		vecMarket.back().push_back(date);
		vecMarket.back().push_back(open);
		vecMarket.back().push_back(high);
		vecMarket.back().push_back(low);
		vecMarket.back().push_back(close);
	}

	std::shared_ptr<StockMysql> spStockMysql = StockMysql::newCase();

	spStockMysql->saveMarket(stock, vecMarket);
	spStockMysql->addStock(stock);
}
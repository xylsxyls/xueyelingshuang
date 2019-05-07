#include "StockAllMarket.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "StockMarket.h"
#include "CGetPath/CGetPathAPI.h"

StockAllMarket::StockAllMarket()
{
	init();
}

StockAllMarket& StockAllMarket::instance()
{
	static StockAllMarket s_stockAllMarket;
	return s_stockAllMarket;
}

void StockAllMarket::init()
{
	m_mysql.connect("127.0.0.1", 3306, "root", "");
}

void StockAllMarket::load()
{
	m_allHistory.clear();
	m_mysql.selectDb("stockmarket");
	std::vector<std::vector<std::string>> vecStock = m_mysql.execute(m_mysql.PreparedStatementCreator("select table_name from information_schema.tables where table_schema='stockmarket'"))->toVector();
	int32_t index = -1;
	while (index++ != vecStock.size() - 1)
	{
		std::string& tableName = vecStock[index][0];
		if (tableName.size() != 6)
		{
			continue;
		}
		const std::string& stock = tableName;
		//std::string name = m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString("stock", "name", "stock='" + stock + "'")))->toVector()[0][0];
		std::vector<std::vector<std::string>> vecMarket = m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString(stock)))->toVector();
		std::map<IntDateTime, std::vector<BigNumber>> history;
		int32_t index = -1;
		while (index++ != vecMarket.size() - 1)
		{
			std::vector<BigNumber> data;
			data.push_back(BigNumber(vecMarket[index][StockMarket::OPEN + 1].c_str()));
			data.push_back(BigNumber(vecMarket[index][StockMarket::HIGH + 1].c_str()));
			data.push_back(BigNumber(vecMarket[index][StockMarket::LOW + 1].c_str()));
			data.push_back(BigNumber(vecMarket[index][StockMarket::CLOSE + 1].c_str()));
			history[IntDateTime(vecMarket[index][0])] = data;
		}
		std::shared_ptr<StockMarket> spStockMarket;
		spStockMarket.reset(new StockMarket);
		spStockMarket->load(stock, "", history);//name
		m_allHistory[stock] = spStockMarket;
	}
}

void StockAllMarket::save(const std::string& folder)
{
	m_mysql.selectDb("stockmarket");

	std::vector<std::string> vecTxt = CGetPath::FindFilePath("txt", folder, 2);

	int32_t index = -1;
	while (index++ != vecTxt.size() - 1)
	{
		std::string& file = vecTxt[index];
		std::string stock = CGetPath::GetName(file, 3);

		std::vector<std::string> vecFields;
		vecFields.push_back("date varchar(10) primary key");
		vecFields.push_back("open varchar(7)");
		vecFields.push_back("high varchar(7)");
		vecFields.push_back("low varchar(7)");
		vecFields.push_back("close varchar(7)");
		m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::createTableString(stock, vecFields)));
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
			IntDateTime date = vec[0];
			std::vector<std::string> price = CStringManager::split(vec[1], "\t");
			std::string& open = price[1];
			std::string& high = txt.m_vectxt[lineIndex][1];
			std::string& low = txt.m_vectxt[lineIndex][2];
			std::string& close = txt.m_vectxt[lineIndex][3];
			auto prepare = m_mysql.PreparedStatementCreator(SqlString::insertString(stock, "date,open,high,low,close"));
			prepare->setString(0, date.dateToString());
			prepare->setString(1, open);
			prepare->setString(2, high);
			prepare->setString(3, low);
			prepare->setString(4, close);
			m_mysql.execute(prepare);
		}
	}
}

std::map<std::string, std::shared_ptr<StockMarket>> StockAllMarket::allHistory()
{
	return m_allHistory;
}

std::vector<std::string> StockAllMarket::allStock()
{
	std::vector<std::string> vecStock;
	for (auto itHistory = m_allHistory.begin(); itHistory != m_allHistory.end(); ++itHistory)
	{
		vecStock.push_back(itHistory->first);
	}
	return vecStock;
}

std::shared_ptr<StockMarket> StockAllMarket::history(const std::string& stock)
{
	auto itStockMarket = m_allHistory.find(stock);
	if (itStockMarket == m_allHistory.end())
	{
		return nullptr;
	}
	return itStockMarket->second;
}
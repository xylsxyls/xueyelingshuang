#include "StockAllIndex.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "StockIndex.h"
#include "StockMarket/StockMarketAPI.h"

StockAllIndex::StockAllIndex()
{
	init();
}

StockAllIndex& StockAllIndex::instance()
{
	static StockAllIndex s_stockAllIndex;
	return s_stockAllIndex;
}

void StockAllIndex::init()
{
	m_mysql.connect("127.0.0.1", 3306, "root", "");
}

std::shared_ptr<StockIndex> StockAllIndex::index(const std::string& stock) const
{
	auto itStockIndex = m_stockAllIndex.find(stock);
	if (itStockIndex == m_stockAllIndex.end())
	{
		return nullptr;
	}
	return itStockIndex->second;
}

std::map<std::string, std::shared_ptr<StockIndex>> StockAllIndex::allIndex() const
{
	return m_stockAllIndex;
}

void StockAllIndex::save() const
{
	StockAllMarket::instance().load();
	std::map<std::string, std::shared_ptr<StockMarket>> allHistory = StockAllMarket::instance().allHistory();

	m_mysql.selectDb("stockindex");

	for (auto itHistory = allHistory.begin(); itHistory != allHistory.end(); ++itHistory)
	{
		const std::string& stock = itHistory->first;
		const std::shared_ptr<StockMarket>& spStockMarket = itHistory->second;

		std::vector<std::string> vecFields;
		vecFields.push_back("date varchar(10) primary key");
		vecFields.push_back("rsi6 varchar(6)");
		vecFields.push_back("rsi12 varchar(6)");
		vecFields.push_back("rsi24 varchar(6)");
		vecFields.push_back("wr10 varchar(6)");
		vecFields.push_back("wr20 varchar(6)");
		m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::createTableIfNotExistString(stock, vecFields)));

		auto history = spStockMarket->history();
		for (auto itStockMarket = history.begin(); itStockMarket != history.end(); ++itStockMarket)
		{
			auto prepare = m_mysql.PreparedStatementCreator(SqlString::insertString(spStockMarket->stock(), "date,rsi6,rsi12,rsi24,wr10,wr20"));
			prepare->setString(0, itStockMarket->first.dateToString());
			prepare->setString(1, StockIndex::rsi(6, itStockMarket->first, spStockMarket).toString());
			prepare->setString(2, StockIndex::rsi(12, itStockMarket->first, spStockMarket).toString());
			prepare->setString(3, StockIndex::rsi(24, itStockMarket->first, spStockMarket).toString());
			prepare->setString(4, StockIndex::wr(10, itStockMarket->first, spStockMarket).toString());
			prepare->setString(5, StockIndex::wr(20, itStockMarket->first, spStockMarket).toString());
			m_mysql.execute(prepare);
		}
	}
}

void StockAllIndex::load()
{
	m_stockAllIndex.clear();
	m_mysql.selectDb("stockindex");
	std::vector<std::vector<std::string>> vecStock = m_mysql.execute(m_mysql.PreparedStatementCreator("select table_name from information_schema.tables where table_schema='stockmarket'"))->toVector();
	int32_t index = -1;
	while (index++ != vecStock.size() - 1)
	{
		const std::string& tableName = vecStock[index][0];
		if (tableName.size() != 6)
		{
			continue;
		}
		const std::string& stock = tableName;
		std::vector<std::vector<std::string>> vecData = m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString(stock, "date,rsi6,rsi12,rsi24,wr10,wr20")))->toVector();
		std::map<IntDateTime, std::vector<BigNumber>> data;
		int32_t dateIndex = -1;
		while (dateIndex++ != vecData.size() - 1)
		{
			int32_t stockIndex = STOCK_RSI6;
			while (stockIndex++ != STOCK_WR20 + 1)
			{
				data[vecData[dateIndex][0]].push_back(vecData[dateIndex][stockIndex].c_str());
			}
		}
		std::shared_ptr<StockIndex> spStockIndex;
		spStockIndex.reset(new StockIndex);
		spStockIndex->load(data);
		m_stockAllIndex[stock] = spStockIndex;
	}
}
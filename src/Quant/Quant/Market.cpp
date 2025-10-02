#include "Market.h"
#include "Util.h"
#include <algorithm>

Market::Market()
	: m_beginTime(0)
	, m_endTime(0)
{
}

void Market::init(uint32_t beginTime, uint32_t endTime)
{
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_stockData.clear();
	m_dateIndex.clear();
}

void Market::addStock(const std::string& stock)
{
	if (m_stockData.find(stock) != m_stockData.end())
	{
		// 股票已存在，不再重复添加
		return;
	}

	// 从Util获取股票数据
	auto data = Util::getAllStockData(stock, m_beginTime, m_endTime);
	m_stockData[stock] = data;

	// 建立日期索引
	auto& dateIndex = m_dateIndex[stock];
	for (size_t i = 0; i < data.size(); ++i)
	{
		if (!data[i].empty())
		{
			// 第一个元素是日期
			uint32_t date = static_cast<uint32_t>(data[i][0]);
			dateIndex[date] = i;
		}
	}
}

const std::vector<int32_t>& Market::getStockData(const std::string& stock, uint32_t date) const
{
	static std::vector<int32_t> emptyVector;

	auto stockIt = m_stockData.find(stock);
	if (stockIt == m_stockData.end())
	{
		return emptyVector;
	}

	auto dateIt = m_dateIndex.find(stock);
	if (dateIt == m_dateIndex.end())
	{
		return emptyVector;
	}

	auto indexIt = dateIt->second.find(date);
	if (indexIt == dateIt->second.end())
	{
		return emptyVector;
	}

	if (indexIt->second < stockIt->second.size())
	{
		return stockIt->second[indexIt->second];
	}

	return emptyVector;
}

const std::vector<std::vector<int32_t>>& Market::getAllStockData(const std::string& stock) const
{
	static std::vector<std::vector<int32_t>> emptyVector;

	auto it = m_stockData.find(stock);
	if (it != m_stockData.end())
	{
		return it->second;
	}
	return emptyVector;
}

uint32_t Market::getBeginTime() const
{
	return m_beginTime;
}

uint32_t Market::getEndTime() const
{
	return m_endTime;
}

bool Market::hasStock(const std::string& stock) const
{
	return m_stockData.find(stock) != m_stockData.end();
}
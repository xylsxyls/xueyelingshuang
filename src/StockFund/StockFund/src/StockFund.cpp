#include "StockFund.h"
#include "StockCharge/StockChargeAPI.h"
#include "StockIndex/StockIndexAPI.h"

bool StockFund::buyStock(const IntDateTime& date, const BigNumber& price, const StockMarket& stockMarket, const BigNumber& rate)
{
	if (!stockMarket.dateExist(date))
	{
		return false;
	}
	if (price < stockMarket.low(date) || price > stockMarket.high(date))
	{
		return false;
	}
	if (rate <= 0 || rate > 1)
	{
		return false;
	}

	std::string stock = stockMarket.stock();
	BigNumber position = (m_available * rate / price.toPrec(4) / 100).toPrec(0) * 100;
	while (price * position + StockCharge::instance().buyFee(stock, price, position) > m_available * rate)
	{
		position = position - 100;
	}
	if (position == 0)
	{
		return false;
	}
	BigNumber charge = StockCharge::instance().buyFee(stock, price, position);
	BigNumber consume = charge + price * position;
	m_available = (m_available - consume).toPrec(2);
	m_stock[stock].push_back(std::pair<BigNumber, BigNumber>(price, position));
	std::string log;
	log.append("购买股票：" + stock + " " + stockMarket.name() + "，");
	log.append("日期：" + date.dateToString() + "，");
	log.append("价格：" + price.toPrec(2).toString() + "，");
	log.append("仓位：" + position.toPrec(0).toString() + ", ");
	log.append("手续费：" + charge.toPrec(2).toString() + "，");
	log.append("总消耗：" + consume.toPrec(2).toString());
	m_stockLog.push_back(log);

	if (m_stockMarket.find(stock) == m_stockMarket.end())
	{
		m_stockMarket[stock] = stockMarket;
	}
	return true;
}

bool StockFund::sellStock(const IntDateTime& date, const BigNumber& price, const std::string& stock, const BigNumber& rate)
{
	auto itStockMarket = m_stockMarket.find(stock);
	if (itStockMarket == m_stockMarket.end())
	{
		return false;
	}
	StockMarket& stockMarket = itStockMarket->second;
	if (!stockMarket.dateExist(date))
	{
		return false;
	}
	if (price < stockMarket.low(date) || price > stockMarket.high(date))
	{
		return false;
	}
	if (rate <= 0 || rate > 1)
	{
		return false;
	}

	auto itStock = m_stock.find(stock);
	if (itStock == m_stock.end())
	{
		return false;
	}
	BigNumber position = 0;
	std::vector<std::pair<BigNumber,BigNumber>>& vecDeal = itStock->second;
	int32_t index = -1;
	while (index++ != vecDeal.size() - 1)
	{
		position = position + vecDeal[index].second;
	}
	BigNumber sellPosition = (position * rate / 100).toPrec(0) * 100;
	BigNumber charge = StockCharge::instance().sellFee(stock, price, sellPosition);
	BigNumber back = price * sellPosition - charge;
	m_available = m_available + back;
	std::string log;
	if (position == sellPosition)
	{
		log.append("清仓股票：" + stock + " " + stockMarket.name() + "，");
		BigNumber hasBuy = 0;
		BigNumber hasSell = 0;
		int32_t index = -1;
		while (index++ != vecDeal.size() - 1)
		{
			BigNumber& currentPrice = vecDeal[index].first;
			BigNumber& currentPosition = vecDeal[index].second;
			if (currentPosition > 0)
			{
				hasBuy = hasBuy + currentPrice * currentPosition;
			}
			else
			{
				hasSell = hasSell + currentPrice * currentPosition * -1;
			}
		}
		hasSell = hasSell + price * sellPosition;
		log.append("总收益：" + ((hasSell / hasBuy.toPrec(6) - 1) * 100).toPrec(2).toString() + "%，");
		m_stock.erase(itStock);
		m_stockMarket.erase(itStockMarket);
	}
	else
	{
		vecDeal.push_back(std::pair<BigNumber, BigNumber>(price, sellPosition * -1));
		log.append("卖出股票：" + stock + " " + stockMarket.name() + "，");
	}
	log.append("日期：" + date.dateToString() + "，");
	log.append("价格：" + price.toPrec(2).toString() + "，");
	log.append("仓位：" + sellPosition.toPrec(0).toString() + ", ");
	log.append("手续费：" + charge.toPrec(2).toString() + "，");
	log.append("总收回：" + back.toPrec(2).toString());
	m_stockLog.push_back(log);
	return true;
}

void StockFund::add(const BigNumber& fund)
{
	m_available = (m_available + fund).toPrec(2);
}

void StockFund::freeze(const BigNumber& rate)
{
	BigNumber freeze = (m_available * rate).toPrec(2);
	m_available = m_available - freeze;
	m_freeze = m_freeze + freeze;
	std::string log;
	log.append("冻结资金：" + freeze.toString() + "，");
	log.append("目前可用资金：" + m_available.toString());
	m_stockLog.push_back(log);
}

void StockFund::free(const BigNumber& rate)
{
	BigNumber free = (m_freeze * rate).toPrec(2);
	m_available = m_available + free;
	m_freeze = m_freeze - free;
	std::string log;
	log.append("解冻资金：" + free.toString() + "，");
	log.append("目前可用资金：" + m_available.toString());
	m_stockLog.push_back(log);
}

BigNumber StockFund::allFund(const IntDateTime& date)
{
	BigNumber stockFund = 0;
	for (auto itStock = m_stock.begin(); itStock != m_stock.end(); ++itStock)
	{
		const std::string& stock = itStock->first;
		const std::vector<std::pair<BigNumber, BigNumber>>& vecDeal = itStock->second;

		auto itStockMarket = m_stockMarket.find(stock);
		if (itStockMarket == m_stockMarket.end())
		{
			return -1;
		}
		if (!itStockMarket->second.dateExist(date))
		{
			return -1;
		}

		BigNumber position = 0;
		int32_t index = -1;
		while (index++ != vecDeal.size() - 1)
		{
			position = position + vecDeal[index].second;
		}
		stockFund = stockFund + itStockMarket->second.close(date) * position;
	}
	BigNumber allFund = stockFund + m_available + m_freeze;
	std::string log;
	log.append("目前总资产：" + allFund.toString());
	m_stockLog.push_back(log);
	return allFund;
}

bool StockFund::stockChg(const std::string& stock, const IntDateTime& date, BigNumber& chg)
{
	auto itStock = m_stock.find(stock);
	if (itStock == m_stock.end())
	{
		return false;
	}
	auto itStockMarket = m_stockMarket.find(stock);
	if (itStockMarket == m_stockMarket.end())
	{
		return false;
	}
	if (!itStockMarket->second.dateExist(date))
	{
		return false;
	}

	std::vector<std::pair<BigNumber, BigNumber>>& vecDeal = itStock->second;
	BigNumber hasBuy = 0;
	BigNumber hasSell = 0;
	BigNumber position = 0;
	int32_t index = -1;
	while (index++ != vecDeal.size() - 1)
	{
		BigNumber& currentPrice = vecDeal[index].first;
		BigNumber& currentPosition = vecDeal[index].second;
		position = position + currentPosition;
		if (currentPosition > 0)
		{
			hasBuy = hasBuy + currentPrice * currentPosition;
		}
		else
		{
			hasSell = hasSell + currentPrice * currentPosition * -1;
		}
	}
	hasSell = hasSell + itStockMarket->second.close(date) * position;
	chg = (hasSell / hasBuy * 100 - 100).toPrec(2);
	std::string log;
	log.append("所持股票：" + stock + " " + itStockMarket->second.name() + "，");
	log.append("目前收益率：" + chg.toString() + "%");
	m_stockLog.push_back(log);
	return true;
}

std::vector<std::string> StockFund::stockLog() const
{
	return m_stockLog;
}

BigNumber StockFund::stockNum() const
{
	return (int32_t)m_stock.size();
}

bool StockFund::stockExist(const std::string& stock) const
{
	return m_stock.find(stock) != m_stock.end();
}

//int main()
//{
//	StockMarket market;
//	market.load("600323");
//	StockFund fund;
//	fund.add(100000);
//
//	IntDateTime time = "2017-08-14";
//	do
//	{
//		BigNumber wr10 = StockIndex::wr(10, time, market);
//		BigNumber rsi6 = StockIndex::rsi(6, time, market);
//		BigNumber rsi24 = StockIndex::rsi(24, time, market);
//
//		if (fund.stockNum() == 0 && wr10 >= 95 && rsi24 <= 45)
//		{
//			fund.buyStock(time, market.close(time), market, 0.5);
//		}
//		else if (fund.stockNum() == 1 && wr10 >= 95 && rsi24 <= 45)
//		{
//			fund.buyStock(time, market.close(time), market, 1);
//		}
//		else if (fund.stockNum() == 1 && wr10 < 50)
//		{
//			BigNumber chg = 0;
//			fund.stockChg(market.stock(), time, chg);
//			fund.sellStock(time, market.close(time), market.stock(), 1);
//		}
//
//	} while (market.getDateNext(time, time));
//
//	BigNumber allFund = fund.allFund("2019-04-26");
//	std::vector<std::string> vec = fund.stockLog();
//	getchar();
//	return 0;
//}
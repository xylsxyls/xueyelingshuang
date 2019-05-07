#include "StockFund.h"
#include "StockCharge/StockChargeAPI.h"
#include "StockMarket/StockMarketAPI.h"

bool StockFund::buyStock(const IntDateTime& date, const BigNumber& price, const std::shared_ptr<StockMarket>& spStockMarket, const BigNumber& rate)
{
	if (!spStockMarket->dateExist(date))
	{
		return false;
	}
	if (price < spStockMarket->low(date) || price > spStockMarket->high(date))
	{
		return false;
	}
	if (rate <= 0 || rate > 1)
	{
		return false;
	}

	std::string stock = spStockMarket->stock();
	BigNumber position = (m_available * rate / price.toPrec(4) / 100).toPrec(0) * 100;
	if (position == 0)
	{
		return false;
	}
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
	m_stock[stock].push_back(std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>(date, std::pair<BigNumber, BigNumber>(price, position)));
	std::string log;
	log.append("购买股票：" + stock + " " + spStockMarket->name() + "，");
	log.append("日期：" + date.dateToString() + "，");
	log.append("价格：" + price.toPrec(2).toString() + "，");
	log.append("仓位：" + position.toPrec(0).toString() + ", ");
	log.append("手续费：" + charge.toPrec(2).toString() + "，");
	log.append("总消耗：" + consume.toPrec(2).toString());
	m_stockLog.push_back(log);

	if (m_stockMarket.find(stock) == m_stockMarket.end())
	{
		m_stockMarket[stock] = spStockMarket;
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
	const std::shared_ptr<StockMarket>& spStockMarket = itStockMarket->second;
	if (!spStockMarket->dateExist(date))
	{
		return false;
	}
	if (price < spStockMarket->low(date) || price > spStockMarket->high(date))
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
	std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& vecDeal = itStock->second;
	int32_t index = -1;
	while (index++ != vecDeal.size() - 1)
	{
		position = position + vecDeal[index].second.second;
	}
	BigNumber sellPosition = (position * rate / 100).toPrec(0) * 100;
	BigNumber charge = StockCharge::instance().sellFee(stock, price, sellPosition);
	BigNumber back = price * sellPosition - charge;
	m_available = m_available + back;
	std::string log;
	if (position == sellPosition)
	{
		log.append("清仓股票：" + stock + " " + spStockMarket->name() + "，");
		BigNumber hasBuy = 0;
		BigNumber hasSell = 0;
		int32_t index = -1;
		while (index++ != vecDeal.size() - 1)
		{
			BigNumber& currentPrice = vecDeal[index].second.first;
			BigNumber& currentPosition = vecDeal[index].second.second;
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
		vecDeal.push_back(std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>(date, std::pair<BigNumber, BigNumber>(price, sellPosition * -1)));
		log.append("卖出股票：" + stock + " " + spStockMarket->name() + "，");
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
		const std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& vecDeal = itStock->second;

		auto itStockMarket = m_stockMarket.find(stock);
		if (itStockMarket == m_stockMarket.end())
		{
			return -1;
		}
		if (!itStockMarket->second->dateExist(date))
		{
			return -1;
		}

		BigNumber position = 0;
		int32_t index = -1;
		while (index++ != vecDeal.size() - 1)
		{
			position = position + vecDeal[index].second.second;
		}
		stockFund = stockFund + itStockMarket->second->close(date) * position;
	}
	BigNumber allFund = stockFund + m_available + m_freeze;
	std::string log;
	log.append("目前总资产：" + allFund.toString());
	m_stockLog.push_back(log);
	return allFund;
}

bool StockFund::stockChg(const std::string& stock, const IntDateTime& date, BigNumber& chg) const
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
	if (!itStockMarket->second->dateExist(date))
	{
		return false;
	}

	const std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& vecDeal = itStock->second;
	BigNumber hasBuy = 0;
	BigNumber hasSell = 0;
	BigNumber position = 0;
	int32_t index = -1;
	while (index++ != vecDeal.size() - 1)
	{
		const BigNumber& currentPrice = vecDeal[index].second.first;
		const BigNumber& currentPosition = vecDeal[index].second.second;
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
	hasSell = hasSell + itStockMarket->second->close(date) * position;
	chg = (hasSell / hasBuy * 100 - 100).toPrec(2);
	//std::string log;
	//log.append("所持股票：" + stock + " " + itStockMarket->second.name() + "，");
	//log.append("目前收益率：" + chg.toString() + "%");
	//m_stockLog.push_back(log);
	return true;
}

std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>> StockFund::trade(const std::string& stock) const
{
	auto itStock = m_stock.find(stock);
	if (itStock == m_stock.end())
	{
		return std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>();
	}
	return itStock->second;
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

std::vector<std::string> StockFund::ownedStock() const
{
	std::vector<std::string> ownedStock;
	for (auto itStock = m_stock.begin(); itStock != m_stock.end(); ++itStock)
	{
		ownedStock.push_back(itStock->first);
	}
	return ownedStock;
}

bool StockFund::hasAvailableFund() const
{
	return m_available != 0;
}

bool StockFund::hasFreezeFund() const
{
	return m_freeze != 0;
}

//int main()
//{
//	StockMarket market;
//	std::string stock = "002882";
//
//	MysqlCpp mysql;
//	mysql.connect("127.0.0.1", 3306, "root", "");
//
//	CStopWatch marketWatch;
//	market.load(stock, mysql);
//	RCSend("histroy = %d", market.history().size());
//	int32_t marketWatchTime = marketWatch.GetWatchTime();
//	StockFund fund;
//	fund.add(100000);
//	StockIndex::instance().load(mysql);
//	CStopWatch indexWatch;
//	IntDateTime time = "2017-12-14";
//	do
//	{
//		BigNumber wr10 = StockIndex::instance().wr10(stock, time);
//		BigNumber rsi6 = StockIndex::instance().rsi6(stock, time);
//		BigNumber rsi24 = StockIndex::instance().rsi24(stock, time);
//
//		//BigNumber wr10 = StockIndex::wr(10, time, market);
//		//BigNumber rsi6 = StockIndex::rsi(6, time, market);
//		//BigNumber rsi24 = StockIndex::rsi(24, time, market);
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
//	int32_t indexWatchTime = indexWatch.GetWatchTime();
//	BigNumber allFund = fund.allFund("2019-04-26");
//	RCSend("marketWatchTime = %d, indexWatchTime = %d, allFund = %s", marketWatchTime, indexWatchTime, allFund.toString().c_str());
//	std::vector<std::string> vec = fund.stockLog();
//	getchar();
//	return 0;
//}
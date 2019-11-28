#include "StockFund.h"
#include "StockCharge/StockChargeAPI.h"
#include "StockMarket/StockMarketAPI.h"

bool StockFund::buyStock(const BigNumber& price, const BigNumber& rate, const std::shared_ptr<StockDay>& spStockDay)
{
	if (price < spStockDay->low() || price > spStockDay->high())
	{
		return false;
	}
	if (rate <= 0 || rate > 1)
	{
		return false;
	}

	std::string stock = spStockDay->stock();
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
	m_stock[stock].push_back(std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>(spStockDay->date(), std::pair<BigNumber, BigNumber>(price, position)));
	std::string log;
	log.append("购买gupiao：" + stock + " " + "name" + "，");
	log.append("日期：" + spStockDay->date().dateToString() + "，");
	log.append("jiage：" + price.toPrec(2).toString() + "，");
	log.append("cangwei：" + position.toPrec(0).toString() + ", ");
	log.append("手续费：" + charge.toPrec(2).toString() + "，");
	log.append("总消耗：" + consume.toPrec(2).toString());
	m_stockLog.push_back(log);

	//if (m_stockMarket.find(stock) == m_stockMarket.end())
	//{
	//	m_stockMarket[stock] = spStockMarket;
	//}
	return true;
}

bool StockFund::sellStock(const BigNumber& price, const BigNumber& rate, const std::shared_ptr<StockDay>& spStockDay)
{
	if (price < spStockDay->low() || price > spStockDay->high())
	{
		return false;
	}
	if (rate <= 0 || rate > 1)
	{
		return false;
	}

	std::string stock = spStockDay->stock();
	IntDateTime date = spStockDay->date();
	auto itStock = m_stock.find(stock);
	if (itStock == m_stock.end())
	{
		return false;
	}
	BigNumber position = 0;
	std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& vecDeal = itStock->second;
	std::vector<IntDateTime> vecTimes;
	std::vector<BigNumber> vecData;
	vecData.push_back(BigNumber(atoi(stock.c_str())));
	int32_t index = -1;
	while (index++ != vecDeal.size() - 1)
	{
		position = position + vecDeal[index].second.second;
		vecTimes.push_back(vecDeal[index].first);
	}
	BigNumber sellPosition = (position * rate / 100).toPrec(0) * 100;
	BigNumber charge = StockCharge::instance().sellFee(stock, price, sellPosition);
	BigNumber back = price * sellPosition - charge;
	BigNumber profit = 0;
	m_available = m_available + back;
	std::string log;
	if (position == sellPosition)
	{
		log.append("qingcanggupiao：" + stock + " " + "name" + "，");
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
		profit = ((hasSell / hasBuy.toPrec(6) - 1) * 100).toPrec(2);
		vecData.push_back(profit);
		log.append("总shouyi：" + profit.toString() + "%，");
		m_stock.erase(itStock);
	}
	else
	{
		vecDeal.push_back(std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>(date, std::pair<BigNumber, BigNumber>(price, sellPosition * -1)));
		log.append("卖出gupiao：" + stock + " " + "name" + "，");
	}
	vecTimes.push_back(date);
	//vecData.push_back((back / allFund(spStockDay)).toPrec(2));
	vecData.push_back((date - vecTimes[0]) / 86400);
	m_dataLog.push_back(std::pair<std::vector<BigNumber>, std::vector<IntDateTime>>(vecData, vecTimes));

	log.append("日期：" + date.dateToString() + "，");
	log.append("jiage：" + price.toPrec(2).toString() + "，");
	log.append("cangwei：" + sellPosition.toPrec(0).toString() + ", ");
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
	log.append("冻结zijin：" + freeze.toString() + "，");
	log.append("目前可用zijin：" + m_available.toString());
	m_stockLog.push_back(log);
}

void StockFund::free(const BigNumber& rate)
{
	BigNumber free = (m_freeze * rate).toPrec(2);
	m_available = m_available + free;
	m_freeze = m_freeze - free;
	std::string log;
	log.append("解冻zijin：" + free.toString() + "，");
	log.append("目前可用zijin：" + m_available.toString());
	m_stockLog.push_back(log);
}

BigNumber StockFund::allFund(const std::map<std::string, std::shared_ptr<StockDay>>& allOwnedStockDayData)
{
	//IntDateTime lastTime = date;
	BigNumber stockFund = 0;
	for (auto itStock = m_stock.begin(); itStock != m_stock.end(); ++itStock)
	{
		const std::string& stock = itStock->first;
		const std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& vecDeal = itStock->second;

		//StockMarket lastTimeMarket;
		//lastTimeMarket.setStock(stock);
		//if (!lastTimeMarket.dateExist(date))
		//{
		//	lastTime = lastTimeMarket.getDatePre(date);
		//}
		//if (lastTime.empty())
		//{
		//	continue;
		//}

		BigNumber position = 0;
		int32_t index = -1;
		while (index++ != vecDeal.size() - 1)
		{
			position = position + vecDeal[index].second.second;
		}
		auto itDayDate = allOwnedStockDayData.find(stock);
		if (itDayDate == allOwnedStockDayData.end())
		{
			RCSend("价格不完整");
			return 0;
		}
		stockFund = stockFund + itDayDate->second->close() * position;
	}
	BigNumber allFund = stockFund + m_available + m_freeze;
	std::string log;
	log.append("目前总zichan：" + allFund.toString());
	m_stockLog.push_back(log);
	return allFund;
}

bool StockFund::stockChg(const std::string& stock, const std::shared_ptr<StockDay>& dateDay, BigNumber& chg) const
{
	auto itStock = m_stock.find(stock);
	if (itStock == m_stock.end())
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

	hasSell = hasSell + dateDay->close() * position;
	chg = (hasSell / hasBuy * 100 - 100).toPrec(2);
	//std::string log;
	//log.append("所持gupiao：" + stock + " " + itStockMarket->second.name() + "，");
	//log.append("目前shouyi率：" + chg.toString() + "%");
	//m_stockLog.push_back(log);
	return true;
}

IntDateTime StockFund::firstBuyDate(const std::string& stock)
{
	IntDateTime firstDate(0, 0);
	auto itStock = m_stock.find(stock);
	if (itStock == m_stock.end() || itStock->second.empty())
	{
		return firstDate;
	}
	return itStock->second.front().first;
}

std::vector<IntDateTime> StockFund::ownedTime(const std::string& stock) const
{
	std::vector<IntDateTime> result;
	auto itStock = m_stock.find(stock);
	if (itStock == m_stock.end())
	{
		return result;
	}
	const std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& stockTime = itStock->second;
	int32_t index = -1;
	while (index++ != stockTime.size() - 1)
	{
		result.push_back(stockTime[index].first);
	}
	return result;
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

void StockFund::printStockLog() const
{
	int32_t index = -1;
	while (index++ != m_stockLog.size() - 1)
	{
		RCSend("%s", m_stockLog[index].c_str());
	}
}

std::vector<std::pair<std::vector<BigNumber>, std::vector<IntDateTime>>> StockFund::dataLog() const
{
	return m_dataLog;
}

void StockFund::clear()
{
	m_available = 0;
	m_freeze = 0;
	m_stock.clear();
	m_stockLog.clear();
	m_dataLog.clear();
	m_all = 0;
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

void StockFund::buyInfo(const std::string& stock, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& buyInfo)
{
	buyInfo.clear();
	auto itBuyInfo = m_stock.find(stock);
	if (itBuyInfo == m_stock.end())
	{
		return;
	}
	buyInfo = itBuyInfo->second;
}

std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>>* StockFund::allBuyInfo()
{
	return &m_stock;
}

//#include "StockIndicator/StockIndicatorAPI.h"
//#include "CStopWatch/CStopWatchAPI.h"
//
//int main()
//{
//	std::string stock = "000001";
//	IntDateTime beginTime = "2009-02-27";
//	IntDateTime endTime = "2019-02-27";
//
//	CStopWatch marketWatch;
//	StockMarket market;
//	market.load(stock, beginTime, endTime);
//	int32_t marketWatchTime = marketWatch.GetWatchTime();
//	StockFund fund;
//	fund.add(100000);
//	std::shared_ptr<StockWrIndicator> spStockWrIndicator = StockIndicator::instance().wr(stock, beginTime, endTime);
//	std::shared_ptr<StockRsiIndicator> spStockRsiIndicator = StockIndicator::instance().rsi(stock, beginTime, endTime);
//	CStopWatch indexWatch;
//	do
//	{
//		IntDateTime time = market.date();
//
//		BigNumber wr10 = spStockWrIndicator->day(time)->wr10();
//		BigNumber rsi6 = spStockRsiIndicator->day(time)->rsi6();
//		BigNumber rsi24 = spStockRsiIndicator->day(time)->rsi24();
//
//		//BigNumber wr10 = StockIndex::wr(10, time, market);
//		//BigNumber rsi6 = StockIndex::rsi(6, time, market);
//		//BigNumber rsi24 = StockIndex::rsi(24, time, market);
//
//		if (fund.stockNum() == 0 && wr10 >= 95 && rsi24 <= 45)
//		{
//			fund.buyStock(market.day()->close(), 0.5, market.day());
//		}
//		else if (fund.stockNum() == 1 && wr10 >= 95 && rsi24 <= 45)
//		{
//			fund.buyStock(market.day()->close(), 1, market.day());
//		}
//		else if (fund.stockNum() == 1 && wr10 < 50)
//		{
//			BigNumber chg = 0;
//			fund.stockChg(market.stock(), time, chg);
//			fund.sellStock(market.day()->close(), 1, market.day());
//		}
//	} while (market.next());
//	int32_t indexWatchTime = indexWatch.GetWatchTime();
//	BigNumber allFund = fund.allFund(endTime);
//	RCSend("marketWatchTime = %d, indexWatchTime = %d, allFund = %s", marketWatchTime, indexWatchTime, allFund.toString().c_str());
//	std::vector<std::string> vec = fund.stockLog();
//	getchar();
//	return 0;
//}
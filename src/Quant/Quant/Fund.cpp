#include "Fund.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include "Util.h"

Position::Position() :
m_price(0),
m_shares(0),
m_date(0),
m_time(ObserveTime::COUNT)
{

}

Trade::Trade() :
m_fee(0)
{

}

bool Trade::empty() const
{
	return m_price == 0;
}

Fund::Fund()
{
	m_initialFund = 0;
	m_availableFund = 0;
}

bool Fund::init(int32_t fund)
{
	if (fund <= 0)
	{
		return false;
	}

	m_initialFund = fund;
	m_availableFund = fund;
	m_positions.clear();
	m_completeTrades.clear();
	m_pendingTOperations.clear();

	return true;
}

void Fund::setMarket(const std::shared_ptr<Market>& spMarket)
{
	m_spMarket = spMarket;
}

std::shared_ptr<Market> Fund::getMarket()
{
	return m_spMarket;
}

bool Fund::buyAll(const std::string& stock, int32_t price, uint32_t date, ObserveTime time)
{
	if (price <= 0)
	{
		return false;
	}

	int32_t shares = calculateMaxShares(stock, price);
	if (shares == 0)
	{
		return false;
	}

	// 计算手续费：价格转换为元，手续费结果转换为分
	BigNumber priceYuan = BigNumber(price).toPrec(2) / 100;
	int32_t fee = (StockCharge::instance().buyFee(stock, priceYuan, BigNumber(shares)) * 100).toInt();

	// 记录全仓买入
	Trade fullBuyTrade;
	fullBuyTrade.m_price = price;
	fullBuyTrade.m_shares = shares;
	fullBuyTrade.m_date = date;
	fullBuyTrade.m_time = time;
	fullBuyTrade.m_fee = fee;

	// 处理全仓买入，完成当前做T操作
	processFullBuy(stock, fullBuyTrade);

	return executeBuy(stock, price, shares, date, time, fee, true);
}

bool Fund::sellAll(const std::string& stock, int32_t price, uint32_t date, ObserveTime time)
{
	auto position = getPosition(stock);
	if (position == nullptr)
	{
		return false;
	}

	// 计算手续费：价格转换为元，手续费结果转换为分
	BigNumber priceYuan = BigNumber(price).toPrec(2) / 100;
	int32_t fee = (StockCharge::instance().sellFee(stock, priceYuan, BigNumber(position->m_shares)) * 100).toInt();

	return executeSell(stock, price, position->m_shares, date, time, fee, false);
}

bool Fund::sellAllForT(const std::string& stock, int32_t price, uint32_t date, ObserveTime time)
{
	auto position = getPosition(stock);
	if (position == nullptr)
	{
		return false;
	}

	// 计算卖出数量：当前持仓 - 1手（100股）
	const int32_t MIN_LOT = 100;
	int32_t sellShares = position->m_shares - MIN_LOT;

	if (sellShares <= 0)
	{
		// 持仓不足，无法做T
		return false;
	}

	// 确保卖出数量是整手数
	sellShares = (sellShares / MIN_LOT) * MIN_LOT;

	// 计算手续费：价格转换为元，手续费结果转换为分
	BigNumber priceYuan = BigNumber(price).toPrec(2) / 100;
	int32_t fee = (StockCharge::instance().sellFee(stock, priceYuan, BigNumber(sellShares)) * 100).toInt();

	return executeSell(stock, price, sellShares, date, time, fee, true);
}

int32_t Fund::getTotalValue(uint32_t date, ObserveTime time)
{
	int32_t total = m_availableFund;

	for (auto it = m_positions.begin(); it != m_positions.end(); ++it)
	{
		const auto& position = it->second;
		int32_t price = getStockPriceFromMarket(it->first, date, time);
		if (price > 0)
		{
			total += position->m_shares * price;
		}
	}

	return total;
}

int32_t Fund::getAvailableFund() const
{
	return m_availableFund;
}

int32_t Fund::getInitialFund() const
{
	return m_initialFund;
}

std::shared_ptr<Position> Fund::getPosition(const std::string& stock) const
{
	auto it = m_positions.find(stock);
	if (it != m_positions.end())
	{
		return it->second;
	}
	return nullptr;
}

const std::map<std::string, std::shared_ptr<Position>>& Fund::getAllPositions() const
{
	return m_positions;
}

const std::map<std::string, std::vector<CompleteTrade>>& Fund::getCompleteTrades() const
{
	return m_completeTrades;
}

std::vector<std::string> Fund::exportTradeRecords() const
{
	std::vector<std::string> records;

	// 添加账户基本信息
	std::ostringstream oss;
	oss << "账户初始资金: " << (BigNumber(m_initialFund) / 100.0).toPrec(2).toString() << "元";
	records.push_back(oss.str());

	oss.str("");
	oss << "当前可用资金: " << (BigNumber(m_availableFund) / 100.0).toPrec(2).toString() << "元";
	records.push_back(oss.str());

	records.push_back("");

	// 输出所有已完成交易
	for (auto stockIt = m_completeTrades.begin(); stockIt != m_completeTrades.end(); ++stockIt)
	{
		const std::string& stock = stockIt->first;
		const std::vector<CompleteTrade>& trades = stockIt->second;

		for (size_t i = 0; i < trades.size(); ++i)
		{
			const CompleteTrade& trade = trades[i];

			uint32_t endDate = trade.m_sellTrade.m_date;
			if (endDate == 0)
			{
				auto it = m_accountSell.find(stock);
				if (it != m_accountSell.end())
				{
					endDate = it->second.m_date;
				}
			}

			//最终收盘价
			int32_t endPrice = getStockPriceFromMarket(stock, endDate, ObserveTime::COUNT);

			// 交易基本信息
			oss.str("");
			oss << "股票 " << stock << " 第 " << (i + 1) << " 笔完整交易:";
			records.push_back(oss.str());

			// 详细输出初始全仓买入
			oss.str("");
			oss << "  初始全仓买入: " << trade.m_buyTrade.m_date << "日 ";
			oss << Util::observeTimeToWatchString(trade.m_buyTrade.m_time) << " ";
			oss << trade.m_buyTrade.m_shares << "股 @ " << (trade.m_buyTrade.m_price / 100.0) << "元";
			oss << "，手续费: " << (trade.m_buyTrade.m_fee / 100.0) << "元";
			oss << "，总成本: " << (BigNumber(trade.m_buyTrade.m_shares * trade.m_buyTrade.m_price +
				trade.m_buyTrade.m_fee) / 100.0).toPrec(2).toString() << "元";
			records.push_back(oss.str());

			// 输出做T操作
			if (!trade.m_vecTOperations.empty())
			{
				oss.str("");
				oss << "  做T操作 " << trade.m_vecTOperations.size() << " 次:";
				records.push_back(oss.str());

				for (size_t j = 0; j < trade.m_vecTOperations.size(); ++j)
				{
					const TOperation& tOp = trade.m_vecTOperations[j];
					oss.str("");
					oss << "    第 " << (j + 1) << " 次做T:";
					records.push_back(oss.str());

					// 输出做T卖出记录
					for (auto sellIt = tOp.m_sellTrade.begin(); sellIt != tOp.m_sellTrade.end(); ++sellIt)
					{
						oss.str("");
						oss << "      卖出: " << sellIt->m_date << "日 ";
						oss << Util::observeTimeToWatchString(sellIt->m_time) << " ";
						oss << sellIt->m_shares << "股 @ " << (sellIt->m_price / 100.0) << "元";
						oss << "，手续费: " << (sellIt->m_fee / 100.0) << "元";
						records.push_back(oss.str());
					}

					// 输出做T买入记录
					for (auto buyIt = tOp.m_buyTrade.begin(); buyIt != tOp.m_buyTrade.end(); ++buyIt)
					{
						oss.str("");
						oss << "      买入: " << buyIt->m_date << "日 ";
						oss << Util::observeTimeToWatchString(buyIt->m_time) << " ";
						oss << buyIt->m_shares << "股 @ " << (buyIt->m_price / 100.0) << "元";
						oss << "，手续费: " << (buyIt->m_fee / 100.0) << "元";
						records.push_back(oss.str());
					}

					// 输出做T收益
					oss.str("");
					oss << "      做T收益: " << (tOp.totalProfit(endPrice) / 100.0) << "元";
					records.push_back(oss.str());
				}

				oss.str("");
				oss << "  做T总收益: " << (trade.allTProfit(endPrice) / 100.0) << "元";
				records.push_back(oss.str());
			}

			// 输出不完整做T操作
			if (!trade.m_incompleteTOperation.m_sellTrade.empty() || !trade.m_incompleteTOperation.m_buyTrade.empty())
			{
				oss.str("");
				oss << "  不完整做T操作:";
				records.push_back(oss.str());

				const TOperation& incompleteOp = trade.m_incompleteTOperation;

				// 输出不完整做T卖出记录
				for (auto sellIt = incompleteOp.m_sellTrade.begin(); sellIt != incompleteOp.m_sellTrade.end(); ++sellIt)
				{
					oss.str("");
					oss << "      卖出: " << sellIt->m_date << "日 ";
					oss << Util::observeTimeToWatchString(sellIt->m_time) << " ";
					oss << sellIt->m_shares << "股 @ " << (sellIt->m_price / 100.0) << "元";
					oss << "，手续费: " << (sellIt->m_fee / 100.0) << "元";
					records.push_back(oss.str());
				}

				// 输出不完整做T买入记录
				for (auto buyIt = incompleteOp.m_buyTrade.begin(); buyIt != incompleteOp.m_buyTrade.end(); ++buyIt)
				{
					oss.str("");
					oss << "      买入: " << buyIt->m_date << "日 ";
					oss << Util::observeTimeToWatchString(buyIt->m_time) << " ";
					oss << buyIt->m_shares << "股 @ " << (buyIt->m_price / 100.0) << "元";
					oss << "，手续费: " << (buyIt->m_fee / 100.0) << "元";
					records.push_back(oss.str());
				}
			}

			// 输出最终卖出信息
			if (!trade.m_sellTrade.empty())
			{
				oss.str("");
				oss << "  最终全仓卖出: " << trade.m_sellTrade.m_date << "日 ";
				oss << Util::observeTimeToWatchString(trade.m_sellTrade.m_time) << " ";
				oss << trade.m_sellTrade.m_shares << "股 @ " << (trade.m_sellTrade.m_price / 100.0) << "元";
				oss << "，手续费: " << (trade.m_sellTrade.m_fee / 100.0) << "元";
				oss << "，总收入: " << (BigNumber(trade.m_sellTrade.m_shares * trade.m_sellTrade.m_price -
					trade.m_sellTrade.m_fee) / 100.0).toPrec(2).toString() << "元";
				records.push_back(oss.str());

				oss.str("");
				oss << "  交易总收益: " <<
					(BigNumber(trade.tradeProfit()) / 100.0).toPrec(2).toString() << "元";
				records.push_back(oss.str());
			}
			else
			{
				records.push_back("  状态: 持有中");
			}

			records.push_back(""); // 空行分隔
		}
	}

	// 添加当前持仓信息
	if (!m_positions.empty())
	{
		records.push_back("当前持仓:");
		for (auto it = m_positions.begin(); it != m_positions.end(); ++it)
		{
			oss.str("");
			oss << "  " << it->first << ": " << it->second->m_shares << "股 @ "
				<< (it->second->m_price / 100.0) << "元";
			oss << "，买入日期: " << it->second->m_date;
			oss << "，买入时间: " << Util::observeTimeToWatchString(it->second->m_time);
			records.push_back(oss.str());
		}
		records.push_back("");
	}

	return records;
}

void Fund::reset()
{
	m_availableFund = m_initialFund;
	m_positions.clear();
	m_completeTrades.clear();
	m_pendingTOperations.clear();
}

void Fund::closeAllTrades(uint32_t date, bool isAccountSell, ObserveTime time)
{
	if (isAccountSell)
	{
		for (auto it = m_positions.begin(); it != m_positions.end(); ++it)
		{
			m_accountSell[it->first].m_date = date;
			m_accountSell[it->first].m_time = time;
			m_accountSell[it->first].m_price = getStockPriceFromMarket(it->first, date, time);
		}
		return;
	}
	for (auto it = m_positions.begin(); it != m_positions.end();)
	{
		std::string stock = it->first;
		std::shared_ptr<Position> position = it->second;

		int32_t price = getStockPriceFromMarket(stock, date, time);
		if (price > 0)
		{
			// 计算手续费：价格转换为元，手续费结果转换为分
			BigNumber priceYuan = BigNumber(price).toPrec(2) / 100;
			int32_t fee = (StockCharge::instance().sellFee(stock, priceYuan, BigNumber(position->m_shares)) * 100).toInt();
			executeSell(stock, price, position->m_shares, date, time, fee, false);
		}

		it = m_positions.begin();
	}
}

int32_t Fund::calculateMaxShares(const std::string& stock, int32_t price) const
{
	if (price == 0 || m_availableFund == 0)
	{
		return 0;
	}

	const int32_t MIN_LOT = 100;
	int32_t estimatedShares = (m_availableFund / price) / MIN_LOT * MIN_LOT;

	if (estimatedShares == 0)
	{
		return 0;
	}

	int32_t low = MIN_LOT;
	int32_t high = estimatedShares;
	int32_t result = 0;
	BigNumber priceYuan = BigNumber(price).toPrec(2) / 100;

	while (low <= high)
	{
		int32_t mid = ((low + high) / (MIN_LOT * 2)) * MIN_LOT;

		int32_t cost = mid * price;
		// 计算手续费：价格转换为元，手续费结果转换为分
		int32_t fee = (StockCharge::instance().buyFee(stock, priceYuan, BigNumber(mid)) * 100).toInt();
		int32_t totalCost = cost + fee;

		if (totalCost <= m_availableFund)
		{
			result = mid;
			low = mid + MIN_LOT;
		}
		else
		{
			high = mid - MIN_LOT;
		}
	}

	return result;
}

bool Fund::executeBuy(const std::string& stock, int32_t price, int32_t shares, uint32_t date, ObserveTime time, int32_t fee, bool isFullPosition)
{
	if (shares == 0)
	{
		return false;
	}

	int32_t cost = shares * price;
	int32_t totalCost = cost + fee;

	if (totalCost > m_availableFund)
	{
		return false;
	}

	m_availableFund -= totalCost;

	auto position = getPosition(stock);
	if (position != nullptr)
	{
		position->m_shares += shares;
	}
	else
	{
		auto newPosition = std::make_shared<Position>();
		newPosition->m_price = price;
		newPosition->m_shares = shares;
		newPosition->m_date = date;
		newPosition->m_time = time;
		m_positions[stock] = newPosition;

		CompleteTrade newTrade;
		newTrade.m_buyTrade.m_price = price;
		newTrade.m_buyTrade.m_shares = shares;
		newTrade.m_buyTrade.m_date = date;
		newTrade.m_buyTrade.m_time = time;
		newTrade.m_buyTrade.m_fee = fee;

		m_completeTrades[stock].push_back(newTrade);
	}

	return true;
}

bool Fund::executeSell(const std::string& stock, int32_t price, int32_t shares, uint32_t date, ObserveTime time, int32_t fee, bool isTOperation)
{
	auto position = getPosition(stock);
	if (position == nullptr)
	{
		return false;
	}

	if (shares > position->m_shares)
	{
		return false;
	}

	int32_t income = shares * price;
	int32_t netIncome = income - fee;

	m_availableFund += netIncome;

	if (isTOperation)
	{
		Trade sellTrade;
		sellTrade.m_price = price;
		sellTrade.m_shares = shares;
		sellTrade.m_date = date;
		sellTrade.m_time = time;
		sellTrade.m_fee = fee;

		processTOperations(stock, sellTrade);
	}
	else
	{
		// 非T操作，结束交易
		Trade sellTrade;
		sellTrade.m_price = price;
		sellTrade.m_shares = shares;
		sellTrade.m_date = date;
		sellTrade.m_time = time;
		sellTrade.m_fee = fee;

		processCompleteTrade(stock, sellTrade);
	}

	position->m_shares -= shares;

	if (position->m_shares == 0)
	{
		m_positions.erase(stock);
	}

	return true;
}

void Fund::processTOperations(const std::string& stock, const Trade& sellTrade)
{
	// 将卖出交易添加到待匹配记录中
	m_pendingTOperations[stock].m_sellTrade.push_back(sellTrade);
}

void Fund::processFullBuy(const std::string& stock, const Trade& buyTrade)
{
	auto pendingIt = m_pendingTOperations.find(stock);
	if (pendingIt != m_pendingTOperations.end() && !pendingIt->second.m_sellTrade.empty())
	{
		CompleteTrade* currentTrade = getCurrentTrade(stock);
		if (currentTrade != nullptr)
		{
			// 将买入交易添加到待匹配记录中
			pendingIt->second.m_buyTrade.push_back(buyTrade);

			// 将完整的做T操作添加到交易记录中
			currentTrade->m_vecTOperations.push_back(pendingIt->second);
		}

		// 清空待匹配记录
		m_pendingTOperations.erase(stock);
	}
}

void Fund::processCompleteTrade(const std::string& stock, const Trade& sellTrade)
{
	CompleteTrade* currentTrade = getCurrentTrade(stock);
	if (currentTrade != nullptr && currentTrade->m_sellTrade.empty())
	{
		currentTrade->m_sellTrade = sellTrade;

		// 如果有未完成的做T操作，将其设置为不完整做T操作
		auto pendingIt = m_pendingTOperations.find(stock);
		if (pendingIt != m_pendingTOperations.end())
		{
			currentTrade->m_incompleteTOperation = pendingIt->second;
			m_pendingTOperations.erase(stock);
		}
	}
}

int32_t Fund::getStockPriceFromMarket(const std::string& stock, uint32_t date, ObserveTime time) const
{
	if (m_spMarket == nullptr)
	{
		return 0;
	}

	const std::vector<int32_t>& dayInfo = m_spMarket->getStockData(stock, date);
	if (dayInfo.size() != (int32_t)Overall::COUNT + (int32_t)ObserveTime::COUNT +
		(int32_t)ObserveTime::COUNT * (int32_t)RangeTime::COUNT * (int32_t)TransType::COUNT)
	{
		return 0;
	}
	if (time == ObserveTime::COUNT)
	{
		return dayInfo[(int32_t)Overall::CLOSE];
	}
	else
	{
		return dayInfo[(int32_t)Overall::COUNT + (int32_t)time];
	}
}

CompleteTrade* Fund::getCurrentTrade(const std::string& stock)
{
	auto it = m_completeTrades.find(stock);
	if (it != m_completeTrades.end() && !it->second.empty())
	{
		CompleteTrade& lastTrade = it->second.back();
		if (lastTrade.m_sellTrade.empty())
		{
			return &lastTrade;
		}
	}
	return nullptr;
}

int32_t Fund::allTDiff() const
{
	int32_t total = 0;
	for (auto stockIt = m_completeTrades.begin(); stockIt != m_completeTrades.end(); ++stockIt)
	{
		const std::vector<CompleteTrade>& trades = stockIt->second;
		for (auto tradeIt = trades.begin(); tradeIt != trades.end(); ++tradeIt)
		{
			total += tradeIt->allTDiff();
		}
	}
	return total;
}

int32_t Fund::allTFee() const
{
	int32_t total = 0;
	for (auto stockIt = m_completeTrades.begin(); stockIt != m_completeTrades.end(); ++stockIt)
	{
		const std::vector<CompleteTrade>& trades = stockIt->second;
		for (auto tradeIt = trades.begin(); tradeIt != trades.end(); ++tradeIt)
		{
			total += tradeIt->allTFee();
		}
	}
	return total;
}

int32_t Fund::allTProfit(uint32_t lastDate) const
{
	int32_t total = 0;
	for (auto stockIt = m_completeTrades.begin(); stockIt != m_completeTrades.end(); ++stockIt)
	{
		int32_t endPrice = getStockPriceFromMarket(stockIt->first, lastDate, ObserveTime::COUNT);
		const std::vector<CompleteTrade>& trades = stockIt->second;
		for (auto tradeIt = trades.begin(); tradeIt != trades.end(); ++tradeIt)
		{
			total += tradeIt->allTProfit(endPrice);
		}
	}
	return total;
}

int32_t Fund::tradeProfit() const
{
	int32_t total = 0;
	for (auto stockIt = m_completeTrades.begin(); stockIt != m_completeTrades.end(); ++stockIt)
	{
		const std::vector<CompleteTrade>& trades = stockIt->second;
		for (auto tradeIt = trades.begin(); tradeIt != trades.end(); ++tradeIt)
		{
			total += tradeIt->tradeProfit();
		}
	}
	return total;
}

int32_t Fund::allBeginEndFee() const
{
	int32_t total = 0;
	for (auto stockIt = m_completeTrades.begin(); stockIt != m_completeTrades.end(); ++stockIt)
	{
		const std::vector<CompleteTrade>& trades = stockIt->second;
		for (auto tradeIt = trades.begin(); tradeIt != trades.end(); ++tradeIt)
		{
			total += tradeIt->beginEndFee();
		}
	}
	return total;
}

int32_t TOperation::sellShares() const
{
	int32_t totalSellShares = 0;
	for (auto it = m_sellTrade.begin(); it != m_sellTrade.end(); ++it)
	{
		totalSellShares += it->m_shares;
	}
	return totalSellShares;
}

int32_t TOperation::buyShares() const
{
	int32_t totalBuyShares = 0;
	for (auto it = m_buyTrade.begin(); it != m_buyTrade.end(); ++it)
	{
		totalBuyShares += it->m_shares;
	}
	return totalBuyShares;
}

int32_t TOperation::diff() const
{
	if (m_sellTrade.empty() || m_buyTrade.empty())
	{
		return 0;
	}

	int32_t totalSellValue = 0;
	int32_t totalSellShares = 0;
	for (auto it = m_sellTrade.begin(); it != m_sellTrade.end(); ++it)
	{
		totalSellValue += it->m_price * it->m_shares;
		totalSellShares += it->m_shares;
	}
	int32_t avgSellPrice = totalSellShares > 0 ? totalSellValue / totalSellShares : 0;

	int32_t totalBuyValue = 0;
	int32_t totalBuyShares = 0;
	for (auto it = m_buyTrade.begin(); it != m_buyTrade.end(); ++it)
	{
		totalBuyValue += it->m_price * it->m_shares;
		totalBuyShares += it->m_shares;
	}
	int32_t avgBuyPrice = totalBuyShares > 0 ? totalBuyValue / totalBuyShares : 0;

	return avgSellPrice - avgBuyPrice;
}

int32_t TOperation::totalFee() const
{
	int32_t total = 0;
	for (auto it = m_sellTrade.begin(); it != m_sellTrade.end(); ++it)
	{
		total += it->m_fee;
	}
	for (auto it = m_buyTrade.begin(); it != m_buyTrade.end(); ++it)
	{
		total += it->m_fee;
	}
	return total;
}

int32_t TOperation::totalProfit(int32_t endPrice) const
{
	int32_t totalSellValue = 0;
	for (auto it = m_sellTrade.begin(); it != m_sellTrade.end(); ++it)
	{
		totalSellValue += (it->m_price - endPrice) * it->m_shares;
	}
	int32_t totalBuyValue = 0;
	for (auto it = m_buyTrade.begin(); it != m_buyTrade.end(); ++it)
	{
		totalBuyValue += (endPrice - it->m_price) * it->m_shares;
	}
	return totalSellValue + totalBuyValue - totalFee();
}

int32_t CompleteTrade::allTDiff() const
{
	int32_t total = 0;
	for (auto it = m_vecTOperations.begin(); it != m_vecTOperations.end(); ++it)
	{
		total += it->diff();
	}
	return total;
}

int32_t CompleteTrade::allTFee() const
{
	int32_t total = 0;
	for (auto it = m_vecTOperations.begin(); it != m_vecTOperations.end(); ++it)
	{
		total += it->totalFee();
	}
	return total;
}

int32_t CompleteTrade::allTProfit(int32_t endPrice) const
{
	int32_t total = 0;
	for (auto it = m_vecTOperations.begin(); it != m_vecTOperations.end(); ++it)
	{
		total += it->totalProfit(endPrice);
	}
	total += m_incompleteTOperation.totalProfit(endPrice);
	return total;
}

int32_t CompleteTrade::tradeProfit() const
{
	if (m_sellTrade.empty())
	{
		return 0;
	}
	return (m_sellTrade.m_price - m_buyTrade.m_price) * m_buyTrade.m_shares +
		allTProfit(m_sellTrade.m_price) - m_sellTrade.m_fee - m_buyTrade.m_fee;
}

int32_t CompleteTrade::beginEndFee() const
{
	return m_buyTrade.m_fee + m_sellTrade.m_fee;
}
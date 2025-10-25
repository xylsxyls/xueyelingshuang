#include "Fund.h"
#include <algorithm>
#include <cmath>
#include <iostream>

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
	m_pendingTrades.clear();

	return true;
}

void Fund::setMarket(const std::shared_ptr<Market>& spMarket)
{
	m_spMarket = spMarket;
}

bool Fund::buyFullPosition(const std::string& stock, int32_t price, uint32_t date, ObserveTime time)
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

	return executeBuy(stock, price, shares, date, time);
}

bool Fund::sellAll(const std::string& stock, int32_t price, uint32_t date, ObserveTime time)
{
	auto position = getPosition(stock);
	if (!position)
	{
		return false;
	}

	return executeSell(stock, price, position->shares, date, time, false);
}

bool Fund::sellForT(const std::string& stock, int32_t price, uint32_t date, ObserveTime time)
{
	auto position = getPosition(stock);
	if (!position)
	{
		return false;
	}

	// 计算卖出数量：当前持仓 - 1手（100股）
	const int32_t MIN_LOT = 100;
	int32_t sellShares = position->shares - MIN_LOT;

	if (sellShares <= 0)
	{
		// 持仓不足，无法做T
		return false;
	}

	// 确保卖出数量是整手数
	sellShares = (sellShares / MIN_LOT) * MIN_LOT;

	return executeSell(stock, price, sellShares, date, time, true);
}

bool Fund::buyForT(const std::string& stock, int32_t price, uint32_t date, ObserveTime time)
{
	return buyFullPosition(stock, price, date, time);
}

int32_t Fund::getTotalValue(uint32_t date, ObserveTime time)
{
	int32_t total = m_availableFund;

	for (const auto& position : m_positions)
	{
		int32_t price = getStockPriceFromMarket(position.stock, date, time);
		if (price > 0)
		{
			total += position.shares * price;
		}
		// 如果找不到价格，默认该股票没有价值（如停牌）
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

const Position* Fund::getPosition(const std::string& stock) const
{
	for (const auto& position : m_positions)
	{
		if (position.stock == stock)
		{
			return &position;
		}
	}
	return nullptr;
}

const std::vector<Position>& Fund::getAllPositions() const
{
	return m_positions;
}

const std::map<std::string, std::vector<CompleteTrade>>& Fund::getCompleteTrades() const
{
	return m_completeTrades;
}

void Fund::reset()
{
	m_availableFund = m_initialFund;
	m_positions.clear();
	m_completeTrades.clear();
	m_pendingTrades.clear();
}

void Fund::closeAllTrades(uint32_t date, ObserveTime time)
{
	// 强制卖出所有持仓，完成未结束的交易
	while (!m_positions.empty())
	{
		const Position& position = m_positions[0];
		int32_t price = getStockPriceFromMarket(position.stock, date, time);
		executeSell(std::string(position.stock), price, position.shares, date, time, false);
	}
}

int32_t Fund::calculateMaxShares(const std::string& stock, int32_t price) const
{
	if (price == 0 || m_availableFund == 0)
	{
		return 0;
	}

	// 中国股市最小交易单位是100股
	const int32_t MIN_LOT = 100;

	// 初步估算最大可买股数（不考虑手续费）
	int32_t estimatedShares = (m_availableFund / price) / MIN_LOT * MIN_LOT;

	if (estimatedShares == 0)
	{
		return 0;
	}

	// 使用二分查找精确计算最大可买股数
	int32_t low = MIN_LOT;
	int32_t high = estimatedShares;
	int32_t result = 0;

	while (low <= high)
	{
		int32_t mid = ((low + high) / (MIN_LOT * 2)) * MIN_LOT; // 取整手数

		int32_t cost = mid * price;
		BigNumber fee = (StockCharge::instance().buyFee(stock, BigNumber(price), BigNumber(mid)) * 100).toInt();
		BigNumber totalCost = BigNumber(cost) + fee;

		if (totalCost <= m_availableFund)
		{
			result = mid;    // 当前股数可行，尝试更多
			low = mid + MIN_LOT;
		}
		else
		{
			high = mid - MIN_LOT;
		}
	}

	return result;
}

bool Fund::executeBuy(const std::string& stock, int32_t price, int32_t shares, uint32_t date, ObserveTime time)
{
	if (shares == 0)
	{
		return false;
	}

	int32_t cost = shares * price;
	int32_t fee = (StockCharge::instance().buyFee(stock, BigNumber(price), BigNumber(shares)) * 100).toInt();
	int32_t totalCost = cost + fee;

	if (totalCost > m_availableFund)
	{
		return false;
	}

	// 扣除资金
	m_availableFund -= totalCost;

	// 更新持仓
	auto position = const_cast<Position*>(getPosition(stock));
	if (position)
	{
		// 已有持仓，增加股数
		position->shares += shares;
	}
	else
	{
		// 新建持仓
		Position newPosition;
		newPosition.stock = stock;
		newPosition.shares = shares;
		newPosition.buyPrice = price;
		newPosition.buyDate = date;
		newPosition.buyTime = time;
		m_positions.push_back(newPosition);

		// 创建新的交易记录
		CompleteTrade newTrade;
		newTrade.buyDate = date;
		newTrade.buyTime = time;
		newTrade.buyPrice = price;
		newTrade.buyShares = shares;
		newTrade.buyFee = fee;
		newTrade.sellDate = 0;
		newTrade.sellShares = 0;
		newTrade.sellPrice = 0;
		newTrade.sellFee = 0;
		newTrade.totalTDiff = 0;
		newTrade.totalTFee = 0;
		newTrade.totalTProfit = 0;
		newTrade.tradeProfit = 0;
		newTrade.isClosed = false;

		m_completeTrades[stock].push_back(newTrade);
	}

	// 记录交易
	SimpleTradeRecord record;
	record.date = date;
	record.time = time;
	record.stock = stock;
	record.operation = "BUY";
	record.price = price;
	record.shares = shares;
	record.fee = fee;
	recordTrade(record);

	return true;
}

bool Fund::executeSell(const std::string& stock, int32_t price, int32_t shares, uint32_t date, ObserveTime time, bool isTOperation)
{
	auto position = const_cast<Position*>(getPosition(stock));
	if (!position)
	{
		return false;
	}

	if (shares > position->shares)
	{
		return false;
	}

	int32_t income = shares * price;
	int32_t fee = (StockCharge::instance().sellFee(stock, BigNumber(price), BigNumber(shares)) * 100).toInt();
	int32_t netIncome = income - fee;

	// 增加资金
	m_availableFund += netIncome;

	// 记录交易
	SimpleTradeRecord record;
	record.date = date;
	record.time = time;
	record.stock = stock;
	record.operation = "SELL";
	record.price = price;
	record.shares = shares;
	record.fee = fee;

	// 处理做T操作
	if (isTOperation)
	{
		processTOperations(stock, record);
	}

	recordTrade(record);

	// 更新持仓
	position->shares -= shares;

	// 判断是否结束交易：非T操作且持仓为0
	if (!isTOperation && position->shares == 0)
	{
		// 删除零持仓
		m_positions.erase(std::remove_if(m_positions.begin(), m_positions.end(), [&stock](const Position& p)
		{
			return p.stock == stock;
		}
		), m_positions.end());

		// 标记交易为已完成
		if (!m_completeTrades[stock].empty())
		{
			CompleteTrade& trade = m_completeTrades[stock].back();
			if (!trade.isClosed)
			{
				trade.sellDate = date;
				trade.sellTime = time;
				trade.sellPrice = price;
				trade.sellShares = shares;
				trade.sellFee = fee;
				trade.isClosed = true;

				// 计算交易总收益
				int32_t buyCost = trade.buyShares * trade.buyPrice + trade.buyFee;
				int32_t sellIncome = trade.sellShares * trade.sellPrice - trade.sellFee;
				trade.tradeProfit = sellIncome - buyCost + trade.totalTProfit;
			}
		}
	}

	return true;
}

void Fund::recordTrade(const SimpleTradeRecord& record)
{
	// 添加到待匹配交易列表
	m_pendingTrades.push_back(record);
}

void Fund::processTOperations(const std::string& stock, const SimpleTradeRecord& sellRecord)
{
	// 查找最近的一笔买入记录（日期早于卖出日期）
	for (auto it = m_pendingTrades.rbegin(); it != m_pendingTrades.rend(); ++it)
	{
		if (it->stock == stock && it->operation == "BUY" && it->date < sellRecord.date)
		{
			// 找到匹配的买入记录，创建做T记录
			TOperation tOp;
			tOp.sellDate = sellRecord.date;
			tOp.sellTime = sellRecord.time;
			tOp.sellPrice = sellRecord.price;
			tOp.sellFee = sellRecord.fee;

			tOp.buyDate = it->date;
			tOp.buyTime = it->time;
			tOp.buyPrice = it->price;
			tOp.buyFee = it->fee;

			tOp.shares = (std::min)(sellRecord.shares, it->shares);
			tOp.priceDiff = sellRecord.price - it->price;
			tOp.totalFee = sellRecord.fee + it->fee;
			tOp.netProfit = (tOp.priceDiff * tOp.shares) - tOp.totalFee;

			// 添加到对应交易的做T记录
			if (!m_completeTrades[stock].empty())
			{
				CompleteTrade& trade = m_completeTrades[stock].back();
				trade.tOperations.push_back(tOp);
				trade.totalTDiff += tOp.priceDiff * tOp.shares;
				trade.totalTFee += tOp.totalFee;
				trade.totalTProfit += tOp.netProfit;
			}

			break;
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
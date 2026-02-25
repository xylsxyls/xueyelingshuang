#include "Strategy.h"
#include <functional>
#include <unordered_map>
#include "Util.h"

StrategyResult::StrategyResult() :
m_strategyMode(StrategyMode::COUNT),
m_totalReturn(0),
m_annualReturn(0),
m_maxDrawdown(0),
m_winRate(0),
m_profitArea(0),
m_healthScore(0),
m_tradeDays(0),
m_totalDays(0)
{

}

ImportParam::ImportParam() :
m_operate(0),
m_virtualSellPrice(0),
m_virtualSellObserveTime(ObserveTime::COUNT),
m_virtualBuyPrice(0),
m_virtualBuyObserveTime(ObserveTime::COUNT),
m_realSellPrice(0),
m_realBuyPrice(0),
m_isFull(false)
{

}

bool ImportParam::empty()
{
	return m_virtualSellPrice == 0 &&
		m_virtualSellObserveTime == ObserveTime::COUNT &&
		m_virtualBuyPrice == 0 &&
		m_virtualBuyObserveTime == ObserveTime::COUNT &&
		m_realSellPrice == 0 &&
		m_realBuyPrice == 0 &&
		m_isFull == false;
}

Strategy::Strategy() :
m_mode(StrategyMode::COUNT)
{

}

Strategy::~Strategy()
{

}

std::string Strategy::describeParam(const std::vector<int32_t>& param)
{
	return "";
}

void Strategy::addStock(const std::string& stock)
{
	m_vecStock.push_back(stock);
}

std::vector<std::string> Strategy::getStock() const
{
	return m_vecStock;
}

void Strategy::setMarket(const std::shared_ptr<Market>& spMarket)
{
	m_spMarket = spMarket;
}

void Strategy::setFund(const std::shared_ptr<Fund>& spFund)
{
	m_spFund = spFund;
}

std::shared_ptr<Fund> Strategy::getFund()
{
	return m_spFund;
}

bool Strategy::fillCheckParam()
{
	return true;
}

void Strategy::setStrategyParam(const std::vector<int32_t>& param)
{
	m_strategyParam = param;
}

std::vector<int32_t> Strategy::getStrategyParam() const
{
	return m_strategyParam;
}

bool Strategy::isStrategyParamValid() const
{
	return true;
}

StrategyMode Strategy::getStrategyMode() const
{
	return m_mode;
}

std::string Strategy::getStrategyName() const
{
	return m_modeName;
}

int32_t Strategy::getCurrentPrice(const std::vector<int32_t>& dayInfo, ObserveTime time)
{
	return dayInfo[(int32_t)Overall::COUNT + (int32_t)time];
}

int32_t Strategy::getDirectBuyPrice(const std::vector<int32_t>& dayInfo, ObserveTime time)
{
	return dayInfo[Util::getPriceMatrixIndex(time, RangeTime::RANGE0, TransType::BEST_BUY)];
}

int32_t Strategy::getDirectSellPrice(const std::vector<int32_t>& dayInfo, ObserveTime time)
{
	return dayInfo[Util::getPriceMatrixIndex(time, RangeTime::RANGE0, TransType::BEST_SELL)];
}

int32_t Strategy::getMinPrice(const std::vector<int32_t>& dayInfo, ObserveTime timeBegin, ObserveTime timeEnd)
{
	if (timeBegin == timeEnd)
	{
		RCSend("range time error");
		return 0;
	}
	int32_t endTimeValue = Util::getTimeValue(timeEnd);
	int32_t rangeEndTimeValue30 = Util::rangeEndTime(Util::getTimeValue(timeBegin), 30);
	if (rangeEndTimeValue30 == endTimeValue)
	{
		return dayInfo[Util::getPriceMatrixIndex(timeBegin, RangeTime::RANGE30, TransType::BEST_BUY)];
	}
	else if (rangeEndTimeValue30 > endTimeValue)
	{
		int32_t rangeEndTimeValue20 = Util::rangeEndTime(Util::getTimeValue(timeBegin), 20);
		if (rangeEndTimeValue20 == endTimeValue)
		{
			return dayInfo[Util::getPriceMatrixIndex(timeBegin, RangeTime::RANGE20, TransType::BEST_BUY)];
		}
		int32_t rangeEndTimeValue10 = Util::rangeEndTime(Util::getTimeValue(timeBegin), 10);
		if (rangeEndTimeValue10 == endTimeValue)
		{
			return dayInfo[Util::getPriceMatrixIndex(timeBegin, RangeTime::RANGE10, TransType::BEST_BUY)];
		}
		RCSend("range time error");
		return 0;
	}
	else
	{
		int32_t minPrice = dayInfo[Util::getPriceMatrixIndex(timeBegin, RangeTime::RANGE30, TransType::BEST_BUY)];
		for (int32_t timeIndex = (int32_t)timeBegin + 1; timeIndex < (int32_t)timeEnd; ++timeIndex)
		{
			int32_t currentMinPrice = dayInfo[Util::getPriceMatrixIndex((ObserveTime)timeIndex,
				RangeTime::RANGE30, TransType::BEST_BUY)];
			if (currentMinPrice < minPrice)
			{
				minPrice = currentMinPrice;
			}
			int32_t currentRangeEndTimeValue30 = Util::rangeEndTime(Util::getTimeValue((ObserveTime)timeIndex), 30);
			if (currentRangeEndTimeValue30 == endTimeValue)
			{
				return minPrice;
			}
			else if (currentRangeEndTimeValue30 > endTimeValue)
			{
				int32_t currentMinPrice20 = dayInfo[Util::getPriceMatrixIndex((ObserveTime)timeIndex,
					RangeTime::RANGE20, TransType::BEST_BUY)];
				if (currentMinPrice20 < minPrice)
				{
					minPrice = currentMinPrice20;
				}
				int32_t currentRangeEndTimeValue20 = Util::rangeEndTime(Util::getTimeValue((ObserveTime)timeIndex), 20);
				if (currentRangeEndTimeValue20 != endTimeValue)
				{
					RCSend("range time error");
				}
				return minPrice;
			}
		}
		return minPrice;
	}
}

int32_t Strategy::getMaxPrice(const std::vector<int32_t>& dayInfo, ObserveTime timeBegin, ObserveTime timeEnd)
{
	if (timeBegin == timeEnd)
	{
		RCSend("range time error");
		return 0;
	}
	int32_t endTimeValue = Util::getTimeValue(timeEnd);
	int32_t rangeEndTimeValue30 = Util::rangeEndTime(Util::getTimeValue(timeBegin), 30);
	if (rangeEndTimeValue30 == endTimeValue)
	{
		return dayInfo[Util::getPriceMatrixIndex(timeBegin, RangeTime::RANGE30, TransType::BEST_SELL)];
	}
	else if (rangeEndTimeValue30 > endTimeValue)
	{
		int32_t rangeEndTimeValue20 = Util::rangeEndTime(Util::getTimeValue(timeBegin), 20);
		if (rangeEndTimeValue20 == endTimeValue)
		{
			return dayInfo[Util::getPriceMatrixIndex(timeBegin, RangeTime::RANGE20, TransType::BEST_SELL)];
		}
		int32_t rangeEndTimeValue10 = Util::rangeEndTime(Util::getTimeValue(timeBegin), 10);
		if (rangeEndTimeValue10 == endTimeValue)
		{
			return dayInfo[Util::getPriceMatrixIndex(timeBegin, RangeTime::RANGE10, TransType::BEST_SELL)];
		}
		RCSend("range time error");
		return 0;
	}
	else
	{
		int32_t maxPrice = dayInfo[Util::getPriceMatrixIndex(timeBegin, RangeTime::RANGE30, TransType::BEST_SELL)];
		for (int32_t timeIndex = (int32_t)timeBegin + 1; timeIndex < (int32_t)timeEnd; ++timeIndex)
		{
			int32_t currentMaxPrice = dayInfo[Util::getPriceMatrixIndex((ObserveTime)timeIndex,
				RangeTime::RANGE30, TransType::BEST_SELL)];
			if (currentMaxPrice > maxPrice)
			{
				maxPrice = currentMaxPrice;
			}
			int32_t currentRangeEndTimeValue30 = Util::rangeEndTime(Util::getTimeValue((ObserveTime)timeIndex), 30);
			if (currentRangeEndTimeValue30 == endTimeValue)
			{
				return maxPrice;
			}
			else if (currentRangeEndTimeValue30 > endTimeValue)
			{
				int32_t currentMaxPrice20 = dayInfo[Util::getPriceMatrixIndex((ObserveTime)timeIndex,
					RangeTime::RANGE20, TransType::BEST_SELL)];
				if (currentMaxPrice20 > maxPrice)
				{
					maxPrice = currentMaxPrice20;
				}
				int32_t currentRangeEndTimeValue20 = Util::rangeEndTime(Util::getTimeValue((ObserveTime)timeIndex), 20);
				if (currentRangeEndTimeValue20 != endTimeValue)
				{
					RCSend("range time error");
				}
				return maxPrice;
			}
		}
		return maxPrice;
	}
}

void Strategy::setImportParam(const ImportParam& import)
{
	m_import = import;
}

ImportParam Strategy::getImportParam()
{
	return m_import;
}

std::vector<std::string> Strategy::strategyLog()
{
	return m_strategyLog;
}

std::vector<uint32_t> Strategy::tradeCount()
{
	return m_tradeCount;
}

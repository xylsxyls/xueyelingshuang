#include "Util.h"
#include "Config.h"
#include <algorithm>
#include "CStringManager/CStringManagerAPI.h"
#include "RedisManager.h"

void Util::getAllIndex(uint32_t allIndex, uint32_t& observeIndex, uint32_t& rangeIndex, uint32_t& transIndex, uint32_t offset)
{
	// 减去偏移量
	uint32_t index = allIndex - offset;

	// 获取各个维度的数量（假设这些枚举值已经定义）
	const uint32_t observeCount = (uint32_t)ObserveTime::COUNT;
	const uint32_t rangeCount = (uint32_t)RangeTime::COUNT;
	const uint32_t transCount = (uint32_t)TransType::COUNT;

	// 计算每个维度的步长
	const uint32_t rangeTransStep = rangeCount * transCount;
	const uint32_t transStep = transCount;

	// 反求各个索引
	observeIndex = index / rangeTransStep;
	uint32_t remainder = index % rangeTransStep;

	rangeIndex = remainder / transStep;
	transIndex = remainder % transStep;
}

std::vector<int32_t> Util::groupToInt(const std::vector<std::string>& vecGroup)
{
	std::vector<int32_t> result;
	result.reserve(vecGroup.size());
	std::transform(vecGroup.begin(), vecGroup.end(), std::back_inserter(result), [](const std::string& str)
	{
		if (str.size() <= 8)
		{
			return 0;
		}
		return std::atoi(str.substr(8, str.length() - 8).c_str());
	});
	return result;
}

std::vector<std::string> Util::allField(bool isCreate)
{
	std::vector<std::string> vecField;
	for (uint32_t overallIndex = 0; overallIndex < (uint32_t)Overall::COUNT; ++overallIndex)
	{
		vecField.push_back(CStringManager::Format(
			(std::string("overall_%u") + (isCreate ? " interger" : "")).c_str(), overallIndex));
	}
	for (uint32_t observeIndex = 0; observeIndex < (uint32_t)ObserveTime::COUNT; ++observeIndex)
	{
		vecField.push_back(CStringManager::Format(
			(std::string("observe_%u") + (isCreate ? " interger" : "")).c_str(), observeIndex));
	}
	for (uint32_t observeIndex = 0; observeIndex < (uint32_t)ObserveTime::COUNT; ++observeIndex)
	{
		for (uint32_t rangeIndex = 0; rangeIndex < (uint32_t)RangeTime::COUNT; ++rangeIndex)
		{
			for (uint32_t transIndex = 0; transIndex < (uint32_t)TransType::COUNT; ++transIndex)
			{
				vecField.push_back(CStringManager::Format(
					(std::string("price_%u_%u_%u") + (isCreate ? " interger" : "")).c_str(),
					observeIndex, rangeIndex, transIndex));
			}
		}
	}
	return vecField;
}

std::vector<std::vector<int32_t>> Util::getAllStockData(const std::string& stock, uint32_t beginTime, uint32_t endTime)
{
	HiRedis& redis = RedisManager::instance().m_redis;
	std::vector<std::vector<int32_t>> stockPrice;
	for (uint32_t overallIndex = 0; overallIndex < (uint32_t)Overall::COUNT; ++overallIndex)
	{
		std::string groupName = stock + ":overall_" + std::to_string(overallIndex);
		std::vector<std::string> group = redis.getOrderGroupByScore(groupName, beginTime, endTime)->toGroup();
		std::vector<int32_t> vecGroup = Util::groupToInt(group);
		stockPrice.push_back(vecGroup);
	}
	for (uint32_t observeIndex = 0; observeIndex < (uint32_t)ObserveTime::COUNT; ++observeIndex)
	{
		std::string groupName = stock + ":observe_" + std::to_string(observeIndex);
		std::vector<std::string> group = redis.getOrderGroupByScore(groupName, beginTime, endTime)->toGroup();
		std::vector<int32_t> vecGroup = Util::groupToInt(group);
		stockPrice.push_back(vecGroup);
	}
	for (uint32_t observeIndex = 0; observeIndex < (uint32_t)ObserveTime::COUNT; ++observeIndex)
	{
		for (uint32_t rangeIndex = 0; rangeIndex < (uint32_t)RangeTime::COUNT; ++rangeIndex)
		{
			for (uint32_t transIndex = 0; transIndex < (uint32_t)TransType::COUNT; ++transIndex)
			{
				std::string groupName = CStringManager::Format("%s:price_%d_%d_%d",
					stock.c_str(), observeIndex, rangeIndex, transIndex);
				std::vector<std::string> group = redis.getOrderGroupByScore(groupName, beginTime, endTime)->toGroup();
				std::vector<int32_t> vecGroup = Util::groupToInt(group);
				stockPrice.push_back(vecGroup);
			}
		}
	}
	std::vector<std::vector<int32_t>> vecResult;
	if (stockPrice.empty())
	{
		return vecResult;
	}
	for (uint32_t index = 0; index < stockPrice[0].size(); ++index)
	{
		std::vector<int32_t> dateInfo;
		for (uint32_t stockPriceIndex = 0; stockPriceIndex < stockPrice.size(); ++stockPriceIndex)
		{
			dateInfo.push_back(stockPrice[stockPriceIndex][index]);
		}
		vecResult.push_back(dateInfo);
	}
	return vecResult;
}

int32_t Util::getOverallIndex(Overall overall)
{
	return static_cast<int32_t>(overall);
}

Overall Util::getOverallFromIndex(int32_t index)
{
	if (index >= 0 && index < static_cast<int32_t>(Overall::COUNT))
	{
		return static_cast<Overall>(index);
	}
	return Overall::COUNT;
}

int32_t Util::getObserveTimeIndex(ObserveTime observeTime)
{
	return static_cast<int32_t>(observeTime);
}

ObserveTime Util::getObserveTimeFromIndex(int32_t index)
{
	if (index >= 0 && index < static_cast<int32_t>(ObserveTime::COUNT))
	{
		return static_cast<ObserveTime>(index);
	}
	return ObserveTime::COUNT;
}

int32_t Util::getPriceMatrixIndex(ObserveTime observeTime, RangeTime rangeTime, TransType transType)
{
	int32_t observeIndex = static_cast<int32_t>(observeTime);
	int32_t rangeIndex = static_cast<int32_t>(rangeTime);
	int32_t transIndex = static_cast<int32_t>(transType);

	// 三维展平公式: index = observeIndex * (RangeTime::COUNT * TransType::COUNT) + 
	//                  rangeIndex * TransType::COUNT + transIndex
	return observeIndex * (static_cast<int32_t>(RangeTime::COUNT) * static_cast<int32_t>(TransType::COUNT)) +
		rangeIndex * static_cast<int32_t>(TransType::COUNT) +
		transIndex;
}

bool Util::getEnumsFromPriceIndex(int32_t index, ObserveTime& observeTime, RangeTime& rangeTime, TransType& transType)
{
	int32_t totalPriceElements = static_cast<int32_t>(ObserveTime::COUNT) *
		static_cast<int32_t>(RangeTime::COUNT) *
		static_cast<int32_t>(TransType::COUNT);

	if (index < 0 || index >= totalPriceElements)
	{
		observeTime = ObserveTime::COUNT;
		rangeTime = RangeTime::COUNT;
		transType = TransType::COUNT;
		return false;
	}

	int32_t transCount = static_cast<int32_t>(TransType::COUNT);
	int32_t rangeCount = static_cast<int32_t>(RangeTime::COUNT);

	// 反向计算三维坐标
	int32_t observeIndex = index / (rangeCount * transCount);
	int32_t remainder = index % (rangeCount * transCount);
	int32_t rangeIndex = remainder / transCount;
	int32_t transIndex = remainder % transCount;

	observeTime = static_cast<ObserveTime>(observeIndex);
	rangeTime = static_cast<RangeTime>(rangeIndex);
	transType = static_cast<TransType>(transIndex);

	return (observeTime != ObserveTime::COUNT) &&
		(rangeTime != RangeTime::COUNT) &&
		(transType != TransType::COUNT);
}

int32_t Util::getTotalFieldCount()
{
	// 总字段数 = Overall数量 + ObserveTime数量 + 三维矩阵数量
	return static_cast<int32_t>(Overall::COUNT) +
		static_cast<int32_t>(ObserveTime::COUNT) +
		(static_cast<int32_t>(ObserveTime::COUNT) *
		static_cast<int32_t>(RangeTime::COUNT) *
		static_cast<int32_t>(TransType::COUNT));
}
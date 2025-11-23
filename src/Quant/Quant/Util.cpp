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

ObserveTime Util::indexToTime(uint32_t dayInfoIndex)
{
	if (dayInfoIndex < (int32_t)Overall::COUNT)
	{
		return ObserveTime::COUNT;
	}
	else if (dayInfoIndex >= (int32_t)Overall::COUNT &&
		dayInfoIndex < (int32_t)Overall::COUNT + (int32_t)ObserveTime::COUNT)
	{
		return (ObserveTime)(dayInfoIndex - (int32_t)Overall::COUNT);
	}
	else
	{
		return (ObserveTime)(((int32_t)dayInfoIndex - (int32_t)Overall::COUNT - (int32_t)ObserveTime::COUNT) /
			((int32_t)RangeTime::COUNT * (int32_t)TransType::COUNT));
	}
}

int32_t Util::bestPrice(ObserveTime time, RangeTime range, TransType trans)
{
	return (int32_t)Overall::COUNT + (int32_t)ObserveTime::COUNT +
		(int32_t)time * (int32_t)RangeTime::COUNT * (int32_t)TransType::COUNT +
		(int32_t)range * (int32_t)TransType::COUNT +
		(int32_t)trans;
}

std::vector<int32_t> Util::groupToInt(const std::vector<std::string>& vecGroup, int32_t origin)
{
	std::vector<int32_t> result;
	result.reserve(vecGroup.size());
	std::transform(vecGroup.begin(), vecGroup.end(), std::back_inserter(result), [origin](const std::string& str)
	{
		if (str.size() <= (size_t)origin)
		{
			return 0;
		}
		return std::atoi(str.substr((size_t)origin, str.length() - (size_t)origin).c_str());
	});
	return result;
}

std::string Util::transYuan(int32_t amount)
{
	if (amount < 0) {
		return "0";
	}

	std::string amountStr = std::to_string(amount);

	if (amountStr.length() < 3) {
		amountStr.insert(0, 3 - amountStr.length(), '0');
	}

	amountStr.insert(amountStr.length() - 2, 1, '.');

	return amountStr;
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
		std::vector<int32_t> vecGroup = Util::groupToInt(group, 8);
		stockPrice.push_back(vecGroup);
	}
	for (uint32_t observeIndex = 0; observeIndex < (uint32_t)ObserveTime::COUNT; ++observeIndex)
	{
		std::string groupName = stock + ":observe_" + std::to_string(observeIndex);
		std::vector<std::string> group = redis.getOrderGroupByScore(groupName, beginTime, endTime)->toGroup();
		std::vector<int32_t> vecGroup = Util::groupToInt(group, 8);
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
				std::vector<int32_t> vecGroup = Util::groupToInt(group, 8);
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

std::string Util::observeTimeToString(ObserveTime time)
{
	switch (time)
	{
	case ObserveTime::TIME0930:
		return "time0930";
	case ObserveTime::TIME0950:
		return "time0950";
	case ObserveTime::TIME1000:
		return "time1000";
	case ObserveTime::TIME1010:
		return "time1010";
	case ObserveTime::TIME1020:
		return "time1020";
	case ObserveTime::TIME1030:
		return "time1030";
	case ObserveTime::TIME1040:
		return "time1040";
	case ObserveTime::TIME1050:
		return "time1050";
	case ObserveTime::TIME1100:
		return "time1100";
	case ObserveTime::TIME1110:
		return "time1110";
	case ObserveTime::TIME1320:
		return "time1320";
	case ObserveTime::TIME1330:
		return "time1330";
	case ObserveTime::TIME1340:
		return "time1340";
	case ObserveTime::TIME1350:
		return "time1350";
	case ObserveTime::TIME1400:
		return "time1400";
	case ObserveTime::TIME1410:
		return "time1410";
	case ObserveTime::TIME1420:
		return "time1420";
	case ObserveTime::TIME1430:
		return "time1430";
	case ObserveTime::TIME1440:
		return "time1440";
	default:
		break;
	}
	return "";
}

ObserveTime Util::observeStringToTime(const std::string& strTime)
{
	if (strTime == "time0930")
	{
		return ObserveTime::TIME0930;
	}
	else if (strTime == "time0950")
	{
		return ObserveTime::TIME0950;
	}
	else if (strTime == "time1000")
	{
		return ObserveTime::TIME1000;
	}
	else if (strTime == "time1010")
	{
		return ObserveTime::TIME1010;
	}
	else if (strTime == "time1020")
	{
		return ObserveTime::TIME1020;
	}
	else if (strTime == "time1030")
	{
		return ObserveTime::TIME1030;
	}
	else if (strTime == "time1040")
	{
		return ObserveTime::TIME1040;
	}
	else if (strTime == "time1050")
	{
		return ObserveTime::TIME1050;
	}
	else if (strTime == "time1100")
	{
		return ObserveTime::TIME1100;
	}
	else if (strTime == "time1110")
	{
		return ObserveTime::TIME1110;
	}
	else if (strTime == "time1320")
	{
		return ObserveTime::TIME1320;
	}
	else if (strTime == "time1330")
	{
		return ObserveTime::TIME1330;
	}
	else if (strTime == "time1340")
	{
		return ObserveTime::TIME1340;
	}
	else if (strTime == "time1350")
	{
		return ObserveTime::TIME1350;
	}
	else if (strTime == "time1400")
	{
		return ObserveTime::TIME1400;
	}
	else if (strTime == "time1410")
	{
		return ObserveTime::TIME1410;
	}
	else if (strTime == "time1420")
	{
		return ObserveTime::TIME1420;
	}
	else if (strTime == "time1430")
	{
		return ObserveTime::TIME1430;
	}
	else if (strTime == "time1440")
	{
		return ObserveTime::TIME1440;
	}
	else
	{
		return ObserveTime::COUNT;
	}
}

std::string Util::observeTimeToWatchString(ObserveTime time)
{
	switch (time)
	{
	case ObserveTime::TIME0930: return "09:30";
	case ObserveTime::TIME0950: return "09:50";
	case ObserveTime::TIME1000: return "10:00";
	case ObserveTime::TIME1010: return "10:10";
	case ObserveTime::TIME1020: return "10:20";
	case ObserveTime::TIME1030: return "10:30";
	case ObserveTime::TIME1040: return "10:40";
	case ObserveTime::TIME1050: return "10:50";
	case ObserveTime::TIME1100: return "11:00";
	case ObserveTime::TIME1110: return "11:10";
	case ObserveTime::TIME1320: return "13:20";
	case ObserveTime::TIME1330: return "13:30";
	case ObserveTime::TIME1340: return "13:40";
	case ObserveTime::TIME1350: return "13:50";
	case ObserveTime::TIME1400: return "14:00";
	case ObserveTime::TIME1410: return "14:10";
	case ObserveTime::TIME1420: return "14:20";
	case ObserveTime::TIME1430: return "14:30";
	case ObserveTime::TIME1440: return "14:40";
	default: return "收盘";
	}
}

int32_t Util::getTimeValue(ObserveTime time)
{
	switch (time)
	{
	case ObserveTime::TIME0930:
		return 930;
	case ObserveTime::TIME0950:
		return 950;
	case ObserveTime::TIME1000:
		return 1000;
	case ObserveTime::TIME1010:
		return 1010;
	case ObserveTime::TIME1020:
		return 1020;
	case ObserveTime::TIME1030:
		return 1030;
	case ObserveTime::TIME1040:
		return 1040;
	case ObserveTime::TIME1050:
		return 1050;
	case ObserveTime::TIME1100:
		return 1100;
	case ObserveTime::TIME1110:
		return 1110;
	case ObserveTime::TIME1320:
		return 1320;
	case ObserveTime::TIME1330:
		return 1330;
	case ObserveTime::TIME1340:
		return 1340;
	case ObserveTime::TIME1350:
		return 1350;
	case ObserveTime::TIME1400:
		return 1400;
	case ObserveTime::TIME1410:
		return 1410;
	case ObserveTime::TIME1420:
		return 1420;
	case ObserveTime::TIME1430:
		return 1430;
	case ObserveTime::TIME1440:
		return 1440;
	case ObserveTime::COUNT:
		return 1500;
	default:
		return -1;  // 无效时间
	}
}

int32_t Util::getRangeValue(RangeTime range)
{
	switch (range)
	{
	case RangeTime::RANGE10:
		return 10;
	case RangeTime::RANGE20:
		return 20;
	case RangeTime::RANGE30:
		return 30;
	default:
		return -1;  // 无效时间
	}
}

int32_t Util::rangeEndTime(int32_t startTime, int32_t addMinutes)
{
	// 1. 输入合法性检查：开始时间必须≥930
	if (startTime < 930) {
		return 0;
	}

	// 2. 解析开始时间为小时和分钟
	int32_t startHour = startTime / 100;
	int32_t startMinute = startTime % 100;

	// 检查开始时间格式合法性（分钟≤59）
	if (startMinute >= 60) {
		return 0;
	}

	// 3. 计算未考虑休市的原始结束时间（分钟总数）
	int32_t totalStartMinutes = startHour * 60 + startMinute;
	int32_t totalEndMinutes = totalStartMinutes + addMinutes;

	// 4. 定义关键时间节点（分钟数）
	const int32_t MORNING_OPEN = 9 * 60 + 30;    // 9:30 = 570分钟
	const int32_t MORNING_CLOSE = 11 * 60 + 30;  // 11:30 = 690分钟
	const int32_t AFTERNOON_OPEN = 13 * 60 + 0;  // 13:00 = 780分钟
	const int32_t MARKET_CLOSE = 15 * 60 + 0;    // 15:00 = 900分钟

	// 5. 处理中午休市时间（11:30-13:00）
	if (totalEndMinutes > MORNING_CLOSE) {
		// 计算上午未休市前剩余的可交易时间
		if (totalStartMinutes < MORNING_CLOSE) {
			int32_t timeUsedInMorning = MORNING_CLOSE - totalStartMinutes;
			if (addMinutes <= timeUsedInMorning) {
				// 未超过上午闭市时间，直接计算
				totalEndMinutes = totalStartMinutes + addMinutes;
			}
			else {
				// 扣除上午已用时间和休市时间（90分钟），剩余时间从下午开始算
				int32_t remainingTime = addMinutes - timeUsedInMorning - 90;
				totalEndMinutes = AFTERNOON_OPEN + remainingTime;
			}
		}
		else if (totalStartMinutes < AFTERNOON_OPEN) {
			// 开始时间在休市期间，直接从下午开盘开始计算
			totalEndMinutes = AFTERNOON_OPEN + addMinutes;
		}
		// 开始时间在下午时段则直接累加（无需处理休市）
	}

	// 6. 处理超过闭市时间的情况
	if (totalEndMinutes >= MARKET_CLOSE) {
		return 1500; // 超过15:00统一返回1500
	}

	// 7. 转换分钟数为目标时间格式（如10:00 → 1000）
	int32_t endHour = totalEndMinutes / 60;
	int32_t endMinute = totalEndMinutes % 60;
	return endHour * 100 + endMinute;
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
	return static_cast<int32_t>(Overall::COUNT) + static_cast<int32_t>(ObserveTime::COUNT) +
		observeIndex * (static_cast<int32_t>(RangeTime::COUNT) * static_cast<int32_t>(TransType::COUNT)) +
		rangeIndex * static_cast<int32_t>(TransType::COUNT) +
		transIndex;
}

bool Util::getEnumsFromPriceIndex(int32_t index, ObserveTime& observeTime, RangeTime& rangeTime, TransType& transType)
{
	int32_t totalPriceElements = getTotalFieldCount();

	int32_t origin = static_cast<int32_t>(Overall::COUNT) + static_cast<int32_t>(ObserveTime::COUNT);

	if (index < origin || index >= totalPriceElements)
	{
		observeTime = ObserveTime::COUNT;
		rangeTime = RangeTime::COUNT;
		transType = TransType::COUNT;
		return false;
	}

	index = index - origin;

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
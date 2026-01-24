#include "Util.h"
#include "Config.h"
#include <algorithm>
#include "CStringManager/CStringManagerAPI.h"
#include "RedisManager.h"
#include "Cini/CiniAPI.h"
#include "Strategy.h"
#include <unordered_map>
#include "QuantStrategyManager.h"

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
	int32_t timeValue = Util::getTimeValue(time);
	if (timeValue == 0)
	{
		return "";
	}
	std::string strTimeValue = std::to_string(timeValue);
	if (strTimeValue.size() == 3)
	{
		strTimeValue = "0" + strTimeValue;
	}
	return "time" + strTimeValue;
}

ObserveTime Util::observeStringToTime(const std::string& strTime)
{
	if (strTime.size() != 8)
	{
		return ObserveTime::COUNT;
	}
	std::string strTimeValue = strTime.substr(4, 4);
	if (strTimeValue[0] == '0')
	{
		strTimeValue = strTimeValue.substr(1, 3);
	}
	return Util::getObserveTime(atoi(strTimeValue.c_str()));
}

std::string Util::observeTimeToWatchString(ObserveTime time)
{
	int32_t timeValue = Util::getTimeValue(time);
	if (timeValue == 0)
	{
		return "异常时间点";
	}
	std::string strTimeValue = std::to_string(timeValue);
	if (strTimeValue.size() == 3)
	{
		strTimeValue = "0" + strTimeValue;
	}
	strTimeValue.insert(2, 1, ':');
	return strTimeValue;
}

int32_t Util::getTimeValue(ObserveTime time)
{
	switch (time)
	{
	case ObserveTime::TIME0930:
		return 930;
	case ObserveTime::TIME0940:
		return 940;
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
	case ObserveTime::TIME1120:
		return 1120;
	case ObserveTime::TIME1310:
		return 1310;
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
	case ObserveTime::TIME1450:
		return 1450;
	case ObserveTime::COUNT:
		return 1500;
	default:
		return 0;  // 无效时间
	}
}

ObserveTime Util::getObserveTime(int32_t timeValue)
{
	switch (timeValue)
	{
	case 930:
		return ObserveTime::TIME0930;
	case 940:
		return ObserveTime::TIME0940;
	case 950:
		return ObserveTime::TIME0950;
	case 1000:
		return ObserveTime::TIME1000;
	case 1010:
		return ObserveTime::TIME1010;
	case 1020:
		return ObserveTime::TIME1020;
	case 1030:
		return ObserveTime::TIME1030;
	case 1040:
		return ObserveTime::TIME1040;
	case 1050:
		return ObserveTime::TIME1050;
	case 1100:
		return ObserveTime::TIME1100;
	case 1110:
		return ObserveTime::TIME1110;
	case 1120:
		return ObserveTime::TIME1120;
	case 1310:
		return ObserveTime::TIME1310;
	case 1320:
		return ObserveTime::TIME1320;
	case 1330:
		return ObserveTime::TIME1330;
	case 1340:
		return ObserveTime::TIME1340;
	case 1350:
		return ObserveTime::TIME1350;
	case 1400:
		return ObserveTime::TIME1400;
	case 1410:
		return ObserveTime::TIME1410;
	case 1420:
		return ObserveTime::TIME1420;
	case 1430:
		return ObserveTime::TIME1430;
	case 1440:
		return ObserveTime::TIME1440;
	case 1450:
		return ObserveTime::TIME1450;
	case 1500:
		return ObserveTime::COUNT;
	default:
		return ObserveTime::COUNT;  // 无效时间
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

int32_t Util::rangeEndTime(int32_t startTime, int32_t addMinutes) {
	// ========== 输入验证 ==========

	// 验证起始时间是否为10的整数倍
	if (startTime % 10 != 0) {
		RCSend("错误：起始时间%d不是10的整数倍", startTime);
		return -1;
	}

	// 验证起始时间是否在930-1500范围内
	if (startTime < 930 || startTime > 1500) {
		RCSend("错误：起始时间%d不在930-1500范围内", startTime);
		return -1;
	}

	// 验证小时和分钟是否有效
	int hour = startTime / 100;
	int minute = startTime % 100;

	if (hour < 9 || hour > 15) {
		RCSend("错误：起始时间%d的小时部分%d无效", startTime, hour);
		return -1;
	}

	if (minute % 10 != 0 || minute >= 60) {
		RCSend("错误：起始时间%d的分钟部分%d无效", startTime, minute);
		return -1;
	}

	// 验证是否为异常时间值（1130-1300之间只有1130是有效的）
	if (startTime > 1130 && startTime < 1300) {
		RCSend("错误：起始时间%d是异常值（应在1130-1300之间跳过）", startTime);
		return -1;
	}

	// 验证午休时间后的异常值
	if (startTime > 1300 && startTime < 1310) {
		RCSend("错误：起始时间%d是异常值（午休后应从1300开始）", startTime);
		return -1;
	}

	// 验证增加时间是否合法
	if (addMinutes < 0) {
		RCSend("错误：增加时间%d不能为负数", addMinutes);
		return -1;
	}

	if (addMinutes % 10 != 0) {
		RCSend("错误：增加时间%d不是10的整数倍", addMinutes);
		return -1;
	}

	// 特殊处理：如果起始时间已经是1500，直接返回1500
	if (startTime == 1500) {
		return 1500;
	}

	// ========== 时间转换逻辑（内联实现） ==========

	// 将时间值转换为分钟数的内联函数
	int startHour = startTime / 100;
	int startMinute = startTime % 100;
	int currentMinutes = startHour * 60 + startMinute;
	int remainingMinutes = addMinutes;

	// 定义交易时间段（分钟数表示）
	const int MORNING_START = 570;   // 9:30
	const int MORNING_END = 690;     // 11:30
	const int AFTERNOON_START = 780; // 13:00
	const int AFTERNOON_END = 900;   // 15:00

	// ========== 时间计算逻辑 ==========

	// 逐步增加时间，考虑午休
	while (remainingMinutes > 0) {
		// 确定当前所在的时间段
		if (currentMinutes < MORNING_END) {
			// 上午段
			int minutesToMorningEnd = MORNING_END - currentMinutes;

			if (remainingMinutes <= minutesToMorningEnd) {
				// 剩余时间在上午段内完成
				currentMinutes += remainingMinutes;
				remainingMinutes = 0;
			}
			else {
				// 跳过午休，进入下午段
				currentMinutes = AFTERNOON_START;
				remainingMinutes -= minutesToMorningEnd;
			}
		}
		else if (currentMinutes < AFTERNOON_START) {
			// 在午休期间（理论上不会进入这里，因为起始时间是有效的交易时间）
			// 直接跳到下午开盘
			currentMinutes = AFTERNOON_START;
		}
		else {
			// 下午段
			int minutesToAfternoonEnd = AFTERNOON_END - currentMinutes;

			if (remainingMinutes <= minutesToAfternoonEnd) {
				// 剩余时间在下午段内完成
				currentMinutes += remainingMinutes;
				remainingMinutes = 0;
			}
			else {
				// 超过下午交易时间，返回1500
				return 1500;
			}
		}
	}

	// ========== 结果转换和验证 ==========

	// 将分钟数转换回时间值
	int endHour = currentMinutes / 60;
	int endMinute = currentMinutes % 60;
	int endTime = endHour * 100 + endMinute;

	// 确保不超过1500
	if (endTime > 1500) {
		return 1500;
	}

	// 确保结果是10的整数倍（理论上应该总是满足）
	if (endTime % 10 != 0) {
		RCSend("警告：计算结果%d不是10的整数倍，进行修正", endTime);
		endTime = (endTime / 10) * 10;
	}

	return endTime;
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

uint32_t Util::calcFutureEndTime(uint32_t beginTime, uint32_t endTime)
{
	Cini ini(g_config.m_currentExePath + "600975.ini", true);
	std::vector<std::string> allSection = ini.getAllSection();
	std::vector<int32_t> tradingDays = Util::groupToInt(allSection, 4);
	auto itBegin = std::find(tradingDays.begin(), tradingDays.end(), (int32_t)beginTime);
	auto itEnd = std::find(tradingDays.begin(), tradingDays.end(), (int32_t)endTime);
	if (itBegin == tradingDays.end() || itEnd == tradingDays.end())
	{
		return endTime;
	}
	uint32_t beginIndex = (uint32_t)(itBegin - tradingDays.begin());
	uint32_t endIndex = (uint32_t)(itEnd - tradingDays.begin());
	uint32_t nextEndIndex = endIndex + (endIndex - beginIndex);
	if (nextEndIndex >= tradingDays.size())
	{
		return endTime;
	}
	return (uint32_t)(tradingDays[nextEndIndex]);
}

uint32_t Util::calcHistoryBeginTime(uint32_t beginTime, uint32_t endTime)
{
	Cini ini(g_config.m_currentExePath + "600975.ini", true);
	std::vector<std::string> allSection = ini.getAllSection();
	std::vector<int32_t> tradingDays = Util::groupToInt(allSection, 4);
	auto itBegin = std::find(tradingDays.begin(), tradingDays.end(), (int32_t)beginTime);
	auto itEnd = std::find(tradingDays.begin(), tradingDays.end(), (int32_t)endTime);
	if (itBegin == tradingDays.end() || itEnd == tradingDays.end())
	{
		return endTime;
	}
	uint32_t beginIndex = (uint32_t)(itBegin - tradingDays.begin());
	uint32_t endIndex = (uint32_t)(itEnd - tradingDays.begin());
	uint32_t preBeginIndex = beginIndex - (endIndex - beginIndex);
	if (preBeginIndex < 0)
	{
		return beginTime;
	}
	return (uint32_t)(tradingDays[preBeginIndex]);
}

std::vector<std::vector<int32_t>> Util::combinatoricsToAllParam(const std::vector<std::vector<int32_t>>& allParam)
{
	std::vector<std::vector<int32_t>> result;

	// 处理空输入的情况
	if (allParam.empty())
	{
		return result;
	}

	// 计算总组合数：各层元素数量的乘积
	size_t total = 1;
	for (size_t i = 0; i < allParam.size(); ++i)
	{
		// 如果任何一层为空，总组合数为0
		if (allParam[i].empty())
		{
			return result;
		}
		total *= allParam[i].size();
	}

	// 生成所有组合
	for (size_t i = 0; i < total; ++i)
	{
		std::vector<int32_t> combination;
		size_t remainder = i;

		// 为每层选择一个元素
		for (size_t j = 0; j < allParam.size(); ++j)
		{
			const std::vector<int32_t>& layer = allParam[j];
			size_t layerSize = layer.size();
			// 计算当前层的索引
			size_t index = remainder % layerSize;
			// 更新余数用于计算下一层
			remainder = remainder / layerSize;
			// 添加当前层选中的元素
			combination.push_back(layer[index]);
		}

		result.push_back(combination);
	}

	return result;
}

std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> Util::synthesisMap(
	const std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>& detectMap,
	const std::map<int32_t, std::vector<std::shared_ptr<StrategyResult>>>& verifyMap)
{
	std::unordered_map<std::string, std::shared_ptr<StrategyResult>> paramVerifyMap;
	for (auto itVerify = verifyMap.begin(); itVerify != verifyMap.end(); ++itVerify)
	{
		const std::vector<std::shared_ptr<StrategyResult>>& vecVerifyStrategyResult = itVerify->second;
		size_t verifyStrategyIndex = -1;
		while (verifyStrategyIndex++ != vecVerifyStrategyResult.size() - 1)
		{
			std::shared_ptr<StrategyResult> spVerifyStrategyResult = vecVerifyStrategyResult[verifyStrategyIndex];
			std::shared_ptr<Strategy> spStrategy =
				QuantStrategyManager::instance().createStrategy(spVerifyStrategyResult->strategyMode);
			std::string paramStr = spStrategy->describeParam(spVerifyStrategyResult->params);
			paramVerifyMap[paramStr] = spVerifyStrategyResult;
		}
	}

	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> result = detectMap;
	for (auto itMap = result.begin(); itMap != result.end(); ++itMap)
	{
		std::vector<std::vector<std::shared_ptr<StrategyResult>>>& resultTable = itMap->second;
		for (size_t strategyIndex = 0; strategyIndex < resultTable.size(); ++strategyIndex)
		{
			std::vector<std::shared_ptr<StrategyResult>>& strategyTable = resultTable[strategyIndex];
			if (strategyTable.empty())
			{
				RCSend("strategyTable empty");
				return result;
			}

			std::shared_ptr<Strategy> spStrategy =
				QuantStrategyManager::instance().createStrategy(strategyTable[0]->strategyMode);
			std::string targetParamStr = spStrategy->describeParam(strategyTable[0]->params);

			auto itVerify = paramVerifyMap.find(targetParamStr);
			if (itVerify == paramVerifyMap.end())
			{
				RCSend("can not find verify strategy result");
				return result;
			}

			strategyTable.push_back(itVerify->second);

			//std::shared_ptr<StrategyResult> spTargetStrategyResult;
			//for (auto itVerify = verifyMap.begin(); itVerify != verifyMap.end(); ++itVerify)
			//{
			//	const std::vector<std::shared_ptr<StrategyResult>>& vecVerifyStrategyResult = itVerify->second;
			//	size_t verifyStrategyIndex = -1;
			//	while (verifyStrategyIndex++ != vecVerifyStrategyResult.size() - 1)
			//	{
			//		std::shared_ptr<StrategyResult> spVerifyStrategyResult = vecVerifyStrategyResult[verifyStrategyIndex];
			//		if (spVerifyStrategyResult->params == strategyTable[0]->params)
			//		{
			//			spTargetStrategyResult = spVerifyStrategyResult;
			//			break;
			//		}
			//	}
			//}
			//if (spTargetStrategyResult == nullptr)
			//{
			//	RCSend("can not find verify strategy result");
			//	return result;
			//}
			//strategyTable.push_back(spTargetStrategyResult);
		}
	}
	return result;
}

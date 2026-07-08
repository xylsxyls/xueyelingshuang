#include "AnalyzeTask.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "Config.h"
#include "Util.h"
#include "CSystem/CSystemAPI.h"

AnalyzeTask::AnalyzeTask():
m_exit(false),
m_queue(nullptr)
{

}

void AnalyzeTask::DoTask()
{
	if (m_queue == nullptr)
	{
		return;
	}
	std::map<int, std::vector<PriceInfo>> mapPriceInfo;
	Ctxt txt(m_path);
	txt.LoadTxt(Ctxt::Load::SPLIT, "\t");
	for (size_t lineIndex = 0; lineIndex < txt.m_vectxt.size(); ++lineIndex)
	{
		if (lineIndex <= 1)
		{
			continue;
		}
		const std::vector<std::string>& line = txt.m_vectxt[lineIndex];
		if (line.size() < 5)
		{
			continue;
		}
		else if (line.size() <= 6)
		{
			std::string strTime = CStringManager::Replace(line[0].c_str(), ":", "");
			int time = atoi(strTime.c_str());
			PriceInfo info;
			std::string strPrice = CStringManager::Replace(line[1].c_str(), " ", "");
			strPrice = CStringManager::Replace(strPrice.c_str(), ".", "");
			info.price = atoi(strPrice.c_str());
			std::string strShares = CStringManager::Replace(line[2].c_str(), " ", "");
			info.shares = atoi(strShares.c_str());
			if (line.size() == 6 && !line[4].empty())
			{
				info.buy_sell = line[4][0];
			}
			mapPriceInfo[time].push_back(info);
		}
		else
		{
			RCSend("analyze error, file = %s, line = %d", m_path.c_str(), (int32_t)lineIndex + 1);
		}
	}
	std::vector<int> dayInfo = toDayInfo(m_path, mapPriceInfo);
	m_queue->push(dayInfo);
	RCSend("end = %s", m_path.c_str());
}

void AnalyzeTask::StopTask()
{
	m_exit = true;
}

void AnalyzeTask::setParam(const std::string& path, LockFreeQueue<std::vector<int>>* queue)
{
	m_path = path;
	m_queue = queue;
}

std::vector<int> AnalyzeTask::toDayInfo(const std::string& path, const std::map<int, std::vector<PriceInfo>>& priceInfo)
{
	std::vector<int> result;
	result.push_back(atoi(CSystem::GetName(m_path, 3).substr(0, 8).c_str()));
	int32_t open = 0;
	auto it = priceInfo.find(925);
	if (it != priceInfo.end() && !it->second.empty())
	{
		open = it->second[0].price;
	}
	result.push_back(open);
	int32_t close = 0;
	it = priceInfo.find(1500);
	if (it != priceInfo.end() && !it->second.empty())
	{
		close = it->second[0].price;
	}
	result.push_back(close);
	for (int32_t time = (int32_t)ObserveTime::TIME0930; time < (int32_t)ObserveTime::COUNT; ++time)
	{
		result.push_back(currentPrice(Util::getTimeValue((ObserveTime)time), priceInfo));
	}
	for (int32_t time = (int32_t)ObserveTime::TIME0930; time < (int32_t)ObserveTime::COUNT; ++time)
	{
		int32_t realTime = Util::getTimeValue((ObserveTime)time);
		std::pair<int32_t, int32_t> direct = directPrice(realTime, priceInfo);
		result.push_back(direct.first);
		result.push_back(direct.second);
		for (int32_t range = (int32_t)RangeTime::RANGE10; range < (int32_t)RangeTime::RANGENEXT; ++range)
		{
			int32_t rangeTime = Util::getRangeValue((RangeTime)range);
			std::pair<int32_t, int32_t> best = bestPrice(calcTimeInfo(realTime,
				Util::rangeEndTime(realTime, rangeTime), priceInfo));
			result.push_back(best.first);
			result.push_back(best.second);
		}
		std::pair<int32_t, int32_t> best = bestPrice(calcTimeInfo(realTime,
			Util::getTimeValue((ObserveTime)(time + 1)), priceInfo));
		result.push_back(best.first);
		result.push_back(best.second);
	}
	
	return result;
}

int32_t AnalyzeTask::currentPrice(int32_t time, const std::map<int, std::vector<PriceInfo>>& priceInfo)
{
	// 1. 查找当前时间对应的记录
	auto it = priceInfo.find(time);
	if (it != priceInfo.end())
	{
		// 存在当前时间节点，检查vector是否非空
		const auto& vec = it->second;
		if (!vec.empty())
		{
			// 取vector最后一个有效记录（buy_sell为'S'）
			int32_t vecCount = (int32_t)vec.size();
			while (vecCount-- != 0)
			{
				const auto& lastInfo = vec[vecCount];
				if (lastInfo.buy_sell != '\0')
				{
					return lastInfo.price + (int32_t)(lastInfo.buy_sell == 'S');
				}
			}
		}
	}

	// 2. 当前时间无记录，查找后续最近的时间节点（时间 > 当前time）
	it = priceInfo.upper_bound(time); // upper_bound返回第一个大于time的迭代器
	if (it != priceInfo.end())
	{
		// 存在后续时间节点，检查vector是否非空
		const auto& vec = it->second;
		if (!vec.empty())
		{
			// 取vector第一个有效记录（buy_sell为'S'）
			int32_t vecIndex = -1;
			while (vecIndex++ != vec.size() - 1)
			{
				const auto& firstInfo = vec[vecIndex];
				if (firstInfo.buy_sell != '\0')
				{
					return firstInfo.price + (int32_t)(firstInfo.buy_sell == 'S');
				}
			}
		}
	}

	// 3. 无任何有效记录，返回0
	RCSend("analyze error");
	return 0;
}

std::pair<int32_t, int32_t> AnalyzeTask::directPrice(
	int32_t time,
	const std::map<int32_t, std::vector<PriceInfo>>& priceInfo)
{
	int32_t buyPrice = 0;
	int32_t sellPrice = 0;

	// 计算买入价格（关注卖单'S'）
	{
		// 基准价格：当前卖1价和买1价（卖1价-1）
		int32_t currentAsk = currentPrice(time, priceInfo);
		int32_t currentBid = currentAsk - 1;

		int32_t accumulatedShares = 0;
		int32_t ignored = g_config.m_ignoreTrans;
		bool buyFound = false;

		auto timeIt = priceInfo.upper_bound(time);
		for (; timeIt != priceInfo.end() && !buyFound; ++timeIt)
		{
			for (const auto& info : timeIt->second)
			{
				// 忽略无效记录
				if (info.buy_sell == '\0')
				{
					continue;
				}

				// 只处理卖单
				if (info.buy_sell != 'S')
				{
					continue;
				}

				// 忽略初始交易
				if (ignored < g_config.m_ignoreTrans)
				{
					ignored++;
					continue;
				}

				// 价格拉升判断
				if (info.price >= currentAsk)
				{
					accumulatedShares = 0;
					ignored = 1;
					++currentBid;
					++currentAsk;
					continue;
				}

				// 累计有效手数
				if (info.price <= currentBid)
				{
					accumulatedShares += info.shares;

					// 达标时记录价格并退出循环
					if (accumulatedShares >= g_config.m_normalShares)
					{
						buyFound = true;
						break;
					}
				}
			}
		}
		buyPrice = currentBid;
	}

	// 计算卖出价格（关注买单'B'）
	{
		// 基准价格：当前卖1价和买1价（卖1价-1）
		int32_t currentAsk = currentPrice(time, priceInfo);
		int32_t currentBid = currentAsk - 1;

		int32_t accumulatedShares = 0;
		bool priceDecreased = false;
		int32_t ignored = g_config.m_ignoreTrans;
		bool sellFound = false;

		auto timeIt = priceInfo.upper_bound(time);
		for (; timeIt != priceInfo.end() && !sellFound; ++timeIt)
		{
			for (const auto& info : timeIt->second)
			{
				// 忽略无效记录
				if (info.buy_sell == '\0')
				{
					continue;
				}

				// 只处理买单
				if (info.buy_sell != 'B')
				{
					continue;
				}

				// 忽略初始交易
				if (ignored < g_config.m_ignoreTrans)
				{
					ignored++;
					continue;
				}

				// 价格下跌判断
				if (info.price <= currentBid)
				{
					accumulatedShares = 0;
					ignored = 1;
					--currentBid;
					--currentAsk;
					continue;
				}

				// 累计有效手数
				if (info.price >= currentAsk)
				{
					accumulatedShares += info.shares;

					// 达标时记录价格并退出循环
					if (accumulatedShares >= g_config.m_normalShares)
					{
						sellFound = true;
						break;
					}
				}
			}
		}
		sellPrice = currentAsk;
	}

	return{ buyPrice, sellPrice };
}

std::pair<int32_t, int32_t> AnalyzeTask::bestPrice(const std::map<int32_t, std::pair<int32_t, int32_t>>& timeInfo)
{
	if (timeInfo.empty())
	{
		RCSend("bid ask empty");
		return{ 0, 0 }; // 空数据返回默认值，可根据实际需求调整
	}

	// 计算最佳买价：从最低价（低到高）累加
	int32_t bestBid = 0;
	int32_t totalBuySum = 0;
	auto it = timeInfo.begin();
	for (; it != timeInfo.end(); ++it)
	{
		// 累加当前价格的买卖总手数（买+卖）
		totalBuySum += it->second.first + it->second.second;
		if (totalBuySum >= g_config.m_normalShares)
		{
			bestBid = it->first;
			break;
		}
	}
	// 若总和不足，根据最高价的买卖标识调整
	if (totalBuySum < g_config.m_normalShares)
    {
		auto highestIt = timeInfo.rbegin();
		// 最高价为买价
		bestBid = highestIt->first;
	}

	// 计算最佳卖价：从最高价（高到低）累加（使用反向迭代器）
	int32_t bestAsk = 0;
	int32_t totalSellSum = 0;
	auto reverseIt = timeInfo.rbegin();
	for (; reverseIt != timeInfo.rend(); ++reverseIt)
	{
		// 累加当前价格的买卖总手数（买+卖）
		totalSellSum += reverseIt->second.first + reverseIt->second.second;
		if (totalSellSum >= g_config.m_normalShares)
		{
			bestAsk = reverseIt->first;
			break;
		}
	}
	// 若总和不足，根据最低价的买卖标识调整
	if (totalSellSum < g_config.m_normalShares)
	{
		auto lowestIt = timeInfo.begin();
		// 最低价为卖价
		bestAsk = lowestIt->first;
	}

	if (bestBid == 0 || bestAsk == 0)
	{
		RCSend("bid ask 0");
	}
	return{ bestBid, bestAsk };
}

std::map<int32_t, std::pair<int32_t, int32_t>> AnalyzeTask::calcTimeInfo(
	int32_t timeBegin,
	int32_t timeEnd,
	const std::map<int32_t, std::vector<PriceInfo>>& priceInfo
	)
{
	std::map<int32_t, std::pair<int32_t, int32_t>> result;

	// 空数据直接返回
	if (priceInfo.empty())
	{
		return result;
	}

	// 找到起始时间的迭代器（利用map有序性，从timeBegin开始遍历）
	auto timeIt = priceInfo.upper_bound(timeBegin);
	// 如果起始时间不存在，找第一个大于timeBegin的位置
	if (timeIt == priceInfo.end())
	{
		return result;
	}

	// 遍历时间范围内的所有节点（不包含timeBegin包含timeEnd）
	while (timeIt != priceInfo.end())
	{
		int32_t currentTime = timeIt->first;
		// 超出结束时间则终止遍历
		if (currentTime > timeEnd)
		{
			break;
		}

		// 遍历当前时间点的所有PriceInfo（用迭代器，不用范围for）
		const std::vector<PriceInfo>& infoList = timeIt->second;
		auto infoIt = infoList.begin();
		while (infoIt != infoList.end())
		{
			// 忽略buy_sell为0的无效数据
			if (infoIt->buy_sell == 0)
			{
				++infoIt;
				continue;
			}

			// 获取当前价格的统计项（不存在则自动初始化）
			auto& priceStat = result[infoIt->price];

			// 累加对应类型的手数
			if (infoIt->buy_sell == 'B')
			{
				priceStat.first += infoIt->shares;  // B手数存左边
			}
			else if (infoIt->buy_sell == 'S')
			{
				priceStat.second += infoIt->shares;  // S手数存右边
			}

			++infoIt;
		}

		// 移动到下一个时间点
		++timeIt;
	}

	return result;
}
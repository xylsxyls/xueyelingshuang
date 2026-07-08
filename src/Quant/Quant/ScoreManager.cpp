#include "ScoreManager.h"
#include "Util.h"
#include "CompetitionManager.h"
#include "VerifyManager.h"
#include <algorithm>
#include "QuantStrategyManager.h"

ScoreManager::ScoreManager()
{

}

ScoreManager& ScoreManager::instance()
{
	static ScoreManager s_scoreManager;
	return s_scoreManager;
}

std::pair<StrategyMode, std::vector<int32_t>> ScoreManager::bestStrategy(uint32_t time)
{
	uint32_t profitBeginTime = Util::calcHistoryTime(time, g_config.m_runDayCount);
	int32_t profitEndTime = time;
	std::string stock = g_config.m_stock;

	// 创建市场数据
	auto marketData = std::make_shared<Market>();
	marketData->init(g_config.m_allBeginTime, g_config.m_allEndTime);
	marketData->addStock(stock);

	// 创建竞赛配置
	CompetitionConfig config;
	config.beginTime = profitBeginTime;
	config.endTime = profitEndTime;
	config.stocks = { stock };
	config.marketData = marketData;
	config.initialFund = g_config.m_initialFund; // 100万初始资金

	// 挂卖价时间点
	// 直卖价时间点
	// 挂买价时间点
	// 直买价时间点
	// 观察价A时间点
	// 观察价B时间点
	// 观察价B-A大于等于多少时当天直买，第二天早上不卖
	// 观察价B-A小于等于多少时当天不买，剩余的部分，当天直买，第二天早上卖
	// 当天早上卖出后反追差价
	std::vector<std::vector<int32_t>> params =
	{
		{
			//(int32_t)ObserveTime::TIME0940, (int32_t)ObserveTime::TIME0950,
			(int32_t)ObserveTime::TIME1000, (int32_t)ObserveTime::TIME1010,
			(int32_t)ObserveTime::TIME1020, (int32_t)ObserveTime::TIME1030, (int32_t)ObserveTime::TIME1040,
			(int32_t)ObserveTime::TIME1050, (int32_t)ObserveTime::TIME1100, (int32_t)ObserveTime::TIME1110,
			//(int32_t)ObserveTime::TIME1120,
		},
		{
			//(int32_t)ObserveTime::TIME0940, (int32_t)ObserveTime::TIME0950,
			(int32_t)ObserveTime::TIME1000, (int32_t)ObserveTime::TIME1010,
			(int32_t)ObserveTime::TIME1020, (int32_t)ObserveTime::TIME1030, (int32_t)ObserveTime::TIME1040,
			(int32_t)ObserveTime::TIME1050, (int32_t)ObserveTime::TIME1100, (int32_t)ObserveTime::TIME1110,
			//(int32_t)ObserveTime::TIME1120,
		},
		{
			//(int32_t)ObserveTime::TIME1310, (int32_t)ObserveTime::TIME1320,
			(int32_t)ObserveTime::TIME1330, (int32_t)ObserveTime::TIME1340,
			(int32_t)ObserveTime::TIME1350, (int32_t)ObserveTime::TIME1400, (int32_t)ObserveTime::TIME1410,
			(int32_t)ObserveTime::TIME1420, (int32_t)ObserveTime::TIME1430, (int32_t)ObserveTime::TIME1440,
			//(int32_t)ObserveTime::TIME1450,
		},
		{
			//(int32_t)ObserveTime::TIME1310, (int32_t)ObserveTime::TIME1320,
			(int32_t)ObserveTime::TIME1330, (int32_t)ObserveTime::TIME1340,
			(int32_t)ObserveTime::TIME1350, (int32_t)ObserveTime::TIME1400, (int32_t)ObserveTime::TIME1410,
			(int32_t)ObserveTime::TIME1420, (int32_t)ObserveTime::TIME1430, (int32_t)ObserveTime::TIME1440,
			//(int32_t)ObserveTime::TIME1450,
		},
		{
			//(int32_t)ObserveTime::TIME0940, (int32_t)ObserveTime::TIME0950,
			(int32_t)ObserveTime::TIME1000, (int32_t)ObserveTime::TIME1010,
			(int32_t)ObserveTime::TIME1020, (int32_t)ObserveTime::TIME1030, (int32_t)ObserveTime::TIME1040,
			(int32_t)ObserveTime::TIME1050, (int32_t)ObserveTime::TIME1100, (int32_t)ObserveTime::TIME1110,
			(int32_t)ObserveTime::TIME1120,
		},
		{
			(int32_t)ObserveTime::TIME1310, (int32_t)ObserveTime::TIME1320,
			(int32_t)ObserveTime::TIME1330, (int32_t)ObserveTime::TIME1340,
			(int32_t)ObserveTime::TIME1350, (int32_t)ObserveTime::TIME1400, (int32_t)ObserveTime::TIME1410,
			//(int32_t)ObserveTime::TIME1420, (int32_t)ObserveTime::TIME1430, (int32_t)ObserveTime::TIME1440,
			//(int32_t)ObserveTime::TIME1450,
		},
		{ 0 },
		{ -2, -3 },
		{ 12, 14, 16 },
		{ 12, 14, 16 },
		{ 0, 1 }
	};
	config.allParam = Util::combinatoricsToAllParam(params);

	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> historyMap =
		VerifyManager::instance().verifyHistory(StrategyMode::WAVE, 2, config);

	RCSend("开始筛选入围名单...");

	// 取出前30%作为入围名单
	std::vector<std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>> vecFrontStrategyResult;
	size_t useCount = historyMap.size() - historyMap.size() / 100 * 70;
	for (auto it = historyMap.rbegin(); useCount != 0; ++it, --useCount)
	{
		for (size_t index = 0; index < it->second.size(); ++index)
		{
			const std::vector<std::shared_ptr<StrategyResult>>& timeStrategyResult = it->second[index];
			bool isUseless = false;
			for (size_t timeIndex = 0; timeIndex < timeStrategyResult.size(); ++timeIndex)
			{
				if (it->second[index][timeIndex]->m_annualTReturn > 1)
				{
					isUseless = true;
				}
			}
			if (isUseless)
			{
				continue;
			}
			vecFrontStrategyResult.push_back(
				std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>(it->second[index], 0));
		}
	}
	RCSend("vecFrontStrategyResult.size = %d", (int32_t)vecFrontStrategyResult.size());

	std::vector<std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>> vecBackStrategyResult;
	useCount = historyMap.size() - historyMap.size() / 100 * 70;
	for (auto it = historyMap.begin(); useCount != 0; ++it, --useCount)
	{
		for (size_t index = 0; index < it->second.size(); ++index)
		{
			vecBackStrategyResult.push_back(
				std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>(it->second[index], 0));
		}
	}

	RCSend("入围名单筛选完成");
	RCSend("开始打分");
	
	// 前两次历史收益年化率差值小于5%，得10分，5%-10%得9分，超过50%不得分
	annualT(vecFrontStrategyResult, 2, 100);
	//annualT(vecBackStrategyResult, 2, 50);
	//annualTDiff(vecBackStrategyResult, 6.18);

	auto vecStrategyResult = vecFrontStrategyResult;
	//vecStrategyResult.insert(vecStrategyResult.end(),
	//	vecBackStrategyResult.begin(), vecBackStrategyResult.end());
	desc(vecStrategyResult);

	//print(vecStrategyResult);

	//异常交易评分，小于5%，得10分，5%-10%得9分，超过50%不得分
	//abnormal(vecStrategyResult);

	//11:00, 11:00, 14:10, 14:40, 11:20, 13:40, 0, -2, 12, 12, 1
	std::vector<int32_t> param =
	{
		(int32_t)ObserveTime::TIME1100,
		(int32_t)ObserveTime::TIME1100,
		(int32_t)ObserveTime::TIME1410,
		(int32_t)ObserveTime::TIME1440,
		(int32_t)ObserveTime::TIME1120,
		(int32_t)ObserveTime::TIME1340,
		0,
		-2,
		12,
		12,
		1
	};
	//return std::pair<StrategyMode, std::vector<int32_t>>(StrategyMode::WAVE, param);
	return std::pair<StrategyMode, std::vector<int32_t>>(StrategyMode::WAVE,
		vecStrategyResult.empty() ? std::vector<int32_t>() : vecStrategyResult[0].first[0]->m_params);
}

// ==================== 年化收益率差值评分 ====================
void ScoreManager::annualTDiff(
	std::vector<std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>>& vecStrategyResult, double highScore)
{
	RCSend("开始评价年化T差值...");
	double maxScore = 0;
	std::vector<double> tmpScore;
	tmpScore.resize(vecStrategyResult.size(), 0);
	std::vector<std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>> tmpResult = vecStrategyResult;
	// 为每个策略计算分数
	for (size_t index = 0; index < vecStrategyResult.size(); ++index)
	{
		const auto& results = vecStrategyResult[index].first;   // 各周期结果，索引0=当前，1=上周期，2=上上周期...
		double score = 0;

		// 至少需要三个周期才能计算前两次历史收益差值
		if (results.size() >= 3)
		{
			// 获取上周期和上上周期的年化收益率
			const BigNumber& prev0 = results[0]->m_annualTReturn;   // 本周期
			const BigNumber& prev1 = results[1]->m_annualTReturn;   // 上周期
			const BigNumber& prev2 = results[2]->m_annualTReturn;   // 上上周期

			// 计算绝对值差值
			BigNumber diff1 = prev0 - prev1;
			if (diff1 < 0)
			{
				diff1 = diff1 * -1;   // 取绝对值
			}
			if (diff1 == 0)
			{
				score = -1;
			}
			score = (BigNumber(1.0) / diff1).toDouble();
			maxScore = (std::max)(score, maxScore);
		}
		else
		{
			// 历史周期不足，分数为0
			score = 0;
		}

		// 将分数赋给当前pair的second
		tmpScore[index] = score;
	}

	for (auto& entry : tmpScore)
	{
		if (entry == -1)
		{
			entry = highScore;
		}
		else
		{
			entry = entry / maxScore * highScore;
		}
	}

	for (size_t index = 0; index < vecStrategyResult.size(); ++index)
	{
		vecStrategyResult[index].second += tmpScore[index];
	}

	desc(vecStrategyResult);
	RCSend("年化T差值评价完成");
}

void ScoreManager::annualT(
	std::vector<std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>>& vecStrategyResult,
	uint32_t count, double highScore)
{
	RCSend("开始评价年化T总和%u...", count);
	if (vecStrategyResult.empty())
	{
		RCSend("年化T总和%u数量为0", count);
		return;
	}
	std::vector<double> vecMaxScore;
	vecMaxScore.resize(count, 0);
	std::vector<std::vector<double>> vecTmpScore;
	//tmpScore.resize(vecStrategyResult.size(), 0);
	// 为每个策略计算分数
	for (size_t index = 0; index < vecStrategyResult.size(); ++index)
	{
		const auto& results = vecStrategyResult[index].first;   // 各周期结果，索引0=当前，1=上周期，2=上上周期...
		std::vector<double> score;
		score.resize(count, 0);

		// 至少需要三个周期才能计算前两次历史收益差值
		if (results.size() < count)
		{
			// 历史周期不足，分数为0
			continue;
		}
		// 获取上周期和上上周期的年化收益率
		uint32_t timeIndex = count;
		while (timeIndex-- != 0)
		{
			score[timeIndex] = results[timeIndex]->m_annualTReturn.toDouble();
			vecMaxScore[timeIndex] = (std::max)(score[timeIndex], vecMaxScore[timeIndex]);
		}

		// 将分数赋给当前pair的second
		vecTmpScore.push_back(score);
	}

	double maxScore = 0;
	for (auto& entry : vecMaxScore)
	{
		maxScore = (std::max)(entry, maxScore);
	}

	RCSend("maxScore = %.2lf", maxScore);

	for (size_t index = 0; index < vecTmpScore.size(); ++index)
	{
		std::vector<double>& tmpScore = vecTmpScore[index];
		for (size_t timeIndex = 0; timeIndex < tmpScore.size(); ++timeIndex)
		{
			tmpScore[timeIndex] = tmpScore[timeIndex] / maxScore * highScore;
		}
	}

	for (size_t index = 0; index < vecStrategyResult.size(); ++index)
	{
		double score = 0;
		const std::vector<double>& tmpScore = vecTmpScore[index];
		for (size_t timeIndex = 0; timeIndex < tmpScore.size(); ++timeIndex)
		{
			score += tmpScore[timeIndex];
		}
		vecStrategyResult[index].second += (score / count);
	}

	desc(vecStrategyResult);
	RCSend("年化T总和%u评价完成", count);
}

//void ScoreManager::annualT(
//	std::vector<std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>>& vecStrategyResult,
//	uint32_t count, double highScore)
//{
//	RCSend("开始评价年化T总和%u...", count);
//	double maxScore = 0;
//	std::vector<double> tmpScore;
//	tmpScore.resize(vecStrategyResult.size(), 0);
//	// 为每个策略计算分数
//	for (size_t index = 0; index < vecStrategyResult.size(); ++index)
//	{
//		const auto& results = vecStrategyResult[index].first;   // 各周期结果，索引0=当前，1=上周期，2=上上周期...
//		double score = 0;
//
//		// 至少需要三个周期才能计算前两次历史收益差值
//		if (results.size() >= count)
//		{
//			// 获取上周期和上上周期的年化收益率
//			uint32_t timeIndex = count;
//			while (timeIndex-- != 0)
//			{
//				score += results[timeIndex]->m_annualTReturn.toDouble();
//			}
//			maxScore = (std::max)(score, maxScore);
//		}
//		else
//		{
//			// 历史周期不足，分数为0
//			score = 0;
//		}
//
//		// 将分数赋给当前pair的second
//		tmpScore[index] = score;
//	}
//
//	for (auto& entry : tmpScore)
//	{
//		entry = entry / maxScore * highScore;
//	}
//
//	for (size_t index = 0; index < vecStrategyResult.size(); ++index)
//	{
//		vecStrategyResult[index].second += tmpScore[index];
//	}
//
//	desc(vecStrategyResult);
//	RCSend("年化T总和%u评价完成", count);
//}

// ==================== 异常交易百分比评分 ====================
void ScoreManager::abnormal(
	std::vector<std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>>& vecStrategyResult,
	uint32_t count, double highScore)
{
	RCSend("开始评价异常交易...");
	// 为每个策略计算分数
	for (size_t index = 0; index < vecStrategyResult.size(); ++index)
	{
		const std::vector<std::shared_ptr<StrategyResult>>& results = vecStrategyResult[index].first;
		double score = 0;

		for (size_t timeIndex = 0; timeIndex < count; ++timeIndex)
		{
			if (results.empty() || results[0] == nullptr)
			{
				continue;
			}
			const auto& tradeCount = results[timeIndex]->m_tradeCount;   // 固定长度为4
			if (tradeCount.size() != 4)
			{
				continue;
			}
			uint32_t buyCount = tradeCount[0];   // 买
			uint32_t sellCount = tradeCount[1];   // 卖
			uint32_t chaseCount = tradeCount[2];   // 反追
			uint32_t cutCount = tradeCount[3];   // 割肉

			uint32_t totalTrades = buyCount + sellCount + chaseCount + cutCount;
			if (totalTrades == 0)
			{
				continue;
			}
			uint32_t abnormalTrades = chaseCount + cutCount;
			// 计算异常交易百分比
			BigNumber percentage = BigNumber(static_cast<double>(abnormalTrades) / totalTrades);
			score += ((percentage - 1) / BigNumber(-1.0) * highScore).toDouble();
		}
		vecStrategyResult[index].second += score / count;
	}

	desc(vecStrategyResult);
	RCSend("异常交易评价完成");
}

void ScoreManager::desc(std::vector<std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>>& vecStrategyResult)
{
	// 按分数降序排序
	std::sort(vecStrategyResult.begin(), vecStrategyResult.end(),
		[](const std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>& a,
		const std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>& b)
	{
		return a.second > b.second;
	});
}

void ScoreManager::print(std::vector<std::pair<std::vector<std::shared_ptr<StrategyResult>>, double>>& vecStrategyResult)
{
	std::shared_ptr<Strategy> spStrategy = QuantStrategyManager::instance().createStrategy(StrategyMode::WAVE);
	for (size_t index = 0; index < vecStrategyResult.size(); ++index)
	{
		const std::vector<std::shared_ptr<StrategyResult>>& vecStrategy = vecStrategyResult[index].first;
		RCSend("评分：%.2lf", vecStrategyResult[index].second);
		for (size_t strategyIndex = 0; strategyIndex < vecStrategy.size(); ++strategyIndex)
		{
			const std::shared_ptr<StrategyResult>& spStrategyResult = vecStrategy[strategyIndex];
			RCSend("    annual: %s, annualT: %s, params: %s", spStrategyResult->m_annualReturn.toString().c_str(),
				spStrategyResult->m_annualTReturn.toString().c_str(),
				spStrategy->describeParam(spStrategyResult->m_params).c_str());
		}
	}
}

double ScoreManager::smallBestScore(const BigNumber& percentage)
{
	if (percentage < 0)
	{
		return 0;
	}
	// 百分比小于5%
	if (percentage < 0.05)
	{
		return 10;
	}
	else if (percentage < 0.1)
	{
		return 9;
	}
	else if (percentage < 0.15)
	{
		return 8;
	}
	else if (percentage < 0.20)
	{
		return 7;
	}
	else if (percentage < 0.25)
	{
		return 6;
	}
	else if (percentage < 0.30)
	{
		return 5;
	}
	else if (percentage < 0.35)
	{
		return 4;
	}
	else if (percentage < 0.40)
	{
		return 3;
	}
	else if (percentage < 0.45)
	{
		return 2;
	}
	else if (percentage < 0.50)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

double ScoreManager::bigBestScore(const BigNumber& percentage)
{
	if (percentage > 0.5)
	{
		return 10;
	}
	else if (percentage > 0.45)
	{
		return 9;
	}
	else if (percentage > 0.4)
	{
		return 8;
	}
	else if (percentage > 0.35)
	{
		return 7;
	}
	else if (percentage > 0.3)
	{
		return 6;
	}
	else if (percentage > 0.25)
	{
		return 5;
	}
	else if (percentage > 0.2)
	{
		return 4;
	}
	else if (percentage > 0.15)
	{
		return 3;
	}
	else if (percentage > 0.1)
	{
		return 2;
	}
	else if (percentage > 0.05)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Config.h"
#include "Market.h"
#include "Strategy.h"
#include "BigNumber/BigNumberAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

/** 竞赛配置结构体
*/
struct CompetitionConfig
{
	uint32_t beginTime;                         // 开始时间
	uint32_t endTime;                           // 结束时间
	std::vector<std::string> stocks;            // 股票列表
	std::shared_ptr<Market> marketData;         // 市场数据
	int32_t initialFund;                        // 初始资金（单位：分）
	std::vector<std::vector<int32_t>> allParam; // 所有参数

	// 默认构造函数
	CompetitionConfig() :
		beginTime(g_config.m_allBeginTime),
		endTime(g_config.m_allEndTime),
		initialFund(g_config.m_initialFund)
	{

	}

	// 带参数的构造函数
	CompetitionConfig(uint32_t begin, uint32_t end,
		const std::vector<std::string>& stockList,
		const std::shared_ptr<Market>& market,
		int32_t fund = g_config.m_initialFund) :
		beginTime(begin),
		endTime(end),
		stocks(stockList),
		marketData(market),
		initialFund(fund)
	{

	}
};

/** 竞赛最终结果结构体
*/
struct CompetitionFinalResult
{
	std::vector<StrategyResult> rankedResults; // 排序后的策略结果
	uint32_t totalStrategies;     // 总策略数
	uint32_t completedStrategies; // 完成策略数
	BigNumber bestReturn;         // 最佳收益率
	BigNumber worstReturn;        // 最差收益率
	BigNumber averageReturn;      // 平均收益率
	BigNumber averageAnnualReturn; // 平均年化收益率
	BigNumber averageMaxDrawdown; // 平均最大回撤
	BigNumber averageWinRate;     // 平均胜率
	BigNumber averageProfitArea;  // 平均收益面积
	BigNumber averageHealthScore; // 平均健康值
	BigNumber medianReturn;       // 中位数收益率
	BigNumber stdDevReturn;       // 收益率标准差

	// 默认构造函数
	CompetitionFinalResult() :
		totalStrategies(0),
		completedStrategies(0),
		bestReturn(0),
		worstReturn(0),
		averageReturn(0),
		averageAnnualReturn(0),
		averageMaxDrawdown(0),
		averageWinRate(0),
		averageProfitArea(0),
		averageHealthScore(0),
		medianReturn(0),
		stdDevReturn(0)
	{
		bestReturn.toPrec(2).toString().c_str();
	}
};

class CompetitionTask : public CTask
{
public:
	CompetitionTask();

public:
	void DoTask();

	void addParam(StrategyMode strategyMode, const CompetitionConfig& config);

	std::vector<std::vector<int32_t>> getAllParam(const std::vector<std::vector<int32_t>>& allParam);

private:
	// 线程ID列表
	std::vector<uint32_t> m_vecThreadId;
	// 配置信息，策略模式，配置信息
	std::map<int32_t, CompetitionConfig> m_competitionConfigMap;
	// 无锁队列用于结果收集
	LockFreeQueue<StrategyResult> m_resultQueue;
	std::vector<StrategyResult> m_intermediateResults;
	CompetitionFinalResult m_finalResult;
};
#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Strategy.h"
#include "Market.h"
#include "Semaphore/SemaphoreAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <vector>
#include <atomic>
#include <thread>
#include <functional>
#include <mutex>
#include <memory>
#include <string>

// 前向声明
class StrategyTask;

/** 竞赛配置结构体
*/
struct CompetitionConfig
{
	uint32_t beginTime;                       // 开始时间
	uint32_t endTime;                         // 结束时间
	std::vector<std::string> stocks;          // 股票列表
	std::shared_ptr<Market> marketData;       // 市场数据
	int32_t initialFund;                      // 初始资金（单位：分）

	// 默认构造函数
	CompetitionConfig() :
		beginTime(0),
		endTime(0),
		initialFund(1000000)
	{

	}

	// 带参数的构造函数
	CompetitionConfig(uint32_t begin, uint32_t end,
		const std::vector<std::string>& stockList,
		const std::shared_ptr<Market>& market,
		int32_t fund = 1000000) :
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

	}
};

/** 竞赛管理器类
* 负责管理策略竞赛的整个生命周期，单一实例
*/
class CompetitionManager
{
private:
	/** 构造函数
	*/
	CompetitionManager();

public:
	/** 获取单一实例
	@return 返回竞赛管理器单一实例
	*/
	static CompetitionManager& instance();

	/** 析构函数
	*/
	~CompetitionManager();

	/** 初始化竞赛管理器
	@param [in] threadCount 线程数量
	@return 返回是否初始化成功
	*/
	bool init(uint32_t threadCount);

	/** 反初始化竞赛管理器
	*/
	void uninit();

	/** 开始竞赛
	@param [in] config 竞赛配置
	@param [in] strategyMode 策略模式
	@param [in] allParams 所有策略参数组合
	@return 返回是否开始成功
	*/
	bool startCompetition(const CompetitionConfig& config,
		StrategyMode strategyMode,
		const std::vector<std::vector<int32_t>>& allParams);

	/** 停止竞赛
	*/
	void stopCompetition();

	/** 获取竞赛状态
	@return 返回是否正在运行
	*/
	bool isRunning() const;

	/** 获取进度
	@return 返回完成进度（0-100）
	*/
	int getProgress() const;

	/** 获取最终结果
	@return 返回竞赛最终结果
	*/
	CompetitionFinalResult getFinalResult() const;

	/** 检查是否已完成
	@return 返回是否已完成
	*/
	bool isCompleted() const;

private:
	/** 结果处理线程函数
	*/
	void resultProcessingThread();

	/** 处理单个策略结果
	@param [in] result 策略结果
	*/
	void processStrategyResult(const StrategyResult& result);

	/** 计算最终排名和指标
	*/
	void calculateFinalRanking();

	/** 清理所有任务和线程
	*/
	void cleanup();

	/** 分发任务到线程池
	*/
	void distributeTasks();

private:
	// 线程ID列表
	std::vector<uint32_t> m_threadIds;

	// 无锁队列用于结果收集
	LockFreeQueue<StrategyResult>* m_pResultQueue;

	// 结果处理线程
	std::thread m_resultThread;

	// 原子变量
	std::atomic<uint32_t> m_completedCount;
	std::atomic<uint32_t> m_totalTasks;
	std::atomic<bool> m_stopFlag;
	std::atomic<bool> m_isRunning;
	std::atomic<bool> m_isCompleted;

	// 中间结果存储
	std::vector<StrategyResult> m_intermediateResults;
	CompetitionFinalResult m_finalResult;

	// 配置信息
	CompetitionConfig m_currentConfig;
	StrategyMode m_currentStrategyMode;
	std::vector<std::vector<int32_t>> m_allParams;

	// 互斥锁用于保护结果数据
	mutable std::mutex m_resultsMutex;
};
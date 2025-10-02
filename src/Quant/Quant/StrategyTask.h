#pragma once
#include "Strategy.h"
#include "Market.h"
#include "Fund.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <memory>
#include <vector>
#include <string>

/** 策略任务类
* 继承CTask，用于在线程池中执行策略回测
*/
class StrategyTask : public CTask
{
public:
	/** 构造函数
	@param [in] taskId 任务ID
	@param [in] spStrategy 策略实例
	@param [in] spMarket 市场数据
	@param [in] beginTime 开始时间
	@param [in] endTime 结束时间
	@param [in] stocks 股票列表
	@param [in] initialFund 初始资金
	@param [in] pResultQueue 结果队列
	@param [in] pCompletedCount 完成计数
	@param [in] totalTasks 总任务数
	@param [in] pStopFlag 停止标志
	*/
	StrategyTask(int32_t taskId,
		const std::shared_ptr<Strategy>& spStrategy,
		const std::shared_ptr<Market>& spMarket,
		uint32_t beginTime,
		uint32_t endTime,
		const std::vector<std::string>& stocks,
		int32_t initialFund,
		LockFreeQueue<StrategyResult>* pResultQueue,
		std::atomic<uint32_t>* pCompletedCount,
		uint32_t totalTasks,
		std::atomic<bool>* pStopFlag);

	/** 析构函数
	*/
	virtual ~StrategyTask() = default;

	/** 执行任务
	*/
	virtual void DoTask() override;

	/** 停止任务
	*/
	virtual void StopTask() override;

private:
	/** 运行策略回测
	@param [in] fund 资金账户
	@return 返回策略结果
	*/
	StrategyResult runStrategy(Fund& fund);

	/** 计算策略指标
	@param [in] fund 资金账户
	@param [in] actualDays 实际交易天数
	@param [in] totalDays 总天数
	@param [in] maxDrawdown 最大回撤
	@param [in] totalProfitArea 总收益面积
	@param [in] dailyValues 每日资产值
	@return 返回策略结果
	*/
	StrategyResult calculateStrategyMetrics(Fund& fund, uint32_t actualDays, uint32_t totalDays,
		BigNumber maxDrawdown, BigNumber totalProfitArea, const std::vector<int32_t>& dailyValues);

	/** 获取交易日列表
	@return 返回交易日列表
	*/
	std::vector<uint32_t> getTradingDays();

	/** 计算健康值
	@param [in] fund 资金账户
	@param [in] totalReturn 总收益率
	@param [in] maxDrawdown 最大回撤
	@param [in] winRate 胜率
	@param [in] dailyValues 每日资产值
	@return 返回健康值
	*/
	BigNumber calculateHealthScore(Fund& fund, BigNumber totalReturn, BigNumber maxDrawdown,
		BigNumber winRate, const std::vector<int32_t>& dailyValues);

private:
	std::shared_ptr<Strategy> m_spStrategy;
	std::shared_ptr<Market> m_spMarket;
	uint32_t m_beginTime;
	uint32_t m_endTime;
	std::vector<std::string> m_stocks;
	int32_t m_initialFund;
	LockFreeQueue<StrategyResult>* m_pResultQueue;
	std::atomic<uint32_t>* m_pCompletedCount;
	uint32_t m_totalTasks;
	std::atomic<bool>* m_pStopFlag;
	std::atomic<bool> m_exit;
};
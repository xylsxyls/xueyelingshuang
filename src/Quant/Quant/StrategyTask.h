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
	*/
	StrategyTask();

	/** 析构函数
	*/
	virtual ~StrategyTask();

	/** 执行任务
	*/
	virtual void DoTask();

	/** 停止任务
	*/
	virtual void StopTask();

	/** 设置参数
	@param [in] beginTime 开始时间
	@param [in] endTime 结束时间
	@param [in] vecStock 股票列表
	@param [in] spStrategy 策略实例
	@param [in] spFund 账户信息
	@param [in] forceEnd 是否强制平仓未完成交易
	@param [in] resultQueue 结果队列
	@param [in] resultSemaphore 收到结果发送的信号
	*/
	void setParam(uint32_t beginTime, uint32_t endTime, const std::vector<std::string>& vecStock,
		const std::shared_ptr<Strategy>& spStrategy, const std::shared_ptr<Fund>& spFund, bool forceEnd,
		LockFreeQueue<std::shared_ptr<StrategyResult>>* resultQueue, Semaphore* resultSemaphore);

	/** 参数是否有效
	@return 返回参数是否有效
	*/
	bool isParamValid();

private:
	/** 运行策略回测
	@param [in] fund 资金账户
	@return 返回策略结果
	*/
	StrategyResult runStrategy(Fund& fund);

	/** 计算策略指标（耗时）
	@param [in] actualDays 实际交易天数
	@param [in] totalDays 总天数
	@param [in] maxDrawdown 最大回撤
	@param [in] totalProfitArea 总收益面积
	@param [in] dailyValues 每日资产值
	@param [in] lastDate 最后一天日期
	@return 返回策略结果
	*/
	StrategyResult calculateStrategyMetrics(uint32_t actualDays, uint32_t totalDays,
		BigNumber maxDrawdown, BigNumber totalProfitArea, const std::vector<int32_t>& dailyValues, uint32_t lastDate);

	/** 计算健康值
	@param [in] totalReturn 总收益率
	@param [in] maxDrawdown 最大回撤
	@param [in] winRate 胜率
	@param [in] dailyValues 每日资产值
	@return 返回健康值
	*/
	BigNumber calculateHealthScore(int32_t totalReturn, BigNumber maxDrawdown,
		BigNumber winRate, const std::vector<int32_t>& dailyValues);

	/** 获取有效交易日
	@param [in] allTradingDays 所有交易日
	@return 返回有效交易日
	*/
	std::vector<int32_t> getTradingDays(const std::vector<int32_t>& allTradingDays);

private:
	// 开始时间
	uint32_t m_beginTime;
	// 结束时间
	uint32_t m_endTime;
	// 股票列表
	std::vector<std::string> m_vecStock;
	// 策略实例
	std::shared_ptr<Strategy> m_spStrategy;
	// 账户信息
	std::shared_ptr<Fund> m_spFund;
	// 是否强制平仓未完成交易
	bool m_forceEnd;
	// 结果队列
	LockFreeQueue<std::shared_ptr<StrategyResult>>* m_resultQueue;
	// 收到结果发送的信号
	Semaphore* m_resultSemaphore;
	// 退出标志
	std::atomic<bool> m_exit;
};
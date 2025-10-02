#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Market.h"
#include "Fund.h"

/** 策略运行结果结构体
*/
struct StrategyResult
{
	int32_t strategyId;           // 策略ID
	std::vector<int32_t> params;  // 策略参数
	BigNumber totalReturn;        // 总收益率
	BigNumber annualReturn;       // 年化收益率
	BigNumber maxDrawdown;        // 最大回撤
	BigNumber winRate;            // 胜率
	BigNumber profitArea;         // 收益面积
	BigNumber healthScore;        // 健康值
	uint32_t tradeDays;           // 实际交易天数
	uint32_t totalDays;           // 总天数（用于调整）

	// 默认构造函数
	StrategyResult() :
		strategyId(0),
		totalReturn(0),
		annualReturn(0),
		maxDrawdown(0),
		winRate(0),
		profitArea(0),
		healthScore(0),
		tradeDays(0),
		totalDays(0)
	{

	}
};

/** 策略基类
* 所有具体交易策略的抽象基类
*/
class Strategy
{
public:
	/** 构造函数
	*/
	Strategy();

	/** 虚析构函数
	*/
	virtual ~Strategy() = default;

	/** 初始化策略
	@param [in] beginTime 回测开始时间
	@param [in] endTime 回测结束时间
	*/
	virtual void init(uint32_t beginTime, uint32_t endTime);

	/** 添加股票代码到策略监控列表
	@param [in] stock 股票代码
	*/
	virtual void addStock(const std::string& stock);

	/** 设置市场数据源
	@param [in] spMarket 市场数据共享指针
	*/
	virtual void setMarket(const std::shared_ptr<Market>& spMarket);

	/** 设置资金账户
	@param [in] spFund 资金账户共享指针
	*/
	virtual void setFund(const std::shared_ptr<Fund>& spFund);

	/** 设置策略参数向量
	@param [in] params 策略参数下标向量
	*/
	void setStrategyParams(const std::vector<int32_t>& params);

	/** 获取策略参数向量
	@return 返回策略参数的下标向量
	*/
	std::vector<int32_t> getStrategyParams() const;

	/** 每个交易日的策略执行入口
	@param [in] date 当前交易日
	@return 返回是否执行成功
	*/
	virtual bool onTradingDay(uint32_t date) = 0;

	/** 获取策略模式枚举
	@return 返回策略模式枚举值
	*/
	virtual StrategyMode getStrategyMode() const = 0;

	/** 获取策略名称（用于日志和显示）
	@return 返回策略名称字符串
	*/
	virtual std::string getStrategyName() const = 0;

	/** 获取策略监控的股票列表
	@return 返回股票代码列表
	*/
	const std::vector<std::string>& getStocks() const;

protected:
	// 回测开始时间
	uint32_t m_beginTime;

	// 回测结束时间
	uint32_t m_endTime;

	// 策略监控的股票列表
	std::vector<std::string> m_vecStock;

	// 市场数据源
	std::shared_ptr<Market> m_spMarket;

	// 资金账户
	std::shared_ptr<Fund> m_spFund;

	// 策略参数向量
	std::vector<int32_t> m_strategyParams;
};
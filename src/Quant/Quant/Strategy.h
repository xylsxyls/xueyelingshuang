#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Market.h"
#include "Fund.h"
#include "Config.h"

/** 策略运行结果结构体
*/
struct StrategyResult
{
	StrategyMode strategyMode;         // 策略模式
	std::vector<int32_t> params;       // 策略参数
	int32_t totalReturn;               // 总收益
	int32_t tReturn;                   // 做T总收益
	BigNumber annualReturn;            // 年化收益率
	BigNumber annualTReturn;           // 做T年化收益率
	BigNumber maxDrawdown;             // 最大回撤
	BigNumber winRate;                 // 胜率
	BigNumber profitArea;              // 收益面积
	BigNumber healthScore;             // 健康值
	uint32_t tradeDays;                // 实际交易天数
	uint32_t totalDays;                // 总天数（用于调整）
	std::vector<std::string> tradeLog; // 交易日志
		
	// 默认构造函数
	StrategyResult() :
		strategyMode(StrategyMode::COUNT),
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
	virtual ~Strategy();

	/** 每个交易日的策略执行入口
	@param [in] date 当前交易日
	@return 返回是否执行成功
	*/
	virtual bool onTradingDay(uint32_t date) = 0;

	/** 描述参数
	@param [in] params 参数组
	@return 返回参数含义
	*/
	virtual std::string describeParam(const std::vector<int32_t>& params);

	/** 初始化策略
	@param [in] beginTime 回测开始时间
	@param [in] endTime 回测结束时间
	*/
	virtual void init(uint32_t beginTime, uint32_t endTime);

	/** 添加股票代码到策略监控列表
	@param [in] stock 股票代码
	*/
	virtual void addStock(const std::string& stock);

	/** 获取策略监控的股票列表
	@return 返回股票代码列表
	*/
	std::vector<std::string> getStock() const;

	/** 设置市场数据源
	@param [in] spMarket 市场数据共享指针
	*/
	virtual void setMarket(const std::shared_ptr<Market>& spMarket);

	/** 设置资金账户
	@param [in] spFund 资金账户共享指针
	*/
	virtual void setFund(const std::shared_ptr<Fund>& spFund);

	/** 获取资金账户
	@return 返回资金账户共享指针
	*/
	virtual std::shared_ptr<Fund> getFund();

	/** 设置策略参数向量
	@param [in] params 策略参数下标向量
	*/
	void setStrategyParam(const std::vector<int32_t>& param);

	/** 获取策略参数向量
	@return 返回策略参数的下标向量
	*/
	std::vector<int32_t> getStrategyParam() const;

	/** 获取策略参数是否有效
	@return 返回策略参数是否有效
	*/
	virtual bool isStrategyParamValid() const;

	/** 获取策略模式枚举
	@return 返回策略模式枚举值
	*/
	StrategyMode getStrategyMode() const;

	/** 获取策略名称
	@return 返回策略名称字符串
	*/
	std::string getStrategyName() const;

	/** 获取当前价格，卖一价
	@param [in] dayInfo 当天分时数据
	@param [in] time 获取时间点
	@return 返回价格
	*/
	int32_t getCurrentPrice(const std::vector<int32_t>& dayInfo, ObserveTime time);

	/** 获取当前直买价格
	@param [in] dayInfo 当天分时数据
	@param [in] time 获取时间点
	@return 返回价格
	*/
	int32_t getDirectBuyPrice(const std::vector<int32_t>& dayInfo, ObserveTime time);

	/** 获取当前直卖价格
	@param [in] dayInfo 当天分时数据
	@param [in] time 获取时间点
	@return 返回价格
	*/
	int32_t getDirectSellPrice(const std::vector<int32_t>& dayInfo, ObserveTime time);

	/** 获取区间最小有效价格
	@param [in] dayInfo 当天分时数据
	@param [in] timeBegin 开始时间点
	@param [in] timeEnd 结束时间点
	@return 返回价格
	*/
	int32_t getMinPrice(const std::vector<int32_t>& dayInfo, ObserveTime timeBegin, ObserveTime timeEnd);

	/** 获取区间最大有效价格
	@param [in] dayInfo 当天分时数据
	@param [in] timeBegin 开始时间点
	@param [in] timeEnd 结束时间点
	@return 返回价格
	*/
	int32_t getMaxPrice(const std::vector<int32_t>& dayInfo, ObserveTime timeBegin, ObserveTime timeEnd);

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
	std::vector<int32_t> m_strategyParam;

	// 策略类型
	StrategyMode m_mode;

	// 策略名称
	std::string m_modeName;
};
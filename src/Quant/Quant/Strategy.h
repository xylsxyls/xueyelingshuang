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
	StrategyMode m_strategyMode;            // 策略模式
	std::vector<int32_t> m_params;          // 策略参数
	int32_t m_totalReturn;                  // 总收益
	int32_t m_tReturn;                      // 做T总收益
	BigNumber m_annualReturn;               // 年化收益率
	BigNumber m_annualTReturn;              // 做T年化收益率
	BigNumber m_maxDrawdown;                // 最大回撤
	BigNumber m_winRate;                    // 胜率
	BigNumber m_profitArea;                 // 收益面积
	BigNumber m_healthScore;                // 健康值
	uint32_t m_tradeDays;                   // 实际交易天数
	uint32_t m_totalDays;                   // 总天数（用于调整）
	std::vector<std::string> m_tradeLog;    // 交易日志
	std::vector<std::string> m_strategyLog; // 策略日志
	std::vector<uint32_t> m_tradeCount;     // 交易次数，正常交易的买卖和反追割肉

	/** 构造函数
	*/
	StrategyResult();
};

/** 导入参数
*/
struct ImportParam
{
	// 操作模式: OPERATE, HOLD, GIVE_UP
	int32_t m_operate;
	// 虚拟卖出价格（每天计算的参考卖价）
	int32_t m_virtualSellPrice;
	// 虚拟卖出时间点
	ObserveTime m_virtualSellObserveTime;
	// 虚拟买入价格（每天计算的参考买价）
	int32_t m_virtualBuyPrice;
	// 虚拟买入时间点
	ObserveTime m_virtualBuyObserveTime;
	// 实际/更新后的卖出起点（用于chase）
	int32_t m_realSellPrice;
	// 实际/更新后的买入起点（用于cut）
	int32_t m_realBuyPrice;
	// 是否满仓
	bool m_isFull;

	/** 构造函数
	*/
	ImportParam();

	/** 是否为空
	@return 返回是否为空
	*/
	bool empty();
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

	/** 填入并检测参数是否合理
	@return 返回是否合理
	*/
	virtual bool fillCheckParam();

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

	/** 设置导入参数
	@param [in] import 导入参数
	*/
	void setImportParam(const ImportParam& import);

	/** 获取导入参数
	@return 返回导入参数
	*/
	ImportParam getImportParam();

	/** 策略日志
	@return 返回策略日志
	*/
	std::vector<std::string> strategyLog();

	/** 交易次数
	@return 返回交易次数
	*/
	std::vector<uint32_t> tradeCount();

protected:
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

	// 导入参数
	ImportParam m_import;

	// 策略日志
	std::vector<std::string> m_strategyLog;

	// 交易次数
	std::vector<uint32_t> m_tradeCount;
};
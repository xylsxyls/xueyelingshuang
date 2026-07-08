#pragma once
#include "Strategy.h"
#include <vector>
#include <map>

/** S1100B1400策略
* 参数化日内交易策略，支持不同时间点和价格参数的组合
*/
class S1100B1400Strategy : public Strategy
{
public:
	/** 构造函数
	*/
	S1100B1400Strategy();

	/** 析构函数
	*/
	virtual ~S1100B1400Strategy();

	/** 每个交易日的策略执行入口，从开始到结束依次循环
	@param [in] date 当前交易日
	@return 返回是否执行成功
	*/
	virtual bool onTradingDay(uint32_t date);

	/** 描述参数
	@param [in] params 参数组
	@return 返回参数含义
	*/
	virtual std::string describeParam(const std::vector<int32_t>& params);

	/** 获取策略参数是否有效
	@return 返回策略参数是否有效
	*/
	virtual bool isStrategyParamValid() const;

private:
	bool m_hasFirstBuy;
};
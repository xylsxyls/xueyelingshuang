#pragma once
#include "StockConditionMacro.h"
#include <map>

class StockCondition;
extern "C" StockConditionAPI StockCondition* StockConditionInterface();

class Strategy;
class StockConditionAPI StockCondition
{
public:
	enum ConditionEnum
	{
		WR_RSI
	};

private:
	/** 构造函数
	*/
	StockCondition();

	/** 析构函数
	*/
	virtual ~StockCondition();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static StockCondition& instance();

public:
	/** 获取策略接口
	@param [in] condition 策略基于条件
	@return 返回策略接口
	*/
	virtual Strategy* getStrategy(ConditionEnum condition = WR_RSI);

	/** 释放所有策略接口
	*/
	virtual void destroyAll();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<ConditionEnum, Strategy*> m_strategy;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
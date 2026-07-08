#pragma once
#include "Strategy.h"
#include <memory>

/** 策略管理器类
* 负责创建交易策略实例
*/
class QuantStrategyManager
{
protected:
	/** 构造函数
	*/
	QuantStrategyManager();

public:
	/** 获取单一实例
	@return 返回策略管理器单一实例
	*/
	static QuantStrategyManager& instance();

	/** 创建策略实例
	@param [in] mode 策略模式枚举
	@return 返回策略指针
	*/
	std::shared_ptr<Strategy> createStrategy(StrategyMode mode);
};
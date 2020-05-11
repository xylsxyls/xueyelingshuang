#pragma once
#include "StockTypeMacro.h"
#include "BigNumber/BigNumberAPI.h"
#include <vector>
#include <memory>
#include "IntDateTime/IntDateTimeAPI.h"

enum FilterType
{
	FILTER_INIT,

	ALL_STOCK,

	RISE_UP,

	FALL_DOWN,
};

enum StrategyType
{
	STRATEGY_INIT,

	SAR_RISE_BACK_COUNT,

	SAR_RISE_BACK,

	CATCH_UP,

	SAR_RISE_BACK_THIRTY_LINE,

	LINE_BACK,

	RECONSTRUCTION,

	STRATEGY_TYPE_SIZE,

	T_ADD_0
};

enum SolutionType
{
	SOLUTION_INIT,

	AVG_FUND_HIGH_SCORE,

	STRATEGY_SET,

	DISPOSABLE_STRATEGY,

	INTEGRATED_STRATEGY,

	OBSERVE_STRATEGY
};

struct StockTypeAPI StockLoadInfo
{
	//是否是自定义
	bool m_isCustomize;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//所有gupiao，在自定义下有效
	std::vector<std::string> m_allStock;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	//是否yejizengzhang
	FilterType m_filterType;
	//是否去除jiejin
	bool m_isDislodgeLiftBan;
	//是否是kechuangban
	bool m_isDislodge688;

	/** 构造函数
	*/
	StockLoadInfo()
	{
		m_isCustomize = false;
		m_filterType = FILTER_INIT;
		m_isDislodgeLiftBan = false;
		m_isDislodge688 = false;
	}
};

struct StockTypeAPI StockInfo
{
	//价格
	BigNumber m_price;
	//百分比 0-100
	BigNumber m_percent;
	//分数
	BigNumber m_score;
	//比例 0-1
	BigNumber m_rate;
};

struct StockTypeAPI ChooseParam
{
	//使用的类型
	StrategyType m_useType;
	//使用的计数类型
	StrategyType m_useCountType;
	//是否是观察类型
	bool m_isObserve;
	//解决方案类型，如果是observer则是内部类型，不是则直接使用
	SolutionType m_solutionType;

	/** 构造函数
	*/
	ChooseParam();

	/** 是否等于
	@param [in] chooseParam 选择参数
	@return 返回是否等于
	*/
	bool operator==(const ChooseParam& chooseParam);

	/** 是否不等于
	@param [in] chooseParam 选择参数
	@return 返回是否等于
	*/
	bool operator!=(const ChooseParam& chooseParam);

	/** 是否不等于
	@param [in] chooseParam1 选择参数1
	@param [in] chooseParam2 选择参数2
	@return 返回是否等于
	*/
	friend bool operator< (const ChooseParam& chooseParam1, const ChooseParam& chooseParam2)
	{
		if (chooseParam1.m_solutionType < chooseParam2.m_solutionType)
		{
			return true;
		}
		if (chooseParam1.m_solutionType > chooseParam2.m_solutionType)
		{
			return false;
		}
		if (chooseParam1.m_useType < chooseParam2.m_useType)
		{
			return true;
		}
		if (chooseParam1.m_useType > chooseParam2.m_useType)
		{
			return false;
		}
		if (chooseParam1.m_useCountType < chooseParam2.m_useCountType)
		{
			return true;
		}
		if (chooseParam1.m_useCountType > chooseParam2.m_useCountType)
		{
			return false;
		}
		if (chooseParam1.m_isObserve < chooseParam2.m_isObserve)
		{
			return true;
		}
		if (chooseParam1.m_isObserve > chooseParam2.m_isObserve)
		{
			return false;
		}
		return false;
	}
};

class Solution;
class Strategy;
struct SolutionInfo;
struct StrategyInfo;
class StockMarket;

class StockTypeAPI StockStorageBase
{
public:
	virtual std::vector<std::string>* filterStock(StrategyType strategyType, const IntDateTime& date) = 0;

	virtual std::shared_ptr<Solution> solution(SolutionType solutionType) = 0;

	virtual std::shared_ptr<Strategy> strategy(StrategyType strategyType) = 0;

	virtual std::shared_ptr<SolutionInfo> solutionInfo(SolutionType solutionType) = 0;

	virtual std::shared_ptr<StrategyInfo> strategyInfo(StrategyType solutionType, const std::string& stock) = 0;

	virtual std::shared_ptr<StockMarket> market(const std::string& stock) = 0;

	virtual IntDateTime moveDay(const IntDateTime& date, int32_t day, const std::shared_ptr<StockMarket>& runMarket = nullptr) = 0;
};
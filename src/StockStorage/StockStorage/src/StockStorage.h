#pragma once
#include "StockStorageMacro.h"
#include <vector>
#include <map>
#include <stdint.h>
#include <memory>
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockType/StockTypeAPI.h"
#include <set>

class StockMarket;
class IndicatorManagerBase;
class Solution;
struct SolutionInfo;
class Strategy;
struct StrategyInfo;

/** 常用数据储存类，load类函数按顺序加载
*/
class StockStorageAPI StockStorage : public StockStorageBase
{
protected:
	StockStorage();

public:
	static StockStorage& instance();

public:
	void init(const IntDateTime& beginTime,
		const IntDateTime& endTime,
		int32_t moveDay,
		const std::vector<std::string>& allStock);

	void loadStrategy(const std::set<StrategyType>& allStrategyType);

	void loadMarket();

	void loadIndicator(const std::set<std::string>& allNeedLoad);

	void loadStrategyInfo(const std::set<StrategyType>& allStrategyType);

	void loadSolutionInfo(const std::set<SolutionType>& allSolutionType, const std::set<StrategyType>& allStrategyType);

	void loadSolution(const std::set<SolutionType>& allSolutionType, const std::vector<ChooseParam>& vecChooseParam);

	void clear();

	std::vector<std::string>* filterStock(StrategyType strategyType, const IntDateTime& date);

	std::shared_ptr<Solution> solution(SolutionType solutionType);

	std::shared_ptr<Strategy> strategy(StrategyType strategyType);

	std::shared_ptr<SolutionInfo> solutionInfo(SolutionType solutionType);

	std::shared_ptr<StrategyInfo> strategyInfo(StrategyType solutionType, const std::string& stock);

	std::shared_ptr<StockMarket> market(const std::string& stock);

	std::shared_ptr<StockMarket> runMarket();

	IntDateTime moveDay(const IntDateTime& date, int32_t day, const std::shared_ptr<StockMarket>& runMarket = nullptr);

	void load();

protected:
	void dislodge688(std::vector<std::string>& allStock);

	void loadFilterStock(StrategyType strategyType, const StockLoadInfo& stockLoadInfo);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//stock
	std::map<std::string, std::shared_ptr<StockMarket>> m_spMarketMap;
	//stock, stocktype
	std::map<std::string, std::map<std::string, std::shared_ptr<IndicatorManagerBase>>> m_spIndicatorMap;

	std::map<SolutionType, std::shared_ptr<Solution>> m_solutionMap;
	std::map<SolutionType, std::shared_ptr<SolutionInfo>> m_solutionInfoMap;
	std::map<StrategyType, std::shared_ptr<Strategy>> m_strategyMap;
	std::map<std::string, std::map<StrategyType, std::shared_ptr<StrategyInfo>>> m_strategyInfoMap;

	bool m_isCustomize;
	std::vector<std::string> m_allStock;
	int32_t m_moveDay;
	IntDateTime m_beginTime;
	IntDateTime m_endTime;
	IntDateTime m_moveBeginTime;
	std::map<StrategyType, std::map<IntDateTime, std::vector<std::string>>> m_filterStock;
	std::shared_ptr<StockMarket> m_spRunMarket;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
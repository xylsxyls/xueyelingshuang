#pragma once
#include "StockSolutionMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <map>
#include <memory>
#include "StockStrategy/StockStrategyAPI.h"

enum SolutionType
{
	SOLUTION_INIT,

	AVG_FUND_HIGH_SCORE,

	STRATEGY_SET,

	DISPOSABLE_STRATEGY,

	INTEGRATED_STRATEGY,

	OBSERVE_STRATEGY
};

struct SolutionInfo;
class StockSolutionAPI Solution
{
public:
	/** ���ò���
	@param [in] spStrategy ���ԣ�Ҫ����������һ�����ڲ��ԣ��ڶ������ڲ鿴ͨ������
	*/
	void init(const std::map<StrategyType, std::pair<std::shared_ptr<Strategy>, std::shared_ptr<Strategy>>>& vecStrategy);

	/** ���ò���
	@param [in] useType ʹ������
	*/
	void setParam(StrategyType useType);

	/** ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@param [in] solutionInfo ���������Ҫ����Ϣ
	@return �Ƿ���ѡ����gupiao
	*/
	virtual bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] sellStock maichu��gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@param [in] solutionInfo ���������Ҫ����Ϣ
	@return �����Ƿ�����Ҫmaichu��gupiao
	*/
	virtual bool sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** ��ȡ�����������
	@return ���ؽ����������
	*/
	SolutionType type();

	/** ��ȡִ�еĲ���
	@param [in] useType ʹ�õ�����
	@return ����ִ�еĲ���
	*/
	std::shared_ptr<Strategy> strategy(StrategyType useType);

	/** ��ȡִ�м����Ĳ���
	@param [in] useType ʹ�õ�����
	@return ����ִ�м����Ĳ���
	*/
	std::shared_ptr<Strategy> strategyCount(StrategyType useType);

	/** ��ȡ��Сͨ������
	@param [in] solutionInfo ���������Ҫ����Ϣ
	@return ������Сͨ������
	*/
	int32_t minPollSize(const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** ��ȡȥ������
	@param [in] solutionInfo ���������Ҫ����Ϣ
	@return ����ȥ������
	*/
	int32_t popSize(int32_t buySize, const std::shared_ptr<SolutionInfo>& solutionInfo);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	StrategyType m_useType;
	std::map<StrategyType, std::pair<std::shared_ptr<Strategy>, std::shared_ptr<Strategy>>> m_mapStrategy;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	SolutionType m_solutionType;
};
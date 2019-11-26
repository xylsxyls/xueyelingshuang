#pragma once
#include "StockTradeMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockSolution/StockSolutionAPI.h"
#include <map>
#include <memory>

class StockDay;
class Solution;
class StockFund;
class StockMarket;
class StockWrIndicator;
class StockRsiIndicator;
class StockSarIndicator;
class StockBollIndicator;
/** gupiaojiaoyi����
*/
class StockTradeAPI StockTrade
{
public:
	StockTrade();

public:
	/** ��ʼ������������Ҫ������ȫ�����غã����غõ���Ϣ�����н�������Ͳ��Թ���
	@param [in] beginTime ��ʼʱ��
	@param [in] endTime ����ʱ��
	@param [in] allStock gupiaochi
	@param [in] solutionType �����������
	@param [in] strategyType ��������
	*/
	void init(const IntDateTime& beginTime,
		const IntDateTime& endTime,
		const std::vector<std::string>& allStock,
		SolutionType solutionType,
		StrategyType strategyType);

	/** ��ʼ������������Ҫ������ȫ�����غã����غõ���Ϣ�����н�������Ͳ��Թ���
	@param [in] beginTime ��ʼʱ��
	@param [in] endTime ����ʱ��
	@param [in] allStock gupiaochi
	@param [in] solutionType �����������
	@param [in] strategyType ��������
	*/
	void init(const IntDateTime& beginTime,
		const IntDateTime& endTime,
		const std::vector<std::string>& allStock,
		const std::vector<SolutionType>& vecSolutionType,
		const std::vector<StrategyType>& vecStrategyType);

	/** ԭʼ��Ϣ����ת����������Ϣ
	*/
	void load();

	/** ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,percent
	@param [in] date ����
	@param [in] stockFund zijin��Ϣ
	@return �Ƿ���ѡ����gupiao
	*/
	bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		StockFund* stockFund,
		SolutionType solutionType,
		StrategyType strategyType);

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] stock gupiao����
	@param [in] date ����
	@param [out] price maichu�۸�
	@param [out] percent maichu������0-100
	@param [in] buyInfo goumai��Ϣ
	@return ����ѯ�ʵ�gupiao�費��Ҫmaichu
	*/
	bool sell(const std::string& stock,
		const IntDateTime& date,
		BigNumber& price,
		BigNumber& rate,
		StockFund* stockFund,
		SolutionType solutionType,
		StrategyType strategyType);

	/** ��ȡ���õ�hangqing��Ϣ
	@param [in] stock gupiao����
	@return ����һ���hangqing��Ϣ
	*/
	std::shared_ptr<StockMarket> market(const std::string& stock);

	/** ��ȡָ������ָ��gupiao���ϵ�������Ч���ݼ���
	@param [in] vecStock gupiao����
	@param [in] date ����
	@param [out] dayData ������Ч���ݼ���
	@return �����Ƿ��ȡ�ɹ�
	*/
	bool stockDayData(const std::vector<std::string>& vecStock,
		const IntDateTime& date,
		std::map<std::string, std::shared_ptr<StockDay>>& dayData);

protected:
	std::shared_ptr<SolutionAllInfo> makeSolutionAllInfo(const IntDateTime& date,
		StockFund* stockFund,
		SolutionType solutionType,
		StrategyType strategyType);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<std::string, std::shared_ptr<StockMarket>> m_spMarketMap;
	std::map<std::string, std::shared_ptr<StockWrIndicator>> m_spWrIndicatorMap;
	std::map<std::string, std::shared_ptr<StockRsiIndicator>> m_spRsiIndicatorMap;
	std::map<std::string, std::shared_ptr<StockSarIndicator>> m_spSarIndicatorMap;
	std::map<std::string, std::shared_ptr<StockBollIndicator>> m_spBollIndicatorMap;

	std::map<SolutionType, std::shared_ptr<Solution>> m_solutionMap;
	std::map<StrategyType, std::shared_ptr<Strategy>> m_strategyMap;

	std::vector<std::string> m_allStock;
	std::map<IntDateTime, std::vector<std::string>> m_filterStock;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
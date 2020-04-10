#pragma once
#include "StockTradeMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockSolution/StockSolutionAPI.h"
#include <map>
#include <memory>
#include "TradeParam.h"

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
		const std::vector<ChooseParam>& vecChooseParam);

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
		const std::vector<ChooseParam>& vecChooseParam);

	/** ԭʼ��Ϣ����ת����������Ϣ
	*/
	void load();

	/** ����jiaoyi����
	@param [in] solutionType �����������
	@param [in] tradeParam jiaoyi����
	*/
	void setTradeParam(SolutionType solutionType, const TradeParam& tradeParam);

	/** ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@param [in] stockFund zijin��Ϣ
	@param [in] solutionType �����������
	@param [in] strategyType ��������
	@param [out] useStrategyType ʹ�õĲ�������
	@param [in] onceDate ���㵥��lirunʱÿ�ν���Ŀ�ʼʱ��
	@return �Ƿ���ѡ����gupiao
	*/
	bool buy(std::vector<std::pair<std::string, StockInfo>>& buyStock,
		const IntDateTime& date,
		SolutionType solutionType,
		StrategyType& useStrategyType);

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] sellStock maichu��gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@param [in] stockFund zijin��Ϣ
	@param [in] solutionType �����������
	@param [in] strategyType ��������
	@return �����Ƿ�����Ҫmaichu��gupiao
	*/
	bool sell(std::vector<std::pair<std::string, StockInfo>>& sellStock,
		const IntDateTime& date,
		SolutionType solutionType);

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
	std::shared_ptr<SolutionInfo> makeSolutionInfo(SolutionType solutionType,
		const std::vector<ChooseParam>& vecStrategyType);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//stock
	std::map<std::string, std::shared_ptr<StockMarket>> m_spMarketMap;
	//stock, stocktype
	std::map<std::string, std::map<std::string, std::shared_ptr<IndicatorManagerBase>>> m_spIndicatorMap;

	std::map<SolutionType, std::shared_ptr<Solution>> m_solutionMap;
	std::map<StrategyType, std::shared_ptr<Strategy>> m_strategyMap;
	std::map<StrategyType, std::shared_ptr<StrategyInfo>> m_strategyInfoMap;

	std::vector<std::string> m_allStock;
	std::map<IntDateTime, std::vector<std::string>> m_filterStock;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
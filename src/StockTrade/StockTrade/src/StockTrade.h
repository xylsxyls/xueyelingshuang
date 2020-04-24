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
	@param [in] solutionType �����������
	@param [in] strategyType ��������
	@param [in] allStock gupiaochi
	*/
	void init(const IntDateTime& beginTime,
		const IntDateTime& endTime,
		SolutionType solutionType,
		const std::vector<ChooseParam>& vecChooseParam,
		const std::vector<std::string>& allStock = std::vector<std::string>());

	/** ��ʼ������������Ҫ������ȫ�����غã����غõ���Ϣ�����н�������Ͳ��Թ���
	@param [in] beginTime ��ʼʱ��
	@param [in] endTime ����ʱ��
	@param [in] solutionType �����������
	@param [in] strategyType ��������
	@param [in] allStock gupiaochi
	*/
	void init(const IntDateTime& beginTime,
		const IntDateTime& endTime,
		const std::vector<SolutionType>& vecSolutionType,
		const std::vector<ChooseParam>& vecChooseParam,
		const std::vector<std::string>& allStock = std::vector<std::string>());

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
	@param [in] solutionType ʹ�õĽ����������
	@param [in] useChooseParam ʹ�õ�ѡ�����
	@return �Ƿ���ѡ����gupiao
	*/
	bool buy(std::vector<std::pair<std::string, StockInfo>>& buyStock,
		const IntDateTime& date,
		SolutionType solutionType,
		std::shared_ptr<ChooseParam>& useChooseParam);

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] sellStock maichu��gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@param [in] solutionType ʹ�õĽ����������
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

	/** ��ȡ����hangqing
	@return ��������hangqing
	*/
	std::shared_ptr<StockMarket> runMarket();

	/** ��ȡָ������ָ��gupiao���ϵ�������Ч���ݼ���
	@param [in] vecStock gupiao����
	@param [in] date ����
	@param [out] dayData ������Ч���ݼ���
	@return �����Ƿ��ȡ�ɹ�
	*/
	bool stockDayData(const std::vector<std::string>& vecStock,
		const IntDateTime& date,
		std::map<std::string, std::shared_ptr<StockDay>>& dayData);
};
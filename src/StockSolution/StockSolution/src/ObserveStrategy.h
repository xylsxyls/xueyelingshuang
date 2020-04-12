#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
class AvgFundHighScore;
struct SolutionInfo;
class StockSolutionAPI ObserveStrategy : public Solution
{
public:
	/** ���캯��
	*/
	ObserveStrategy();

public:
	/** ��ʼ��
	@param [in] stockNum ����goumai��gupiao����
	@param [in] calcDays ��������
	*/
	void init(int32_t stockNum, int32_t calcDays);

	/** ���ò���
	@param [in] solutionInfo ���������Ϣ
	*/
	void setSolutionInfo(const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** ���ò��ԣ��������ò���֮��
	@param [in] useType ��������
	@param [in] useCountType ������������
	*/
	void setStrategyType(StrategyType useType, StrategyType useCountType);

	/** ����ʹ�õķ�������
	@param [in] solutionType ʹ�õķ�������
	*/
	void setSolutionType(SolutionType solutionType);

	/** ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@return �Ƿ���ѡ����gupiao�����ϲ�Ϊ0��������goumai��gupiao
	*/
	virtual bool buy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date);

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] sellStock maichu��gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@return �����Ƿ�����Ҫmaichu��gupiao�����ϲ�Ϊ0��������maichu��gupiao
	*/
	virtual bool sell(std::vector<std::pair<std::string, StockInfo>>& sellStock, const IntDateTime& date);

protected:
	/** ��ȡ��ǰ����
	@param [in] stock gupiao����
	@param [in] date ����
	@param [in] days ��ǰ������
	@return ������ǰ����
	*/
	IntDateTime getBeforeDay(const std::string& stock, const IntDateTime& date, int32_t days);

	/** ��ȡ��Ӧ��hangqing��Ϣ
	@param [in] stock gupiao����
	@return ����hangqing��Ϣ
	*/
	std::shared_ptr<StockMarket> getMarket(const std::string& stock);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<SolutionType, std::shared_ptr<Solution>> m_solutionMap;
	std::shared_ptr<Solution> m_useSolution;
	StrategyType m_strategyType;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_stockNum;
	int32_t m_calcDays;
};
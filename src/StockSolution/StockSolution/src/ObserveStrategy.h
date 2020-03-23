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
	@param [in] strategyType ������
	@param [in] calcDays ��������
	*/
	void init(StrategyType strategyType, int32_t calcDays);

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

protected:
	/** ��ȡ��ǰ����
	@param [in] stock gupiao����
	@param [in] date ����
	@param [in] days ��ǰ������
	@param [in] solutionInfo ���������Ҫ����Ϣ
	@return ������ǰ����
	*/
	IntDateTime getBeforeDay(const std::string& stock,
		const IntDateTime& date,
		int32_t days,
		const std::shared_ptr<SolutionInfo>& solutionInfo);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<AvgFundHighScore> m_avgSolution;
	StrategyType m_strategyType;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_stockNum;
	int32_t m_calcDays;
};
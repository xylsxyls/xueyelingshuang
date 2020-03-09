#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
class AvgFundHighScore;
struct SolutionInfo;
class StockSolutionAPI IntegratedStrategy : public Solution
{
public:
	/** ���캯��
	*/
	IntegratedStrategy();

public:
	/** ��ʼ��
	@param [in] stockNum gupiao����
	@param [in] vecStrategyType ִ������
	*/
	void init(int32_t stockNum, const std::vector<StrategyType>& vecStrategyType);

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
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<AvgFundHighScore> m_avgSolution;
	std::vector<StrategyType> m_vecStrategyType;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
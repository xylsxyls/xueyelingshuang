#pragma once
#include "Solution.h"
#include <memory>
#include "ChooseParam.h"

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
	@param [in] vecStrategyType ִ������
	*/
	void init(const std::vector<ChooseParam>& vecChooseParam);

	/** ���ò���
	@param [in] solutionInfo ���������Ϣ
	*/
	void setSolutionInfo(const std::shared_ptr<SolutionInfo>& solutionInfo);

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
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<AvgFundHighScore> m_avgSolution;
	std::vector<ChooseParam> m_vecChooseParam;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
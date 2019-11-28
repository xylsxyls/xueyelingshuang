#pragma once
#include "StockSolutionMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <map>
#include <memory>

enum SolutionType
{
	SOLUTION_INIT,

	AVG_FUND_HIGH_SCORE
};

struct SolutionAllInfo;
class Strategy;
class StockSolutionAPI Solution
{
public:
	/** ���ò���
	@param [in] spStrategy ����
	*/
	void init(const std::shared_ptr<Strategy>& spStrategy);

	/** ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@param [in] solutionAllInfo ���������Ҫ����Ϣ
	@return �Ƿ���ѡ����gupiao
	*/
	virtual bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo) = 0;

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] sellStock maichu��gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@param [in] solutionAllInfo ���������Ҫ����Ϣ
	@return �����Ƿ�����Ҫmaichu��gupiao
	*/
	virtual bool sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo) = 0;

	/** ��ȡ�����������
	@return ���ؽ����������
	*/
	SolutionType type();

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<Strategy> m_spStrategy;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	SolutionType m_solutionType;
};
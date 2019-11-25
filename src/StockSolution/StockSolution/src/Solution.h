#pragma once
#include "StockSolutionMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <map>
#include <memory>
#include "StockSolutionMacro.h"

struct SolutionInfo;
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
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,rate
	@param [in] date ����
	@param [in] allBuyInfo ���е���goumai��Ϣ
	@return �Ƿ���ѡ����gupiao
	*/
	virtual bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& allSolutionInfo) = 0;

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] stock gupiao����
	@param [in] date ����
	@param [out] price maichu�۸�
	@param [out] percent maichu������0-100
	@param [in] buyInfo goumai��Ϣ
	@return ����ѯ�ʵ�gupiao�費��Ҫmaichu
	*/
	virtual bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& rate,
		const std::shared_ptr<SolutionInfo>& solutionInfo) = 0;

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<Strategy> m_spStrategy;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
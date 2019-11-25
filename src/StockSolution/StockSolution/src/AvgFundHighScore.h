#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
struct SolutionAllInfo;
struct SolutionInfo;
class StockSolutionAPI AvgFundHighScore : public Solution
{
public:
	/** ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,rate
	@param [in] date ����
	@param [in] allBuyInfo ���е���goumai��Ϣ
	@return �Ƿ���ѡ����gupiao
	*/
	bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& allSolutionInfo);

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] stock gupiao����
	@param [in] date ����
	@param [out] price maichu�۸�
	@param [out] rate maichu������0-1
	@param [in] buyInfo goumai��Ϣ
	@return ����ѯ�ʵ�gupiao�費��Ҫmaichu
	*/
	bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& rate,
		const std::shared_ptr<SolutionInfo>& solutionInfo);
};
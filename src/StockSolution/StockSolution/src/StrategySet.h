#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
struct SolutionInfo;
class StockSolutionAPI StrategySet : public Solution
{
public:
	/** ���캯��
	*/
	StrategySet();

public:
	/** ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@param [in] solutionInfo ���������Ҫ����Ϣ
	@return �Ƿ���ѡ����gupiao
	*/
	virtual bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionInfo>& solutionInfo);
};
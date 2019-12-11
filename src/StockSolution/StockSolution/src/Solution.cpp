#include "Solution.h"

void Solution::init(const std::vector<std::shared_ptr<Strategy>>& vecStrategy)
{
	m_vecStrategy = vecStrategy;
}

bool Solution::buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock, const IntDateTime& date, const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	return false;
}

bool Solution::sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock, const IntDateTime& date, const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	return false;
}

SolutionType Solution::type()
{
	return m_solutionType;
}
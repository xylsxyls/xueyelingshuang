#include "Solution.h"

void Solution::init(const std::vector<std::shared_ptr<Strategy>>& vecStrategy)
{
	m_vecStrategy = vecStrategy;
}

SolutionType Solution::type()
{
	return m_solutionType;
}
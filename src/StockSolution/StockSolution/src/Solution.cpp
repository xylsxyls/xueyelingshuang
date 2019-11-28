#include "Solution.h"

void Solution::init(const std::shared_ptr<Strategy>& spStrategy)
{
	m_spStrategy = spStrategy;
}

SolutionType Solution::type()
{
	return m_solutionType;
}
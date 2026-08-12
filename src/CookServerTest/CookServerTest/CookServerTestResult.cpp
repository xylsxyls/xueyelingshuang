#include "CookServerTestResult.h"

CookServerTestCaseResult::CookServerTestCaseResult() :
m_pass(false),
m_statusCode(0),
m_costMs(0)
{

}

CookServerStressResult::CookServerStressResult() :
m_executed(false),
m_pass(false),
m_threadCount(0),
m_loopCount(0),
m_totalRequests(0),
m_successRequests(0),
m_failedRequests(0),
m_totalCostMs(0),
m_maxCostMs(0),
m_minCostMs(0),
m_wallCostMs(0)
{

}
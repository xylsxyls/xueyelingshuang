#include "CTaskPoolModels.h"

CTaskPoolLoad::CTaskPoolLoad() :
m_initialThreads(0),
m_currentThreads(0),
m_successfulGrowCount(0),
m_queuedTasks(0),
m_runningThreads(0),
m_idleThreads(0),
m_idleMilliseconds(0),
m_sinceResizeMilliseconds(0)
{

}

CTaskPoolDecision::CTaskPoolDecision(size_t target, uint64_t idleMilliseconds) :
m_targetThreads(target),
m_shrinkIdleMilliseconds(idleMilliseconds)
{

}

CTaskThreadPoolConfig::CTaskThreadPoolConfig() :
m_initialThreads(2),
m_maximumThreads(64),
m_checkMilliseconds(100)
{

}

CTaskPoolStatistics::CTaskPoolStatistics() :
m_threads(0),
m_grows(0),
m_shrinks(0),
m_errors(0),
m_accepting(false)
{

}
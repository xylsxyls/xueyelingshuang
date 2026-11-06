#include "CTaskPoolPolicy.h"
#include <algorithm>
#include <limits>

CTaskPoolPolicy::~CTaskPoolPolicy()
{

}

CTaskPoolDecision CTaskPoolPolicy::evaluate(const CTaskPoolLoad& load)
{
    if (load.m_idleThreads == 0 && load.m_queuedTasks >= load.m_currentThreads)
    {
        const size_t increment = (std::max)(size_t(1), load.m_currentThreads / 2);
        const size_t maximum = (std::numeric_limits<size_t>::max)();
        return CTaskPoolDecision(load.m_currentThreads > maximum - increment ? maximum : load.m_currentThreads + increment);
    }
    if (load.m_currentThreads > load.m_initialThreads && load.m_idleThreads == load.m_currentThreads)
    {
        return CTaskPoolDecision(load.m_currentThreads - 1, 1000);
    }
    return CTaskPoolDecision(load.m_currentThreads);
}
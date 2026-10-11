#include "CookServerTestStressTask.h"

CookServerTestStressTask::CookServerTestStressTask(const std::function<void(const std::atomic<bool>&)>& operation) :
CTask(StressWork), m_operation(operation), m_stop(false), m_failed(false)
{
}

void CookServerTestStressTask::DoTask()
{
    try { m_operation(m_stop); }
    catch (...) { m_failed.store(true); }
}

void CookServerTestStressTask::StopTask()
{
    m_stop.store(true);
}

bool CookServerTestStressTask::failed() const
{
    return m_failed.load();
}
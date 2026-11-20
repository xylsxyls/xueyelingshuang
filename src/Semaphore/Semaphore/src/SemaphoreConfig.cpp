#include "SemaphoreConfig.h"

SemaphoreWaitPolicy::SemaphoreWaitPolicy() :
m_promotionMilliseconds(32),
m_localSlice(32),
m_mobileSlice(1),
m_posixSlice(2),
m_capacityAttempts(4096),
m_capacitySleep(1),
m_contentionYieldInterval(16),
m_initialPauses(16),
m_maximumPauses(256)
{

}

SemaphoreConfig::SemaphoreConfig() :
m_prepareEvent(false),
m_prepareCount(false)
{

}

SemaphorePoolConfig& SemaphoreConfig::configureEventPool()
{
    m_prepareEvent = true;
    return m_event;
}

SemaphorePoolConfig& SemaphoreConfig::configureCountPool()
{
    m_prepareCount = true;
    return m_count;
}

SemaphoreResult SemaphoreConfig::setWaitPolicy(const SemaphoreWaitPolicy& policy)
{
    if (policy.m_localSlice == 0 || policy.m_localSlice == UINT32_MAX || policy.m_promotionMilliseconds == UINT32_MAX ||
        policy.m_mobileSlice == 0 || policy.m_mobileSlice > 999 || policy.m_posixSlice == 0 || policy.m_posixSlice > 999 ||
        policy.m_capacitySleep == 0 || policy.m_capacitySleep == UINT32_MAX || policy.m_contentionYieldInterval == 0 ||
        policy.m_initialPauses > policy.m_maximumPauses || policy.m_maximumPauses > 1024)
    {
        return SemaphoreResult(SEMAPHORE_INVALID);
    }
    m_policy = policy;
    return SemaphoreResult();
}

const SemaphoreWaitPolicy& SemaphoreConfig::waitPolicy() const
{
    return m_policy;
}

const SemaphorePoolConfig* SemaphoreConfig::eventPool() const
{
    return m_prepareEvent ? &m_event : nullptr;
}

const SemaphorePoolConfig* SemaphoreConfig::countPool() const
{
    return m_prepareCount ? &m_count : nullptr;
}
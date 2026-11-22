#pragma once

template<class T>
LockFreeQueueResult LibuvPoolSettings::configureQueue(LockFreeQueue<T>& queue, const LibuvQueueSettings& config)
{
    LockFreeQueueResult result = queue.configurePool(config);
    if (result == LockFreeQueueSuccess)
    {
        result = queue.configureAutomaticMaintenance(config.m_maintenanceInterval, config.m_maintenanceBudget);
    }
    if (result == LockFreeQueueSuccess)
    {
        result = queue.configureCapacityWait(config.m_capacityRetries, config.m_sleepMilliseconds);
    }
    if (result == LockFreeQueueSuccess)
    {
        result = queue.configureConsumerYieldInterval(config.m_consumerInterval);
    }
    if (result == LockFreeQueueSuccess)
    {
        result = queue.configureProducerCounters(config.m_distributed);
    }
    if (result == LockFreeQueueSuccess)
    {
        result = queue.configureCancelableReservations(config.m_cancelable);
    }
    if (result == LockFreeQueueSuccess)
    {
        result = queue.setGrowPolicy(config.m_growPolicy);
    }
    if (result == LockFreeQueueSuccess)
    {
        result = queue.setShrinkPolicy(config.m_shrinkPolicy);
    }
    return result;
}
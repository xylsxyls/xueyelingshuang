#pragma once

template<class T>
inline bool ProcessWorkQueueOptions::apply(LockFreeQueue<T>& queue, bool postQueue) const
{
    return valid() &&
        queue.configureProducerCounters(m_distributedCounters < 0 ? postQueue : m_distributedCounters != 0) == LockFreeQueueSuccess &&
        queue.configureAutomaticMaintenance(m_maintenanceInterval, m_maintenanceBudget) == LockFreeQueueSuccess &&
        queue.configureCapacityWait(m_capacityRetries, m_capacitySleepMilliseconds) == LockFreeQueueSuccess &&
        queue.configureConsumerYieldInterval(m_consumerYieldInterval) == LockFreeQueueSuccess &&
        queue.configureCancelableReservations(m_cancelableReservations) == LockFreeQueueSuccess;
}
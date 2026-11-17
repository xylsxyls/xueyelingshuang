#ifndef LOCK_FREE_QUEUE_BENCHMARK_CONTROL_INL
#define LOCK_FREE_QUEUE_BENCHMARK_CONTROL_INL

inline LockFreeQueueBenchmarkControl::LockFreeQueueBenchmarkControl() :
m_start(false), m_stop(false), m_failed(false), m_ready(0), m_done(0), m_ended(0), m_refused(0), m_capacityRefused(0)
{

}

#endif
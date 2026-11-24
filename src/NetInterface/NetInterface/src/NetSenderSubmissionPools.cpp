#include "NetSenderSubmissionPools.h"
#include "NetInterfaceMacro.h"
#include "NetSenderSubmissionState.h"

NetSenderSubmissionOptions::NetSenderSubmissionOptions() :
m_requestBuckets(128), m_receiptBuckets(128), m_distributedCounters(true), m_cancelableReservations(false),
m_maintenanceInterval(64), m_maintenanceBudget(8), m_capacityRetriesBeforeSleep(4096), m_capacitySleepMilliseconds(1),
m_consumerYieldInterval(4096), m_inputBatchBudget(8), m_inputBatchSize(32),
m_queuePollBudget(16), m_requestPollBudget(16), m_receiptPollBudget(16), m_deliveryTimeoutMs(5000)
{

}

/** 按实际池槽大小换算默认物理存储预算，调用者可显式覆盖
@param [in] slotBytes 包含引用计数与池槽元数据的单槽大小
@return 初始化后可增长的物理池配置
*/
static LockFreeHashMapPoolConfig NetSubmissionStorageConfig(size_t slotBytes)
{
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 128;
    config.maximumCapacity = 8388608 / slotBytes;
    config.directoryCapacity = 128;
    config.lookupCacheCapacity = 0;
    return config;
}

NetSenderSubmissionPools::NetSenderSubmissionPools()
{
    m_requests = NetSubmissionStorageConfig(sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetSenderSubmissionEntry>>));
    m_frames = NetSubmissionStorageConfig(sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetSenderSubmissionEntry>>));
    m_controls = NetSubmissionStorageConfig(sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetSenderSubmissionEntry>>));
    m_fronts = NetSubmissionStorageConfig(sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetSenderSubmissionFront>>));
    m_receipts = NetSubmissionStorageConfig(sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetSenderSubmissionReceipt>>));
    m_inputs = NetSubmissionStorageConfig(sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetSenderSubmissionInput>>));
    m_payloads = NetSubmissionStorageConfig(sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetSenderSubmissionPayload>>));
    m_controlPayloads = NetSubmissionStorageConfig(sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetSenderSubmissionPayload>>));
    m_requestIndex = NetSubmissionStorageConfig(sizeof(LockFreeHashMap<uint64_t, NetObjectReference<NetSenderSubmissionFront>>::Slot));
    m_receiptIndex = NetSubmissionStorageConfig(sizeof(LockFreeHashMap<uint64_t, NetObjectReference<NetSenderSubmissionReceipt>>::Slot));
    m_queue.initialCapacity = 128;
    m_queue.maximumCapacity = 65536;
    m_queue.maximumProducers = 128;
}
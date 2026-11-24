#include "NetConnectionPoolConfig.h"
#include "NetInterfaceMacro.h"
#include "NetConnectionQueue.h"
#include "NetInterfaceTypes.h"

NetConnectionPoolConfig::NetConnectionPoolConfig() :
m_decoderRetentionBytes(NET_INTERFACE_DEFAULT_DECODER_RETENTION),
m_compressionMinPayload(1024),
m_controlBurst(32),
m_controlPayloadLimit(4096),
m_progressFrameBudget(512),
m_progressStepBudget(64),
m_compressionLevel(1)
{
    m_dataRecords.initialCapacity = 128;
    m_dataRecords.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetConnectionWrite>>);
    m_dataRecords.directoryCapacity = 128;
    m_dataRecords.lookupCacheCapacity = 0;
    m_controlRecords = m_dataRecords;
}

bool NetConnectionPoolConfig::validCodecSettings() const
{
    return m_decoderRetentionBytes >= -1 && m_compressionLevel >= 0 && m_compressionLevel <= 9 &&
        m_controlBurst != 0 && m_controlPayloadLimit >= -1 && m_progressFrameBudget != 0 && m_progressStepBudget != 0;
}
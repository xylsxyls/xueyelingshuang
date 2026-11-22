#include "ProcessWorkConfig.h"

ProcessWorkConfig::ProcessWorkConfig() :
m_sendMappingReserveBytes(m_receiveMemory.m_reserveBytes), m_sendMappingReservePercent(m_receiveMemory.m_reservePercent),
m_configureManager(false)
{

}

bool ProcessWorkConfig::setSendMappingReserve(uint64_t bytes, uint32_t percent)
{
    if (percent > 90)
    {
        return false;
    }
    m_sendMappingReserveBytes = bytes;
    m_sendMappingReservePercent = percent;
    return true;
}
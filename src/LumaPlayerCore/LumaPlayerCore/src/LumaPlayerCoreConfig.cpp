#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerCoreConfig.h"

LumaPlayerCoreConfig::LumaPlayerCoreConfig() :
m_controlCommandTimeoutMs(5000),
m_openCommandTimeoutMs(30000),
m_idleWaitMs(10),
m_videoEarlyTolerance100ns(20000),
m_audioLead100ns(1200000),
m_loopBufferMaxBytes(64 * 1024 * 1024),
m_defaultFrameDuration100ns(400000),
m_defaultRatePermille(1000),
m_minRatePermille(100),
m_maxRatePermille(3000)
{

}
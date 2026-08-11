#include "LogManagerConfig.h"

LogManagerConfig::LogManagerConfig() :
m_fileId(0),
m_path(""),
m_maxFileBytes(100LL * 1024LL * 1024LL),
m_maxFileCount(5),
m_checkFileSizeInterval(100),
m_outputConsole(false),
m_archiveOldLog(true)
{

}
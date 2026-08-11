#include "LogManagerFile.h"
#include "LogManagerHelper.h"

LogManagerFile::LogManagerFile() :
m_config(),
m_logDir(""),
m_baseName(""),
m_linkPath(""),
m_currentEntityPath(""),
m_logFile(nullptr),
m_currentIndex(0),
m_writeCountSinceSizeCheck(0),
m_currentFileLock(nullptr),
m_linkFileLock(nullptr)
{

}

LogManagerFile::~LogManagerFile()
{
	reset();
}

void LogManagerFile::reset()
{
	if (m_logFile != nullptr)
	{
		m_logFile->flush();
		m_logFile->close();
		delete m_logFile;
		m_logFile = nullptr;
	}
	LogManagerHelper::closeFileLock(m_currentFileLock);
	LogManagerHelper::closeFileLock(m_linkFileLock);
	m_currentEntityPath.clear();
	m_currentIndex = 0;
	m_writeCountSinceSizeCheck = 0;
}
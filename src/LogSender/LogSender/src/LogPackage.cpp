#include "LogPackage.h"

LogPackage::LogPackage(LogLevel logLevel,
					   const std::string& fileName,
					   const std::string& funName,
					   bool isSendNet,
					   bool isSendScreen,
					   bool isWriteLog)
{
	m_logLevel = logLevel;
	m_fileName = fileName;
	m_funName = funName;
	m_isSendNet = isSendNet;
	m_isSendScreen = isSendScreen;
	m_isWriteLog = isWriteLog;
}
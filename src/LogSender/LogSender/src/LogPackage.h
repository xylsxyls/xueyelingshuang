#pragma once

class LogPackage
{
public:
	enum LogLevel
	{
		LOG_DEBUG = 0x00000001,
		LOG_INFO = 0x00000010,
		LOG_WARNING = 0x00000100,
		LOG_ERROR = 0x00001000,
		LOG_FATAL = 0x00010000,
	};
	
	//��־�ȼ�
	LogLevel m_logLevel;
	//�����־���ļ���
	std::string m_fileName;
	//�����־�ĺ�����
	std::string m_funName;
	//�Ƿ������緢��
	bool m_isSendNet;
	//�Ƿ�����Ļ����
	bool m_isSendScreen;
	//�Ƿ�д����־
	bool m_isWriteLog;

	/** ���캯��
	*/
	LogPackage(LogLevel logLevel,
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
};
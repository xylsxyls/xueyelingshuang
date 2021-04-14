#pragma once

#ifdef _MSC_VER
struct __declspec(novtable) LogPackage
#elif __unix__
struct LogPackage
#endif
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
	//�Ƿ������緢��
	bool m_isSendNet;
	//�Ƿ�����Ļ����
	bool m_isSendScreen;
	//�Ƿ�д����־
	bool m_isWriteLog;
	//�����־���ļ���
	const char* m_fileName;
	//�����־�ĺ�����
	const char* m_funName;
	//��־�ļ����ļ���
	const char* m_logName;

	/** ���캯��
	*/
	LogPackage(LogLevel logLevel,
		bool isSendNet,
		bool isSendScreen,
		bool isWriteLog,
		const char* fileName,
		const char* funName,
		const char* logName)
	{
		m_logLevel = logLevel;
		m_isSendNet = isSendNet;
		m_isSendScreen = isSendScreen;
		m_isWriteLog = isWriteLog;
		m_fileName = fileName;
		m_funName = funName;
		m_logName = logName;
	}
};
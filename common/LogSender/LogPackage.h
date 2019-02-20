#pragma once

struct __declspec(novtable) LogPackage
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

	/** ���캯��
	*/
	LogPackage(LogLevel logLevel,
		bool isSendNet,
		bool isSendScreen,
		bool isWriteLog,
		const char* fileName,
		const char* funName)
	{
		m_logLevel = logLevel;
		m_isSendNet = isSendNet;
		m_isSendScreen = isSendScreen;
		m_isWriteLog = isWriteLog;
		m_fileName = fileName;
		m_funName = funName;
	}
};
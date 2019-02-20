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

	//日志等级
	LogLevel m_logLevel;
	//是否向网络发送
	bool m_isSendNet;
	//是否向屏幕发送
	bool m_isSendScreen;
	//是否写入日志
	bool m_isWriteLog;
	//输出日志的文件名
	const char* m_fileName;
	//输出日志的函数名
	const char* m_funName;

	/** 构造函数
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
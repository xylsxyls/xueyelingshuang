#pragma once
#include "CorrespondParamMacro.h"

#define SERVER_NAME "ServerName"
#define CLIENT_NAME "ClientName"
#define LOGIN_NAME "LoginName"
#define CLIENT_ID "ClientId"
#define PREDEFINE "Predefine"

#define LOG_FUN_NAME "LogFunName"
#define LOG_FILE_NAME "LogFileName"
#define LOG_PROCESS_NAME "LogProcessName"
#define LOG_LEVEL "LogLevel"
#define LOG_IS_SEND_SCREEN "IsSendScreen"
#define LOG_IS_SEND_NET "IsSendNet"
#define LOG_IS_WRITE_LOG "IsWriteLog"
#define LOG_BUFFER "Buffer"
#define LOG_THREAD_ID "ThreadId"
#define LOG_INT_DATE_TIME "IntDateTime"
#define LOG_LOGIN_NAME "LoginName"
#define LOG_UNINIT "Uninit"

class CorrespondParamAPI CorrespondParam
{
public:
	enum ProtocolId
	{
		CLIENT_INIT = 0,
		SERVER_INIT,
		PROTO_MESSAGE,
		JSON,
		XML
	};
};
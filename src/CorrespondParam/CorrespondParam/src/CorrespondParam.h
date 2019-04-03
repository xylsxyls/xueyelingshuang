#pragma once
#include "CorrespondParamMacro.h"

enum MyEnum
{
	SERVER_NAME = 1,
	CLIENT_NAME,
	LOGIN_NAME,
	CLIENT_ID,
	PREDEFINE,
	
	LOG_FUN_NAME,
	LOG_FILE_NAME,
	LOG_PROCESS_NAME,
	LOG_LEVEL,
	LOG_IS_SEND_SCREEN,
	LOG_IS_SEND_NET,
	LOG_IS_WRITE_LOG,
	LOG_BUFFER,
	LOG_THREAD_ID,
	LOG_INT_DATE_TIME,
	LOG_LOGIN_NAME,
	LOG_UNINIT,
};


class CorrespondParamAPI CorrespondParam
{
public:
	enum ProtocolId
	{
		CLIENT_INIT = 0,
		SERVER_INIT,
		PROTO_MESSAGE,
		JSON,
		XML,
		HEART
	};
};
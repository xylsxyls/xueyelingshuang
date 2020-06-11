#pragma once
#include "CorrespondParamMacro.h"

enum MyEnum
{
	CLIENT_PID = 1,
	CONNECT_ID,
	CLIENT_NAME,
	SERVER_NAME,
	LOGIN_NAME,

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
	LOG_UNINIT
};

enum StockEnum
{
	STOCK_RSI6 = 0,
	STOCK_RSI12,
	STOCK_RSI24,
	STOCK_WR10,
	STOCK_WR20
};


class CorrespondParamAPI CorrespondParam
{
public:
	enum ProtocolId
	{
		INIT,
		CLIENT_INIT,
		SERVER_INIT,
		PROTO_MESSAGE,
		JSON,
		XML,
		HEART
	};
};
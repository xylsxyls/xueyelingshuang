#pragma once
#define _CMysqlAPI
#include "CMysql.h"
#include "CIp.h"
#include "CUser.h"
#include "CDataBase.h"
#include "CTable.h"
#include "CField.h"
#include "CFieldValue.h"
#include "CRecord.h"

#ifdef NDEBUG
	#pragma comment(lib,"CMysqlAPI.lib")
#else
	#pragma comment(lib,"CMysqlAPId.lib")
#endif
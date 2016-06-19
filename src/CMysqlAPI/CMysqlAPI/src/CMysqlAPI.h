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

#ifdef _DEBUG
	#pragma comment(lib,"CMysqlAPId.lib")
#else
	#pragma comment(lib,"CMysqlAPI.lib")
#endif
#pragma once
#define _CMysqlAPI
#include "CMysql.h"
#include "CAttri.h"
#include "CCondition.h"
#include "CDataBase.h"
#include "CHostIP.h"
#include "CMysqlManager.h"
#include "CRecord.h"
#include "CSelect.h"
#include "CTable.h"
#include "CTableField.h"
#include "CUpdate.h"
#include "CUser.h"
#include "CValue.h"

#ifdef _DEBUG
#pragma comment(lib,"CMysqld.lib")
#else
#pragma comment(lib,"CMysql.lib")
#endif
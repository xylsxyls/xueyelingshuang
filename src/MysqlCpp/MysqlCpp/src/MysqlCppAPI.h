#pragma once
#ifdef _WIN32
#define _MysqlCppAPI
#endif
#include "MysqlCpp.h"
#include "MysqlSqlString.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"MysqlCppd.lib")
#else
#pragma comment(lib,"MysqlCpp.lib")
#endif
#endif
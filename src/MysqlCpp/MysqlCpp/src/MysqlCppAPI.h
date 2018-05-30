#pragma once
#define _MysqlCppAPI
#include "MysqlCpp.h"

#ifdef _DEBUG
#pragma comment(lib,"MysqlCppd.lib")
#else
#pragma comment(lib,"MysqlCpp.lib")
#endif
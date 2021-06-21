#pragma once
#ifdef _WIN32
#define _QSQLiteAPI
#endif
#include "QSQLite.h"
#include "QSQLiteSqlString.h"
#include "QSQLitePrepareStatement.h"
#include "QSQLiteResultSet.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"QSQLited.lib")
#else
#pragma comment(lib,"QSQLite.lib")
#endif
#endif
#pragma once
#ifdef _WIN32
#define _SQLiteAPI
#endif
#include "SQLite.h"
#include "SQLitePrepareStatement.h"
#include "SQLiteResultSet.h"
#include "sqlite3.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"SQLited.lib")
#else
#pragma comment(lib,"SQLite.lib")
#endif
#endif
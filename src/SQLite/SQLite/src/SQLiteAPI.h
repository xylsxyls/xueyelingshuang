#pragma once
#define _SQLiteAPI
#include "SQLite.h"

#ifdef _DEBUG
#pragma comment(lib,"SQLited.lib")
#else
#pragma comment(lib,"SQLite.lib")
#endif
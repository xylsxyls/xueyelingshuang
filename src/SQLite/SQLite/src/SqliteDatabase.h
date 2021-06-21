#pragma once
#include <stdint.h>

struct sqlite3;
struct sqlite3_stmt;
struct SqliteDatabase
{
	sqlite3* m_db;
	sqlite3_stmt* m_stmt;
	int32_t m_stepResult;
	bool m_isBeginRead;

	SqliteDatabase();

	~SqliteDatabase();
};
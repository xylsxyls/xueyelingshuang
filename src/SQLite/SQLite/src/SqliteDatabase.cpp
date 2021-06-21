#include "SqliteDatabase.h"
#include "sqlite3.h"
#include <stdint.h>
#include <stdio.h>

SqliteDatabase::SqliteDatabase():
m_db(nullptr),
m_stmt(nullptr),
m_stepResult(0),
m_isBeginRead(false)
{

}

SqliteDatabase::~SqliteDatabase()
{
	if (m_stmt != nullptr)
	{
		int32_t result = sqlite3_finalize(m_stmt);
		if (result != 0)
		{
			printf("~SqliteStmt sqlite3_finalize failed, result = %d\n", result);
		}
		m_stmt = nullptr;
		m_stepResult = 0;
		m_isBeginRead = false;
	}

	if (m_db != nullptr)
	{
		int32_t result = sqlite3_close(m_db);
		if (result != 0)
		{
			printf("sqlite3_close failed, result = %d\n", result);
		}
		m_db = nullptr;
	}
}
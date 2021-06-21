#pragma once
#include "SQLiteMacro.h"
#include <stdint.h>
#include <string>
#include <memory>

struct SqliteDatabase;

/** sql��������
*/
class SQLiteAPI SQLitePrepareStatement
{
	friend class SQLiteResultSet;
public:
	/** ���캯��
	@param [in] db ���ָ��
	*/
	SQLitePrepareStatement(const std::shared_ptr<SqliteDatabase>& spDb);

public:
	bool empty();

	bool prepare(const std::string& sqlString);

	void setBlob(uint32_t index, const std::string& value);

	void setBoolean(uint32_t index, bool value);

	void setInt(uint32_t index, int32_t value);

	void setDouble(uint32_t index, double value);

	//����\0��ֹͣ�洢�ˣ��洢���ݲ���\0
	void setString(uint32_t index, const std::string& value);

	void setUnsignedInt(uint32_t index, uint32_t value);

	void setLongLong(uint32_t index, int64_t value);

	void setUnsignedLongLong(uint32_t index, uint64_t value);

	bool exec();

	//����Ҫ���������滻�ַ�����ʱ��Ҫ��ִ��reset��reset���ִ�н����գ����������prepare���ַ���
	void reset();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<SqliteDatabase> m_spDb;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
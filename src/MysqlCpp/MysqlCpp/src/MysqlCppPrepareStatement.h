#pragma once
#include <string>
#include <stdint.h>
#include "MysqlCppMacro.h"

namespace sql
{
	class PreparedStatement;
}

/** �����ͬ��sql::PrepareStatement
*/
class MysqlCppAPI MysqlCppPrepareStatement
{
public:
	/** ���캯��
	@param [in] prepareStatement ʵ�ʵĲ���ָ��
	*/
	MysqlCppPrepareStatement(sql::PreparedStatement* prepareStatement);

	/** ��������
	*/
	~MysqlCppPrepareStatement();

public:
	void setBigInt(unsigned int parameterIndex, const std::string& value);

	void setBlob(unsigned int parameterIndex, std::istream * blob);

	void setBoolean(unsigned int parameterIndex, bool value);

	void setDateTime(unsigned int parameterIndex, const std::string& value);

	void setDouble(unsigned int parameterIndex, double value);

	void setInt(unsigned int parameterIndex, int32_t value);

	void setUInt(unsigned int parameterIndex, uint32_t value);

	void setInt64(unsigned int parameterIndex, int64_t value);

	void setUInt64(unsigned int parameterIndex, uint64_t value);

	void setNull(unsigned int parameterIndex, int sqlType);

	void setString(unsigned int parameterIndex, const std::string& value);

	sql::PreparedStatement* preparedStatement() const;

private:
	sql::PreparedStatement* m_preparedStatement;
};
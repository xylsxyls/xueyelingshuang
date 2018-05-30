#include <SDKDDKVer.h>
#include "CTable.h"
#include "CMysqlManager.h"
#include "CDataBase.h"
#include "CCondition.h"
#include "CUpdate.h"
#include "CSelect.h"
#include "mysql.h"

CTable::CTable() :
m_recordError(NULL)
{
	//从外部导入管理者指针
	m_pMysqlManager = NULL;
	m_ifHasT_e_s_t = 0;
	m_pDataBase = NULL;
}

CTable::CTable(CMysqlManager *pMysqlManager, CDataBase *pDataBase, const std::string& TableName, bool IfHasT_e_s_t) :
m_recordError(NULL)
{
	//从外部导入管理者指针
	m_pMysqlManager = pMysqlManager;
	//先把新开的指针放到容器里
	m_pMysqlManager->m_listTable.push_back(this);

	m_tableName = TableName;
	m_ifHasT_e_s_t = IfHasT_e_s_t;
	m_pDataBase = pDataBase;
}

void CTable::Close()
{
	//因为在类内，所以无法直接delete this，也就无法通过管理者删除空间了，只能内部手动释放mysql连接线，然后在外部释放本类

	//找到容器里这个指针并删除，删除管理者中本类的地址，在析构执行完毕之后本类空间也被释放
	m_pMysqlManager->DeleteOne(this);
}

CRecord& CTable::operator[](int32_t num)
{
	int32_t n = 0;
	for (auto it = m_listRecord.begin(); it != m_listRecord.end(); it++)
{
		if (n == num) return *it;
		n++;
	}
	return m_recordError;
}

int32_t CTable::Add(CRecord* pRecord)
{
	//因为Add函数会多次操作数据库，所以不论在外部有没有设置回滚，函数内部都要设置回滚

	//查看是否有新字段添加，但是不为新字段设置属性
	for (auto itFieldInRecord = pRecord->m_mapValue.begin(); itFieldInRecord != pRecord->m_mapValue.end(); itFieldInRecord++)
{
		for (auto itFieldInTable = m_mapAttri.begin(); itFieldInTable != m_mapAttri.end(); itFieldInTable++)
{
			//如果记录中有一个字段和Table中的相等了说明不是新字段
			if (itFieldInRecord->first == itFieldInTable->first) goto Next;
		}
		//如果自然退出则说明全都不相等则说明该字段是新字段
		int32_t nResult = AddNewFieldWithNoAttri(itFieldInRecord->first);
		//如果添加新字段失败则返回
		if (nResult != 0) return nResult;
	Next:;
	}

	//根据listReviseAttri提供的修改属性的字段名进行属性修改
	if (pRecord->m_listReviseAttri.size() != 0)
{
		for (auto itRevise = pRecord->m_listReviseAttri.begin(); itRevise != pRecord->m_listReviseAttri.end(); itRevise++)
{
			int32_t nResult = ReviseAttri(&m_mapAttri[*itRevise]);
			//说明有属性修改不成功
			if (nResult != 0) return nResult;
		}
	}
	//修改完清空
	pRecord->m_listReviseAttri.clear();

	//在属性和字段添加全部成功之后添加记录
	std::string SQL = "INSERT INTO " + m_tableName + " SET " + pRecord->toString();

	//首先尝试添加，如果添加成功说明用户的目的为添加记录
	return mysql_query(m_pDataBase->m_mysql, SQL.c_str());
}

int32_t CTable::Delete(CCondition* pCondition)
{
	std::string SQL = "DELETE FROM " + m_tableName + " WHERE " + pCondition->toString();

	return mysql_query(m_pDataBase->m_mysql, SQL.c_str());
}

int32_t CTable::UpdateRecord(CUpdate* pUpdate, CCondition* pCondition)
{
	std::string SQL = "UPDATE " + m_tableName + " SET " + pUpdate->toString() + " WHERE " + pCondition->toString();
	return mysql_query(m_pDataBase->m_mysql, SQL.c_str());
}

CTable CTable::SelectRecord(CSelect *pSelect, CCondition* pCondition)
{
	//用于存放查询到的记录

	CTable ResultTable;

	std::string SQL = "SELECT " + pSelect->toString() + " FROM " + pSelect->toTableString() + " WHERE " + pCondition->toString();

	int32_t nResult = mysql_query(m_pDataBase->m_mysql, SQL.c_str());
	if (nResult != 0) return CTable();

	//存储得到的结果，必须先有这步才可以查询字段属性信息
	MYSQL_RES* result = mysql_store_result(m_pDataBase->m_mysql);
	//先取出字段属性信息
	MYSQL_RES* resField = mysql_list_fields(m_pDataBase->m_mysql, m_tableName.c_str(), "%");

	//从pSelect中获取字段名
	auto itSelect = pSelect->m_listTf.begin();
	//先存字段属性
	int32_t j = -1;
	while (j++ != resField->field_count - 1)
{
		std::string strFieldName = itSelect->m_table + "." + itSelect->m_field;
		itSelect++;
		//存储
		ResultTable.m_mapAttri[strFieldName].m_type = ((resField->fields) + j)->type;
		ResultTable.m_mapAttri[strFieldName].m_length = ((resField->fields) + j)->length;
		ResultTable.m_mapAttri[strFieldName].m_strFieldName = strFieldName;
		//得到属性集
		int32_t flags = ((resField->fields) + j)->flags;
		if ((flags & NOT_NULL_FLAG) == NOT_NULL_FLAG) ResultTable.m_mapAttri[strFieldName].m_bNotNull = 1;
		if ((flags & PRI_KEY_FLAG) == PRI_KEY_FLAG) ResultTable.m_mapAttri[strFieldName].m_bPrimaryKey = 1;
		if ((flags & UNIQUE_KEY_FLAG) == UNIQUE_KEY_FLAG) ResultTable.m_mapAttri[strFieldName].m_bUniqueKey = 1;
		if ((flags & BLOB_FLAG) == BLOB_FLAG) ResultTable.m_mapAttri[strFieldName].m_bBlob = 1;
		if ((flags & ZEROFILL_FLAG) == ZEROFILL_FLAG) ResultTable.m_mapAttri[strFieldName].m_bZeroFill = 1;
		if ((flags & AUTO_INCREMENT_FLAG) == AUTO_INCREMENT_FLAG) ResultTable.m_mapAttri[strFieldName].m_bAutoIncrement = 1;
		//如果有默认值获取默认值
		if ((std::string)((resField->fields) + j)->def != "")
{
			ResultTable.m_mapAttri[strFieldName].m_hasDefault = 1;
			ResultTable.m_mapAttri[strFieldName].m_vDefault = (std::string)((resField->fields) + j)->def;
		}
	}

	//开始获取字段值
	int32_t i = -1;
	//行数
	while (i++ != result->row_count - 1)
{
		int32_t j = -1;
		//先找到第一行的值，然后根据当前行数转next找到对应的行
		MYSQL_ROWS* pRow = result->data->data;
		int32_t temp = i;
		while (temp-- != 0) pRow = pRow->next;
		//准备一个空记录
		CRecord record(&ResultTable);
		//从pSelect中获取字段名
		auto itSelect = pSelect->m_listTf.begin();
		//列数循环，把有效值循环加到记录中
		while (j++ != result->field_count - 1)
{
			//分别获得每列的字段名
			std::string FieldName = itSelect->m_table + "." + itSelect->m_field;
			itSelect++;
			//字段值
			std::string strValue = pRow->data[j];
			//把字符串根据类型做转换
			if (m_mapAttri[FieldName].m_type == 253)
{
				//形成一个记录
				record[FieldName] = strValue;
			}
			else if (m_mapAttri[FieldName].m_type == 3)
{
				record[FieldName] = atoi(strValue.c_str());
			}
			else if (m_mapAttri[FieldName].m_type == 5)
{
				record[FieldName] = (double)atof(strValue.c_str());
			}
		}
		ResultTable.m_listRecord.push_back(record);
	}
	return ResultTable;
}

int32_t CTable::ReviseAttri(CAttri* pAttri)
{
	return 0;
}

int32_t CTable::AddNewFieldWithNoAttri(const std::string& strFieldName)
{
	return 0;
}

void CTable::ImportTable(const std::string& mysql_exe_path, const std::string& sqlpath)
{
	std::string para = "";
	std::string strPasswd = m_pDataBase->m_mysql->passwd;
	//首先是主机和用户名
	para = para + " -h" + m_pDataBase->m_mysql->host + " -u" + m_pDataBase->m_mysql->user;
	//如果有密码加上密码
	if (strPasswd != "")  para = para + " -p" + strPasswd;
	//最后是需要导入的数据库名
	para = para + " " + m_pDataBase->m_mysql->db + " < " + sqlpath;

	//执行程序，ShellExecute不行
	system(("\"" + mysql_exe_path + "\"" + para).c_str());
	//ShellExecute(NULL,_T("open"),_T(mysql_exe_path),para,NULL,SW_SHOWNORMAL);
	return;
}

void CTable::ExportTable(const std::string& mysqldump_exe_path, const std::string& sqlpath)
{
	std::string para = "";
	std::string strPasswd = m_pDataBase->m_mysql->passwd;
	//首先是主机和用户名
	para = para + " -h" + m_pDataBase->m_mysql->host + " -u" + m_pDataBase->m_mysql->user;
	//如果有密码加上密码
	if (strPasswd != "")  para = para + " -p" + strPasswd;
	//最后是需要导出的数据库和表名
	para = para + " " + m_pDataBase->m_mysql->db + " " + m_tableName + " > " + sqlpath;

	//执行程序，ShellExecute不行
	system(("\"" + mysqldump_exe_path + "\"" + para).c_str());
	//ShellExecute(NULL,_T("open"),_T(mysqldump_exe_path),para,NULL,SW_SHOWNORMAL);
	return;
}

void CTable::Refresh()
{
	mysql_query(m_pDataBase->m_mysql, ("SELECT * FROM " + m_tableName).c_str());

	//存储得到的结果，必须先有这步才可以查询字段属性信息
	MYSQL_RES* result = mysql_store_result(m_pDataBase->m_mysql);
	//先取出字段属性信息
	MYSQL_RES* resField = mysql_list_fields(m_pDataBase->m_mysql, m_tableName.c_str(), "%");

	//先存字段属性
	int32_t j = -1;
	while (j++ != resField->field_count - 1)
{
		std::string strFieldName = m_tableName + "." + ((resField->fields) + j)->name;
		//存储
		m_mapAttri[strFieldName].m_type = ((resField->fields) + j)->type;
		m_mapAttri[strFieldName].m_length = ((resField->fields) + j)->length;
		m_mapAttri[strFieldName].m_strFieldName = strFieldName;
		//得到属性集
		int32_t flags = ((resField->fields) + j)->flags;
		if ((flags & NOT_NULL_FLAG) == NOT_NULL_FLAG) m_mapAttri[strFieldName].m_bNotNull = 1;
		if ((flags & PRI_KEY_FLAG) == PRI_KEY_FLAG) m_mapAttri[strFieldName].m_bPrimaryKey = 1;
		if ((flags & UNIQUE_KEY_FLAG) == UNIQUE_KEY_FLAG) m_mapAttri[strFieldName].m_bUniqueKey = 1;
		if ((flags & BLOB_FLAG) == BLOB_FLAG) m_mapAttri[strFieldName].m_bBlob = 1;
		if ((flags & ZEROFILL_FLAG) == ZEROFILL_FLAG) m_mapAttri[strFieldName].m_bZeroFill = 1;
		if ((flags & AUTO_INCREMENT_FLAG) == AUTO_INCREMENT_FLAG) m_mapAttri[strFieldName].m_bAutoIncrement = 1;
		//如果有默认值获取默认值
		if ((std::string)((resField->fields) + j)->def != "")
{
			m_mapAttri[strFieldName].m_hasDefault = 1;
			m_mapAttri[strFieldName].m_vDefault = (std::string)((resField->fields) + j)->def;
		}
	}

	//开始获取字段值
	int32_t i = -1;
	//行数
	while (i++ != result->row_count - 1)
{
		int32_t j = -1;
		//先找到第一行的值，然后根据当前行数转next找到对应的行
		MYSQL_ROWS* pRow = result->data->data;
		int32_t temp = i;
		while (temp-- != 0) pRow = pRow->next;
		//准备一个空记录
		CRecord record(this);
		//列数循环，把有效值循环加到记录中
		while (j++ != result->field_count - 1)
{
			//分别获得每列的字段名
			std::string FieldName = m_tableName + "." + ((result->fields) + j)->name;
			//字段值
			std::string strValue = pRow->data[j];
			//把字符串根据类型做转换
			if (m_mapAttri[FieldName].m_type == 253)
{
				//形成一个记录
				record[FieldName] = strValue;
			}
			else if (m_mapAttri[FieldName].m_type == 3)
{
				record[FieldName] = atoi(strValue.c_str());
			}
			else if (m_mapAttri[FieldName].m_type == 5)
{
				record[FieldName] = (double)atof(strValue.c_str());
			}
		}
		m_listRecord.push_back(record);
	}
	return;
}

int32_t CTable::size()
{
	return m_listRecord.size();
}
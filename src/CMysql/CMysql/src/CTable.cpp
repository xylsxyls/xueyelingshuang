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
	//���ⲿ���������ָ��
	m_pMysqlManager = NULL;
	m_ifHasT_e_s_t = 0;
	m_pDataBase = NULL;
}

CTable::CTable(CMysqlManager *pMysqlManager, CDataBase *pDataBase, const std::string& TableName, bool IfHasT_e_s_t) :
m_recordError(NULL)
{
	//���ⲿ���������ָ��
	m_pMysqlManager = pMysqlManager;
	//�Ȱ��¿���ָ��ŵ�������
	m_pMysqlManager->m_listTable.push_back(this);

	m_tableName = TableName;
	m_ifHasT_e_s_t = IfHasT_e_s_t;
	m_pDataBase = pDataBase;
}

void CTable::Close()
{
	//��Ϊ�����ڣ������޷�ֱ��delete this��Ҳ���޷�ͨ��������ɾ���ռ��ˣ�ֻ���ڲ��ֶ��ͷ�mysql�����ߣ�Ȼ�����ⲿ�ͷű���

	//�ҵ����������ָ�벢ɾ����ɾ���������б���ĵ�ַ��������ִ�����֮����ռ�Ҳ���ͷ�
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
	//��ΪAdd�������β������ݿ⣬���Բ������ⲿ��û�����ûع��������ڲ���Ҫ���ûع�

	//�鿴�Ƿ������ֶ���ӣ����ǲ�Ϊ���ֶ���������
	for (auto itFieldInRecord = pRecord->m_mapValue.begin(); itFieldInRecord != pRecord->m_mapValue.end(); itFieldInRecord++)
{
		for (auto itFieldInTable = m_mapAttri.begin(); itFieldInTable != m_mapAttri.end(); itFieldInTable++)
{
			//�����¼����һ���ֶκ�Table�е������˵���������ֶ�
			if (itFieldInRecord->first == itFieldInTable->first) goto Next;
		}
		//�����Ȼ�˳���˵��ȫ���������˵�����ֶ������ֶ�
		int32_t nResult = AddNewFieldWithNoAttri(itFieldInRecord->first);
		//���������ֶ�ʧ���򷵻�
		if (nResult != 0) return nResult;
	Next:;
	}

	//����listReviseAttri�ṩ���޸����Ե��ֶ������������޸�
	if (pRecord->m_listReviseAttri.size() != 0)
{
		for (auto itRevise = pRecord->m_listReviseAttri.begin(); itRevise != pRecord->m_listReviseAttri.end(); itRevise++)
{
			int32_t nResult = ReviseAttri(&m_mapAttri[*itRevise]);
			//˵���������޸Ĳ��ɹ�
			if (nResult != 0) return nResult;
		}
	}
	//�޸������
	pRecord->m_listReviseAttri.clear();

	//�����Ժ��ֶ����ȫ���ɹ�֮����Ӽ�¼
	std::string SQL = "INSERT INTO " + m_tableName + " SET " + pRecord->toString();

	//���ȳ�����ӣ������ӳɹ�˵���û���Ŀ��Ϊ��Ӽ�¼
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
	//���ڴ�Ų�ѯ���ļ�¼

	CTable ResultTable;

	std::string SQL = "SELECT " + pSelect->toString() + " FROM " + pSelect->toTableString() + " WHERE " + pCondition->toString();

	int32_t nResult = mysql_query(m_pDataBase->m_mysql, SQL.c_str());
	if (nResult != 0) return CTable();

	//�洢�õ��Ľ�������������ⲽ�ſ��Բ�ѯ�ֶ�������Ϣ
	MYSQL_RES* result = mysql_store_result(m_pDataBase->m_mysql);
	//��ȡ���ֶ�������Ϣ
	MYSQL_RES* resField = mysql_list_fields(m_pDataBase->m_mysql, m_tableName.c_str(), "%");

	//��pSelect�л�ȡ�ֶ���
	auto itSelect = pSelect->m_listTf.begin();
	//�ȴ��ֶ�����
	int32_t j = -1;
	while (j++ != resField->field_count - 1)
{
		std::string strFieldName = itSelect->m_table + "." + itSelect->m_field;
		itSelect++;
		//�洢
		ResultTable.m_mapAttri[strFieldName].m_type = ((resField->fields) + j)->type;
		ResultTable.m_mapAttri[strFieldName].m_length = ((resField->fields) + j)->length;
		ResultTable.m_mapAttri[strFieldName].m_strFieldName = strFieldName;
		//�õ����Լ�
		int32_t flags = ((resField->fields) + j)->flags;
		if ((flags & NOT_NULL_FLAG) == NOT_NULL_FLAG) ResultTable.m_mapAttri[strFieldName].m_bNotNull = 1;
		if ((flags & PRI_KEY_FLAG) == PRI_KEY_FLAG) ResultTable.m_mapAttri[strFieldName].m_bPrimaryKey = 1;
		if ((flags & UNIQUE_KEY_FLAG) == UNIQUE_KEY_FLAG) ResultTable.m_mapAttri[strFieldName].m_bUniqueKey = 1;
		if ((flags & BLOB_FLAG) == BLOB_FLAG) ResultTable.m_mapAttri[strFieldName].m_bBlob = 1;
		if ((flags & ZEROFILL_FLAG) == ZEROFILL_FLAG) ResultTable.m_mapAttri[strFieldName].m_bZeroFill = 1;
		if ((flags & AUTO_INCREMENT_FLAG) == AUTO_INCREMENT_FLAG) ResultTable.m_mapAttri[strFieldName].m_bAutoIncrement = 1;
		//�����Ĭ��ֵ��ȡĬ��ֵ
		if ((std::string)((resField->fields) + j)->def != "")
{
			ResultTable.m_mapAttri[strFieldName].m_hasDefault = 1;
			ResultTable.m_mapAttri[strFieldName].m_vDefault = (std::string)((resField->fields) + j)->def;
		}
	}

	//��ʼ��ȡ�ֶ�ֵ
	int32_t i = -1;
	//����
	while (i++ != result->row_count - 1)
{
		int32_t j = -1;
		//���ҵ���һ�е�ֵ��Ȼ����ݵ�ǰ����תnext�ҵ���Ӧ����
		MYSQL_ROWS* pRow = result->data->data;
		int32_t temp = i;
		while (temp-- != 0) pRow = pRow->next;
		//׼��һ���ռ�¼
		CRecord record(&ResultTable);
		//��pSelect�л�ȡ�ֶ���
		auto itSelect = pSelect->m_listTf.begin();
		//����ѭ��������Чֵѭ���ӵ���¼��
		while (j++ != result->field_count - 1)
{
			//�ֱ���ÿ�е��ֶ���
			std::string FieldName = itSelect->m_table + "." + itSelect->m_field;
			itSelect++;
			//�ֶ�ֵ
			std::string strValue = pRow->data[j];
			//���ַ�������������ת��
			if (m_mapAttri[FieldName].m_type == 253)
{
				//�γ�һ����¼
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
	//�������������û���
	para = para + " -h" + m_pDataBase->m_mysql->host + " -u" + m_pDataBase->m_mysql->user;
	//����������������
	if (strPasswd != "")  para = para + " -p" + strPasswd;
	//�������Ҫ��������ݿ���
	para = para + " " + m_pDataBase->m_mysql->db + " < " + sqlpath;

	//ִ�г���ShellExecute����
	system(("\"" + mysql_exe_path + "\"" + para).c_str());
	//ShellExecute(NULL,_T("open"),_T(mysql_exe_path),para,NULL,SW_SHOWNORMAL);
	return;
}

void CTable::ExportTable(const std::string& mysqldump_exe_path, const std::string& sqlpath)
{
	std::string para = "";
	std::string strPasswd = m_pDataBase->m_mysql->passwd;
	//�������������û���
	para = para + " -h" + m_pDataBase->m_mysql->host + " -u" + m_pDataBase->m_mysql->user;
	//����������������
	if (strPasswd != "")  para = para + " -p" + strPasswd;
	//�������Ҫ���������ݿ�ͱ���
	para = para + " " + m_pDataBase->m_mysql->db + " " + m_tableName + " > " + sqlpath;

	//ִ�г���ShellExecute����
	system(("\"" + mysqldump_exe_path + "\"" + para).c_str());
	//ShellExecute(NULL,_T("open"),_T(mysqldump_exe_path),para,NULL,SW_SHOWNORMAL);
	return;
}

void CTable::Refresh()
{
	mysql_query(m_pDataBase->m_mysql, ("SELECT * FROM " + m_tableName).c_str());

	//�洢�õ��Ľ�������������ⲽ�ſ��Բ�ѯ�ֶ�������Ϣ
	MYSQL_RES* result = mysql_store_result(m_pDataBase->m_mysql);
	//��ȡ���ֶ�������Ϣ
	MYSQL_RES* resField = mysql_list_fields(m_pDataBase->m_mysql, m_tableName.c_str(), "%");

	//�ȴ��ֶ�����
	int32_t j = -1;
	while (j++ != resField->field_count - 1)
{
		std::string strFieldName = m_tableName + "." + ((resField->fields) + j)->name;
		//�洢
		m_mapAttri[strFieldName].m_type = ((resField->fields) + j)->type;
		m_mapAttri[strFieldName].m_length = ((resField->fields) + j)->length;
		m_mapAttri[strFieldName].m_strFieldName = strFieldName;
		//�õ����Լ�
		int32_t flags = ((resField->fields) + j)->flags;
		if ((flags & NOT_NULL_FLAG) == NOT_NULL_FLAG) m_mapAttri[strFieldName].m_bNotNull = 1;
		if ((flags & PRI_KEY_FLAG) == PRI_KEY_FLAG) m_mapAttri[strFieldName].m_bPrimaryKey = 1;
		if ((flags & UNIQUE_KEY_FLAG) == UNIQUE_KEY_FLAG) m_mapAttri[strFieldName].m_bUniqueKey = 1;
		if ((flags & BLOB_FLAG) == BLOB_FLAG) m_mapAttri[strFieldName].m_bBlob = 1;
		if ((flags & ZEROFILL_FLAG) == ZEROFILL_FLAG) m_mapAttri[strFieldName].m_bZeroFill = 1;
		if ((flags & AUTO_INCREMENT_FLAG) == AUTO_INCREMENT_FLAG) m_mapAttri[strFieldName].m_bAutoIncrement = 1;
		//�����Ĭ��ֵ��ȡĬ��ֵ
		if ((std::string)((resField->fields) + j)->def != "")
{
			m_mapAttri[strFieldName].m_hasDefault = 1;
			m_mapAttri[strFieldName].m_vDefault = (std::string)((resField->fields) + j)->def;
		}
	}

	//��ʼ��ȡ�ֶ�ֵ
	int32_t i = -1;
	//����
	while (i++ != result->row_count - 1)
{
		int32_t j = -1;
		//���ҵ���һ�е�ֵ��Ȼ����ݵ�ǰ����תnext�ҵ���Ӧ����
		MYSQL_ROWS* pRow = result->data->data;
		int32_t temp = i;
		while (temp-- != 0) pRow = pRow->next;
		//׼��һ���ռ�¼
		CRecord record(this);
		//����ѭ��������Чֵѭ���ӵ���¼��
		while (j++ != result->field_count - 1)
{
			//�ֱ���ÿ�е��ֶ���
			std::string FieldName = m_tableName + "." + ((result->fields) + j)->name;
			//�ֶ�ֵ
			std::string strValue = pRow->data[j];
			//���ַ�������������ת��
			if (m_mapAttri[FieldName].m_type == 253)
{
				//�γ�һ����¼
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
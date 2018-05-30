#pragma once
#include "CAttri.h"
#include "CRecord.h"
#include <list>
#include "CMysqlMacro.h"

class CMysqlManager;
class CField;
class CRecord;
class CDataBase;
class CCondition;
class CUpdate;
class CSelect;
class CAttri;
//�����������ܣ��ֱ���ֶκͼ�¼
//�������ֶ������ֶ����Ե�map����¼�����ֶ������ֶ�ֵ��map
class CMysqlAPI CTable
{
public:
	//���ڸ�ջ����
	CTable();
	//ֻ�ǳ�ʼ��
	CTable(CMysqlManager *pMysqlManager, CDataBase *pDataBase, const std::string& TableName, bool IfHasT_e_s_t = 0);
	void Close();

public:
	CRecord& operator[](int32_t num);

public:
	//AddRecord��DeleteRecord�����е�CRecord���岻ͬ
	//��Ӽ�¼��CRecord�м�¼��һ���ֶ����Ժ��ֶ�ֵ����Щ�ϳ�һ����¼
	//bool�ķ���ֵԭ����������������������ܳ��ֲ岻�������
	int32_t Add(CRecord* pRecord);
	//ɾ����¼��CRecord�м�¼��һ���ֶ����Ժ��ֶ�ֵ����Ϊ��ѯ���������ϼ�ɾ����AndOrΪ1��ʾand��Ϊ0��ʾor��Ĭ��Ϊand
	int32_t Delete(CCondition* pCondition);
	//�޸ļ�¼
	int32_t UpdateRecord(CUpdate* pUpdate, CCondition* pCondition);
	//��ѯ��¼
	CTable SelectRecord(CSelect *pSelect, CCondition* pCondition);

	//�޸��ֶ�����
	int32_t ReviseAttri(CAttri* pAttri);
	//������ֶ�
	int32_t AddNewFieldWithNoAttri(const std::string& strFieldName);

	//�����ű��룬�����ṹ������
	void ImportTable(const std::string& mysql_exe_path, const std::string& sqlpath);
	//�����ű�����������ṹ������
	void ExportTable(const std::string& mysqldump_exe_path, const std::string& sqlpath);

	//�����ڴ�
	void Refresh();

	//��ü�¼����
	int32_t size();

public:
	//�����ݿ�ָ�뵼�����������ڲ���������
	CDataBase* m_pDataBase;
	CMysqlManager* m_pMysqlManager;
	//��¼�Ƿ����testĬ���ֶ�
	bool m_ifHasT_e_s_t;
	CRecord m_recordError;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//�ڴ��еı�����
	std::list<CRecord> m_listRecord;
	std::map<std::string, CAttri> m_mapAttri;
	std::string m_tableName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
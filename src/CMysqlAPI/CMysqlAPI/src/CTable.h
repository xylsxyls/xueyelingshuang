#pragma once
#include <SDKDDKVer.h>
#include <afx.h>
#include "mysql.h"
#include "CMysqlManager.h"
#include "CField.h"
#include "CRecord.h"
#include "vec.inl"
#include "API.h"
#include <vector>
using namespace std;

class CMysqlManager;
class CField;
class CRecord;

class API CTable{
	//表有两个功能，分别管字段和记录
public:
	MYSQL *mysql;
	CMysqlManager *pMysqlManager;

public:
	CString TableName;
	//记录是否存在test默认字段
	BOOL IfHasT_e_s_t;

public:
	CTable(CMysqlManager *pMysqlManager,CString TableName,BOOL IfHasT_e_s_t = 0);
	~CTable();

public:
	//AddRecord和DeleteRecord传参中的CRecord含义不同
	//添加记录，CRecord中记录有一组字段属性和字段值，这些合成一条记录
	//BOOL的返回值原因是如果设置了主键，可能出现插不进的情况
	BOOL AddRecord(CRecord* pSetRecord);
	//删除记录，CRecord中记录有一组字段属性和字段值，作为查询条件，符合即删除，AndOr为1表示and，为0表示or，默认为and
	void DeleteRecord(CRecord* pConditionRecord);
	//修改记录
	void ReviseRecord(CRecord* pSetRecord,CRecord* pConditionRecord);
	//查询记录
	vector<CRecord> SelectRecord(CRecord* pCondition = NULL,int LineBegin = 0,int LineEnd = 0,CRecord* pSelectField = NULL,CRecord *pOrderField = NULL,int AscDesc = 1);
	//搜索关键词
	vector<CRecord> SearchKey(CString FieldName,CString KeyName);

	//添加字段
	void AddField(CField *pField,BOOL ifExists = 1);
	//删除字段
	BOOL DropField(CField *pField,BOOL ifExists = 1);
	//获取字段信息，由于mysql提供的API内部存在静态变量，由于不知道有没有加锁，可能会线程不安全
	vector<CField> GetField();
	
};
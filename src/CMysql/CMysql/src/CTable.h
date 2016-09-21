#pragma once
#include <afxwin.h>
#include "CAttri.h"
#include "CRecord.h"
#include <list>
using namespace std;

class CMysqlManager;
class CField;
class CRecord;
class CDataBase;
class CCondition;
class CUpdate;
class CSelect;
class CAttri;
class CTable{
	//表有两个功能，分别管字段和记录
	//表里有字段名和字段属性的map，记录里有字段名和字段值的map
public:
	//把数据库指针导进来引用它内部的连接线
	CDataBase *pDataBase;
	CMysqlManager *pMysqlManager;

public:
	//作为返回值时放在list中
	list<CRecord> listRecord;

public:
	CString TableName;
	//记录是否存在test默认字段
	bool IfHasT_e_s_t;

	map<CString,CAttri> mapAttri;

public:
	//用于给栈变量
	CTable();
	//只是初始化
	CTable(CMysqlManager *pMysqlManager,CDataBase *pDataBase,CString TableName,bool IfHasT_e_s_t = 0);
	void Close();

public:
	CRecord operator[](int num);

public:
	//AddRecord和DeleteRecord传参中的CRecord含义不同
	//添加记录，CRecord中记录有一组字段属性和字段值，这些合成一条记录
	//bool的返回值原因是如果设置了主键，可能出现插不进的情况
	int Add(CRecord* pRecord);
	//删除记录，CRecord中记录有一组字段属性和字段值，作为查询条件，符合即删除，AndOr为1表示and，为0表示or，默认为and
	int Delete(CCondition* pCondition);
	//修改记录
	int UpdateRecord(CUpdate* pUpdate,CCondition* pCondition);
	//查询记录
	CTable SelectRecord(CSelect *pSelect,CCondition* pCondition);

	//将整张表导入，导入表结构和数据
	void ImportTable(CString mysql_exe_path,CString sqlpath);
	//将整张表导出，导出表结构和数据
	void ExportTable(CString mysqldump_exe_path,CString sqlpath);
};
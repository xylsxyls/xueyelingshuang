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
//表有两个功能，分别管字段和记录
//表里有字段名和字段属性的map，记录里有字段名和字段值的map
class CMysqlAPI CTable
{
public:
	//用于给栈变量
	CTable();
	//只是初始化
	CTable(CMysqlManager *pMysqlManager, CDataBase *pDataBase, const std::string& TableName, bool IfHasT_e_s_t = 0);
	void Close();

public:
	CRecord& operator[](int32_t num);

public:
	//AddRecord和DeleteRecord传参中的CRecord含义不同
	//添加记录，CRecord中记录有一组字段属性和字段值，这些合成一条记录
	//bool的返回值原因是如果设置了主键，可能出现插不进的情况
	int32_t Add(CRecord* pRecord);
	//删除记录，CRecord中记录有一组字段属性和字段值，作为查询条件，符合即删除，AndOr为1表示and，为0表示or，默认为and
	int32_t Delete(CCondition* pCondition);
	//修改记录
	int32_t UpdateRecord(CUpdate* pUpdate, CCondition* pCondition);
	//查询记录
	CTable SelectRecord(CSelect *pSelect, CCondition* pCondition);

	//修改字段属性
	int32_t ReviseAttri(CAttri* pAttri);
	//添加新字段
	int32_t AddNewFieldWithNoAttri(const std::string& strFieldName);

	//将整张表导入，导入表结构和数据
	void ImportTable(const std::string& mysql_exe_path, const std::string& sqlpath);
	//将整张表导出，导出表结构和数据
	void ExportTable(const std::string& mysqldump_exe_path, const std::string& sqlpath);

	//更新内存
	void Refresh();

	//获得记录条数
	int32_t size();

public:
	//把数据库指针导进来引用它内部的连接线
	CDataBase* m_pDataBase;
	CMysqlManager* m_pMysqlManager;
	//记录是否存在test默认字段
	bool m_ifHasT_e_s_t;
	CRecord m_recordError;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//内存中的表内容
	std::list<CRecord> m_listRecord;
	std::map<std::string, CAttri> m_mapAttri;
	std::string m_tableName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
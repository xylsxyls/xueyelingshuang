#include <SDKDDKVer.h>
#include "CTable.h"
#include "CMysqlManager.h"
#include "CDataBase.h"
#include "CCondition.h"
#include "CUpdate.h"
#include "CSelect.h"


CTable::CTable(){
	//从外部导入管理者指针
	this->pMysqlManager = NULL;
	this->IfHasT_e_s_t = 0;
	this->pDataBase = NULL;
}

CTable::CTable(CMysqlManager *pMysqlManager,CDataBase *pDataBase,CString TableName,bool IfHasT_e_s_t){
	//从外部导入管理者指针
	this->pMysqlManager = pMysqlManager;
	//先把新开的指针放到容器里
	this->pMysqlManager->listTable.push_back(this);

	this->TableName = TableName;
	this->IfHasT_e_s_t = IfHasT_e_s_t;
	this->pDataBase = pDataBase;
}

void CTable::Close(){
	//因为在类内，所以无法直接delete this，也就无法通过管理者删除空间了，只能内部手动释放mysql连接线，然后在外部释放本类
	
	//找到容器里这个指针并删除，删除管理者中本类的地址，在析构执行完毕之后本类空间也被释放
	pMysqlManager->DeleteOne(this);
}

CRecord CTable::operator[](int num){
	int n = 0;
	for(auto it = listRecord.begin();it != listRecord.end();it++){
		if(n == num) return *it;
		n++;
	}
	return CRecord(NULL);
}

int CTable::Add(CRecord* pRecord){
	//因为Add函数会多次操作数据库，所以不论在外部有没有设置回滚，函数内部都要设置回滚
	
	//查看是否有新字段添加，但是不为新字段设置属性
	for(auto itFieldInRecord = pRecord->mapValue.begin();itFieldInRecord != pRecord->mapValue.end();itFieldInRecord++){
		for(auto itFieldInTable = mapAttri.begin();itFieldInTable != mapAttri.end();itFieldInTable++){
			//如果记录中有一个字段和Table中的相等了说明不是新字段
			if(itFieldInRecord->first == itFieldInTable->first) goto Next;
		}
		//如果自然退出则说明全都不相等则说明该字段是新字段
		int nResult = AddNewFieldWithNoAttri(itFieldInRecord->first);
		//如果添加新字段失败则返回
		if(nResult != 0) return nResult;
		Next:;
	}

	//根据listReviseAttri提供的修改属性的字段名进行属性修改
	if(pRecord->listReviseAttri.size() != 0){
		for(auto itRevise = pRecord->listReviseAttri.begin();itRevise != pRecord->listReviseAttri.end();itRevise++){
			int nResult = ReviseAttri(&mapAttri[*itRevise]);
			//说明有属性修改不成功
			if(nResult != 0) return nResult;
		}
	}
	//修改完清空
	pRecord->listReviseAttri.clear();

	//在属性和字段添加全部成功之后添加记录
	CString SQL = "INSERT INTO " + TableName + " SET " + pRecord->ToCString();
	
	//首先尝试添加，如果添加成功说明用户的目的为添加记录
	return mysql_query(pDataBase->mysql,SQL);
}

int CTable::Delete(CCondition* pCondition){
	CString SQL = "DELETE FROM " + TableName + " WHERE " + pCondition->strSQL;

	return mysql_query(pDataBase->mysql,SQL);
}

int CTable::UpdateRecord(CUpdate* pUpdate,CCondition* pCondition){
	CString SQL = "UPDATE " + TableName + " SET " + pUpdate->ToCString() + " WHERE " + pCondition->strSQL;
	//AfxMessageBox(SQL);
	//return mysql_query(pDataBase->mysql,SQL);
	return 0;
}

CTable CTable::SelectRecord(CSelect *pSelect,CCondition* pCondition){
	//用于存放查询到的记录
	
	CTable vecRecord;
	CString SQL = "SELECT " + pSelect->ToCString() + " FROM " + TableName + " WHERE " + pCondition->strSQL;
	int x = 3;
	/*
	//行数
	//CString strBeginEndLine = "";
	//if(LineEnd > LineBegin && LineBegin >= 0) strBeginEndLine.Format("%d,%d",LineBegin,LineEnd);
	//显示的列
	CString strSelectField = "";
	if(pSelectField == NULL) strSelectField = "*";
	else strSelectField = pSelectField->OutPutFieldName();
	//升降排序依据的列
	CString strOrderField = "";
	if(pOrderField != NULL) strOrderField = pOrderField->OutPutFieldName();
	//1表示升序，0表示降序
	CString strAscDesc = "";
	if(AscDesc == 1) strAscDesc = "asc";
	if(AscDesc == 0) strAscDesc = "desc";

	//CString SQL = "SELECT abc,bcd FROM abcdefg WHERE bcd='yi' AND cde='YI' ORDER BY def AND efg LIMIT 0,1";// order by strchar limit 0,2;";

	CString SQL = "select " + strSelectField + " from " + TableName;
	if(pCondition != NULL) SQL = SQL + " where " + pCondition->RecordToCString();

	if(strOrderField != "") SQL = SQL + " order by " + strOrderField + " " + strAscDesc;

	if(strBeginEndLine != "") SQL = SQL + " limit " + strBeginEndLine;
	
	int nIsSucceed = mysql_query(mysql,SQL);

	const char *s = mysql_error(mysql);
	//存储得到的结果
	MYSQL_RES* result = mysql_store_result(mysql);
	
	int i = -1;
	//行数
	while(i++ != result->row_count - 1){
		int j = -1;
		//先找到第一行的值，然后根据当前行数转next找到对应的行
		MYSQL_ROWS* pRow = result->data->data;
		int temp = i;
		while(temp-- != 0) pRow = pRow->next;
		//准备一个空记录
		CRecord record;
		//列数循环，把有效值循环加到记录中
		while(j++ != result->field_count - 1){
			//字段和字段值是根据列数来循环的，所以同用一个变量
			CString FieldName = ((result->fields) + j)->name; //分别获得每行的字段名
			int nType = ((result->fields) + j)->type; //类型
			int length = ((result->fields) + j)->length; //长度
			CField field(FieldName,nType,length);
			CString strValue = pRow->data[j];
			CFieldValue FieldValue(field,strValue);
			record = record + FieldValue;
		}
		vecRecord.push_back(record);
	}

	if(nIsSucceed != 0) pMysqlManager->MysqlSucceed = 0;*/
	return vecRecord;
}

int CTable::ReviseAttri(CAttri* pAttri){
	return 0;
}

int CTable::AddNewFieldWithNoAttri(CString strFieldName){
	return 0;
}

void CTable::ImportTable(CString mysql_exe_path,CString sqlpath){
	CString para = "";
	CString strPasswd = pDataBase->mysql->passwd;
	//首先是主机和用户名
	para = para + " -h" + pDataBase->mysql->host + " -u" + pDataBase->mysql->user;
	//如果有密码加上密码
	if(strPasswd != "")  para = para + " -p" + strPasswd;
	//最后是需要导入的数据库名
	para = para + " " + pDataBase->mysql->db + " < " + sqlpath;

	//执行程序，ShellExecute不行
	system("\"" + mysql_exe_path + "\"" + para);
	//ShellExecute(NULL,_T("open"),_T(mysql_exe_path),para,NULL,SW_SHOWNORMAL);
	return;
}

void CTable::ExportTable(CString mysqldump_exe_path,CString sqlpath){
	CString para = "";
	CString strPasswd = pDataBase->mysql->passwd;
	//首先是主机和用户名
	para = para + " -h" + pDataBase->mysql->host + " -u" + pDataBase->mysql->user;
	//如果有密码加上密码
	if(strPasswd != "")  para = para + " -p" + strPasswd;
	//最后是需要导出的数据库和表名
	para = para + " " + pDataBase->mysql->db + " " + TableName + " > " + sqlpath;
	
	//执行程序，ShellExecute不行
	system("\"" + mysqldump_exe_path + "\"" + para);
	//ShellExecute(NULL,_T("open"),_T(mysqldump_exe_path),para,NULL,SW_SHOWNORMAL);
	return;
}

void CTable::Refresh(){
	mysql_query(pDataBase->mysql,"SELECT * FROM " + TableName);

	//存储得到的结果，必须先有这步才可以查询字段属性信息
	MYSQL_RES* result = mysql_store_result(pDataBase->mysql);
	//先取出字段属性信息
	MYSQL_RES* resField = mysql_list_fields(pDataBase->mysql,TableName,"%");
	
	//先存字段属性
	int j = -1;
	while(j++ != resField->field_count - 1){
		CString strFieldName = ((resField->fields) + j)->name;
		//存储
		mapAttri[strFieldName].Type = ((resField->fields) + j)->type;
		mapAttri[strFieldName].nLength = ((resField->fields) + j)->length;
		mapAttri[strFieldName].strFieldName = strFieldName;
		//得到属性集
		int flags = ((resField->fields) + j)->flags;
		if((flags & NOT_NULL_FLAG      ) == NOT_NULL_FLAG      ) mapAttri[strFieldName].bNotNull       = 1;
		if((flags & PRI_KEY_FLAG       ) == PRI_KEY_FLAG       ) mapAttri[strFieldName].bPrimaryKey    = 1;
		if((flags & UNIQUE_KEY_FLAG    ) == UNIQUE_KEY_FLAG    ) mapAttri[strFieldName].bUniqueKey     = 1;
		if((flags & BLOB_FLAG          ) == BLOB_FLAG          ) mapAttri[strFieldName].bBlob          = 1;
		if((flags & ZEROFILL_FLAG      ) == ZEROFILL_FLAG      ) mapAttri[strFieldName].bZeroFill      = 1;
		if((flags & AUTO_INCREMENT_FLAG) == AUTO_INCREMENT_FLAG) mapAttri[strFieldName].bAutoIncrement = 1;
		//如果有默认值获取默认值
		if((CString)((resField->fields) + j)->def != ""){
			mapAttri[strFieldName].bHasDefault = 1;
			mapAttri[strFieldName].vDefault = (CString)((resField->fields) + j)->def;
		}
	}

	//开始获取字段值
	int i = -1;
	//行数
	while(i++ != result->row_count - 1){
		int j = -1;
		//先找到第一行的值，然后根据当前行数转next找到对应的行
		MYSQL_ROWS* pRow = result->data->data;
		int temp = i;
		while(temp-- != 0) pRow = pRow->next;
		//准备一个空记录
		CRecord record(this);
		//列数循环，把有效值循环加到记录中
		while(j++ != result->field_count - 1){
			//分别获得每列的字段名
			CString FieldName = ((result->fields) + j)->name;
			//字段值
			CString strValue = pRow->data[j];
			//形成一个记录
			record[FieldName] = strValue;
		}
		listRecord.push_back(record);
	}
	return;
}
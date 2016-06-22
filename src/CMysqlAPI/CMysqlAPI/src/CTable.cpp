#include "CTable.h"

CTable::CTable(CMysqlManager *pMysqlManager,CString TableName,BOOL IfHasT_e_s_t){
	//从外部导入管理者指针
	this->pMysqlManager = pMysqlManager;
	//先把新开的指针放到容器里
	this->pMysqlManager->vecTable.push_back(this);

	this->TableName = TableName;
	this->IfHasT_e_s_t = IfHasT_e_s_t;
	mysql = mysql_init(NULL);
}

CTable::~CTable(){
	//因为在类内，所以无法直接delete this，也就无法通过管理者删除空间了，只能内部手动释放mysql连接线，然后在外部释放本类
	//关闭类内连接线
	if(mysql != 0){
		mysql_close(mysql);
		mysql = 0;
	}
	//找到容器里这个指针并删除，删除管理者中本类的地址，在析构执行完毕之后本类空间也被释放
	VecErase(&(pMysqlManager->vecTable),this);
}

BOOL CTable::AddRecord(CRecord* pSetRecord){
	if(pMysqlManager->MysqlSucceed != 1) return 0;

	//有强转，不需要判断是否为设置记录
	CString SQL = "insert into " + TableName + " set " + pSetRecord->ConditionToSet().RecordToCString();
	
	int nIsSucceed = mysql_query(mysql,SQL);
	CString error = mysql_error(mysql);
	
	if(nIsSucceed != 0){
		//进入这个分支说明可能是主键重复的错误和真的错误，都应该返回0
		//如果找不到primary说明不是主键重复的错误，则为真的错误
		if(error.Find("'PRIMARY'") < 0) pMysqlManager->MysqlSucceed = 0;
		return 0;
	}
	return 1;
}

void CTable::DeleteRecord(CRecord* pConditionRecord){
	if(pMysqlManager->MysqlSucceed != 1) return;

	//首先要判断是否是条件记录，如果不是条件记录则直接返回错误
	if(pConditionRecord->IsConditionRecord() == 0){
		pMysqlManager->MysqlSucceed = 0;
		return;
	}

	CString SQL = "delete from " + TableName + " where " + pConditionRecord->RecordToCString();

	int nIsSucceed = mysql_query(mysql,SQL);
	const char *s = mysql_error(mysql);
	if(nIsSucceed != 0) pMysqlManager->MysqlSucceed = 0;
	return;
}

void CTable::ReviseRecord(CRecord* pSetRecord,CRecord* pConditionRecord){
	if(pMysqlManager->MysqlSucceed != 1) return;

	//首先要判断是否是条件记录，如果不是条件记录则直接返回错误，Set有强转，不需要判断了
	if(pConditionRecord->IsConditionRecord() == 0){
		pMysqlManager->MysqlSucceed = 0;
		return;
	}

	CString SQL = "update " + TableName + " set " + pSetRecord->ConditionToSet().RecordToCString() + " where " + pConditionRecord->RecordToCString();

	int nIsSucceed = mysql_query(mysql,SQL);
	if(nIsSucceed != 0) pMysqlManager->MysqlSucceed = 0;
	return;
}

vector<CRecord> CTable::SelectRecord(CRecord* pCondition,int LineBegin,int LineEnd,CRecord* pSelectField,CRecord *pOrderField,int AscDesc){
	//用于存放查询到的记录
	vector<CRecord> vecRecord;
	if(pMysqlManager->MysqlSucceed != 1) return vecRecord;
	
	if(pCondition != NULL && pCondition->IsConditionRecord() == 0){
		pMysqlManager->MysqlSucceed = 0;
		return vecRecord;
	}
	//行数
	CString strBeginEndLine = "";
	if(LineEnd > LineBegin && LineBegin >= 0) strBeginEndLine.Format("%d,%d",LineBegin,LineEnd);
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

	if(nIsSucceed != 0) pMysqlManager->MysqlSucceed = 0;
	return vecRecord;
}

vector<CRecord> CTable::SearchKey(CString FieldName,CString KeyName){
	//用于存放查询到的记录
	vector<CRecord> vecRecord;
	if(pMysqlManager->MysqlSucceed != 1) return vecRecord;

	CString SQL = "select * from " + TableName + " where " + FieldName + " like '" + KeyName + "'";

	int nIsSucceed = mysql_query(mysql,SQL);

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

	if(nIsSucceed != 0) pMysqlManager->MysqlSucceed = 0;
	return vecRecord;
}

void CTable::AddField(CField *pField,BOOL ifExists){
	if(pMysqlManager->MysqlSucceed != 1) return;

	CString SQL = "alter table " + TableName + " add " + pField->FieldName + " " + pField->Type + "(" + pField->strLength + ")";
	//如果有默认值
	if(pField->Default != 0) SQL = SQL + " not null default " + pField->strDefault;
	//如果是主键
	if(pField->ifPrimaryKey == 1) SQL = SQL + " primary key";

	//执行
	int nIsSucceed = mysql_query(mysql,SQL);
	const char*s = mysql_error(mysql);

	//如果添加字段成功，且是新开的表第一次加完字段时需要删除字段T_e_s_t
	if(nIsSucceed == 0 && IfHasT_e_s_t == 1){
		DropField(&CField("T_e_s_t"));
		IfHasT_e_s_t = 0;
	}

	//如果不是必定加入，且mysql返回错误，则需要把执行条件改为0
	if(ifExists == 0 && nIsSucceed != 0) pMysqlManager->MysqlSucceed = 0;
	return;
}

BOOL CTable::DropField(CField *pField,BOOL ifExists){
	if(pMysqlManager->MysqlSucceed != 1) return 0;

	vector<CField> vecField = GetField();
	BOOL ifExist = 0;
	int i = -1;
	while(i++ != vecField.size() - 1){
		ifExist = ifExist || (vecField.at(i).FieldName == pField->FieldName);
	}
	
	int nIsSucceed = -1;
	//如果表中不存在该字段，且不是必定删除，说明即将要删一个不存在的字段，直接返回错误和0
	if(ifExist == 0 && ifExists == 0){
		pMysqlManager->MysqlSucceed = 0;
		return 0;
	}
	//如果表中不存在该字段，且是必定删除，说明必定要删除这个字段，所以不需要执行mysql，则直接返回1
	if(ifExist == 0 && ifExists == 1) return 1;
	//如果表中存在该字段，不管是不是必定删除都需要执行mysql来判断是否已经删除成功，删除成功返回1，失败返回错误和0
	nIsSucceed = mysql_query(mysql,"alter table " + TableName + " drop column " + pField->FieldName);
	if(nIsSucceed == 0) return 1;
	else{
		pMysqlManager->MysqlSucceed = 0;
		return 0;
	}
}

vector<CField> CTable::GetField(){
	if(pMysqlManager->MysqlSucceed != 1){
		vector<CField> error;
		return error;
	}

	//先搜索整张表
	mysql_query(mysql,"select * from " + TableName);
	//存储得到的结果
	MYSQL_RES* result = mysql_store_result(mysql);
	
	vector<CField> vecField;
	int i = -1;
	while(i++ != result->field_count - 1){
		CField Field(((result->fields) + i)->name,((result->fields) + i)->type,((result->fields) + i)->length);
		vecField.push_back(Field);
	}
	return vecField;
}

void CTable::ImportTable(CString mysql_exe_path,CString sqlpath){
	if(pMysqlManager->MysqlSucceed != 1) return;

	CString para = "";
	CString strPasswd = mysql->passwd;
	//首先是主机和用户名
	para = para + " -h" + mysql->host + " -u" + mysql->user;
	//如果有密码加上密码
	if(strPasswd != "")  para = para + " -p" + strPasswd;
	//最后是需要导入的数据库名
	para = para + " " + mysql->db + " < " + sqlpath;

	//执行程序，ShellExecute不行
	system("\"" + mysql_exe_path + "\"" + para);
	//ShellExecute(NULL,_T("open"),_T(mysql_exe_path),para,NULL,SW_SHOWNORMAL);
	return;
}

void CTable::ExportTable(CString mysqldump_exe_path,CString sqlpath){
	if(pMysqlManager->MysqlSucceed != 1) return;
	
	CString para = "";
	CString strPasswd = mysql->passwd;
	//首先是主机和用户名
	para = para + " -h" + mysql->host + " -u" + mysql->user;
	//如果有密码加上密码
	if(strPasswd != "")  para = para + " -p" + strPasswd;
	//最后是需要导出的数据库和表名
	para = para + " " + mysql->db + " " + TableName + " > " + sqlpath;
	
	//执行程序，ShellExecute不行
	system("\"" + mysqldump_exe_path + "\"" + para);
	//ShellExecute(NULL,_T("open"),_T(mysqldump_exe_path),para,NULL,SW_SHOWNORMAL);
	return;
}
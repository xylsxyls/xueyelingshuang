#include <SDKDDKVer.h>
#include "CDataBase.h"

CDataBase::CDataBase(bool* bSucceed,CMysqlManager* pMysqlManager,CString IP,int port,CString User,CString PassWord,CString dbName){
	//从外部导入管理者指针
	this->pMysqlManager = pMysqlManager;
	//先把新开的指针放到容器里
	this->pMysqlManager->listDataBase.push_back(this);

	this->IP = IP;
	this->port = port;
	this->User = User;
	this->PassWord = PassWord;
	this->dbName = dbName;
	mysql = mysql_init(NULL);

	MYSQL *IsSucceed = mysql_real_connect(mysql,IP,User,PassWord,dbName,port,NULL,0);
	//如果连接失败则释放，但是不释放管理者，管理者只有在CMysql被释放时才会释放
	if(IsSucceed == NULL){
		this->pMysqlManager->DeleteOne(this);
		*bSucceed = 0;
	}
	*bSucceed = 1;
}

CTable* CDataBase::CreateTable(CString TableName,CString FieldName,BOOL type,int length,bool ifExists,bool AutoCommit){
	int nIsSucceed = -1;
	CString strType;
	if(type == 1) strType = "varchar";
	else if(type == 2) strType = "int";
	else if(type == 3) strType = "double";
	CString strLength;
	strLength.Format("%d",length);
	//建表，默认有一个test的int字段，因为数据库无法建立空表
	if(ifExists == 0) nIsSucceed = mysql_query(mysql,"create table " + TableName + "(" + FieldName + " " + strType + "(" + strLength + "))");
	if(ifExists == 1) nIsSucceed = mysql_query(mysql,"create table if not exists " + TableName + "(" + FieldName + " " + strType + "(" + strLength + "))");

	CTable* pTable = NULL;
	if(nIsSucceed == 0){
		//新开一个表接口并为他的内部建立一个连接线
		pTable = new CTable(pMysqlManager,TableName,1);
		//把数据库里的内部连接线重新再制造一个放进CTable内
		mysql_real_connect(pTable->mysql,IP,User,PassWord,dbName,port,NULL,0);
	}
	return pTable;
}

int CDataBase::DropTable(CString TableName,bool ifExists){
	int nIsSucceed = -1;
	//删表
	if(ifExists == 0) nIsSucceed = mysql_query(mysql,"drop table " + TableName);
	else if(ifExists == 1) nIsSucceed = mysql_query(mysql,"drop table if exists " + TableName);

	return nIsSucceed;
}

CTable* CDataBase::OpenTableInterface(CString TableName,bool AutoCommit){
	CTable* pTable = NULL;
	//返回0表示确实有这张表
	mysql_query(mysql,"SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA='" + dbName + "' AND TABLE_NAME='" + TableName + "'");
	//结果中如果行数 == 1说明确实有这张表
	if(mysql_store_result(mysql)->row_count == 1){
		//新开一个表接口并为他的内部建立一个连接线
		pTable = new CTable(pMysqlManager,TableName);
		//把数据库里的内部连接线重新再制造一个放进CTable内
		mysql_real_connect(pTable->mysql,IP,User,PassWord,dbName,port,NULL,0);
	}
	return pTable;
}

void CDataBase::CloseTableInterface(CTable* pTable){
	pMysqlManager->DeleteOne(pTable);
	return;
}
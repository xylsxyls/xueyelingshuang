#include "CDataBase.h"

CDataBase::CDataBase(CMysqlManager* pMysqlManager,CString Ip,int port,CString User,CString PassWord,CString dbName){
	//从外部导入管理者指针
	this->pMysqlManager = pMysqlManager;
	//先把新开的指针放到容器里
	this->pMysqlManager->vecDatabase.push_back(this);

	this->Ip = Ip;
	this->port = port;
	this->User = User;
	this->PassWord = PassWord;
	this->dbName = dbName;
	mysql = mysql_init(NULL);

	MYSQL *IsSucceed = mysql_real_connect(this->mysql,Ip,User,PassWord,dbName,port,NULL,0);
	//如果连接失败则释放，但是不释放管理者，管理者只有在CMysql被释放时才会释放
	if(IsSucceed == NULL){
		//释放空间
		mysql_close(mysql);
		mysql = 0;
		//所有成员函数的执行条件改为0
		pMysqlManager->MysqlSucceed = 0;
	}
}

CDataBase::CDataBase(CMysqlManager* pMysqlManager,CString dbName){
	//从外部导入管理者指针
	this->pMysqlManager = pMysqlManager;
	//先把新开的指针放到容器里
	this->pMysqlManager->vecDatabase.push_back(this);

	Ip = "";
	port = 0;
	User = "";
	PassWord = "";
	this->dbName = dbName;
	mysql = mysql_init(NULL);
}

CDataBase::~CDataBase(){
	//关闭类内连接线
	if(mysql != 0){
		mysql_close(mysql);
		mysql = 0;
	}
	//找到容器里这个指针并删除
	VecErase(&(pMysqlManager->vecDatabase),this);
}

CTable* CDataBase::CreateTable(CString TableName,BOOL ifExists){
	if(pMysqlManager->MysqlSucceed != 1) return 0;

	int nIsSucceed = -1;
	//建表，默认有一个test的int字段，因为数据库无法建立空表
	if(ifExists == 0) nIsSucceed = mysql_query(mysql,"create table " + TableName + "(T_e_s_t int(1))");
	if(ifExists == 1) nIsSucceed = mysql_query(mysql,"create table if not exists " + TableName + "(T_e_s_t int(1))");

	CTable* pTable = NULL;
	if(nIsSucceed == 0){
		//新开一个表接口并为他的内部建立一个连接线
		pTable = new CTable(pMysqlManager,TableName,1);
		//把数据库里的内部连接线重新再制造一个放进CTable内
		mysql_real_connect(pTable->mysql,Ip,User,PassWord,dbName,port,NULL,0);
	}
	else pMysqlManager->MysqlSucceed = 0;
	return pTable;
}

void CDataBase::DropTable(CString TableName,BOOL ifExists){
	if(pMysqlManager->MysqlSucceed != 1) return;

	int nIsSucceed = -1;
	//删表
	if(ifExists == 0) nIsSucceed = mysql_query(mysql,"drop table " + TableName);
	if(ifExists == 1) nIsSucceed = mysql_query(mysql,"drop table if exists " + TableName);

	if(nIsSucceed != 0) pMysqlManager->MysqlSucceed = 0;
	return;
}

CTable* CDataBase::OpenTableInterface(CString TableName){
	if(pMysqlManager->MysqlSucceed != 1) return 0;

	CTable* pTable = NULL;
	//返回0表示确实有这张表
	mysql_query(mysql,"SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA='" + dbName + "' AND TABLE_NAME='" + TableName + "'");
	//结果中如果行数 == 1说明确实有这张表
	if(mysql_store_result(mysql)->row_count == 1){
		//新开一个表接口并为他的内部建立一个连接线
		pTable = new CTable(pMysqlManager,TableName);
		//把数据库里的内部连接线重新再制造一个放进CTable内
		mysql_real_connect(pTable->mysql,Ip,User,PassWord,dbName,port,NULL,0);
	}
	else pMysqlManager->MysqlSucceed = 0;
	return pTable;
}

void CDataBase::CloseTableInterface(CTable* pTable){
	if(pMysqlManager->MysqlSucceed != 1) return;

	delete pTable;
	pTable = 0;
}
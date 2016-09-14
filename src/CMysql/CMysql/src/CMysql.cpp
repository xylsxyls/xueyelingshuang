#include "CMysql.h"

CMysql::CMysql(CString IP,CString User,CString PassWord,CString dbName,int port){
	//初始化管理者
	pMysqlManager = new CMysqlManager;

	//初始化数据库
	mysql = mysql_init(NULL);
	//默认连接到本机mysql的test数据库，连接之后就默认开始读这个数据库
	MYSQL *IsSucceed = mysql_real_connect(mysql,IP,User,PassWord,dbName,port,NULL,0);
	//如果连接失败则释放，但是不释放管理者，管理者只有在CMysql被释放时才会释放
	if(IsSucceed == NULL){
		//释放空间
		mysql_close(mysql);
		mysql = 0;
		//所有成员函数的执行条件改为0
		pMysqlManager->MysqlSucceed = 0;
	}
	
	//把初始化的参数传给类中的记录参数类
	this->Ip = IP;
	this->port = port;
	this->User = User;
	this->PassWord = PassWord;
	this->dbName = dbName;
}

CMysql::~CMysql(){
	delete pMysqlManager;
	pMysqlManager = 0;
	//释放空间，判断不等于0是因为有可能开始就没连接上直接被置为0了
	if(mysql != 0){
		mysql_close(mysql);
		mysql = 0;
	}
}

//只是将Ip和port的记录存入新开的类
CIp* CMysql::OpenIpInterface(){
	return OpenIpInterface(Ip,port);
}

CIp* CMysql::OpenIpInterface(CString Ip,int port){
	//在创建时内存所有成员变量均被初始化
	return new CIp(pMysqlManager,Ip,port);
}

CUser* CMysql::OpenUserInterface(){
	return OpenUserInterface(User,PassWord);
}

CUser* CMysql::OpenUserInterface(CString User,CString PassWord){
	//在创建时内存所有成员变量均被初始化
	return new CUser(pMysqlManager,Ip,port,User,PassWord);
}

CDataBase* CMysql::OpenDataBaseInterface(){
	return OpenDataBaseInterface(dbName);
}

CDataBase* CMysql::OpenDataBaseInterface(CString dbName){
	//在创建时内存所有成员变量均被初始化
	CDataBase* pDataBase = new CDataBase(pMysqlManager,dbName);

	//将新开的CDataBase里的连接线进行连接
	MYSQL *IsSucceed = mysql_real_connect(pDataBase->mysql,Ip,User,PassWord,dbName,port,NULL,0);
	//如果连接失败则释放，但是不释放管理者，管理者只有在CMysql被释放时才会释放
	if(IsSucceed == NULL){
		delete pDataBase;
		pDataBase = 0;
		//所有成员函数的执行条件改为0
		pMysqlManager->MysqlSucceed = 0;
	}
	return pDataBase;
}

CTable* CMysql::OpenTableInterface(CString TableName,BOOL AutoCommit){
	//在创建时内存所有成员变量均被初始化
	CTable* pTable = new CTable(pMysqlManager,TableName);

	//将新开的CDataBase里的连接线进行连接
	MYSQL *IsSucceed = mysql_real_connect(pTable->mysql,Ip,User,PassWord,dbName,port,NULL,0);
	//如果连接失败则释放，但是不释放管理者，管理者只有在CMysql被释放时才会释放
	if(IsSucceed == NULL){
		delete pTable;
		pTable = 0;
		//所有成员函数的执行条件改为0
		pMysqlManager->MysqlSucceed = 0;
	}
	return pTable;
}

/*
void CMysql::SelectOtherIPDataBase(CString IP,CString User,CString PassWord,CString dbName,int port){
	if(MysqlSucceed != 1) return;

	mysql_close(mysql);
	mysql = mysql_init(NULL);
	IsSucceed = mysql_real_connect(mysql,IP,User,PassWord,dbName,port,NULL,0);
	//由于这个值是指针，当不为空时才说明执行成功
	if(IsSucceed != 0){
		ConnectParameter.IP = IP;
		ConnectParameter.User = User;
		ConnectParameter.PassWord = PassWord;
		ConnectParameter.dbName = dbName;
		ConnectParameter.port = port;
	}
	else MysqlSucceed = 0;
	return;
}



void CMysql::SelectTable(CString TableName){
	if(MysqlSucceed != 1) return;

	ConnectParameter.TableName = TableName;
	return;
}*/

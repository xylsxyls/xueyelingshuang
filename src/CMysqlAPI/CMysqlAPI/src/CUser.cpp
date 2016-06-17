#include "CUser.h"

CUser::CUser(CMysqlManager *pMysqlManager,CString Ip,int port,CString User,CString PassWord){
	//从外部导入管理者指针
	this->pMysqlManager = pMysqlManager;
	//先把新开的指针放到容器里
	this->pMysqlManager->vecUser.push_back(this);

	this->Ip = Ip;
	this->port = port;
	this->User = User;
	this->PassWord = PassWord;

	mysql = mysql_init(NULL);
}

CUser::~CUser(){
	//关闭类内连接线
	if(mysql != 0){
		mysql_close(mysql);
		mysql = 0;
	}
	//找到容器里这个指针并删除
	VecErase(&(pMysqlManager->vecUser),this);
}

CDataBase* CUser::CreateDataBase(CString NewdbName,BOOL ifExists){
	if(pMysqlManager->MysqlSucceed != 1) return 0;

	int nIsSucceed = -1;
	//判断是否创建成功
	if(ifExists == 0) nIsSucceed = mysql_query(mysql,"create database " + NewdbName);
	if(ifExists == 1) nIsSucceed = mysql_query(mysql,"create database if not exists " + NewdbName);

	CDataBase* pDataBase = NULL;
	//如果成功则转到该数据库
	if(nIsSucceed == 0) pDataBase = OpenDataBaseInterface(NewdbName);
	//执行到这句说明创建数据库失败，之后的所有类成员函数不会运行
	else pMysqlManager->MysqlSucceed = 0;
	return pDataBase;
}

void CUser::DropDataBase(CDataBase *pDataBase){
	if(pMysqlManager->MysqlSucceed != 1) return;

	int nIsSucceed = mysql_query(mysql,"drop database " + pDataBase->dbName);
	if(nIsSucceed == 0) delete pDataBase;
	//如果删除失败
	else pMysqlManager->MysqlSucceed = 0;
	return;
}

CDataBase* CUser::OpenDataBaseInterface(CString dbName){
	if(pMysqlManager->MysqlSucceed != 1) return 0;

	//连接这个数据库看是否能连接上，连接上表示确实有这个数据库，连接不上返回错误
	CDataBase* pDataBase = new CDataBase(pMysqlManager,Ip,port,User,PassWord,dbName);
	//如果连接不上，虽然确实有返回值，但是内部成员函数控制值变为0
	if(pMysqlManager->MysqlSucceed == 0){
		delete pDataBase;
		pDataBase = 0;
	}
	
	return pDataBase;
}

void CUser::CloseDataBaseInterface(CDataBase *pDataBase){
	if(pMysqlManager->MysqlSucceed != 1) return;

	delete pDataBase;
	pDataBase = 0;
}
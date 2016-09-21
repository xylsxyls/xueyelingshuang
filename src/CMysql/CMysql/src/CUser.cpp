#include <SDKDDKVer.h>
#include "CUser.h"
#include "CMysqlManager.h"
#include "CDataBase.h"

CUser::CUser(CMysqlManager *pMysqlManager,CString IP,int port,CString User,CString PassWord){
	//从外部导入管理者指针
	this->pMysqlManager = pMysqlManager;
	//先把新开的指针放到容器里
	this->pMysqlManager->listUser.push_back(this);

	this->IP = IP;
	this->port = port;
	this->User = User;
	this->PassWord = PassWord;

	mysql = mysql_init(NULL);
}

void CUser::Close(){
	//关闭类内连接线
	if(mysql != 0){
		mysql_close(mysql);
		mysql = 0;
	}
	//找到容器里这个指针并删除
	pMysqlManager->DeleteOne(this);
}

CDataBase* CUser::CreateDataBase(CString NewdbName,bool ifExists){
	int nIsSucceed = -1;
	//判断是否创建成功
	if(ifExists == 0) nIsSucceed = mysql_query(mysql,"create database " + NewdbName);
	if(ifExists == 1) nIsSucceed = mysql_query(mysql,"create database if not exists " + NewdbName);

	CDataBase* pDataBase = NULL;
	//如果成功则转到该数据库
	if(nIsSucceed == 0) pDataBase = OpenDataBaseInterface(NewdbName);
	return pDataBase;
}

int CUser::DropDataBase(CDataBase *pDataBase){
	int nIsSucceed = mysql_query(mysql,"drop database " + pDataBase->dbName);
	if(nIsSucceed == 0) pMysqlManager->DeleteOne(pDataBase);
	//如果删除失败，返回失败值
	return nIsSucceed;
}

CDataBase* CUser::OpenDataBaseInterface(CString dbName){
	bool bSucceed = 0;
	//连接这个数据库看是否能连接上，连接上表示确实有这个数据库，连接不上返回错误
	CDataBase* pDataBase = new CDataBase(&bSucceed,pMysqlManager,IP,port,User,PassWord,dbName);
	//如果连接不上，返回0
	if(bSucceed == 0){
		//在连接失败时构造函数内部已经有delete this的操作，所以只需要重置成0即可
		pDataBase = 0;
	}
	return pDataBase;
}

void CUser::CloseDataBaseInterface(CDataBase *pDataBase){
	pMysqlManager->DeleteOne(pDataBase);
	return;
}
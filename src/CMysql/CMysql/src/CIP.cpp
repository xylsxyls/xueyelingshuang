#include <SDKDDKVer.h>
#include "CIP.h"
#include "CDataBase.h"
#include "CMysqlManager.h"

CIP::CIP(CMysqlManager *pMysqlManager,CString IP,int port){
	//从外部导入管理者指针
	this->pMysqlManager = pMysqlManager;
	//先把新开的指针放到容器里
	this->pMysqlManager->listIP.push_back(this);

	this->IP = IP;
	this->port = port;
	mysql = mysql_init(NULL);
}

CDataBase* CIP::OpenUserDataBaseInterface(CString User,CString PassWord,CString dbName){
	bool bSucceed = 0;
	CDataBase* pDataBase = new CDataBase(&bSucceed,pMysqlManager,IP,port,User,PassWord,dbName);
	if(bSucceed == 0) pDataBase = 0;
	return pDataBase;
}

void CIP::CloseDataBaseInterface(CDataBase* pDataBase){
	pMysqlManager->DeleteOne(pDataBase);
	return;
}
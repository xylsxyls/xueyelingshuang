#include <SDKDDKVer.h>
#include "CMysql.h"
#include "CMysqlManager.h"
#include "CDataBase.h"
#include "CTable.h"
#include "CIP.h"
#include "CUser.h"

CMysql::CMysql(CString IP,int port,CString User,CString PassWord,CString dbName){
	//初始化管理者
	pMysqlManager = new CMysqlManager;
	
	//把初始化的参数传给类中的记录参数类
	this->IP = IP;
	this->port = port;
	this->User = User;
	this->PassWord = PassWord;
	this->dbName = dbName;
}

CMysql::~CMysql(){
	delete pMysqlManager;
}

//只是将IP和port的记录存入新开的类
CIP* CMysql::OpenIPInterface(){
	return OpenIPInterface(IP,port);
}

CIP* CMysql::OpenIPInterface(CString IP,int port){
	//在创建时内存所有成员变量均被初始化
	return new CIP(pMysqlManager,IP,port);
}

CUser* CMysql::OpenUserInterface(){
	return OpenUserInterface(User,PassWord);
}

CUser* CMysql::OpenUserInterface(CString User,CString PassWord){
	//在创建时内存所有成员变量均被初始化
	return new CUser(pMysqlManager,IP,port,User,PassWord);
}

CDataBase* CMysql::OpenDataBaseInterface(){
	return OpenDataBaseInterface(dbName);
}

CDataBase* CMysql::OpenDataBaseInterface(CString dbName,bool AutoCommit){
	bool bSucceed = 0;
	CDataBase* pDataBase = new CDataBase(&bSucceed,pMysqlManager,IP,port,User,PassWord,dbName);
	if(bSucceed == 0) pDataBase = 0;
	return pDataBase;
}

CTable* CMysql::OpenTableInterface(CString TableName,bool AutoCommit){
	CDataBase* pDataBase = OpenDataBaseInterface(dbName);
	//在创建时内存所有成员变量均被初始化
	CTable* pTable = new CTable(pMysqlManager,pDataBase,TableName);

	//将新开的CDataBase里的连接线进行连接
	MYSQL *IsSucceed = mysql_real_connect(pTable->pDataBase->mysql,IP,User,PassWord,dbName,port,NULL,0);
	//如果连接失败则释放，但是不释放管理者，管理者只有在CMysql被释放时才会释放
	if(IsSucceed == NULL){
		delete pTable;
		pTable = 0;
	}
	pTable->Refresh();
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

#include "CTableField.h"
#include "CCondition.h"
#include "CUpdate.h"
#include "CSelect.h"

int main(){
	//打开表接口
	CMysql mysql;
	CTable* pTable = mysql.OpenTableInterface("test",1);
	CString xxx = (*pTable)[0]["name"].toValue();

	//增
	//设置记录
	CRecord rec(pTable);
	rec["name"] = "e";
	rec["number"] = 5;
	//pTable->Add(&rec);

	//定义条件
	CCondition con;
	//条件and
	con && (CTableField("test","name") == "d");
	con && (CTableField("test","number") == 4);
	//条件or，左联合
	//con || (CTableField("User","ID") <= CTableField("Department","UserID"));
	//条件not
	//!con;

	//删
	//int x = pTable->Delete(&con);

	//改
	CUpdate upd;
	upd["ID"] = 10005;
	pTable->UpdateRecord(&upd,&con);

	//查
	CSelect sel;
	sel("test")["name"];
	sel("test")["ID"];
	CTable table = pTable->SelectRecord(&sel,&con);
	CString strDepartName = table[0]["Depart.Name"].toValue();
	int nUserID = table[0]["User.ID"].toValue();
	
	//rec["User"].pTable->mapAttri["User"].Name = "123456";
	//修改字段属性
	rec["User"]->nLength = 256;
	pTable->Add(&rec);

	return 0;
}
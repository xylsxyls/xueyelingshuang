#include <SDKDDKVer.h>
#include "CMysqlManager.h"
#include "CIP.h"
#include "CUser.h"
#include "CDataBase.h"
#include "CTable.h"

CMysqlManager::CMysqlManager(){

}

CMysqlManager::~CMysqlManager(){
	//释放listIP
	for(auto itIP = listIP.begin();itIP != listIP.end();itIP++){
		delete *itIP;
	}
	listIP.clear();

	//释放listUser
	for(auto itUser = listUser.begin();itUser != listUser.end();itUser++){
		delete *itUser;
	}
	listUser.clear();

	//释放listDataBase
	for(auto itDataBase = listDataBase.begin();itDataBase != listDataBase.end();itDataBase++){
		delete *itDataBase;
	}
	listDataBase.clear();

	//释放listTable
	for(auto itTable = listTable.begin();itTable != listTable.end();itTable++){
		delete *itTable;
	}
	listTable.clear();
}

bool CMysqlManager::DeleteOne(CIP* pDelete){
	//关闭类内连接线
	if(pDelete->mysql != 0){
		mysql_close(pDelete->mysql);
	}
	for(auto itIP = listIP.begin();itIP != listIP.end();itIP++){
		if(*itIP == pDelete){
			//释放指针
			delete pDelete;
			//释放list里的指针
			listIP.erase(itIP);
			return 1;
		}
	}
	AfxMessageBox("无指定指针，CIP*");
	return 0;
}

bool CMysqlManager::DeleteOne(CUser* pDelete){
	//关闭类内连接线
	if(pDelete->mysql != 0){
		mysql_close(pDelete->mysql);
	}
	for(auto itUser = listUser.begin();itUser != listUser.end();itUser++){
		if(*itUser == pDelete){
			//释放指针
			delete pDelete;
			//释放list里的指针
			listUser.erase(itUser);
			return 1;
		}
	}
	AfxMessageBox("无指定指针，CUser*");
	return 0;
}

bool CMysqlManager::DeleteOne(CDataBase* pDelete){
	//关闭类内连接线
	if(pDelete->mysql != 0){
		mysql_close(pDelete->mysql);
	}
	for(auto itDataBase = listDataBase.begin();itDataBase != listDataBase.end();itDataBase++){
		if(*itDataBase == pDelete){
			//释放指针
			delete pDelete;
			//释放list里的指针
			listDataBase.erase(itDataBase);
			return 1;
		}
	}
	AfxMessageBox("无指定指针，CDataBase*");
	return 0;
}

bool CMysqlManager::DeleteOne(CTable* pDelete){
	//关闭类内连接线
	if(pDelete->pDataBase->mysql != 0){
		mysql_close(pDelete->pDataBase->mysql);
	}
	for(auto itTable = listTable.begin();itTable != listTable.end();itTable++){
		if(*itTable == pDelete){
			//释放指针
			delete pDelete;
			//释放list里的指针
			listTable.erase(itTable);
			return 1;
		}
	}
	AfxMessageBox("无指定指针，CTable*");
	return 0;
}
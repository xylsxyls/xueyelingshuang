#include "CMysqlManager.h"

CMysqlManager::CMysqlManager(){
	MysqlSucceed = 1;
}

CMysqlManager::~CMysqlManager(){
	FreeVec(&vecIp);
	FreeVec(&vecUser);
	FreeVec(&vecDatabase);
	FreeVec(&vecTable);
}
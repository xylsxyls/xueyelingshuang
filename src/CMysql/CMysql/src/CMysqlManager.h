#pragma once
#include <list>
using namespace std;

class CIP;
class CUser;
class CDataBase;
class CTable;

class CMysqlManager{
	//所有当前CMysql下开的内存全部会放在管理者的向量中，管理者一旦被删除则所有内存释放
public:
	//用来记录内存开辟情况，所有开辟的内存地址都会在这里记录
	list<CIP      *> listIP      ;
	list<CUser    *> listUser    ;
	list<CDataBase*> listDataBase;
	list<CTable   *> listTable   ;

	bool DeleteOne(CIP      * pDelete);
	bool DeleteOne(CUser    * pDelete);
	bool DeleteOne(CDataBase* pDelete);
	bool DeleteOne(CTable   * pDelete);

public:
	CMysqlManager();
	//如果管理者被删除则把所有的内存地址全部删除
	~CMysqlManager();
};
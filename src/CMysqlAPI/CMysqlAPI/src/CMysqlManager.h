#pragma once
#include <vector>
using namespace std;
#include "CIp.h"
#include "CUser.h"
#include "CDataBase.h"
#include "CTable.h"
#include "API.h"
#include "vec.inl"

class CIp;
class CUser;
class CDataBase;
class CTable;

//class __declspec( dllexport ) CString;
//template  class __declspec( dllexport ) std::allocator<int>;
//template  class __declspec( dllexport ) std::vector<int,std::allocator<int>>;

class CMysqlAPI CMysqlManager{
	//所有当前CMysql下开的内存全部会放在管理者的向量中，管理者一旦被删除则所有内存释放
public:
	//这个值用来检测传参，调用是否出现错误，只有当这个值为1的时候才会执行成员函数，一旦出现错误，这个值变为0
	BOOL MysqlSucceed;

	//用来记录内存开辟情况，所有开辟的内存地址都会在这里记录
	vector<CIp      *> vecIp      ;
	vector<CUser    *> vecUser    ;
	vector<CDataBase*> vecDatabase;
	vector<CTable   *> vecTable   ;
	
public:
	CMysqlManager();
	//如果管理者被删除则把所有的内存地址全部删除
	~CMysqlManager();
};
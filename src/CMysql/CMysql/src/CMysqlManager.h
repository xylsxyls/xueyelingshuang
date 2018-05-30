#pragma once
#include <list>
#include "CMysqlMacro.h"

class CHostIP;
class CUser;
class CDataBase;
class CTable;

template class CMysqlAPI std::allocator < CHostIP      * > ;
template class CMysqlAPI std::allocator < CUser    * > ;
template class CMysqlAPI std::allocator < CDataBase* > ;
template class CMysqlAPI std::allocator < CTable   * > ;
template class CMysqlAPI std::list < CHostIP      *, std::allocator<CHostIP      *> > ;
template class CMysqlAPI std::list < CUser    *, std::allocator<CUser    *> > ;
template class CMysqlAPI std::list < CDataBase*, std::allocator<CDataBase*> > ;
template class CMysqlAPI std::list < CTable   *, std::allocator<CTable   *> > ;

//所有当前CMysql下开的内存全部会放在管理者的向量中，管理者一旦被删除则所有内存释放
class CMysqlAPI CMysqlManager
{
public:
	CMysqlManager();
	//如果管理者被删除则把所有的内存地址全部删除
	~CMysqlManager();

public:
	bool DeleteOne(CHostIP      * pDelete);
	bool DeleteOne(CUser    * pDelete);
	bool DeleteOne(CDataBase* pDelete);
	bool DeleteOne(CTable   * pDelete);

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//用来记录内存开辟情况，所有开辟的内存地址都会在这里记录
	std::list<CHostIP      *> m_listIP;
	std::list<CUser    *> m_listUser;
	std::list<CDataBase*> m_listDataBase;
	std::list<CTable   *> m_listTable;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
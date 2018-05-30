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

//���е�ǰCMysql�¿����ڴ�ȫ������ڹ����ߵ������У�������һ����ɾ���������ڴ��ͷ�
class CMysqlAPI CMysqlManager
{
public:
	CMysqlManager();
	//��������߱�ɾ��������е��ڴ��ַȫ��ɾ��
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
	//������¼�ڴ濪����������п��ٵ��ڴ��ַ�����������¼
	std::list<CHostIP      *> m_listIP;
	std::list<CUser    *> m_listUser;
	std::list<CDataBase*> m_listDataBase;
	std::list<CTable   *> m_listTable;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
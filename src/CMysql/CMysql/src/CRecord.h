#pragma once
#include <list>
#include <map>
#include "CValue.h"
#include "CMysqlMacro.h"

class CTable;
class CMysqlAPI CRecord
{
public:
	CRecord(CTable* pTable);
	CRecord(const CRecord& record);
	CRecord operator=(const CRecord& record);

public:
	//��ѯ�˴�����ֶ������ᵼ�¼�¼����ֶ�����
	CRecord& operator[](const std::string& Field);
	//����¼��ֵ�ֶ�ֵ�������������
	CRecord operator=(const CValue& value);

public:
	//һ������������������ԣ������ֶ���������
	CAttri* operator->();
	CValue toValue();

public:
	std::string toString();

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//��ʹ��table�ж����ֶ�����ģʽ����Ϊ�޷�������ֶ�
	std::map<std::string, CValue> m_mapValue;
	//�洢�޸ĵ��ֶ�������Addִ��֮�����
	std::list<std::string> m_listReviseAttri;
	std::string m_strCurrentField;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	CTable* m_pTable;
};
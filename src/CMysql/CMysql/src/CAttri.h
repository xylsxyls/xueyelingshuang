#pragma once
#include "CValue.h"
#include "CMysqlMacro.h"

class CMysqlAPI CAttri
{
public:
	CAttri();
	CAttri(const CAttri& attri);
	CAttri operator= (const CAttri& attri);

	//CAttri& operator->();
	void a();

public:
	int32_t m_type;
	int32_t m_length;
	bool m_hasDefault;
	CValue m_vDefault;
	bool m_bPrimaryKey;
	bool m_bUniqueKey;
	bool m_bNotNull;
	bool m_bAutoIncrement;
	bool m_bBlob;
	bool m_bZeroFill;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_strFieldName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
#pragma once
#include <SDKDDKVer.h>
#include <afx.h>
#include <windows.h>
#include "CField.h"

class CField;
class __declspec(dllexport) CFieldValue{
public:
	CString strValue;
	int nValue;

	//字段值和字段属性的关系，=，>，<，>=，<=，!=
	CString CompareFlag;

	//字段值的属性
	CField Field;
	
public:
	CFieldValue(                                                     );
	CFieldValue(CString strValue,CField     Field                    );
	CFieldValue(int       nValue,CField     Field                    );
	CFieldValue(CField     Field,CString strValue                    );
	CFieldValue(CField     Field,int       nValue                    );
	CFieldValue(CField     Field,CString CompareFlag,CString strValue);
	CFieldValue(CField     Field,CString CompareFlag,int       nValue);

public:
	//如果传入的数据和属性中指定的属性不匹配，会以属性中的为准，并返回0，完全匹配返回1
	BOOL SetValue(CField     Field,CString    strValue               );
	BOOL SetValue(CField     Field,int          nValue               );
	BOOL SetValue(CString strValue,CString CompareFlag,CField *pField);
	BOOL SetValue(int       nValue,CString CompareFlag,CField *pField);
};
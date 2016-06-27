// CWMPClosedCaption2.h : 由 Microsoft Visual C++ 创建的 ActiveX 控件包装类的声明

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CWMPClosedCaption2

class CWMPClosedCaption2 : public COleDispatchDriver
{
public:
	CWMPClosedCaption2() {}		// 调用 COleDispatchDriver 默认构造函数
	CWMPClosedCaption2(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	CWMPClosedCaption2(const CWMPClosedCaption2& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// 属性
public:

// 操作
public:

	CString get_SAMIStyle()
	{
		CString result;
		InvokeHelper(0x3b7, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	void put_SAMIStyle(LPCTSTR newValue)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x3b7, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	CString get_SAMILang()
	{
		CString result;
		InvokeHelper(0x3b8, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	void put_SAMILang(LPCTSTR newValue)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x3b8, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	CString get_SAMIFileName()
	{
		CString result;
		InvokeHelper(0x3b9, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	void put_SAMIFileName(LPCTSTR newValue)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x3b9, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	CString get_captioningId()
	{
		CString result;
		InvokeHelper(0x3ba, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	void put_captioningId(LPCTSTR newValue)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x3ba, DISPATCH_PROPERTYPUT, VT_EMPTY, NULL, parms, newValue);
	}
	long get_SAMILangCount()
	{
		long result;
		InvokeHelper(0x3bb, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
		return result;
	}
	CString getSAMILangName(long nIndex)
	{
		CString result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x3bc, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, nIndex);
		return result;
	}
	long getSAMILangID(long nIndex)
	{
		long result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x3bd, DISPATCH_METHOD, VT_I4, (void*)&result, parms, nIndex);
		return result;
	}
	long get_SAMIStyleCount()
	{
		long result;
		InvokeHelper(0x3be, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
		return result;
	}
	CString getSAMIStyleName(long nIndex)
	{
		CString result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x3bf, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, nIndex);
		return result;
	}


};

// CWMPCdromCollection.h : 由 Microsoft Visual C++ 创建的 ActiveX 控件包装类的声明

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CWMPCdromCollection

class CWMPCdromCollection : public COleDispatchDriver
{
public:
	CWMPCdromCollection() {}		// 调用 COleDispatchDriver 默认构造函数
	CWMPCdromCollection(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	CWMPCdromCollection(const CWMPCdromCollection& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// 属性
public:

// 操作
public:

	long get_count()
	{
		long result;
		InvokeHelper(0x12d, DISPATCH_PROPERTYGET, VT_I4, (void*)&result, NULL);
		return result;
	}
	LPDISPATCH Item(long lIndex)
	{
		LPDISPATCH result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x12e, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, parms, lIndex);
		return result;
	}
	LPDISPATCH getByDriveSpecifier(LPCTSTR bstrDriveSpecifier)
	{
		LPDISPATCH result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x12f, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, parms, bstrDriveSpecifier);
		return result;
	}


};

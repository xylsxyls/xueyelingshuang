// CWMPPlaylistCollection.h : 由 Microsoft Visual C++ 创建的 ActiveX 控件包装类的声明

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CWMPPlaylistCollection

class CWMPPlaylistCollection : public COleDispatchDriver
{
public:
	CWMPPlaylistCollection() {}		// 调用 COleDispatchDriver 默认构造函数
	CWMPPlaylistCollection(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	CWMPPlaylistCollection(const CWMPPlaylistCollection& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// 属性
public:

// 操作
public:

	LPDISPATCH newPlaylist(LPCTSTR bstrName)
	{
		LPDISPATCH result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x228, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, parms, bstrName);
		return result;
	}
	LPDISPATCH getAll()
	{
		LPDISPATCH result;
		InvokeHelper(0x229, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, NULL);
		return result;
	}
	LPDISPATCH getByName(LPCTSTR bstrName)
	{
		LPDISPATCH result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x22a, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, parms, bstrName);
		return result;
	}
	void remove(LPDISPATCH pItem)
	{
		static BYTE parms[] = VTS_DISPATCH ;
		InvokeHelper(0x22c, DISPATCH_METHOD, VT_EMPTY, NULL, parms, pItem);
	}
	void setDeleted(LPDISPATCH pItem, BOOL varfIsDeleted)
	{
		static BYTE parms[] = VTS_DISPATCH VTS_BOOL ;
		InvokeHelper(0x230, DISPATCH_METHOD, VT_EMPTY, NULL, parms, pItem, varfIsDeleted);
	}
	BOOL isDeleted(LPDISPATCH pItem)
	{
		BOOL result;
		static BYTE parms[] = VTS_DISPATCH ;
		InvokeHelper(0x231, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, pItem);
		return result;
	}
	LPDISPATCH importPlaylist(LPDISPATCH pItem)
	{
		LPDISPATCH result;
		static BYTE parms[] = VTS_DISPATCH ;
		InvokeHelper(0x232, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, parms, pItem);
		return result;
	}


};

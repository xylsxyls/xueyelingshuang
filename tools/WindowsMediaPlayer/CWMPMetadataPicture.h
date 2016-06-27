// CWMPMetadataPicture.h : 由 Microsoft Visual C++ 创建的 ActiveX 控件包装类的声明

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CWMPMetadataPicture

class CWMPMetadataPicture : public COleDispatchDriver
{
public:
	CWMPMetadataPicture() {}		// 调用 COleDispatchDriver 默认构造函数
	CWMPMetadataPicture(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	CWMPMetadataPicture(const CWMPMetadataPicture& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// 属性
public:

// 操作
public:

	CString get_mimeType()
	{
		CString result;
		InvokeHelper(0x41b, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	CString get_pictureType()
	{
		CString result;
		InvokeHelper(0x41c, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	CString get_Description()
	{
		CString result;
		InvokeHelper(0x41d, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	CString get_URL()
	{
		CString result;
		InvokeHelper(0x41e, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}


};

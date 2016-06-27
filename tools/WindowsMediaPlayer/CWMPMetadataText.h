// CWMPMetadataText.h : 由 Microsoft Visual C++ 创建的 ActiveX 控件包装类的声明

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CWMPMetadataText

class CWMPMetadataText : public COleDispatchDriver
{
public:
	CWMPMetadataText() {}		// 调用 COleDispatchDriver 默认构造函数
	CWMPMetadataText(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	CWMPMetadataText(const CWMPMetadataText& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// 属性
public:

// 操作
public:

	CString get_Description()
	{
		CString result;
		InvokeHelper(0x420, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	CString get_text()
	{
		CString result;
		InvokeHelper(0x41f, DISPATCH_PROPERTYGET, VT_BSTR, (void*)&result, NULL);
		return result;
	}


};

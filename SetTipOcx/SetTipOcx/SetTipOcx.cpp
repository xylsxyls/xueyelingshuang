// SetTipOcx.cpp : CSetTipOcxApp 和 DLL 注册的实现。

#include "stdafx.h"
#include "SetTipOcx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CSetTipOcxApp theApp;

const GUID CDECL _tlid = { 0x2EA857A6, 0x6F8A, 0x45A4, { 0xB7, 0xF1, 0x12, 0x37, 0x94, 0x89, 0x50, 0xFC } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;



// CSetTipOcxApp::InitInstance - DLL 初始化

BOOL CSetTipOcxApp::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();

	if (bInit)
	{
		// TODO: 在此添加您自己的模块初始化代码。
	}

	return bInit;
}



// CSetTipOcxApp::ExitInstance - DLL 终止

int CSetTipOcxApp::ExitInstance()
{
	// TODO: 在此添加您自己的模块终止代码。

	return COleControlModule::ExitInstance();
}



// DllRegisterServer - 将项添加到系统注册表

STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}



// DllUnregisterServer - 将项从系统注册表中移除

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}

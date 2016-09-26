// SetTipOcxCtrl.cpp : CSetTipOcxCtrl ActiveX 控件类的实现。

#include "stdafx.h"
#include "SetTipOcx.h"
#include "SetTipOcxCtrl.h"
#include "SetTipOcxPropPage.h"
#include "afxdialogex.h"
#include "CGetPath/CGetPathAPI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CSetTipOcxCtrl, COleControl)

// 消息映射

BEGIN_MESSAGE_MAP(CSetTipOcxCtrl, COleControl)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()

// 调度映射

BEGIN_DISPATCH_MAP(CSetTipOcxCtrl, COleControl)
	DISP_FUNCTION_ID(CSetTipOcxCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION_ID(CSetTipOcxCtrl, "init", dispidinit, init, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION_ID(CSetTipOcxCtrl, "show", dispidshow, show, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION_ID(CSetTipOcxCtrl, "uninit", dispiduninit, uninit, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()

// 事件映射

BEGIN_EVENT_MAP(CSetTipOcxCtrl, COleControl)
END_EVENT_MAP()

// 属性页

// TODO: 按需要添加更多属性页。请记住增加计数!
BEGIN_PROPPAGEIDS(CSetTipOcxCtrl, 1)
	PROPPAGEID(CSetTipOcxPropPage::guid)
END_PROPPAGEIDS(CSetTipOcxCtrl)

// 初始化类工厂和 guid

IMPLEMENT_OLECREATE_EX(CSetTipOcxCtrl, "SETTIPOCX.SetTipOcxCtrl.1",
	0x93ba3e69, 0x5e03, 0x4aea, 0xac, 0x5b, 0x99, 0xfc, 0x4e, 0x2, 0xb1, 0x2f)

// 键入库 ID 和版本

IMPLEMENT_OLETYPELIB(CSetTipOcxCtrl, _tlid, _wVerMajor, _wVerMinor)

// 接口 ID

const IID IID_DSetTipOcx = { 0x6C149F97, 0x4F38, 0x437D, { 0x8E, 0xE3, 0x87, 0x34, 0x30, 0x2A, 0x6F, 0x99 } };
const IID IID_DSetTipOcxEvents = { 0xD80C0124, 0x300, 0x4DC4, { 0x9D, 0x1C, 0x55, 0x64, 0x2, 0x35, 0xFF, 0xB4 } };

// 控件类型信息

static const DWORD _dwSetTipOcxOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CSetTipOcxCtrl, IDS_SETTIPOCX, _dwSetTipOcxOleMisc)

// CSetTipOcxCtrl::CSetTipOcxCtrlFactory::UpdateRegistry -
// 添加或移除 CSetTipOcxCtrl 的系统注册表项
#include <objsafe.h>
#include "helpers.h"

BOOL CSetTipOcxCtrl::CSetTipOcxCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: 验证您的控件是否符合单元模型线程处理规则。
	// 有关更多信息，请参考 MFC 技术说明 64。
	// 如果您的控件不符合单元模型规则，则
	// 必须修改如下代码，将第六个参数从
	// afxRegApartmentThreading 改为 0。

	if (bRegister){
		BOOL retval = AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_SETTIPOCX,
			IDB_SETTIPOCX,
			afxRegApartmentThreading,
			_dwSetTipOcxOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);

		// mark as safe for scripting--failure OK
		HRESULT hr = CreateComponentCategory(CATID_SafeForScripting, 
			L"Controls that are safely scriptable");

		if (SUCCEEDED(hr))
			// only register if category exists
				RegisterCLSIDInCategory(m_clsid, CATID_SafeForScripting);
		// don't care if this call fails

		// mark as safe for data initialization
		hr = CreateComponentCategory(CATID_SafeForInitializing, 
			L"Controls safely initializable from persistent data");

		if (SUCCEEDED(hr))
			// only register if category exists
				RegisterCLSIDInCategory(m_clsid, CATID_SafeForInitializing);
		// don't care if this call fails
		return retval;
	}
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


// CSetTipOcxCtrl::CSetTipOcxCtrl - 构造函数

CSetTipOcxCtrl::CSetTipOcxCtrl()
{
	InitializeIIDs(&IID_DSetTipOcx, &IID_DSetTipOcxEvents);
	// TODO: 在此初始化控件的实例数据。
}

// CSetTipOcxCtrl::~CSetTipOcxCtrl - 析构函数

CSetTipOcxCtrl::~CSetTipOcxCtrl()
{
	// TODO: 在此清理控件的实例数据。
}

// CSetTipOcxCtrl::OnDraw - 绘图函数

void CSetTipOcxCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if (!pdc)
		return;

	// TODO: 用您自己的绘图代码替换下面的代码。
	pdc->FillRect(rcBounds, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
	pdc->Ellipse(rcBounds);
}

// CSetTipOcxCtrl::DoPropExchange - 持久性支持

void CSetTipOcxCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: 为每个持久的自定义属性调用 PX_ 函数。
}


// CSetTipOcxCtrl::OnResetState - 将控件重置为默认状态

void CSetTipOcxCtrl::OnResetState()
{
	COleControl::OnResetState();  // 重置 DoPropExchange 中找到的默认值

	// TODO: 在此重置任意其他控件状态。
}


// CSetTipOcxCtrl::AboutBox - 向用户显示“关于”框

void CSetTipOcxCtrl::AboutBox()
{
	CDialogEx dlgAbout(IDD_ABOUTBOX_SETTIPOCX);
	dlgAbout.DoModal();
}


// CSetTipOcxCtrl 消息处理程序


void CSetTipOcxCtrl::init(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	//TODO: 在此添加调度处理程序代码
	CGetPath get;
	CString strOcxPath = get.GetRegOcxPath("93BA3E69-5E03-4AEA-AC5B-99FC4E02B12F");
	ShellExecute(NULL, _T("open"), "\"" + strOcxPath + "Tip.exe\"", NULL, NULL, SW_HIDE);
	return;
}


void CSetTipOcxCtrl::show(LPCTSTR strText)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	//TODO: 在此添加调度处理程序代码
	//先搜索Tip.exe进程，因为该进程最多存在一个，获取句柄用户发送消息
	CGetPath get;
	HWND hwnd = get.GetHwndByProcessId(get.GetProcessID("Tip.exe").at(0));
	int length = strlen(strText);
	char* pszText = (char*)calloc(length + 1,1);
	memcpy(pszText,strText,strlen(strText));
	COPYDATASTRUCT cpd; // 给COPYDATASTRUCT结构赋值
	cpd.dwData = 0;
	cpd.cbData = length + 1;
	cpd.lpData = pszText;
	::SendMessage(hwnd,WM_COPYDATA,NULL,(LPARAM)&cpd);
	free(pszText);
	return;
}


void CSetTipOcxCtrl::uninit(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: 在此添加调度处理程序代码
	CGetPath get;
	HWND hwnd = get.GetHwndByProcessId(get.GetProcessID("Tip.exe").at(0));
	::SendMessage(hwnd,10003,NULL,NULL);
	return;
}

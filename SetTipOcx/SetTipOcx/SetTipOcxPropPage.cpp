// SetTipOcxPropPage.cpp : CSetTipOcxPropPage 属性页类的实现。

#include "stdafx.h"
#include "SetTipOcx.h"
#include "SetTipOcxPropPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CSetTipOcxPropPage, COlePropertyPage)

// 消息映射

BEGIN_MESSAGE_MAP(CSetTipOcxPropPage, COlePropertyPage)
END_MESSAGE_MAP()

// 初始化类工厂和 guid

IMPLEMENT_OLECREATE_EX(CSetTipOcxPropPage, "SETTIPOCX.SetTipOcxPropPage.1",
	0x6b9bc337, 0xff57, 0x4b95, 0x88, 0xfe, 0x59, 0x20, 0x8b, 0x42, 0x54, 0xfd)

// CSetTipOcxPropPage::CSetTipOcxPropPageFactory::UpdateRegistry -
// 添加或移除 CSetTipOcxPropPage 的系统注册表项

BOOL CSetTipOcxPropPage::CSetTipOcxPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_SETTIPOCX_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}

// CSetTipOcxPropPage::CSetTipOcxPropPage - 构造函数

CSetTipOcxPropPage::CSetTipOcxPropPage() :
	COlePropertyPage(IDD, IDS_SETTIPOCX_PPG_CAPTION)
{
}

// CSetTipOcxPropPage::DoDataExchange - 在页和属性间移动数据

void CSetTipOcxPropPage::DoDataExchange(CDataExchange* pDX)
{
	DDP_PostProcessing(pDX);
}

// CSetTipOcxPropPage 消息处理程序

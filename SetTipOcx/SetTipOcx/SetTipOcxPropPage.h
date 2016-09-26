#pragma once

// SetTipOcxPropPage.h : CSetTipOcxPropPage 属性页类的声明。


// CSetTipOcxPropPage : 有关实现的信息，请参阅 SetTipOcxPropPage.cpp。

class CSetTipOcxPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CSetTipOcxPropPage)
	DECLARE_OLECREATE_EX(CSetTipOcxPropPage)

// 构造函数
public:
	CSetTipOcxPropPage();

// 对话框数据
	enum { IDD = IDD_PROPPAGE_SETTIPOCX };

// 实现
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 消息映射
protected:
	DECLARE_MESSAGE_MAP()
};


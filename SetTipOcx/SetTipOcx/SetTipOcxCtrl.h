#pragma once

// SetTipOcxCtrl.h : CSetTipOcxCtrl ActiveX 控件类的声明。


// CSetTipOcxCtrl : 有关实现的信息，请参阅 SetTipOcxCtrl.cpp。

class CSetTipOcxCtrl : public COleControl
{
	DECLARE_DYNCREATE(CSetTipOcxCtrl)

// 构造函数
public:
	CSetTipOcxCtrl();

// 重写
public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();

// 实现
protected:
	~CSetTipOcxCtrl();

	DECLARE_OLECREATE_EX(CSetTipOcxCtrl)    // 类工厂和 guid
	DECLARE_OLETYPELIB(CSetTipOcxCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CSetTipOcxCtrl)     // 属性页 ID
	DECLARE_OLECTLTYPE(CSetTipOcxCtrl)		// 类型名称和杂项状态

// 消息映射
	DECLARE_MESSAGE_MAP()

// 调度映射
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// 事件映射
	DECLARE_EVENT_MAP()

// 调度和事件 ID
public:
	enum {
		dispiduninit = 3L,
		dispidshow = 2L,
		dispidinit = 1L
	};
protected:
	void init(void);
	void show(LPCTSTR strText);
	void uninit(void);
};


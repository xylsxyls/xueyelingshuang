#pragma once
#include <afxwin.h>
#include <list>
#include <gdiplusgraphics.h>
#include <gdiplus.h>
#include <wingdi.h>
using namespace Gdiplus;
using namespace std;

#define DlgOnPaintMessage(pPicControl) \
	if(pPicControl->nInit >= 0){\
		pPicControl->bifHasPaintMessage = 1;\
		pPicControl->Invalidate();\
		pPicControl->RedrawWindow();\
	}


class CPicControl : public CButton{
public:
	CPicControl();
	CString FontForm;
	int FontSize;
	
	list<CString> listPath;
	list<CRect> listPicRect;
	list<CRect> listRcRect;
	list<list<CString>> listlistText;
	list<list<COLORREF>> listlistColor;

	CString strPath;
	CRect picRect;
	CRect rcRect;
	list<CString> prelistText;
	list<COLORREF> prelistColor;

	bool bDown;
	//0表示初始化，1表示进入虚函数，需要鉴定，2表示刚改过，直接重画
	int nInit;
	bool bifHasPaintMessage;
	//list的顺序是常规按下移动
	void init(CRect rcRect,UINT nID,CWnd* pParentWnd,
		list<CString> listPath = list<CString>(),list<CRect> listPicRect = list<CRect>(),list<CRect> listRcRect = list<CRect>(),
		list<list<CString>> listlistText = list<list<CString>>(),list<list<COLORREF>> listlistColor = list<list<COLORREF>>());
	//改变图片
	void ChangePicAndText(list<CString> listPath = list<CString>(),list<CRect> listPicRect = list<CRect>(),list<CRect> listRcRect = list<CRect>(),
		list<list<CString>> listlistText = list<list<CString>>(),list<list<COLORREF>> listlistColor = list<list<COLORREF>>());
	//默认全button界面和全图片
	void DrawPicCDC(CString strPath,CRect picRect = CRect(0,0,0,0),CRect rcRect = CRect(0,0,0,0));
	//写文字
	void DrawTextCDC(list<CString> listText,list<COLORREF> listColor);
	//常规
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};
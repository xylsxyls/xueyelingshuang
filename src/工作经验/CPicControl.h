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
	CRect rcRectInClient;
	list<CString> prelistText;
	list<COLORREF> prelistColor;

	bool bDown;
	//0表示初始化，1表示进入虚函数，需要鉴定，2表示刚改过，直接重画
	int nInit;
	bool bifHasPaintMessage;

	//控件客户端大小
	CRect rcClient;

	//list的顺序是常规按下移动
	void init(CRect rcRect,UINT nID,CWnd* pParentWnd,
		list<CString> listPath = list<CString>(),list<CRect> listPicRect = list<CRect>(),list<CRect> listRcRect = list<CRect>(),
		list<list<CString>> listlistText = list<list<CString>>(),list<list<COLORREF>> listlistColor = list<list<COLORREF>>());
	//改变图片
	void ChangePicAndText(list<CString> listPath = list<CString>(),list<CRect> listPicRect = list<CRect>(),list<CRect> listRcRect = list<CRect>(),
		list<list<CString>> listlistText = list<list<CString>>(),list<list<COLORREF>> listlistColor = list<list<COLORREF>>());
	//默认全button界面和全图片
	void DrawPicCDC(Bitmap* pmemBitmap,CString strPath,CRect picRect = CRect(0,0,0,0),CRect rcRect = CRect(0,0,0,0),bool ifOnPaint = 0);
	//写文字
	void DrawTextCDC(Bitmap* pmemBitmap,list<CString> listText,list<COLORREF> listColor,bool ifOnPaint = 0);
	//常规
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//画图和文字
	void DrawPicAndText(Bitmap* pmemBitmap);
	//画一张图
	static void DrawOnePic(Bitmap* pmemBitmap,CString strPath,CRect picRect,CRect rcRect);
	//画图
	static void Draw(HDC hdc,Bitmap* pmemBitmap,int x,int y);
	//清空画布
	void ClearBitmap(Bitmap* pmemBitmap);
	//在Dlg的OnPaint中执行
	static void DrawInDlgOnPaint(HDC hdc,CString strPath,CRect picRect,CRect rcRect,int x,...);


	DECLARE_MESSAGE_MAP()
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
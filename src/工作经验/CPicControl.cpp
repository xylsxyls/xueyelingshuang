#include "stdafx.h"
#include "CPicControl.h"

CPicControl::CPicControl(){
	FontForm = "宋体";
	//#define FontForm "楷体"
	FontSize = 13;
	//#define FontSize 13
	this->nInit = -1;
}

void CPicControl::init(CRect rcRectInClient,UINT nID,CWnd* pParentWnd,list<CString> listPath,list<CRect> listPicRect,list<CRect> listRcRect,list<list<CString>> listlistText,list<list<COLORREF>> listlistColor){
	Create("",/////按钮上显示的文本  
		WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,///如果没有制定WS_VISIBLE还要调用ShowWindow将其显示出来  WS_CHILD|
		rcRectInClient,/////左上角的坐标(0，0),长度为100，100  
		pParentWnd,//父窗口  
		nID);

	this->rcRectInClient = rcRectInClient;
	this->bDown = 0;
	this->nInit = 0;
	this->bifHasPaintMessage = 0;
	GetClientRect(&rcClient);
	ChangePicAndText(listPath,listPicRect,listRcRect,listlistText,listlistColor);
	return;
}

void CPicControl::ChangePicAndText(list<CString> listPath,list<CRect> listPicRect,list<CRect> listRcRect,list<list<CString>> listlistText,list<list<COLORREF>> listlistColor){
	this->listPath = listPath;
	this->listPicRect = listPicRect;
	this->listRcRect = listRcRect;
	this->listlistText = listlistText;
	this->listlistColor = listlistColor;

	if(listPath.size() < 3){
		int i = listPath.size();
		while(i++ != 3) this->listPath.push_back("");
	}
	if(listPicRect.size() < 3){
		int i = listPicRect.size();
		while(i++ != 3) this->listPicRect.push_back(CRect());
	}
	if(listRcRect.size() < 3){
		int i = listRcRect.size();
		while(i++ != 3) this->listRcRect.push_back(CRect());
	}
	if(listlistText.size() < 3){
		int i = listlistText.size();
		while(i++ != 3) this->listlistText.push_back(list<CString>());
	}
	if(listlistColor.size() < 3){
		int i = listlistColor.size();
		while(i++ != 3) this->listlistColor.push_back(list<COLORREF>());
	}

	//必须要给内存一张画布，没有这步GDI+无法绘图，画布大小应设置为Client大小
	Bitmap memBitmap(rcClient.Width(),rcClient.Height());

	DrawPicCDC(&memBitmap,*(this->listPath.begin()),*(this->listPicRect.begin()),*(this->listRcRect.begin()));
	DrawTextCDC(&memBitmap,*(this->listlistText.begin()),*(this->listlistColor.begin()));
	Draw(GetDC()->m_hDC,&memBitmap,0,0);
	if(nInit == 2) nInit = 1;
	return;
}

void CPicControl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	nInit = 1;
	//必须要给内存一张画布，没有这步GDI+无法绘图，画布大小应设置为Client大小
	Bitmap memBitmap(rcClient.Width(),rcClient.Height());
	DrawPicAndText(&memBitmap);
	Draw(GetDC()->m_hDC,&memBitmap,0,0);
	nInit = 1;
	// TODO:  添加您的代码以绘制指定项
}

void CPicControl::ClearBitmap(Bitmap* pmemBitmap){
	Graphics emptyMemGraphics(pmemBitmap);
	SolidBrush brush(TRANSPARENT);
	emptyMemGraphics.FillRectangle(&brush,rcClient.left,rcClient.top,rcClient.Width(),rcClient.Height());
}

void CPicControl::DrawInDlgOnPaint(HDC hdc,CString strPath,CRect picRect,CRect rcRect,int number,...){
	va_list parameterlist;
	va_start(parameterlist,number);
	if(picRect == CRect() && strPath != ""){
		CStringW strWide = CT2CW(strPath);
		Image image(strWide);
		picRect.SetRect(0,0,image.GetWidth(),image.GetHeight());
	}

	CPicControl* pPicControlFirst = NULL;
	if(number > 0 && rcRect == CRect()){
		//如果有控件并且显示区为空CRect那么获取父窗口的大小
		pPicControlFirst = va_arg(parameterlist,CPicControl*);
		if(pPicControlFirst->nInit != -1) pPicControlFirst->GetParent()->GetClientRect(&rcRect);
	}
	Bitmap memBitmap(rcRect.Width(),rcRect.Height());
	//画背景
	if(strPath != "") DrawOnePic(&memBitmap,strPath,picRect,rcRect);
	//如果不为空，说明确实有取到第一个控件，必须画在背景之后
	if(pPicControlFirst->nInit != -1){
		pPicControlFirst->DrawPicCDC(&memBitmap,*pPicControlFirst->listPath.begin(),
			*pPicControlFirst->listPicRect.begin(),pPicControlFirst->rcRectInClient,TRUE);
		pPicControlFirst->DrawTextCDC(&memBitmap,*pPicControlFirst->listlistText.begin(),
			*pPicControlFirst->listlistColor.begin(),TRUE);
		number--;
	}
	//开始画或者画剩余的
	while(number-- != 0){
		CPicControl* pPicControl = va_arg(parameterlist,CPicControl*);
		if(pPicControl->nInit != -1){
			pPicControl->DrawPicCDC(&memBitmap,*pPicControl->listPath.begin(),*pPicControl->listPicRect.begin(),pPicControl->rcRectInClient,TRUE);
			pPicControl->DrawTextCDC(&memBitmap,*pPicControl->listlistText.begin(),*pPicControl->listlistColor.begin(),TRUE);
		}
	}
	Draw(hdc,&memBitmap,0,0);
	va_end(parameterlist);
	return;
}

void CPicControl::DrawPicAndText(Bitmap* pmemBitmap){

	CString strDownPath = *(++listPath.begin());
	//如果是按下状态，且按下状态路径不为空那么显示按下状态，其余情况显示常规状态
	if(bDown == 1 && strDownPath != ""){
		DrawPicCDC(pmemBitmap,*(++listPath.begin()),*(++listPicRect.begin()),*(++listRcRect.begin()));
	}
	else if(*(listPath.begin()) != ""){
		DrawPicCDC(pmemBitmap,*(listPath.begin()),*(listPicRect.begin()),*(listRcRect.begin()));
	}

	//如果有文字需要显示
	if(bDown == 1 && (++listlistText.begin())->size() != 0){
		DrawTextCDC(pmemBitmap,*(++listlistText.begin()),*(++listlistColor.begin()));
	}
	else{
		DrawTextCDC(pmemBitmap,*(listlistText.begin()),*(listlistColor.begin()));
	}
	return;
}

void CPicControl::DrawOnePic(Bitmap* pmemBitmap,CString strPath,CRect picRect,CRect rcRect){
	//这里可以通过memGraphics在memBitmap上绘制多个图元
	Graphics memGraphics(pmemBitmap);

	CStringW strWide = CT2CW(strPath);
	Image image(strWide);
	memGraphics.DrawImage(&image,RectF((float)rcRect.left,(float)rcRect.top,(float)rcRect.Width(),(float)rcRect.Height()),
		Gdiplus::REAL(picRect == CRect() ? 0 : picRect.left),
		Gdiplus::REAL(picRect == CRect() ? 0 : picRect.top),
		Gdiplus::REAL(picRect == CRect() ? image.GetWidth() : picRect.Width()),
		Gdiplus::REAL(picRect == CRect() ? image.GetHeight() : picRect.Height()),Gdiplus::UnitPixel);
}

void CPicControl::Draw(HDC hdc,Bitmap* pmemBitmap,int x,int y){
	Graphics gr(hdc);
	gr.DrawImage(pmemBitmap,x,y);
	return;
}

void CPicControl::DrawPicCDC(Bitmap* pmemBitmap,CString strPath,CRect picRect,CRect rcRect,bool ifOnPaint){
	//如果是在OnPaint中除了路径全部跳过检查
	if(ifOnPaint){
		if(strPath == "") return;
		goto begin;
	}
	if(nInit == 0) return;
	else if(nInit == 1){
		if(this->strPath == strPath && this->picRect == picRect && this->rcRect == rcRect) return;
	}
	if(strPath == "") return;
	nInit = 2;

begin:
	//OnPaint中画的位置会有偏移
	if(ifOnPaint == 1){
		this->rcRect = CRect(rcRect.left - rcRectInClient.left,rcRect.top - rcRectInClient.top,
			rcRect.Width() - rcRectInClient.Width(),rcRect.Height() - rcRectInClient.Height());
	}
	else this->rcRect = rcRect;
	this->strPath = strPath;
	this->picRect = picRect;


	DrawOnePic(pmemBitmap,strPath,picRect,rcRect == CRect() ? rcClient : rcRect);
	return;
}

void CPicControl::DrawTextCDC(Bitmap* pmemBitmap,list<CString> listText,list<COLORREF> listColor,bool ifOnPaint){
	//如果是在OnPaint中全部跳过检查
	if(ifOnPaint){
		goto begin;
	}
	if(nInit == 0) return;
	else if(nInit == 1){
		//记录上一次参数，如果相同就直接返回
		if(prelistText == listText && prelistColor == listColor) return;
	}
	nInit = 2;

begin:
	//如果没有内容要输入则直接返回
	if(listText.size() == 0) return;
	prelistText = listText;
	prelistColor = listColor;

	if(listColor.size() < listText.size()){
		int i = listColor.size();
		while(i++ != listText.size()) listColor.push_back(COLORREF());
	}

	Gdiplus::Graphics TextGraphics(pmemBitmap);
	
	//文字去毛边，在透明或默认背景下文字边缘会有黑色
	TextGraphics.SetTextRenderingHint(TextRenderingHint(TextRenderingHintAntiAlias));
	
	CString strOneLine;
	float xx = 0.0f;
	float yy = 0.0f;
	if(ifOnPaint == 1){
		xx = xx + rcRectInClient.left;
		yy = yy + rcRectInClient.top;
	}
	auto itlistText = listText.begin();
	if(itlistText != listText.end()) strOneLine = *itlistText;
	auto itlistColor = listColor.begin();
	while(itlistText != listText.end()){
		bool bWrap = 0;
		CStringW strWide;
		//使用方框换行输出
		//文字
		if((*itlistText)[0] == '#'){
			bWrap = 1;
			strWide = CT2CW(itlistText->Mid(1,itlistText->GetLength() - 1));
		}
		else strWide = CT2CW(*itlistText);
		//字体
		CStringW strFontForm = CT2CW(FontForm);
		FontFamily fontFamily(strFontForm);
		Gdiplus::Font font(&fontFamily,(float)FontSize);
		//格式
		StringFormat stringFormat;
		//stringFormat.SetAlignment(Gdiplus::StringAlignmentCenter);
		//stringFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);
		//文字颜色
		SolidBrush TextBrush(Gdiplus::Color(GetRValue(*itlistColor),GetGValue(*itlistColor),GetBValue(*itlistColor)));
		Gdiplus::PointF origin(xx,yy);

		CRect rcRectTemp;
		this->GetClientRect(&rcRectTemp);
		RectF rectf(xx,yy,(float)rcRectTemp.Width(),(float)rcRectTemp.Height());
		//写文字
		if(bWrap == 1) TextGraphics.DrawString(strWide,-1,&font,rectf,&stringFormat,&TextBrush);
		else TextGraphics.DrawString(strWide,-1,&font,origin,&TextBrush);
		//转出HFONT
		LOGFONTA logfont;
		font.GetLogFontA(&TextGraphics,&logfont);
		CFont cFont;
		cFont.CreateFontIndirect(&logfont);
		//如果要获取的是GetDC()->m_hDC，必须提在一个变量里获取长度的时候才有效，直接使用GetDC()->m_hDC不行
		HDC hDC = TextGraphics.GetHDC();
		//hFont = CreateFont(logfont.lfHeight,logfont.lfWidth,0,0,FW_THIN,true,false,false,
		//	CHINESEBIG5_CHARSET,OUT_CHARACTER_PRECIS,CLIP_CHARACTER_PRECIS,DEFAULT_QUALITY,FF_MODERN,FontForm);
		SelectObject(hDC,cFont);

		CString strText = *itlistText;
		//获取一个字符串的长宽
		SIZE size;
		GetTextExtentPoint32(hDC,strOneLine,strlen(strOneLine),&size);
		int nOneLine = size.cx;
		//加上下一条字符串存入缓冲区
		itlistText++;
		if(itlistText != listText.end()) strOneLine = strOneLine + *itlistText;
		GetTextExtentPoint32(hDC,strOneLine,strlen(strOneLine),&size);
		int nOneLineAdd = size.cx;
		if(nOneLineAdd >= rcRectTemp.Width() || (strText.GetLength() >= 1 && strText[strText.GetLength() - 1] == '\n')){
			if(itlistText != listText.end()) strOneLine = *itlistText;
			if(ifOnPaint == 1) xx = (float)rcRectInClient.left;
			else xx = 0;
			yy = yy + size.cy;
		}
		else{
			if(ifOnPaint == 1) xx = (float)nOneLine + rcRectInClient.left;
			else xx = (float)nOneLine;
		}
		//必须有对应释放否则会出现ObjectBusy，把获取句柄放在循环外也不行
		TextGraphics.ReleaseHDC(hDC);
		itlistColor++;
	}
	return;
}

BEGIN_MESSAGE_MAP(CPicControl, CButton)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSELEAVE()
	ON_WM_MOUSEHOVER()
	ON_WM_LBUTTONUP()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


void CPicControl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(TRACKMOUSEEVENT);
	tme.dwFlags = TME_LEAVE | TME_HOVER;//要触发的消息
	tme.hwndTrack = this->m_hWnd;
	tme.dwHoverTime = 10;// 若不设此参数，则无法触发mouseHover

	//MOUSELEAVE|MOUSEHOVER消息由此函数触发
	::_TrackMouseEvent(&tme);

	CButton::OnMouseMove(nFlags, point);
}


void CPicControl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	bDown = 1;
	CButton::OnLButtonDown(nFlags, point);
}


void CPicControl::OnMouseLeave()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	//必须要给内存一张画布，没有这步GDI+无法绘图，画布大小应设置为Client大小
	Bitmap memBitmap(rcClient.Width(),rcClient.Height());


	DrawPicCDC(&memBitmap,*(listPath.begin()),*(listPicRect.begin()),*(listRcRect.begin()));
	DrawTextCDC(&memBitmap,*(listlistText.begin()),*(listlistColor.begin()));
	Draw(GetDC()->m_hDC,&memBitmap,0,0);
	if(nInit == 2) nInit = 1;
	CButton::OnMouseLeave();
}


void CPicControl::OnMouseHover(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//必须要给内存一张画布，没有这步GDI+无法绘图，画布大小应设置为Client大小
	Bitmap memBitmap(rcClient.Width(),rcClient.Height());

	CString strDownPath = *(++listPath.begin());
	CString strMovePath = *(++++listPath.begin());
	//如果是按下状态，且按下状态路径不为空则显示按下状态
	if(bDown == 1 && strDownPath != ""){
		DrawPicCDC(&memBitmap,*(++listPath.begin()),*(++listPicRect.begin()),*(++listRcRect.begin()));

	}
	//如果是弹起状态，且移动状态路径不为空则显示移动状态
	else if(bDown == 0 && strMovePath != ""){
		DrawPicCDC(&memBitmap,*(++++listPath.begin()),*(++++listPicRect.begin()),*(++++listRcRect.begin()));

	}
	//其余全部显示常规状态
	else{
		DrawPicCDC(&memBitmap,*(listPath.begin()),*(listPicRect.begin()),*(listRcRect.begin()));
	}

	//如果是按下状态，且按下状态路径不为空则显示按下状态
	if(bDown == 1 && (++listlistText.begin())->size() != 0){
		DrawTextCDC(&memBitmap,*(++listlistText.begin()),*(++listlistColor.begin()));
	}
	//如果是弹起状态，且移动状态路径不为空则显示移动状态
	else if(bDown == 0 && (++++listlistText.begin())->size() != 0){
		DrawTextCDC(&memBitmap,*(++++listlistText.begin()),*(++++listlistColor.begin()));
	}
	//其余全部显示常规状态
	else{
		DrawTextCDC(&memBitmap,*(listlistText.begin()),*(listlistColor.begin()));
	}
	Draw(GetDC()->m_hDC,&memBitmap,0,0);
	if(nInit == 2) nInit = 1;
	CButton::OnMouseHover(nFlags, point);
}


void CPicControl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	bDown = 0;
	CWnd::OnLButtonUp(nFlags, point);
}

BOOL CPicControl::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	return TRUE;
	return CButton::OnEraseBkgnd(pDC);
}
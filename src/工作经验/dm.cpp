// Machine generated IDispatch wrapper class(es) created with ClassWizard

#include "stdafx.h"
#include "dm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// Idmsoft properties

/////////////////////////////////////////////////////////////////////////////
// Idmsoft operations

CString Idmsoft::Ver()
{
	CString result;
	InvokeHelper(0x1, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
	return result;
}

long Idmsoft::SetPath(LPCTSTR path)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x2, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		path);
	return result;
}

CString Idmsoft::Ocr(long x1, long y1, long x2, long y2, LPCTSTR color, double sim)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_R8;
	InvokeHelper(0x3, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, color, sim);
	return result;
}

long Idmsoft::FindStr(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim, VARIANT* x, VARIANT* y)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x4, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, str, color, sim, x, y);
	return result;
}

long Idmsoft::GetResultCount(LPCTSTR str)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x5, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		str);
	return result;
}

long Idmsoft::GetResultPos(LPCTSTR str, long index, VARIANT* x, VARIANT* y)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x6, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		str, index, x, y);
	return result;
}

long Idmsoft::StrStr(LPCTSTR s, LPCTSTR str)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR;
	InvokeHelper(0x7, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		s, str);
	return result;
}

long Idmsoft::SendCommand(LPCTSTR cmd)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x8, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		cmd);
	return result;
}

long Idmsoft::UseDict(long index)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x9, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		index);
	return result;
}

CString Idmsoft::GetBasePath()
{
	CString result;
	InvokeHelper(0xa, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
	return result;
}

long Idmsoft::SetDictPwd(LPCTSTR pwd)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0xb, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		pwd);
	return result;
}

CString Idmsoft::OcrInFile(long x1, long y1, long x2, long y2, LPCTSTR pic_name, LPCTSTR color, double sim)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8;
	InvokeHelper(0xc, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, pic_name, color, sim);
	return result;
}

long Idmsoft::Capture(long x1, long y1, long x2, long y2, LPCTSTR file)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR;
	InvokeHelper(0xd, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, file);
	return result;
}

long Idmsoft::KeyPress(long vk)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xe, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		vk);
	return result;
}

long Idmsoft::KeyDown(long vk)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xf, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		vk);
	return result;
}

long Idmsoft::KeyUp(long vk)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x10, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		vk);
	return result;
}

long Idmsoft::LeftClick()
{
	long result;
	InvokeHelper(0x11, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::RightClick()
{
	long result;
	InvokeHelper(0x12, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::MiddleClick()
{
	long result;
	InvokeHelper(0x13, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::LeftDoubleClick()
{
	long result;
	InvokeHelper(0x14, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::LeftDown()
{
	long result;
	InvokeHelper(0x15, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::LeftUp()
{
	long result;
	InvokeHelper(0x16, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::RightDown()
{
	long result;
	InvokeHelper(0x17, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::RightUp()
{
	long result;
	InvokeHelper(0x18, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::MoveTo(long x, long y)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x19, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x, y);
	return result;
}

long Idmsoft::MoveR(long rx, long ry)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x1a, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		rx, ry);
	return result;
}

CString Idmsoft::GetColor(long x, long y)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x1b, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x, y);
	return result;
}

CString Idmsoft::GetColorBGR(long x, long y)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x1c, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x, y);
	return result;
}

CString Idmsoft::RGB2BGR(LPCTSTR rgb_color)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x1d, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		rgb_color);
	return result;
}

CString Idmsoft::BGR2RGB(LPCTSTR bgr_color)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x1e, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		bgr_color);
	return result;
}

long Idmsoft::UnBindWindow()
{
	long result;
	InvokeHelper(0x1f, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::CmpColor(long x, long y, LPCTSTR color, double sim)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_BSTR VTS_R8;
	InvokeHelper(0x20, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x, y, color, sim);
	return result;
}

long Idmsoft::ClientToScreen(long hwnd, VARIANT* x, VARIANT* y)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x21, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, x, y);
	return result;
}

long Idmsoft::ScreenToClient(long hwnd, VARIANT* x, VARIANT* y)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x22, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, x, y);
	return result;
}

long Idmsoft::ShowScrMsg(long x1, long y1, long x2, long y2, LPCTSTR msg, LPCTSTR color)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR;
	InvokeHelper(0x23, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, msg, color);
	return result;
}

long Idmsoft::SetMinRowGap(long row_gap)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x24, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		row_gap);
	return result;
}

long Idmsoft::SetMinColGap(long col_gap)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x25, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		col_gap);
	return result;
}

long Idmsoft::FindColor(long x1, long y1, long x2, long y2, LPCTSTR color, double sim, long dir, VARIANT* x, VARIANT* y)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_R8 VTS_I4 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x26, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, color, sim, dir, x, y);
	return result;
}

CString Idmsoft::FindColorEx(long x1, long y1, long x2, long y2, LPCTSTR color, double sim, long dir)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_R8 VTS_I4;
	InvokeHelper(0x27, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, color, sim, dir);
	return result;
}

long Idmsoft::SetWordLineHeight(long line_height)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x28, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		line_height);
	return result;
}

long Idmsoft::SetWordGap(long word_gap)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x29, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		word_gap);
	return result;
}

long Idmsoft::SetRowGapNoDict(long row_gap)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x2a, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		row_gap);
	return result;
}

long Idmsoft::SetColGapNoDict(long col_gap)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x2b, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		col_gap);
	return result;
}

long Idmsoft::SetWordLineHeightNoDict(long line_height)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x2c, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		line_height);
	return result;
}

long Idmsoft::SetWordGapNoDict(long word_gap)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x2d, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		word_gap);
	return result;
}

long Idmsoft::GetWordResultCount(LPCTSTR str)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x2e, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		str);
	return result;
}

long Idmsoft::GetWordResultPos(LPCTSTR str, long index, VARIANT* x, VARIANT* y)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x2f, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		str, index, x, y);
	return result;
}

CString Idmsoft::GetWordResultStr(LPCTSTR str, long index)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4;
	InvokeHelper(0x30, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		str, index);
	return result;
}

CString Idmsoft::GetWords(long x1, long y1, long x2, long y2, LPCTSTR color, double sim)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_R8;
	InvokeHelper(0x31, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, color, sim);
	return result;
}

CString Idmsoft::GetWordsNoDict(long x1, long y1, long x2, long y2, LPCTSTR color)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR;
	InvokeHelper(0x32, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, color);
	return result;
}

long Idmsoft::SetShowErrorMsg(long show)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x33, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		show);
	return result;
}

long Idmsoft::GetClientSize(long hwnd, VARIANT* width, VARIANT* height)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x34, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, width, height);
	return result;
}

long Idmsoft::MoveWindow(long hwnd, long x, long y)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x35, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, x, y);
	return result;
}

CString Idmsoft::GetColorHSV(long x, long y)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x36, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x, y);
	return result;
}

CString Idmsoft::GetAveRGB(long x1, long y1, long x2, long y2)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x37, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2);
	return result;
}

CString Idmsoft::GetAveHSV(long x1, long y1, long x2, long y2)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x38, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2);
	return result;
}

long Idmsoft::GetForegroundWindow()
{
	long result;
	InvokeHelper(0x39, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::GetForegroundFocus()
{
	long result;
	InvokeHelper(0x3a, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::GetMousePointWindow()
{
	long result;
	InvokeHelper(0x3b, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::GetPointWindow(long x, long y)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x3c, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x, y);
	return result;
}

CString Idmsoft::EnumWindow(long parent, LPCTSTR title, LPCTSTR class_name, long filter)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_BSTR VTS_I4;
	InvokeHelper(0x3d, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		parent, title, class_name, filter);
	return result;
}

long Idmsoft::GetWindowState(long hwnd, long flag)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x3e, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, flag);
	return result;
}

long Idmsoft::GetWindow(long hwnd, long flag)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x3f, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, flag);
	return result;
}

long Idmsoft::GetSpecialWindow(long flag)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x40, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		flag);
	return result;
}

long Idmsoft::SetWindowText(long hwnd, LPCTSTR text)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0x41, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, text);
	return result;
}

long Idmsoft::SetWindowSize(long hwnd, long width, long height)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x42, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, width, height);
	return result;
}

long Idmsoft::GetWindowRect(long hwnd, VARIANT* x1, VARIANT* y1, VARIANT* x2, VARIANT* y2)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x43, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, x1, y1, x2, y2);
	return result;
}

CString Idmsoft::GetWindowTitle(long hwnd)
{
	CString result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x44, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		hwnd);
	return result;
}

CString Idmsoft::GetWindowClass(long hwnd)
{
	CString result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x45, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		hwnd);
	return result;
}

long Idmsoft::SetWindowState(long hwnd, long flag)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x46, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, flag);
	return result;
}

long Idmsoft::CreateFoobarRect(long hwnd, long x, long y, long w, long h)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x47, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, x, y, w, h);
	return result;
}

long Idmsoft::CreateFoobarRoundRect(long hwnd, long x, long y, long w, long h, long rw, long rh)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x48, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, x, y, w, h, rw, rh);
	return result;
}

long Idmsoft::CreateFoobarEllipse(long hwnd, long x, long y, long w, long h)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x49, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, x, y, w, h);
	return result;
}

long Idmsoft::CreateFoobarCustom(long hwnd, long x, long y, LPCTSTR pic, LPCTSTR trans_color, double sim)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8;
	InvokeHelper(0x4a, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, x, y, pic, trans_color, sim);
	return result;
}

long Idmsoft::FoobarFillRect(long hwnd, long x1, long y1, long x2, long y2, LPCTSTR color)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR;
	InvokeHelper(0x4b, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, x1, y1, x2, y2, color);
	return result;
}

long Idmsoft::FoobarDrawText(long hwnd, long x, long y, long w, long h, LPCTSTR text, LPCTSTR color, long align)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_I4;
	InvokeHelper(0x4c, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, x, y, w, h, text, color, align);
	return result;
}

long Idmsoft::FoobarDrawPic(long hwnd, long x, long y, LPCTSTR pic, LPCTSTR trans_color)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR;
	InvokeHelper(0x4d, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, x, y, pic, trans_color);
	return result;
}

long Idmsoft::FoobarUpdate(long hwnd)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x4e, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd);
	return result;
}

long Idmsoft::FoobarLock(long hwnd)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x4f, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd);
	return result;
}

long Idmsoft::FoobarUnlock(long hwnd)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x50, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd);
	return result;
}

long Idmsoft::FoobarSetFont(long hwnd, LPCTSTR font_name, long size, long flag)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_I4 VTS_I4;
	InvokeHelper(0x51, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, font_name, size, flag);
	return result;
}

long Idmsoft::FoobarTextRect(long hwnd, long x, long y, long w, long h)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x52, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, x, y, w, h);
	return result;
}

long Idmsoft::FoobarPrintText(long hwnd, LPCTSTR text, LPCTSTR color)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_BSTR;
	InvokeHelper(0x53, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, text, color);
	return result;
}

long Idmsoft::FoobarClearText(long hwnd)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x54, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd);
	return result;
}

long Idmsoft::FoobarTextLineGap(long hwnd, long gap)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x55, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, gap);
	return result;
}

long Idmsoft::Play(LPCTSTR file)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x56, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		file);
	return result;
}

long Idmsoft::FaqCapture(long x1, long y1, long x2, long y2, long quality, long delay, long time)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x57, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, quality, delay, time);
	return result;
}

long Idmsoft::FaqRelease(long handle)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x58, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		handle);
	return result;
}

CString Idmsoft::FaqSend(LPCTSTR server, long handle, long request_type, long time_out)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x59, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		server, handle, request_type, time_out);
	return result;
}

long Idmsoft::Beep(long fre, long delay)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x5a, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		fre, delay);
	return result;
}

long Idmsoft::FoobarClose(long hwnd)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x5b, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd);
	return result;
}

long Idmsoft::MoveDD(long dx, long dy)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x5c, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		dx, dy);
	return result;
}

long Idmsoft::FaqGetSize(long handle)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x5d, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		handle);
	return result;
}

long Idmsoft::LoadPic(LPCTSTR pic_name)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x5e, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		pic_name);
	return result;
}

long Idmsoft::FreePic(LPCTSTR pic_name)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x5f, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		pic_name);
	return result;
}

long Idmsoft::GetScreenData(long x1, long y1, long x2, long y2)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x60, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2);
	return result;
}

long Idmsoft::FreeScreenData(long handle)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x61, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		handle);
	return result;
}

long Idmsoft::WheelUp()
{
	long result;
	InvokeHelper(0x62, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::WheelDown()
{
	long result;
	InvokeHelper(0x63, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::SetMouseDelay(LPCTSTR type, long delay)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4;
	InvokeHelper(0x64, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		type, delay);
	return result;
}

long Idmsoft::SetKeypadDelay(LPCTSTR type, long delay)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4;
	InvokeHelper(0x65, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		type, delay);
	return result;
}

CString Idmsoft::GetEnv(long index, LPCTSTR name)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0x66, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		index, name);
	return result;
}

long Idmsoft::SetEnv(long index, LPCTSTR name, LPCTSTR value)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_BSTR;
	InvokeHelper(0x67, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		index, name, value);
	return result;
}

long Idmsoft::SendString(long hwnd, LPCTSTR str)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0x68, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, str);
	return result;
}

long Idmsoft::DelEnv(long index, LPCTSTR name)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0x69, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		index, name);
	return result;
}

CString Idmsoft::GetPath()
{
	CString result;
	InvokeHelper(0x6a, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
	return result;
}

long Idmsoft::SetDict(long index, LPCTSTR dict_name)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0x6b, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		index, dict_name);
	return result;
}

long Idmsoft::FindPic(long x1, long y1, long x2, long y2, LPCTSTR pic_name, LPCTSTR delta_color, double sim, long dir, VARIANT* x, VARIANT* y)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_I4 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x6c, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, pic_name, delta_color, sim, dir, x, y);
	return result;
}

CString Idmsoft::FindPicEx(long x1, long y1, long x2, long y2, LPCTSTR pic_name, LPCTSTR delta_color, double sim, long dir)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_I4;
	InvokeHelper(0x6d, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, pic_name, delta_color, sim, dir);
	return result;
}

long Idmsoft::SetClientSize(long hwnd, long width, long height)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x6e, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, width, height);
	return result;
}

long Idmsoft::ReadInt(long hwnd, LPCTSTR addr, long type)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_I4;
	InvokeHelper(0x6f, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, addr, type);
	return result;
}

float Idmsoft::ReadFloat(long hwnd, LPCTSTR addr)
{
	float result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0x70, DISPATCH_METHOD, VT_R4, (void*)&result, parms,
		hwnd, addr);
	return result;
}

double Idmsoft::ReadDouble(long hwnd, LPCTSTR addr)
{
	double result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0x71, DISPATCH_METHOD, VT_R8, (void*)&result, parms,
		hwnd, addr);
	return result;
}

CString Idmsoft::FindInt(long hwnd, LPCTSTR addr_range, long int_value_min, long int_value_max, long type)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x72, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		hwnd, addr_range, int_value_min, int_value_max, type);
	return result;
}

CString Idmsoft::FindFloat(long hwnd, LPCTSTR addr_range, float float_value_min, float float_value_max)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_R4 VTS_R4;
	InvokeHelper(0x73, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		hwnd, addr_range, float_value_min, float_value_max);
	return result;
}

CString Idmsoft::FindDouble(long hwnd, LPCTSTR addr_range, double double_value_min, double double_value_max)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_R8 VTS_R8;
	InvokeHelper(0x74, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		hwnd, addr_range, double_value_min, double_value_max);
	return result;
}

CString Idmsoft::FindString(long hwnd, LPCTSTR addr_range, LPCTSTR string_value, long type)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_BSTR VTS_I4;
	InvokeHelper(0x75, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		hwnd, addr_range, string_value, type);
	return result;
}

long Idmsoft::GetModuleBaseAddr(long hwnd, LPCTSTR module_name)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0x76, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, module_name);
	return result;
}

CString Idmsoft::MoveToEx(long x, long y, long w, long h)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x77, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x, y, w, h);
	return result;
}

CString Idmsoft::MatchPicName(LPCTSTR pic_name)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x78, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		pic_name);
	return result;
}

long Idmsoft::AddDict(long index, LPCTSTR dict_info)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0x79, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		index, dict_info);
	return result;
}

long Idmsoft::EnterCri()
{
	long result;
	InvokeHelper(0x7a, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::LeaveCri()
{
	long result;
	InvokeHelper(0x7b, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::WriteInt(long hwnd, LPCTSTR addr, long type, long v)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_I4 VTS_I4;
	InvokeHelper(0x7c, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, addr, type, v);
	return result;
}

long Idmsoft::WriteFloat(long hwnd, LPCTSTR addr, float v)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_R4;
	InvokeHelper(0x7d, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, addr, v);
	return result;
}

long Idmsoft::WriteDouble(long hwnd, LPCTSTR addr, double v)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_R8;
	InvokeHelper(0x7e, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, addr, v);
	return result;
}

long Idmsoft::WriteString(long hwnd, LPCTSTR addr, long type, LPCTSTR v)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_I4 VTS_BSTR;
	InvokeHelper(0x7f, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, addr, type, v);
	return result;
}

long Idmsoft::AsmAdd(LPCTSTR asm_ins)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x80, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		asm_ins);
	return result;
}

long Idmsoft::AsmClear()
{
	long result;
	InvokeHelper(0x81, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::AsmCall(long hwnd, long mode)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x82, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, mode);
	return result;
}

long Idmsoft::FindMultiColor(long x1, long y1, long x2, long y2, LPCTSTR first_color, LPCTSTR offset_color, double sim, long dir, VARIANT* x, VARIANT* y)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_I4 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x83, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, first_color, offset_color, sim, dir, x, y);
	return result;
}

CString Idmsoft::FindMultiColorEx(long x1, long y1, long x2, long y2, LPCTSTR first_color, LPCTSTR offset_color, double sim, long dir)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_I4;
	InvokeHelper(0x84, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, first_color, offset_color, sim, dir);
	return result;
}

CString Idmsoft::AsmCode(long base_addr)
{
	CString result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x85, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		base_addr);
	return result;
}

CString Idmsoft::Assemble(LPCTSTR asm_code, long base_addr, long is_upper)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4 VTS_I4;
	InvokeHelper(0x86, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		asm_code, base_addr, is_upper);
	return result;
}

long Idmsoft::SetWindowTransparent(long hwnd, long v)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x87, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, v);
	return result;
}

CString Idmsoft::ReadData(long hwnd, LPCTSTR addr, long len)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_I4;
	InvokeHelper(0x88, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		hwnd, addr, len);
	return result;
}

long Idmsoft::WriteData(long hwnd, LPCTSTR addr, LPCTSTR data)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_BSTR;
	InvokeHelper(0x89, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, addr, data);
	return result;
}

CString Idmsoft::FindData(long hwnd, LPCTSTR addr_range, LPCTSTR data)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_BSTR;
	InvokeHelper(0x8a, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		hwnd, addr_range, data);
	return result;
}

long Idmsoft::SetPicPwd(LPCTSTR pwd)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x8b, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		pwd);
	return result;
}

long Idmsoft::Log(LPCTSTR info)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x8c, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		info);
	return result;
}

CString Idmsoft::FindStrE(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8;
	InvokeHelper(0x8d, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, str, color, sim);
	return result;
}

CString Idmsoft::FindColorE(long x1, long y1, long x2, long y2, LPCTSTR color, double sim, long dir)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_R8 VTS_I4;
	InvokeHelper(0x8e, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, color, sim, dir);
	return result;
}

CString Idmsoft::FindPicE(long x1, long y1, long x2, long y2, LPCTSTR pic_name, LPCTSTR delta_color, double sim, long dir)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_I4;
	InvokeHelper(0x8f, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, pic_name, delta_color, sim, dir);
	return result;
}

CString Idmsoft::FindMultiColorE(long x1, long y1, long x2, long y2, LPCTSTR first_color, LPCTSTR offset_color, double sim, long dir)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_I4;
	InvokeHelper(0x90, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, first_color, offset_color, sim, dir);
	return result;
}

long Idmsoft::SetExactOcr(long exact_ocr)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x91, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		exact_ocr);
	return result;
}

CString Idmsoft::ReadString(long hwnd, LPCTSTR addr, long type, long len)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_I4 VTS_I4;
	InvokeHelper(0x92, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		hwnd, addr, type, len);
	return result;
}

long Idmsoft::FoobarTextPrintDir(long hwnd, long dir)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x93, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, dir);
	return result;
}

CString Idmsoft::OcrEx(long x1, long y1, long x2, long y2, LPCTSTR color, double sim)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_R8;
	InvokeHelper(0x94, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, color, sim);
	return result;
}

long Idmsoft::SetDisplayInput(LPCTSTR mode)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x95, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		mode);
	return result;
}

long Idmsoft::GetTime()
{
	long result;
	InvokeHelper(0x96, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::GetScreenWidth()
{
	long result;
	InvokeHelper(0x97, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::GetScreenHeight()
{
	long result;
	InvokeHelper(0x98, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::BindWindowEx(long hwnd, LPCTSTR display, LPCTSTR mouse, LPCTSTR keypad, LPCTSTR public_desc, long mode)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR VTS_I4;
	InvokeHelper(0x99, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, display, mouse, keypad, public_desc, mode);
	return result;
}

CString Idmsoft::GetDiskSerial()
{
	CString result;
	InvokeHelper(0x9a, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
	return result;
}

CString Idmsoft::Md5(LPCTSTR str)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x9b, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		str);
	return result;
}

CString Idmsoft::GetMac()
{
	CString result;
	InvokeHelper(0x9c, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
	return result;
}

long Idmsoft::ActiveInputMethod(long hwnd, LPCTSTR id)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0x9d, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, id);
	return result;
}

long Idmsoft::CheckInputMethod(long hwnd, LPCTSTR id)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0x9e, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, id);
	return result;
}

long Idmsoft::FindInputMethod(LPCTSTR id)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x9f, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		id);
	return result;
}

long Idmsoft::GetCursorPos(VARIANT* x, VARIANT* y)
{
	long result;
	static BYTE parms[] =
		VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0xa0, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x, y);
	return result;
}

long Idmsoft::BindWindow(long hwnd, LPCTSTR display, LPCTSTR mouse, LPCTSTR keypad, long mode)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_BSTR VTS_BSTR VTS_I4;
	InvokeHelper(0xa1, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, display, mouse, keypad, mode);
	return result;
}

long Idmsoft::FindWindow(LPCTSTR class_name, LPCTSTR title_name)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR;
	InvokeHelper(0xa2, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		class_name, title_name);
	return result;
}

long Idmsoft::GetScreenDepth()
{
	long result;
	InvokeHelper(0xa3, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::SetScreen(long width, long height, long depth)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0xa4, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		width, height, depth);
	return result;
}

long Idmsoft::ExitOs(long type)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xa5, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		type);
	return result;
}

CString Idmsoft::GetDir(long type)
{
	CString result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xa6, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		type);
	return result;
}

long Idmsoft::GetOsType()
{
	long result;
	InvokeHelper(0xa7, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::FindWindowEx(long parent, LPCTSTR class_name, LPCTSTR title_name)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_BSTR;
	InvokeHelper(0xa8, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		parent, class_name, title_name);
	return result;
}

long Idmsoft::SetExportDict(long index, LPCTSTR dict_name)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0xa9, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		index, dict_name);
	return result;
}

CString Idmsoft::GetCursorShape()
{
	CString result;
	InvokeHelper(0xaa, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
	return result;
}

long Idmsoft::DownCpu(long rate)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xab, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		rate);
	return result;
}

CString Idmsoft::GetCursorSpot()
{
	CString result;
	InvokeHelper(0xac, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
	return result;
}

long Idmsoft::SendString2(long hwnd, LPCTSTR str)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0xad, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, str);
	return result;
}

long Idmsoft::FaqPost(LPCTSTR server, long handle, long request_type, long time_out)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0xae, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		server, handle, request_type, time_out);
	return result;
}

CString Idmsoft::FaqFetch()
{
	CString result;
	InvokeHelper(0xaf, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
	return result;
}

CString Idmsoft::FetchWord(long x1, long y1, long x2, long y2, LPCTSTR color, LPCTSTR word)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR;
	InvokeHelper(0xb0, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, color, word);
	return result;
}

long Idmsoft::CaptureJpg(long x1, long y1, long x2, long y2, LPCTSTR file, long quality)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_I4;
	InvokeHelper(0xb1, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, file, quality);
	return result;
}

long Idmsoft::FindStrWithFont(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim, LPCTSTR font_name, long font_size, long flag, VARIANT* x, VARIANT* y)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_BSTR VTS_I4 VTS_I4 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0xb2, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, str, color, sim, font_name, font_size, flag, x, y);
	return result;
}

CString Idmsoft::FindStrWithFontE(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim, LPCTSTR font_name, long font_size, long flag)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_BSTR VTS_I4 VTS_I4;
	InvokeHelper(0xb3, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, str, color, sim, font_name, font_size, flag);
	return result;
}

CString Idmsoft::FindStrWithFontEx(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim, LPCTSTR font_name, long font_size, long flag)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_BSTR VTS_I4 VTS_I4;
	InvokeHelper(0xb4, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, str, color, sim, font_name, font_size, flag);
	return result;
}

CString Idmsoft::GetDictInfo(LPCTSTR str, LPCTSTR font_name, long font_size, long flag)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_I4 VTS_I4;
	InvokeHelper(0xb5, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		str, font_name, font_size, flag);
	return result;
}

long Idmsoft::SaveDict(long index, LPCTSTR file)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0xb6, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		index, file);
	return result;
}

long Idmsoft::GetWindowProcessId(long hwnd)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xb7, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd);
	return result;
}

CString Idmsoft::GetWindowProcessPath(long hwnd)
{
	CString result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xb8, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		hwnd);
	return result;
}

long Idmsoft::LockInput(long lock)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xb9, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		lock);
	return result;
}

CString Idmsoft::GetPicSize(LPCTSTR pic_name)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0xba, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		pic_name);
	return result;
}

long Idmsoft::GetID()
{
	long result;
	InvokeHelper(0xbb, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::CapturePng(long x1, long y1, long x2, long y2, LPCTSTR file)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR;
	InvokeHelper(0xbc, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, file);
	return result;
}

long Idmsoft::CaptureGif(long x1, long y1, long x2, long y2, LPCTSTR file, long delay, long time)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_I4 VTS_I4;
	InvokeHelper(0xbd, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, file, delay, time);
	return result;
}

long Idmsoft::ImageToBmp(LPCTSTR pic_name, LPCTSTR bmp_name)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR;
	InvokeHelper(0xbe, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		pic_name, bmp_name);
	return result;
}

long Idmsoft::FindStrFast(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim, VARIANT* x, VARIANT* y)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0xbf, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, str, color, sim, x, y);
	return result;
}

CString Idmsoft::FindStrFastEx(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8;
	InvokeHelper(0xc0, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, str, color, sim);
	return result;
}

CString Idmsoft::FindStrFastE(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8;
	InvokeHelper(0xc1, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, str, color, sim);
	return result;
}

long Idmsoft::EnableDisplayDebug(long enable_debug)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xc2, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		enable_debug);
	return result;
}

long Idmsoft::CapturePre(LPCTSTR file)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0xc3, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		file);
	return result;
}

long Idmsoft::Reg(LPCTSTR code)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0xc4, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		code);
	return result;
}

CString Idmsoft::GetMachineCode()
{
	CString result;
	InvokeHelper(0xc5, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
	return result;
}

long Idmsoft::SetClipboard(LPCTSTR data)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0xc6, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		data);
	return result;
}

CString Idmsoft::GetClipboard()
{
	CString result;
	InvokeHelper(0xc7, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
	return result;
}

long Idmsoft::GetNowDict()
{
	long result;
	InvokeHelper(0xc8, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::Is64Bit()
{
	long result;
	InvokeHelper(0xc9, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::GetColorNum(long x1, long y1, long x2, long y2, LPCTSTR color, double sim)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_R8;
	InvokeHelper(0xca, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, color, sim);
	return result;
}

CString Idmsoft::EnumWindowByProcess(LPCTSTR process_name, LPCTSTR title, LPCTSTR class_name, long filter)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_BSTR VTS_I4;
	InvokeHelper(0xcb, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		process_name, title, class_name, filter);
	return result;
}

long Idmsoft::GetDictCount(long index)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xcc, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		index);
	return result;
}

long Idmsoft::GetLastError()
{
	long result;
	InvokeHelper(0xcd, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

CString Idmsoft::GetNetTime()
{
	CString result;
	InvokeHelper(0xce, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
	return result;
}

long Idmsoft::EnableGetColorByCapture(long en)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xcf, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		en);
	return result;
}

long Idmsoft::CheckUAC()
{
	long result;
	InvokeHelper(0xd0, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::SetUAC(long uac)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xd1, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		uac);
	return result;
}

long Idmsoft::DisableFontSmooth()
{
	long result;
	InvokeHelper(0xd2, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::CheckFontSmooth()
{
	long result;
	InvokeHelper(0xd3, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::SetDisplayAcceler(long level)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xd4, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		level);
	return result;
}

long Idmsoft::FindWindowByProcess(LPCTSTR process_name, LPCTSTR class_name, LPCTSTR title_name)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_BSTR;
	InvokeHelper(0xd5, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		process_name, class_name, title_name);
	return result;
}

long Idmsoft::FindWindowByProcessId(long process_id, LPCTSTR class_name, LPCTSTR title_name)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_BSTR;
	InvokeHelper(0xd6, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		process_id, class_name, title_name);
	return result;
}

CString Idmsoft::ReadIni(LPCTSTR section, LPCTSTR key, LPCTSTR file)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_BSTR;
	InvokeHelper(0xd7, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		section, key, file);
	return result;
}

long Idmsoft::WriteIni(LPCTSTR section, LPCTSTR key, LPCTSTR v, LPCTSTR file)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR;
	InvokeHelper(0xd8, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		section, key, v, file);
	return result;
}

long Idmsoft::RunApp(LPCTSTR path, long mode)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4;
	InvokeHelper(0xd9, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		path, mode);
	return result;
}

long Idmsoft::delay(long mis)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xda, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		mis);
	return result;
}

long Idmsoft::FindWindowSuper(LPCTSTR spec1, long flag1, long type1, LPCTSTR spec2, long flag2, long type2)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4 VTS_I4 VTS_BSTR VTS_I4 VTS_I4;
	InvokeHelper(0xdb, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		spec1, flag1, type1, spec2, flag2, type2);
	return result;
}

CString Idmsoft::ExcludePos(LPCTSTR all_pos, long type, long x1, long y1, long x2, long y2)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0xdc, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		all_pos, type, x1, y1, x2, y2);
	return result;
}

CString Idmsoft::FindNearestPos(LPCTSTR all_pos, long type, long x, long y)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0xdd, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		all_pos, type, x, y);
	return result;
}

CString Idmsoft::SortPosDistance(LPCTSTR all_pos, long type, long x, long y)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0xde, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		all_pos, type, x, y);
	return result;
}

long Idmsoft::FindPicMem(long x1, long y1, long x2, long y2, LPCTSTR pic_info, LPCTSTR delta_color, double sim, long dir, VARIANT* x, VARIANT* y)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_I4 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0xdf, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, pic_info, delta_color, sim, dir, x, y);
	return result;
}

CString Idmsoft::FindPicMemEx(long x1, long y1, long x2, long y2, LPCTSTR pic_info, LPCTSTR delta_color, double sim, long dir)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_I4;
	InvokeHelper(0xe0, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, pic_info, delta_color, sim, dir);
	return result;
}

CString Idmsoft::FindPicMemE(long x1, long y1, long x2, long y2, LPCTSTR pic_info, LPCTSTR delta_color, double sim, long dir)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_I4;
	InvokeHelper(0xe1, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, pic_info, delta_color, sim, dir);
	return result;
}

CString Idmsoft::AppendPicAddr(LPCTSTR pic_info, long addr, long size)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4 VTS_I4;
	InvokeHelper(0xe2, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		pic_info, addr, size);
	return result;
}

long Idmsoft::WriteFile(LPCTSTR file, LPCTSTR content)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR;
	InvokeHelper(0xe3, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		file, content);
	return result;
}

long Idmsoft::Stop(long id)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xe4, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		id);
	return result;
}

long Idmsoft::SetDictMem(long index, long addr, long size)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0xe5, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		index, addr, size);
	return result;
}

CString Idmsoft::GetNetTimeSafe()
{
	CString result;
	InvokeHelper(0xe6, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
	return result;
}

long Idmsoft::ForceUnBindWindow(long hwnd)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xe7, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd);
	return result;
}

CString Idmsoft::ReadIniPwd(LPCTSTR section, LPCTSTR key, LPCTSTR file, LPCTSTR pwd)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR;
	InvokeHelper(0xe8, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		section, key, file, pwd);
	return result;
}

long Idmsoft::WriteIniPwd(LPCTSTR section, LPCTSTR key, LPCTSTR v, LPCTSTR file, LPCTSTR pwd)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR;
	InvokeHelper(0xe9, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		section, key, v, file, pwd);
	return result;
}

long Idmsoft::DecodeFile(LPCTSTR file, LPCTSTR pwd)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR;
	InvokeHelper(0xea, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		file, pwd);
	return result;
}

long Idmsoft::KeyDownChar(LPCTSTR key_str)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0xeb, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		key_str);
	return result;
}

long Idmsoft::KeyUpChar(LPCTSTR key_str)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0xec, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		key_str);
	return result;
}

long Idmsoft::KeyPressChar(LPCTSTR key_str)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0xed, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		key_str);
	return result;
}

long Idmsoft::KeyPressStr(LPCTSTR key_str, long delay)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4;
	InvokeHelper(0xee, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		key_str, delay);
	return result;
}

long Idmsoft::EnableKeypadPatch(long en)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xef, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		en);
	return result;
}

long Idmsoft::EnableKeypadSync(long en, long time_out)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0xf0, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		en, time_out);
	return result;
}

long Idmsoft::EnableMouseSync(long en, long time_out)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0xf1, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		en, time_out);
	return result;
}

long Idmsoft::DmGuard(long en, LPCTSTR type)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR;
	InvokeHelper(0xf2, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		en, type);
	return result;
}

long Idmsoft::FaqCaptureFromFile(long x1, long y1, long x2, long y2, LPCTSTR file, long quality)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_I4;
	InvokeHelper(0xf3, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, file, quality);
	return result;
}

CString Idmsoft::FindIntEx(long hwnd, LPCTSTR addr_range, long int_value_min, long int_value_max, long type, long step, long multi_thread, long mode)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0xf4, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		hwnd, addr_range, int_value_min, int_value_max, type, step, multi_thread, mode);
	return result;
}

CString Idmsoft::FindFloatEx(long hwnd, LPCTSTR addr_range, float float_value_min, float float_value_max, long step, long multi_thread, long mode)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_R4 VTS_R4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0xf5, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		hwnd, addr_range, float_value_min, float_value_max, step, multi_thread, mode);
	return result;
}

CString Idmsoft::FindDoubleEx(long hwnd, LPCTSTR addr_range, double double_value_min, double double_value_max, long step, long multi_thread, long mode)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_R8 VTS_R8 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0xf6, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		hwnd, addr_range, double_value_min, double_value_max, step, multi_thread, mode);
	return result;
}

CString Idmsoft::FindStringEx(long hwnd, LPCTSTR addr_range, LPCTSTR string_value, long type, long step, long multi_thread, long mode)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0xf7, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		hwnd, addr_range, string_value, type, step, multi_thread, mode);
	return result;
}

CString Idmsoft::FindDataEx(long hwnd, LPCTSTR addr_range, LPCTSTR data, long step, long multi_thread, long mode)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_BSTR VTS_BSTR VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0xf8, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		hwnd, addr_range, data, step, multi_thread, mode);
	return result;
}

long Idmsoft::EnableRealMouse(long en, long mousedelay, long mousestep)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0xf9, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		en, mousedelay, mousestep);
	return result;
}

long Idmsoft::EnableRealKeypad(long en)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xfa, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		en);
	return result;
}

long Idmsoft::SendStringIme(LPCTSTR str)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0xfb, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		str);
	return result;
}

long Idmsoft::FoobarDrawLine(long hwnd, long x1, long y1, long x2, long y2, LPCTSTR color, long style, long width)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_I4 VTS_I4;
	InvokeHelper(0xfc, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, x1, y1, x2, y2, color, style, width);
	return result;
}

CString Idmsoft::FindStrEx(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8;
	InvokeHelper(0xfd, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, str, color, sim);
	return result;
}

long Idmsoft::IsBind(long hwnd)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0xfe, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd);
	return result;
}

long Idmsoft::RegEx(LPCTSTR code, LPCTSTR Ver)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR;
	InvokeHelper(0xff, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		code, Ver);
	return result;
}

long Idmsoft::GetDmCount()
{
	long result;
	InvokeHelper(0x100, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::DisableScreenSave()
{
	long result;
	InvokeHelper(0x101, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::DisablePowerSave()
{
	long result;
	InvokeHelper(0x102, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

long Idmsoft::SetMemoryHwndAsProcessId(long en)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x103, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		en);
	return result;
}

long Idmsoft::FindShape(long x1, long y1, long x2, long y2, LPCTSTR offset_color, double sim, long dir, VARIANT* x, VARIANT* y)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_R8 VTS_I4 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x104, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, offset_color, sim, dir, x, y);
	return result;
}

CString Idmsoft::FindShapeE(long x1, long y1, long x2, long y2, LPCTSTR offset_color, double sim, long dir)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_R8 VTS_I4;
	InvokeHelper(0x105, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, offset_color, sim, dir);
	return result;
}

CString Idmsoft::FindShapeEx(long x1, long y1, long x2, long y2, LPCTSTR offset_color, double sim, long dir)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_R8 VTS_I4;
	InvokeHelper(0x106, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, offset_color, sim, dir);
	return result;
}

CString Idmsoft::FindStrS(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim, VARIANT* x, VARIANT* y)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x107, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, str, color, sim, x, y);
	return result;
}

CString Idmsoft::FindStrExS(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8;
	InvokeHelper(0x108, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, str, color, sim);
	return result;
}

CString Idmsoft::FindStrFastS(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim, VARIANT* x, VARIANT* y)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x109, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, str, color, sim, x, y);
	return result;
}

CString Idmsoft::FindStrFastExS(long x1, long y1, long x2, long y2, LPCTSTR str, LPCTSTR color, double sim)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8;
	InvokeHelper(0x10a, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, str, color, sim);
	return result;
}

CString Idmsoft::FindPicS(long x1, long y1, long x2, long y2, LPCTSTR pic_name, LPCTSTR delta_color, double sim, long dir, VARIANT* x, VARIANT* y)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_I4 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x10b, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, pic_name, delta_color, sim, dir, x, y);
	return result;
}

CString Idmsoft::FindPicExS(long x1, long y1, long x2, long y2, LPCTSTR pic_name, LPCTSTR delta_color, double sim, long dir)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_BSTR VTS_BSTR VTS_R8 VTS_I4;
	InvokeHelper(0x10c, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		x1, y1, x2, y2, pic_name, delta_color, sim, dir);
	return result;
}

long Idmsoft::ClearDict(long index)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x10d, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		index);
	return result;
}

CString Idmsoft::GetMachineCodeNoMac()
{
	CString result;
	InvokeHelper(0x10e, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
	return result;
}

long Idmsoft::GetClientRect(long hwnd, VARIANT* x1, VARIANT* y1, VARIANT* x2, VARIANT* y2)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x10f, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd, x1, y1, x2, y2);
	return result;
}

long Idmsoft::EnableFakeActive(long en)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x110, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		en);
	return result;
}

long Idmsoft::GetScreenDataBmp(long x1, long y1, long x2, long y2, VARIANT* data, VARIANT* size)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_PVARIANT VTS_PVARIANT;
	InvokeHelper(0x111, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, data, size);
	return result;
}

long Idmsoft::EncodeFile(LPCTSTR file, LPCTSTR pwd)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR;
	InvokeHelper(0x112, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		file, pwd);
	return result;
}

CString Idmsoft::GetCursorShapeEx(long type)
{
	CString result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x113, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		type);
	return result;
}

long Idmsoft::FaqCancel()
{
	long result;
	InvokeHelper(0x114, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
	return result;
}

CString Idmsoft::IntToData(long int_value, long type)
{
	CString result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x115, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		int_value, type);
	return result;
}

CString Idmsoft::FloatToData(float float_value)
{
	CString result;
	static BYTE parms[] =
		VTS_R4;
	InvokeHelper(0x116, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		float_value);
	return result;
}

CString Idmsoft::DoubleToData(double double_value)
{
	CString result;
	static BYTE parms[] =
		VTS_R8;
	InvokeHelper(0x117, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		double_value);
	return result;
}

CString Idmsoft::StringToData(LPCTSTR string_value, long type)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR VTS_I4;
	InvokeHelper(0x118, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		string_value, type);
	return result;
}

long Idmsoft::SetMemoryFindResultToFile(LPCTSTR file)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x119, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		file);
	return result;
}

long Idmsoft::EnableBind(long en)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x11a, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		en);
	return result;
}

long Idmsoft::SetSimMode(long mode)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x11b, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		mode);
	return result;
}

long Idmsoft::LockMouseRect(long x1, long y1, long x2, long y2)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x11c, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2);
	return result;
}

long Idmsoft::SendPaste(long hwnd)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x11d, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		hwnd);
	return result;
}

long Idmsoft::IsDisplayDead(long x1, long y1, long x2, long y2, long t)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4 VTS_I4 VTS_I4 VTS_I4;
	InvokeHelper(0x11e, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		x1, y1, x2, y2, t);
	return result;
}

long Idmsoft::GetKeyState(long vk)
{
	long result;
	static BYTE parms[] =
		VTS_I4;
	InvokeHelper(0x11f, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		vk);
	return result;
}

long Idmsoft::CopyFile(LPCTSTR src_file, LPCTSTR dst_file, long over)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR VTS_I4;
	InvokeHelper(0x120, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		src_file, dst_file, over);
	return result;
}

long Idmsoft::IsFileExist(LPCTSTR file)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x121, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		file);
	return result;
}

long Idmsoft::DeleteFile(LPCTSTR file)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x122, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		file);
	return result;
}

long Idmsoft::MoveFile(LPCTSTR src_file, LPCTSTR dst_file)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR VTS_BSTR;
	InvokeHelper(0x123, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		src_file, dst_file);
	return result;
}

long Idmsoft::CreateFolder(LPCTSTR folder_name)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x124, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		folder_name);
	return result;
}

long Idmsoft::DeleteFolder(LPCTSTR folder_name)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x125, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		folder_name);
	return result;
}

long Idmsoft::GetFileLength(LPCTSTR file)
{
	long result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x126, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		file);
	return result;
}

CString Idmsoft::ReadFile(LPCTSTR file)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x127, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		file);
	return result;
}

long Idmsoft::WaitKey(long key_code, long time_out)
{
	long result;
	static BYTE parms[] =
		VTS_I4 VTS_I4;
	InvokeHelper(0x128, DISPATCH_METHOD, VT_I4, (void*)&result, parms,
		key_code, time_out);
	return result;
}

/******************************************************************************
*  版权所有（C）2008-2015，上海二三四五网络科技有限公司                      *
*  保留所有权利。                                                            *
******************************************************************************
*  作者 : 张凯杰
*  版本 : 1.0
*****************************************************************************/
/** 修改记录:
日期       版本    修改人             修改内容
--------------------------------------------------------------------------
******************************************************************************/
#include "pdfapp/ui/wnd/RCPdfEdit.h"
#include "pdfapp/def/RCPdfWndIdDef.h"
#include "pdfapp/manage/RCPdfUIManager.h"
#include "common/RCWindowOSVersion.h"
#include "base/RCTrace.h"

BEGIN_NAMESPACE_RC

RCPdfEdit::RCPdfEdit()
{
    m_hBkBrush = ::CreateSolidBrush(RGB(255, 255, 255));
    m_bkgColor = RGB(255, 255, 255);
    m_textClr = RGB(51, 51, 51);
    hCalcDC = ::CreateCompatibleDC(NULL);
}

RCPdfEdit::~RCPdfEdit()
{
    if (m_hBkBrush)
    {
        ::DeleteObject(m_hBkBrush);
        m_hBkBrush = NULL;
    }
    if (hCalcDC)
    {
        ::DeleteObject(hCalcDC);
    }
}

HBRUSH RCPdfEdit::OnCtlColorEdit(CDCHandle dc, CEdit edit)
{
    if (dc != NULL)
    {
        dc.SetBkMode(TRANSPARENT);
        dc.SetBkColor(m_bkgColor);
        dc.SetTextColor(m_textClr);
        dc.SetDCBrushColor(m_bkgColor);
        return (HBRUSH)m_hBkBrush;
    }
    SetMsgHandled(FALSE);
    return (HBRUSH)::GetStockObject(HOLLOW_BRUSH);
}

void RCPdfEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    //解决xp输入文字残影的问题
    InvalidateRect(NULL);
    DefWindowProc();
}

void RCPdfEdit::SetTextColor(COLORREF clr)
{
    m_textClr = clr;
}

LRESULT RCPdfEdit::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = FALSE;
    if (!RCWindowOSVersion::IsWin7OrLater())
    {
        //解决xp下 delete键 残影问题
        InvalidateRect(NULL);
    }
    
    return 0;
}

LRESULT RCPdfEdit::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    //::PostMessage(RCPdfUIManager::Instance().GetHwndById(IDR_CANVAS_WND), WM_MOUSEWHEEL, wParam, lParam);
    POINT pt;
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);
    ScreenToClient(&pt);
    short delta = GET_WHEEL_DELTA_WPARAM(wParam);
    if (delta > 0)
    {
        ::PostMessage(m_hWnd, WM_VSCROLL, SB_LINEUP, 0);
    }
    else
    {
        ::PostMessage(m_hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
    }
    return 0;
}

LRESULT RCPdfEdit::OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = FALSE;
    VScroll(wParam, lParam);
    Invalidate(TRUE);
    return 0;
}

void RCPdfEdit::OnSize(UINT nType, CSize size)
{
    ChangeSize();
    POINT pt;
    ::GetCaretPos(&pt);//获取光标相对于控件的位置
    int nLineIndex = 0;
    int pos = CharFromPos(pt, &nLineIndex);
    
    SCROLLINFO si = { 0 };
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    ::GetScrollInfo(m_hWnd, SB_VERT, &si);

    si.nPos = nLineIndex - pt.y / textHeight;
    
    si.fMask = SIF_POS;
    
    ::SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);
    si.fMask = SIF_ALL;
    ::GetScrollInfo(m_hWnd, SB_VERT, &si);
    m_pos = si.nPos;

    Invalidate(FALSE);
}

void RCPdfEdit::Init()
{
    HGDIOBJ hOldFont = ::SelectObject(hCalcDC, GetFont());
    RCString title(_T("1"));
    SIZE textSize;
    ::GetTextExtentPoint32(hCalcDC, title.c_str(), title.length(), &textSize);
    ::SelectObject(hCalcDC, hOldFont);
    textHeight = textSize.cy;
}

void RCPdfEdit::VScroll(WPARAM wParam, LPARAM lParam)
{
    SCROLLINFO si = { 0 };
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    ::GetScrollInfo(m_hWnd, SB_VERT, &si);
    uint16_t message = LOWORD(wParam);
    ChangeScrollInfo(si, message);
    ::SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);
    ::GetScrollInfo(m_hWnd, SB_VERT, &si);
    m_pos = si.nPos;
}

void RCPdfEdit::ChangeSize()
{
    if (::IsIconic(::GetParent(m_hWnd)))
    {
        return;
    }

    if (GetClientCRectDPI().Width() == 0 || GetClientCRectDPI().Height() == 0)
    {
        return;
    }

    SCROLLINFO si;

    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    ::GetScrollInfo(m_hWnd, SB_VERT, &si);

    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE;
    si.nMin = 0;
    si.nMax = 0;
    si.nPage = GetClientCRectDPI().Width();
    SetScrollInfo(SB_HORZ, &si, TRUE);
    int32_t maxLines = GetMaxLines();
    if (maxLines == 0)
    {
        si.nMax = 0;
    }
    else
    {
        si.nMax = maxLines - 1;
    }
    si.nPage = GetClientCRectDPI().Height() / textHeight;
    SetScrollInfo(SB_VERT, &si, TRUE);

    int32_t icurxpos = GetScrollPos(SB_HORZ);
    int32_t icurypos = GetScrollPos(SB_VERT);

    if (icurxpos < m_xOldPos || icurypos < m_yOldPos)
    {
        ScrollWindow(m_xOldPos - icurxpos, 0);
        ScrollWindow(0, m_yOldPos - icurypos);

    }
    m_xOldPos = icurxpos;
    m_yOldPos = icurypos;
    m_pos = icurypos;
}

void RCPdfEdit::ChangeScrollInfo(SCROLLINFO& si, uint32_t message)
{
    switch (message)
    {
        case SB_TOP:
        {
            si.nPos = si.nMin;
            break;
        }
        case SB_BOTTOM:
        {
            si.nPos = si.nMax;
            break;
        }
        case SB_LINEUP:
        {
            si.nPos--;
            break;
        }
        case SB_LINEDOWN:
        {
            si.nPos++;
            break;
        }
        case SB_PAGEUP:
        {
            si.nPos -= 0;
            break;
        }
        case SB_PAGEDOWN:
        {
            si.nPos += 0;
            break;
        }
        case SB_THUMBTRACK:
        {
            si.nPos = si.nTrackPos;
            break;
        }
    }
    si.fMask = SIF_POS;
}

CRect RCPdfEdit::GetClientCRectDPI()
{
    CRect rect;
    GetClientRect(rect);
    return rect;
}

int32_t RCPdfEdit::GetMaxLines()
{
    HGDIOBJ hOldFont = ::SelectObject(hCalcDC, GetFont());
    int32_t length = ::GetWindowTextLength(m_hWnd);
    TCHAR* pText = new TCHAR[length + 1];
    pText[length] = 0;
    ::GetWindowText(m_hWnd, pText, length + 1);
    CRect rc = GetClientCRectDPI();
    ::DrawText(hCalcDC, pText, length, &rc, DT_CALCRECT | DT_WORDBREAK);
    delete pText;
    ::SelectObject(hCalcDC, hOldFont);
    return rc.Height() / textHeight;
}

END_NAMESPACE_RC
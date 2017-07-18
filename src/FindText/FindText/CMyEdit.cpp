#include "stdafx.h"
#include "CMyEdit.h"
#include "CSystem/CSystemAPI.h"
#include "CCharset/CCharsetAPI.h"

BEGIN_MESSAGE_MAP(CMyEdit, CEdit)
    ON_WM_LBUTTONDBLCLK()
    ON_WM_RBUTTONUP()
END_MESSAGE_MAP()


void CMyEdit::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    // TODO:  在此添加消息处理程序代码和/或调用默认值
    //行号
    SelectPosLine(point);
    if (GetSelText() != "")
    {
        CSystem::OpenFile(GetSelText());
    }
}


void CMyEdit::OnRButtonUp(UINT nFlags, CPoint point)
{
    // TODO:  在此添加消息处理程序代码和/或调用默认值
    ::SetFocus(m_hWnd);
    SelectPosLine(point);
    PopupMenu(point);
}

void CMyEdit::SelectPosLine(const CPoint& point)
{
    //行号
    int nLineIndex = HIWORD(CharFromPos(point));
    int nBegin;
    int nEnd;
    if ((nBegin = LineIndex(nLineIndex)) != -1)
    {
        nEnd = nBegin + LineLength(nBegin);
        CString strSelText;
        GetWindowText(strSelText);
        strSelText = strSelText.Mid(nBegin, nEnd - nBegin);
        int offset = 0;
        if (strSelText[0] == 'l')
        {
            string line = CCharset::UnicodeToAnsi(strSelText.GetBuffer()).c_str();
            int x = atoi((&line[0]) + 5);
            int findLine = atoi(CCharset::UnicodeToAnsi(strSelText.GetBuffer()).c_str() + 5);
            strSelText.ReleaseBuffer();
            CString strLine;
            strLine.Format(_T("line:%d，GBK："), findLine);
            offset = strLine.GetLength();
        }
        strSelText = strSelText.Mid(offset, strSelText.GetLength() - offset);
        SetSel(nBegin + offset, nEnd);
        SetSelText(CCharset::UnicodeToAnsi(wstring(strSelText.GetBuffer())));
        strSelText.ReleaseBuffer();
    }
}

void CMyEdit::PopupMenu(const CPoint& point)const
{
    CPoint pointTemp = point;
    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_DEFAULT, 10001, L"打开文件位置");
    ::SetForegroundWindow(this->GetSafeHwnd());
    ClientToScreen(&pointTemp);
    int nSelect = ::TrackPopupMenu(menu.GetSafeHmenu(), TPM_LEFTALIGN | TPM_RETURNCMD, pointTemp.x, pointTemp.y, 0, this->GetSafeHwnd(), NULL);
    if (nSelect == 10001)
    {
        if (GetSelText() != "")
        {
            CSystem::OpenFolderAndSelectFile(GetSelText());
        }
    }
    menu.DestroyMenu();
}

void CMyEdit::SetSelText(const string& text)
{
    selText = text;
}

string CMyEdit::GetSelText()const
{
    return selText;
}
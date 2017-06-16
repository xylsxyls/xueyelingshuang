#pragma once
#include <afxwin.h>
#include <string>
using namespace std;

class CMyEdit : public CEdit
{
public:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);

public:
    void SelectPosLine(const CPoint& point);
    void PopupMenu(const CPoint& point)const;
    void SetSelText(const string& text);
    string GetSelText()const;

private:
    string selText;
public:
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};
#pragma once
#include "resource.h"
#include <string>
using namespace std;

// CPicDlg 对话框

class CPicDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CPicDlg)

public:
	CPicDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPicDlg();

// 对话框数据
	enum { IDD = IDD_DIALOGPIC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnPaint();
public:
    bool init(const string& picture);
public:
    HBITMAP picHBITMAP;
    string picture;
    int width;
    int height;
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
};

#pragma once


// CDrawDlg 对话框
#include "resource.h"

class CDrawDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDrawDlg)

public:
	CDrawDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDrawDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};


// TipDlg.h : 头文件
//

#pragma once
#define MYWM_NOTIFYICON WM_USER+5
#define DELETEICON      WM_USER+6

// CTipDlg 对话框
class CTipDlg : public CDialogEx
{
// 构造
public:
	CTipDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_TIP_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CStatic *pStatic;
	CString strTip;
	int nAddTimes;
	int nSix;
	//在顶部添加一个提示
	void AddTip(CString strTip);
	//显示界面
	void ShowTip();
	//创建一个托盘图标
	void CreateNotifyIcon();
	LRESULT OnNotifyIcon(WPARAM wParam, LPARAM lParam);
	void OnCancel();
	LRESULT DeleteIcon(WPARAM wParam, LPARAM lParam);
	LRESULT ShowText(WPARAM wParam, LPARAM lParam);
	LRESULT Exit(WPARAM wParam, LPARAM lParam);
};

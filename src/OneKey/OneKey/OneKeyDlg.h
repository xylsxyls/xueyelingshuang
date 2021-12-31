
// OneKeyDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include <vector>

#define WM_DESTROY_HERO_HEAD 10001
#define WM_RESET_HERO_HEAD 10002
#define WM_UPDATE_HERO_HEAD 10003
#define WM_MOVE_HERO_HEAD 10004

class CHeroHeadDlg;
// COneKeyDlg 对话框
class COneKeyDlg : public CDialogEx
{
// 构造
public:
	COneKeyDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_OneKey_DIALOG };

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
	afx_msg void OnBnClickedButton1();
	afx_msg void OnDestroy();
	CButton m_button;
	CComboBox m_type;
	afx_msg void OnSelchangeCombo1();
	CEdit m_edit;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnDestroyHeroHead(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnResetHeroHead(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateHeroHead(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnMoveHeroHead(WPARAM wParam, LPARAM lParam);

	//std::vector<std::vector<xyls::Color>> m_vecCheckColor;
	CWnd* m_deskWnd;
	CDC* m_deskCDC;
	HDC m_deskHDC;
	afx_msg void OnDropFiles(HDROP hDropInfo);

private:
	std::vector<CHeroHeadDlg*> m_vecHeroHeadDlg;
};

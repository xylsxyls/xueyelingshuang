
// MFCTestDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include <vector>

// CMFCTestDlg �Ի���
class CMFCTestDlg : public CDialogEx
{
// ����
public:
	CMFCTestDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_MFCTEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
    BOOL PreTranslateMessage(MSG* pMsg);
    void CalcProbability();
    CEdit m_edit;
    std::vector<double> m_vecSmall;
    std::vector<double> m_vecMid;
    std::vector<double> m_vecBig;
};


// OneKeyDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// COneKeyDlg �Ի���
class COneKeyDlg : public CDialogEx
{
// ����
public:
	COneKeyDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_OneKey_DIALOG };

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
	afx_msg void OnDestroy();
	CButton m_button;
	CComboBox m_type;
	afx_msg void OnSelchangeCombo1();
	CEdit m_edit;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};

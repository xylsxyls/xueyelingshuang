
// MFCTestDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "CMyEdit.h"
#include <vector>
using namespace std;

// CMFCTestDlg 对话框
class CFindTextDlg : public CDialogEx
{
// 构造
public:
	CFindTextDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
    enum { IDD = IDD_FINDTEXT_DIALOG };

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
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnBnClickedButton1();
    afx_msg void OnBnClickedButton2();

public:
    //? 打开文件查找
    //? path [in] 打开文件的路径
    //? key [in] 搜索的关键词
    //? bAddFileName [in][out] 当前缓冲区中是否已经添加了文件名一行
    void OpenFileFind(const string& path, const string& key, bool& bAddFileName);

    //? 查找文件名
    //? path [in] 打开文件的路径
    //? key [in] 搜索的关键词
    //? bAddFileName [in][out] 当前缓冲区中是否已经添加了文件名一行
    void FindFromFileName(const string& path, const string& key, bool& bAddFileName);

    //?单个路径一次查找
    void FindFromPath(const string& path, const string& key);

    //?搜索结束
    //? bAddFileName [in][out] 当前缓冲区中是否已经添加了文件名一行
    void FindEnd(bool& bAddFileName);

    //?搜集窗口信息
    void GetInfoFromWindow();

    //?执行搜索
    void Search();

    //?展示搜索结果
    void ShowSearchResult();

private:
    //?是否是排除在外的后缀
    bool IsOutFormat(const string& path, const vector<string>& vecOutFormat);
    //?获取编辑框字符串到string
    string GetCEditString(const CEdit& m_edit);
    //?获取勾选框内容到bool
    bool GetCButtonBool(const CButton& m_edit);

private:
    CEdit m_path;
    CEdit m_outFormat;
    CEdit m_text;
    CMyEdit m_find;
    string m_strPath;
    string m_strOutFormat;
    string m_strFind;
    CButton m_case;
    CButton m_btnFind;
    CButton m_fileNameCheck;
    string strLineFlag;
    string strKey;
    bool bBigSmallCheck;
    bool bFileNameCheck;
    CButton m_suffixCheck;
    bool bsuffixCheck;
};

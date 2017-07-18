
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
    enum
    {
        GBK,
        Unicode,
        UTF8
    };

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

    //?按照不同编码格式查找一行并添加
    //? oneLine [in] 搜索的这一行，gbk格式
    //? path [in] 打开文件的路径
    //? key [in] 搜索的关键词
    //? bAddFileName [in][out] 当前缓冲区中是否已经添加了文件名一行
    //? line [in] 行号
    //? addString [out] 要添加的字符串
    //? format [in] 标明此字符串是什么格式的
    //? 返回找到的位置，0开始
    int FindAdd(const string& oneLine, const string& path, const string& key, bool& bAddFileName, int line, string& addString, const string& format);

private:
    //?是否是排除在外的后缀
    bool IsOutFormat(const string& path, const vector<string>& vecOutFormat);
    //?获取编辑框字符串到string
    string GetCEditString(const CEdit& m_edit);
    //?获取下拉框中字符串到int
    int GetCComboBoxInt(const CComboBox& m_combo);
    //?获取勾选框内容到bool
    bool GetCButtonBool(const CButton& m_edit);
    //?获取utf-8的每行偏移位置
    //? line [in] 行号，从0开始
    //? oneLine [in] 每行内容
    int GetUtf8Offset(int line, const string& oneLine);
    //?获取unicode的每行偏移位置
    //? line [in] 行号，从0开始
    //? oneLine [in][out] 每行内容，会在string末尾添加一个\0
    int GetUnicodeOffset(int line, string& oneLine);
    //?对比并添加字符串
    //? addOffset [in] 需要添加的字符串中关键词偏移量
    //? addString [in] 需要添加的字符串
    //? showOffset [in] 需要展示的字符集对应所找到关键词的偏移量
    //? path [in] 打开文件的路径
    //? bAddFileName [in][out] 当前缓冲区中是否已经添加了文件名一行
    //? ifCurAdd [in][out] 当前是否已经添加
    void CompareShowAdd(int addOffset, const string& addString, int showOffset, const string& path, bool& bAddFileName, bool& ifCurAdd);

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
    CComboBox m_chara;
    int showChara;
public:
    
};

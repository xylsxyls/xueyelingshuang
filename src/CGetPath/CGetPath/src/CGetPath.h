#pragma once
#include <afxwin.h>
#include "CGetPathMacro.h"
#include <vector>
using namespace std;

class CGetPathAPI CGetPath{
public:
	//返回ocx所在路径，带\符号
	CString GetRegOcxPath(CString classid);
	//返回本进程所在路径，带\符号
	CString GetCurrentExePath();
	//通过窗口获取文件夹路径，不能获取文件路径
	CString GetFolderFromWindow(HWND hWnd);
	//浏览窗口选取文件获取文件路径，成功返回绝对路径，失败返回CString字符串-1
	CString GetFileFromWindow();
	//FileStr传文件名或后缀名不带点，不查找文件夹，得到的绝对路径放到vector里，这个函数是递归函数
	//strPath如果传必须传文件夹，寻找文件夹以下，如果不传则寻找当前路径及以下的所有文件夹
	//flag = 1表示查找文件，2表示查找文件后缀名
	vector<CString> FindFilePath(CString FileStr,CString strPath = "",BOOL flag = 1);
};
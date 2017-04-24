#pragma once
#include <afxwin.h>
#include "CGetPathMacro.h"
#include <vector>
using namespace std;

class CGetPathAPI CGetPath{
public:
	//返回ocx所在路径，带\符号
	string GetRegOcxPath(string classid);
	//返回本进程所在路径，带\符号
	string GetCurrentExePath();
	//通过窗口获取文件夹路径，不能获取文件路径，带\符号，传入MFC句柄
	string GetFolderFromWindow(HWND hWnd);
	//浏览窗口选取文件获取文件路径，成功返回绝对路径，失败返回CString字符串-1
	string GetFileFromWindow();
#ifdef WTL
    static std::string GetFileFromWindow(HWND hwnd);
#endif
	//FileStr传文件名或后缀名不带点，不查找文件夹，得到的绝对路径放到vector里，这个函数是递归函数
	//strPath如果传必须传文件夹，寻找文件夹以下，如果不传则寻找当前路径及以下的所有文件夹，带不带\\都可以
	//flag = 1表示查找文件，2表示查找文件后缀名
	vector<string> FindFilePath(string FileStr,string strPath = "",BOOL flag = 1);
	//获取进程pid，传入进程名
	vector<int> GetProcessID(string strProcessName);
	//根据pid获取窗口句柄
	HWND GetHwndByProcessId(DWORD dwProcessId);
};
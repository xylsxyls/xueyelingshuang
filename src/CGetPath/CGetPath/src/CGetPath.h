#pragma once
#include "CGetPathMacro.h"
#include <string>
#include <Windef.h>

class CGetPathAPI CGetPath
{
public:
	//通过窗口获取文件夹路径，不能获取文件路径，带\符号，传入MFC句柄
    static std::string GetFolderFromWindow(HWND hWnd);
	//浏览窗口选取文件获取文件路径，成功返回绝对路径，失败返回CString字符串-1
	static std::string GetFileFromWindow(HWND hwnd);
#ifdef WTL
    static std::string GetFileFromWTLWindow(HWND hwnd);
#endif
};
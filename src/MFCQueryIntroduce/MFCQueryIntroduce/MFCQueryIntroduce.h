
// MFCQueryIntroduce.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// CMFCQueryIntroduceApp:
// 有关此类的实现，请参阅 MFCQueryIntroduce.cpp
//

class CMFCQueryIntroduceApp : public CWinApp
{
public:
	CMFCQueryIntroduceApp();

// 重写
public:
	virtual BOOL InitInstance();
    ULONG_PTR m_gdiplusToken;
// 实现

	DECLARE_MESSAGE_MAP()
};

extern CMFCQueryIntroduceApp theApp;
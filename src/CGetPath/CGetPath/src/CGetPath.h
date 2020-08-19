#pragma once
#include "CGetPathMacro.h"
#include <string>
#include <Windef.h>

class CGetPathAPI CGetPath
{
public:
	//ͨ�����ڻ�ȡ�ļ���·�������ܻ�ȡ�ļ�·������\���ţ�����MFC���
    static std::string GetFolderFromWindow(HWND hWnd);
	//�������ѡȡ�ļ���ȡ�ļ�·�����ɹ����ؾ���·����ʧ�ܷ���CString�ַ���-1
	static std::string GetFileFromWindow(HWND hwnd);
#ifdef WTL
    static std::string GetFileFromWTLWindow(HWND hwnd);
#endif
};
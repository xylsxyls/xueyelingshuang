#include <SDKDDKVer.h>
#include <afxdisp.h>
#include "CGetPath.h"

std::string CGetPath::GetFolderFromWindow(HWND hWnd)
{
	std::string cstrSelectPath;
	BROWSEINFO stBrowseInfo;
	memset(&stBrowseInfo, 0, sizeof(stBrowseInfo));
	stBrowseInfo.hwndOwner = hWnd;
	stBrowseInfo.lpszTitle = "��ѡ���ļ���";
	stBrowseInfo.ulFlags = BIF_NEWDIALOGSTYLE;
	LPITEMIDLIST pidl = SHBrowseForFolder(&stBrowseInfo);
	if(pidl)
    {
		TCHAR szfolderPath[MAX_PATH] = {};
		SHGetPathFromIDList(pidl, szfolderPath);
		cstrSelectPath = szfolderPath;
		LPMALLOC lpMalloc;
		if(SUCCEEDED(SHGetMalloc(&lpMalloc)))
        {
			lpMalloc->Free(pidl);
			lpMalloc->Release();
		}
	}
    if (cstrSelectPath != "" && cstrSelectPath[cstrSelectPath.length() - 1] != '\\')
    {
        cstrSelectPath.append("\\");
    }
	return cstrSelectPath;
}

std::string CGetPath::GetFileFromWindow(HWND hwnd)
{
	TCHAR szBuffer[MAX_PATH] = { 0 };
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
    //"Exe�ļ�(*.exe)*.exe�����ļ�(*.*)*.*"
	ofn.lpstrFilter = "�����ļ�(*.*)*.*";
    //Ĭ�ϵ��ļ�·�� 
	ofn.lpstrInitialDir = "";
    //����ļ��Ļ����� 
	ofn.lpstrFile = szBuffer;
	ofn.nMaxFile = sizeof(szBuffer) / sizeof(*szBuffer);
	ofn.nFilterIndex = 0;
    //��־����Ƕ�ѡҪ����OFN_ALLOWMULTISELECT
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
	GetOpenFileName(&ofn);
	return std::string(szBuffer);
}

#ifdef WTL
std::string CGetPath::GetFileFromWTLWindow(HWND hwnd)
{
    wchar_t* filter = L"image files (*.png)\0*.png\0All Files (*.*)\0*.*\0\0";
    CFileDialog dlg(true, 0, 0, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, filter, hwnd);
    if (dlg.DoModal() != IDOK)
	{
		return "";
    }
	std::wstring ss = dlg.m_szFileName;
	return CStringManager::UnicodeToAnsi(ss);
}
#endif
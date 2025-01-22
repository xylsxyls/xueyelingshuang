#include <SDKDDKVer.h>
#include <afxdisp.h>
#include "CGetPath.h"

std::string CGetPath::GetFolderFromWindow(HWND hWnd)
{
	std::string cstrSelectPath;
	BROWSEINFO stBrowseInfo;
	memset(&stBrowseInfo, 0, sizeof(stBrowseInfo));
	stBrowseInfo.hwndOwner = hWnd;
	stBrowseInfo.lpszTitle = "请选择文件夹";
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
    //"Exe文件(*.exe)*.exe所有文件(*.*)*.*"
	ofn.lpstrFilter = "所有文件(*.*)*.*";
    //默认的文件路径 
	ofn.lpstrInitialDir = "";
    //存放文件的缓冲区 
	ofn.lpstrFile = szBuffer;
	ofn.nMaxFile = sizeof(szBuffer) / sizeof(*szBuffer);
	ofn.nFilterIndex = 0;
    //标志如果是多选要加上OFN_ALLOWMULTISELECT
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
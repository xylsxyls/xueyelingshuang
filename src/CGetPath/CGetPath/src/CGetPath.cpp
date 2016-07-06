#include <SDKDDKVer.h>
#include "CGetPath.h"
#include <shlobj.h>
#include <afxdlgs.h>

CString CGetPath::GetRegOcxPath(CString classid){
	CString strSubKey;
	HKEY hKey;
	LPBYTE lpData;
	strSubKey.Format(_T("CLSID\\{%s}\\InprocServer32"),classid);
	RegOpenKeyEx(HKEY_CLASSES_ROOT,strSubKey,0,KEY_READ,&hKey);
	DWORD dwType = REG_SZ;
	lpData = new BYTE[1024];
	memset(lpData,0,1024);
	DWORD cbData = 1024;
	RegQueryValueEx(hKey,_T(""),NULL,&dwType,lpData,&cbData);
	CString temp = "";
	temp = lpData;
	CString result = temp.Left(temp.ReverseFind('\\') + 1);
	return result;
}

CString CGetPath::GetCurrentExePath(){
	CString temp = "",result = "";
	TCHAR szFilePath[MAX_PATH + 1] = {};
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	temp = szFilePath;
	result = temp.Left(temp.ReverseFind('\\') + 1);
	return result;
}

CString CGetPath::GetFolderFromWindow(HWND hWnd){
	CString cstrSelectPath;
	BROWSEINFO stBrowseInfo;
	memset(&stBrowseInfo, 0, sizeof(stBrowseInfo));
	stBrowseInfo.hwndOwner = hWnd;
	stBrowseInfo.lpszTitle = _T("请选择文件夹");
	stBrowseInfo.ulFlags = BIF_NEWDIALOGSTYLE;
	LPITEMIDLIST pidl = SHBrowseForFolder(&stBrowseInfo);
	if(pidl){
		TCHAR szfolderPath[MAX_PATH] = {};
		SHGetPathFromIDList(pidl, szfolderPath);
		cstrSelectPath.Format(_T("%s"), szfolderPath);
		LPMALLOC lpMalloc;
		if(SUCCEEDED(SHGetMalloc(&lpMalloc))){
			lpMalloc->Free(pidl);
			lpMalloc->Release();
		}
	}
	return cstrSelectPath;
}

CString CGetPath::GetFileFromWindow(){
	CFileDialog dlg(1,NULL,NULL,OFN_HIDEREADONLY ,"All Files(*.*)|*.*||");
	int x = dlg.DoModal();
	if(x != IDOK) return "-1";
	return dlg.GetPathName();
}

void RecursionFindFile(CString strPath,CString FileStr,vector<CString> *pPathVector,BOOL flag){
	CFileFind finder;
	BOOL IsVisit = finder.FindFile(strPath + _T("\\*.*")) ; //_T()的作用是使系统支持Unicode编码
	//系统中有些文件无法访问，会使finder变成0，那么当进入下面的循环时就会报错，返回之后上层的finder还是有值的，所以直接返回就行
	if(IsVisit == 0) return;

	CString         TempFileName   = ""; //当前寻找到的文件或文件夹名
	BOOL            IfNeedKeepFind = 1 ; //判断是否把当前路径全部找完，自然退出while

	while(IfNeedKeepFind){
		//如果是0说明已经全部找完，但是GetFileName();会返回倒数第二个找到的文件名，之后继续得到最后一个，但是FindNextFile();就一直返回0了
		IfNeedKeepFind = finder.FindNextFile();
		TempFileName = finder.GetFileName();
		if(TempFileName == "" || TempFileName == "." || TempFileName == "..") continue;
		//if内为判断是否为文件夹
		if(FILE_ATTRIBUTE_DIRECTORY == (GetFileAttributes(finder.GetFilePath()) & FILE_ATTRIBUTE_DIRECTORY)){
			//如果确实为文件夹则把文件夹绝对路径和需要找的文件名传进去
			RecursionFindFile(finder.GetFilePath(),FileStr,pPathVector,flag);
			//递归结束之后应该直接进入下一次查询
			continue;
		}
		//只有在确定不是文件夹之后才可以判断是否为需要寻找的文件，避免文件与文件夹重名
		//1表示找文件全名
		if(flag == 1){
			if(TempFileName == FileStr){
				pPathVector->push_back(finder.GetFilePath());
			}
		}
		//2表示找文件后缀名
		else if(flag == 2){
			if(TempFileName.Mid(TempFileName.ReverseFind('.') + 1) == FileStr) pPathVector->push_back(finder.GetFilePath());
		}
	}
	return;
}

vector<CString> CGetPath::FindFilePath(CString FileStr,CString strPath,BOOL flag){
	if(strPath == ""){
		char szPath[MAX_PATH] = {};
		GetModuleFileName(NULL,szPath,MAX_PATH);//这里得到的是进程目录，末尾是exe
		char *temp = _tcsrchr(szPath,'\\'); //从右向左数第一次出现指向'\\'的指针
		memset(temp,0,50);
		strPath = szPath;
	}
	vector<CString> VecPath;
	RecursionFindFile(strPath,FileStr,&VecPath,flag); //strPath为当前进程路径的上层目录
	return VecPath;
}
#include <SDKDDKVer.h>
#include "CGetPath.h"
#include <shlobj.h>
#include <Tlhelp32.h>
#include "CStringManager/CStringManagerAPI.h"
#include <afxdlgs.h>
#include <afxdisp.h>

string CGetPath::GetRegOcxPath(string classid){
	string strSubKey;
	HKEY hKey;
	LPBYTE lpData;
	CStringManager::Format(strSubKey,_T("CLSID\\{%s}\\InprocServer32"), classid);
	RegOpenKeyEx(HKEY_CLASSES_ROOT,strSubKey.c_str(),0,KEY_READ,&hKey);
	DWORD dwType = REG_SZ;
	lpData = new BYTE[1024];
	memset(lpData,0,1024);
	DWORD cbData = 1024;
	RegQueryValueEx(hKey,_T(""),NULL,&dwType,lpData,&cbData);
	string temp = "";
	temp = (char*)lpData;
	string result = temp.substr(temp.find_last_of('\\') + 1);
	return result;
}

string CGetPath::GetCurrentExePath(){
	string temp = "",result = "";
	char szFilePath[MAX_PATH + 1] = {};
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	temp = szFilePath;
	result = temp.substr(0, temp.find_last_of("/\\") + 1);
	return result;
}

string CGetPath::GetFolderFromWindow(HWND hWnd){
	string cstrSelectPath;
	BROWSEINFO stBrowseInfo;
	memset(&stBrowseInfo, 0, sizeof(stBrowseInfo));
	stBrowseInfo.hwndOwner = hWnd;
	stBrowseInfo.lpszTitle = _T("请选择文件夹");
	stBrowseInfo.ulFlags = BIF_NEWDIALOGSTYLE;
	LPITEMIDLIST pidl = SHBrowseForFolder(&stBrowseInfo);
	if(pidl){
		TCHAR szfolderPath[MAX_PATH] = {};
		SHGetPathFromIDList(pidl, szfolderPath);
		CStringManager::Format(cstrSelectPath, _T("%s"), szfolderPath);
		LPMALLOC lpMalloc;
		if(SUCCEEDED(SHGetMalloc(&lpMalloc))){
			lpMalloc->Free(pidl);
			lpMalloc->Release();
		}
	}
	if (cstrSelectPath != "") cstrSelectPath = cstrSelectPath + "\\";
	return cstrSelectPath;
}

#define NDEBUG

string CGetPath::GetFileFromWindow(HWND hwnd)
{
	TCHAR szBuffer[MAX_PATH] = { 0 };
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = _T("所有文件(*.*)*.*");//"Exe文件(*.exe)*.exe所有文件(*.*)*.*"
	ofn.lpstrInitialDir = _T("");//默认的文件路径 
	ofn.lpstrFile = szBuffer;//存放文件的缓冲区 
	ofn.nMaxFile = sizeof(szBuffer) / sizeof(*szBuffer);
	ofn.nFilterIndex = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;//标志如果是多选要加上OFN_ALLOWMULTISELECT
	BOOL bSel = GetOpenFileName(&ofn);
	return string(szBuffer);
}

#ifdef WTL
std::string RCGetPath::GetFileFromWindow(HWND hwnd){
    wchar_t* filter = L"image files (*.png)\0*.png\0All Files (*.*)\0*.*\0\0";
    CFileDialog dlg(true, 0, 0, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, filter, hwnd);
    if (dlg.DoModal() == IDOK){
        std::wstring ss = dlg.m_szFileName;
        UnicodeToAnsi(ss);
        return UnicodeToAnsi(ss);
    }
    else return "";
}
#endif

void RecursionFindFile(string strPath,string FileStr,vector<string> *pPathVector,BOOL flag){
	CFileFind finder;
    //_T()的作用是使系统支持Unicode编码
	BOOL IsVisit = finder.FindFile((strPath + "\\*.*").c_str());
	//系统中有些文件无法访问，会使finder变成0，那么当进入下面的循环时就会报错，返回之后上层的finder还是有值的，所以直接返回就行
	if(IsVisit == 0) return;

    //当前寻找到的文件或文件夹名
	string TempFileName   = "";
    //判断是否把当前路径全部找完，自然退出while
	BOOL   IfNeedKeepFind = 1 ;

	while(IfNeedKeepFind){
		//如果是0说明已经全部找完，但是GetFileName();会返回倒数第二个找到的文件名，之后继续得到最后一个，但是FindNextFile();就一直返回0了
		IfNeedKeepFind = finder.FindNextFile();
		TempFileName = finder.GetFileName();
		if(TempFileName == "" || TempFileName == "." || TempFileName == "..") continue;
		//if内为判断是否为文件夹
		if(FILE_ATTRIBUTE_DIRECTORY == (GetFileAttributes(finder.GetFilePath()) & FILE_ATTRIBUTE_DIRECTORY)){
			//如果确实为文件夹则把文件夹绝对路径和需要找的文件名传进去
			RecursionFindFile((LPSTR)(LPCTSTR)finder.GetFilePath(),FileStr,pPathVector,flag);
			//递归结束之后应该直接进入下一次查询
			continue;
		}
		//只有在确定不是文件夹之后才可以判断是否为需要寻找的文件，避免文件与文件夹重名
		//1表示找文件全名
		if(flag == 1){
			if(TempFileName == FileStr){
				pPathVector->push_back((LPSTR)(LPCTSTR)finder.GetFilePath());
			}
		}
		//2表示找文件后缀名
		else if(flag == 2){
			if(TempFileName.substr(TempFileName.find_last_of('.') + 1,TempFileName.length() - TempFileName.find_last_of('.') - 1) == FileStr) pPathVector->push_back((LPSTR)(LPCTSTR)finder.GetFilePath());
		}
        //3表示查找所有文件，不做过滤全部添加进来
        else if (flag == 3)
        {
            pPathVector->push_back((LPSTR)(LPCTSTR)finder.GetFilePath());
        }
	}
	return;
}

vector<string> CGetPath::FindFilePath(string FileStr,string strPath,BOOL flag){
	if(strPath == ""){
		char szPath[MAX_PATH] = {};
        //这里得到的是进程目录，末尾是exe
		GetModuleFileName(NULL,szPath,MAX_PATH);
        //从右向左数第一次出现指向'\\'的指针
		char *temp = _tcsrchr(szPath,'\\');
		memset(temp,0,50);
		strPath = szPath;
	}
	vector<string> VecPath;
    //strPath为当前进程路径的上层目录
	RecursionFindFile(strPath,FileStr,&VecPath,flag);
	return VecPath;
}

vector<int> CGetPath::GetProcessID(string strProcessName){
	HANDLE myhProcess;
	PROCESSENTRY32 mype;
	mype.dwSize = sizeof(PROCESSENTRY32); 
	BOOL mybRet;
	//进行进程快照
    //TH32CS_SNAPPROCESS快照所有进程
	myhProcess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	//开始进程查找
	mybRet = Process32First(myhProcess,&mype);
	vector<int> vec;
	//循环比较，得出ProcessID
	while(mybRet){
		if(strProcessName == mype.szExeFile) vec.push_back(mype.th32ProcessID);
		mybRet = Process32Next(myhProcess,&mype);
	}
	return vec;
}

typedef struct{
	HWND  hWnd ;
	DWORD dwPid;
}WNDINFO;

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam){   
	WNDINFO* pInfo = (WNDINFO*)lParam;
	DWORD dwProcessId = 0;
	GetWindowThreadProcessId(hWnd, &dwProcessId);
	if(dwProcessId == pInfo->dwPid){
		pInfo->hWnd = hWnd;
		return FALSE;
	}
	return TRUE;
}

HWND CGetPath::GetHwndByProcessId(DWORD dwProcessId){
	WNDINFO info = {0};
	info.hWnd = NULL;
	info.dwPid = dwProcessId;
	EnumWindows(EnumWindowsProc,(LPARAM)&info);
	return info.hWnd;
}
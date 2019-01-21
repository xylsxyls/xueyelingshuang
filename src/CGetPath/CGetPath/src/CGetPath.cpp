#include <SDKDDKVer.h>
#include <afxdlgs.h>
#include <afxdisp.h>
#include "CGetPath.h"
#include <shlobj.h>
#include <Tlhelp32.h>
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"

std::string CGetPath::GetRegOcxPath(const std::string& classid)
{
	std::string strSubKey;
	HKEY hKey;
	LPBYTE lpData;
    CStringManager::Format(strSubKey, _T("CLSID\\{%s}\\InprocServer32"), classid);
    RegOpenKeyEx(HKEY_CLASSES_ROOT, strSubKey.c_str(), 0, KEY_READ, &hKey);
	DWORD dwType = REG_SZ;
    lpData = new BYTE[1024];
    memset(lpData, 0, 1024);
	DWORD cbData = 1024;
    RegQueryValueEx(hKey, _T(""), NULL, &dwType, lpData, &cbData);
	std::string temp;
	temp = (char*)lpData;
    std::string result = temp.substr(temp.find_last_of('\\') + 1);
	return result;
}

std::string CGetPath::GetCurrentExePath()
{
	char szFilePath[1024] = {};
	::GetModuleFileName(NULL, szFilePath, 1024);
    return CGetPath::GetName(szFilePath, 4);
}

std::string CGetPath::GetCurrentExeName()
{
	char szFilePath[1024] = {};
	::GetModuleFileName(NULL, szFilePath, 1024);
	return CGetPath::GetName(szFilePath, 3);
}

std::string CGetPath::GetFolderFromWindow(HWND hWnd)
{
	std::string cstrSelectPath;
	BROWSEINFO stBrowseInfo;
	memset(&stBrowseInfo, 0, sizeof(stBrowseInfo));
	stBrowseInfo.hwndOwner = hWnd;
	stBrowseInfo.lpszTitle = _T("��ѡ���ļ���");
	stBrowseInfo.ulFlags = BIF_NEWDIALOGSTYLE;
	LPITEMIDLIST pidl = SHBrowseForFolder(&stBrowseInfo);
	if(pidl)
    {
		TCHAR szfolderPath[MAX_PATH] = {};
		SHGetPathFromIDList(pidl, szfolderPath);
		CStringManager::Format(cstrSelectPath, _T("%s"), szfolderPath);
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

#define NDEBUG

std::string CGetPath::GetFileFromWindow(HWND hwnd)
{
	TCHAR szBuffer[MAX_PATH] = { 0 };
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
    //"Exe�ļ�(*.exe)*.exe�����ļ�(*.*)*.*"
	ofn.lpstrFilter = _T("�����ļ�(*.*)*.*");
    //Ĭ�ϵ��ļ�·�� 
	ofn.lpstrInitialDir = _T("");
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
std::std::string RCGetPath::GetFileFromWindow(HWND hwnd){
    wchar_t* filter = L"image files (*.png)\0*.png\0All Files (*.*)\0*.*\0\0";
    CFileDialog dlg(true, 0, 0, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, filter, hwnd);
    if (dlg.DoModal() == IDOK){
        std::wstd::string ss = dlg.m_szFileName;
        UnicodeToAnsi(ss);
        return UnicodeToAnsi(ss);
    }
    else return "";
}
#endif

void RecursionFindFile(const std::string& strPath, const std::string& FileStr, std::vector<std::string> *pPathVector, int32_t flag, std::vector<std::string> *pUnVisitPath)
{
	CFileFind finder;
    //_T()��������ʹϵͳ֧��Unicode����
	BOOL IsVisit = finder.FindFile((strPath + "\\*.*").c_str());
	//ϵͳ����Щ�ļ����޷����ʣ���ʹfinder���0����ô�����������ѭ��ʱ�ͻᱨ������֮���ϲ��finder������ֵ�ģ�����ֱ�ӷ��ؾ���
    if (IsVisit == 0)
    {
        if (pUnVisitPath != NULL)
        {
            pUnVisitPath->push_back(strPath + "\\");
        }
        return;
    }

    //��ǰѰ�ҵ����ļ����ļ�����
	std::string tempFileName;
    //��ǰѰ�ҵ����ļ����ļ���·��
    std::string tempFilePath;
    //�ж��Ƿ�ѵ�ǰ·��ȫ�����꣬��Ȼ�˳�while
	BOOL   ifNeedKeepFind = 1;

	while(ifNeedKeepFind)
    {
		//�����0˵���Ѿ�ȫ�����꣬����GetFileName();�᷵�ص����ڶ����ҵ����ļ�����֮������õ����һ��������FindNextFile();��һֱ����0��
		ifNeedKeepFind = finder.FindNextFile();
		tempFileName = finder.GetFileName();
        tempFilePath = (LPSTR)(LPCTSTR)finder.GetFilePath();
        if (tempFileName == "" || tempFileName == "." || tempFileName == "..")
        {
            continue;
        }
		//if��Ϊ�ж��Ƿ�Ϊ�ļ���
        if (FILE_ATTRIBUTE_DIRECTORY == (GetFileAttributes(tempFilePath.c_str()) & FILE_ATTRIBUTE_DIRECTORY))
        {
			//���ȷʵΪ�ļ�������ļ��о���·������Ҫ�ҵ��ļ�������ȥ
            RecursionFindFile(tempFilePath, FileStr, pPathVector, flag, pUnVisitPath);
			//�ݹ����֮��Ӧ��ֱ�ӽ�����һ�β�ѯ
			continue;
		}
		//ֻ����ȷ�������ļ���֮��ſ����ж��Ƿ�Ϊ��ҪѰ�ҵ��ļ��������ļ����ļ�������
        switch (flag)
        {
            //1��ʾ���ļ�ȫ��
            case 1:
            {
                if (tempFileName == FileStr)
                {
                    pPathVector->push_back(tempFilePath);
                }
                break;
            }
            //2��ʾ���ļ���׺��
            case 2:
            {
                if (CGetPath::GetName(tempFilePath, 2) == FileStr)
                {
                    pPathVector->push_back(tempFilePath);
                }
                break;
            }
            //3��ʾ���������ļ�����������ȫ����ӽ���
            case 3:
            {
                pPathVector->push_back(tempFilePath);
                break;
            }
            default:
                break;
        }
	}
	return;
}

std::vector<std::string> CGetPath::FindFilePath(const std::string& FileStr, const std::string& strPath, int32_t flag, std::vector<std::string> *pUnVisitPath)
{
    std::string path = strPath;
    if (path == "")
    {
        path = CGetPath::GetCurrentExePath();
        path.erase(--path.end());
	}
    //����ϵͳ�ض��򣬷�ֹ64λϵͳ����System32ʱ���뵽SysWOW64��
    CSystem::ForbidRedir();
	std::vector<std::string> VecPath;
    //strPathΪ��ǰ����·�����ϲ�Ŀ¼
    RecursionFindFile(path, FileStr, &VecPath, flag, pUnVisitPath);
    //�ָ��ض���
    CSystem::RecoveryRedir();
	return VecPath;
}

std::vector<int32_t> CGetPath::GetProcessID(const std::string& strProcessName)
{
	HANDLE myhProcess;
	PROCESSENTRY32 mype;
	mype.dwSize = sizeof(PROCESSENTRY32); 
	BOOL mybRet;
	//���н��̿���
    //TH32CS_SNAPPROCESS�������н���
    myhProcess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	//��ʼ���̲���
    mybRet = Process32First(myhProcess, &mype);
	std::vector<int32_t> vec;
	//ѭ���Ƚϣ��ó�ProcessID
	while(mybRet)
    {
        if (strProcessName == mype.szExeFile)
        {
            vec.push_back(mype.th32ProcessID);
        }
		mybRet = Process32Next(myhProcess,&mype);
	}
	return vec;
}

typedef struct
{
	HWND  hWnd ;
	DWORD dwPid;
}WNDINFO;

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{   
	WNDINFO* pInfo = (WNDINFO*)lParam;
	DWORD dwProcessId = 0;
	GetWindowThreadProcessId(hWnd, &dwProcessId);
	if(dwProcessId == pInfo->dwPid)
    {
		pInfo->hWnd = hWnd;
		return FALSE;
	}
	return TRUE;
}

HWND CGetPath::GetHwndByProcessId(DWORD dwProcessId)
{
    WNDINFO info = { 0 };
	info.hWnd = NULL;
	info.dwPid = dwProcessId;
    EnumWindows(EnumWindowsProc, (LPARAM)&info);
	return info.hWnd;
}

std::string CGetPath::GetName(const std::string& path, int32_t flag)
{
    int32_t left = (int32_t)path.find_last_of("/\\");
    std::string name = CStringManager::Mid(path, left + 1, path.length() - left - 1);
	int32_t point = (int32_t)name.find_last_of(".");
    switch (flag)
    {
        case 1:
        {
            return name;
        }
        case 2:
        {
            return CStringManager::Mid(name, point + 1, point == -1 ? 0 : name.length() - point - 1);
        }
        case 3:
        {
            return CStringManager::Mid(name, 0, point == -1 ? name.length() : point);
        }
        case 4:
        {
            return CStringManager::Mid(path, 0, left + 1);
        }
        default:
            return "";
    }
}

std::string CGetPath::GetAppdataRoamingPath()
{
	TCHAR szPath[MAX_PATH] = { 0 };
	if (::SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath) < 0)
	{
		return "";
	}
	return szPath;
}

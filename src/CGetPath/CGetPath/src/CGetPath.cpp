#include <SDKDDKVer.h>
#include <afxdisp.h>
#include "CGetPath.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"

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
		CStringManager::Format(cstrSelectPath, "%s", szfolderPath);
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
				if (CStringManager::MakeLower(CGetPath::GetName(tempFilePath, 2)) == CStringManager::MakeLower(FileStr))
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
        path = CSystem::GetCurrentExePath();
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

std::string CGetPath::GetName(const std::string& path, int32_t flag)
{
	int32_t left = (int32_t)path.find_last_of("/\\");
	std::string name = path.substr(left + 1, path.length() - left - 1);
	int32_t point = (int32_t)name.find_last_of(".");
	switch (flag)
	{
	case 1:
	{
		return name;
	}
	case 2:
	{
		return name.substr(point + 1, point == -1 ? 0 : name.length() - point - 1);
	}
	case 3:
	{
		return name.substr(0, point == -1 ? name.length() : point);
	}
	case 4:
	{
		return path.substr(0, left + 1);
	}
	default:
		return "";
	}
}
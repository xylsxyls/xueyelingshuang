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
	stBrowseInfo.lpszTitle = "请选择文件夹";
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

void RecursionFindFile(const std::string& strPath, const std::string& FileStr, std::vector<std::string> *pPathVector, int32_t flag, std::vector<std::string> *pUnVisitPath)
{
	CFileFind finder;
    //_T()的作用是使系统支持Unicode编码
	BOOL IsVisit = finder.FindFile((strPath + "\\*.*").c_str());
	//系统中有些文件夹无法访问，会使finder变成0，那么当进入下面的循环时就会报错，返回之后上层的finder还是有值的，所以直接返回就行
    if (IsVisit == 0)
    {
        if (pUnVisitPath != NULL)
        {
            pUnVisitPath->push_back(strPath + "\\");
        }
        return;
    }

    //当前寻找到的文件或文件夹名
	std::string tempFileName;
    //当前寻找到的文件或文件夹路径
    std::string tempFilePath;
    //判断是否把当前路径全部找完，自然退出while
	BOOL   ifNeedKeepFind = 1;

	while(ifNeedKeepFind)
    {
		//如果是0说明已经全部找完，但是GetFileName();会返回倒数第二个找到的文件名，之后继续得到最后一个，但是FindNextFile();就一直返回0了
		ifNeedKeepFind = finder.FindNextFile();
		tempFileName = finder.GetFileName();
        tempFilePath = (LPSTR)(LPCTSTR)finder.GetFilePath();
        if (tempFileName == "" || tempFileName == "." || tempFileName == "..")
        {
            continue;
        }
		//if内为判断是否为文件夹
        if (FILE_ATTRIBUTE_DIRECTORY == (GetFileAttributes(tempFilePath.c_str()) & FILE_ATTRIBUTE_DIRECTORY))
        {
			//如果确实为文件夹则把文件夹绝对路径和需要找的文件名传进去
            RecursionFindFile(tempFilePath, FileStr, pPathVector, flag, pUnVisitPath);
			//递归结束之后应该直接进入下一次查询
			continue;
		}
		//只有在确定不是文件夹之后才可以判断是否为需要寻找的文件，避免文件与文件夹重名
        switch (flag)
        {
            //1表示找文件全名
            case 1:
            {
                if (tempFileName == FileStr)
                {
                    pPathVector->push_back(tempFilePath);
                }
                break;
            }
            //2表示找文件后缀名
            case 2:
            {
				if (CStringManager::MakeLower(CGetPath::GetName(tempFilePath, 2)) == CStringManager::MakeLower(FileStr))
                {
                    pPathVector->push_back(tempFilePath);
                }
                break;
            }
            //3表示查找所有文件，不做过滤全部添加进来
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
    //禁用系统重定向，防止64位系统访问System32时进入到SysWOW64中
    CSystem::ForbidRedir();
	std::vector<std::string> VecPath;
    //strPath为当前进程路径的上层目录
    RecursionFindFile(path, FileStr, &VecPath, flag, pUnVisitPath);
    //恢复重定向
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
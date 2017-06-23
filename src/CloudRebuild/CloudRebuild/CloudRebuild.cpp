#include <SDKDDKVer.h>
#include "CloudRebuild.h"
#include <string>
#include "CGetPath/CGetPathAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "Ctxt/CtxtAPI.h"
#include <afxwin.h>
using namespace std;

int main(){
    DWORD begin = ::GetTickCount();
    string strFile;
    //第二个是真正的执行文件
    for (int i = 0; i < __argc; i++)
    {
        if (i == 0)
        {
            continue;
        }
        strFile = __argv[i];
    }
    if (strFile == "")
    {
        printf("非法执行！");
    }
    Ctxt txt(strFile);
    txt.LoadTxt(2, "~!@#$%^&*()");
    string pathPre = CGetPath::GetName(strFile, 4);
    CStringManager::Delete(pathPre, pathPre.length() - 1, 1);
    pathPre = CGetPath::GetName(pathPre, 4);
    string strName = CGetPath::GetName(strFile, 3);
    if (strName[strName.length() - 1] == 'g')
    {
        int i = -1;
        while (i++ != txt.vectxt.size() - 1)
        {
            string dllname = txt.vectxt.at(i).at(0);
            CStringManager::Replace(dllname, "\r", "");
            CStringManager::Replace(dllname, "\n", "");
            CStringManager::Replace(dllname, "\t", "");
            CStringManager::Replace(dllname, " ", "");
            string strCall = "call \"" + pathPre + dllname + "\\version_debug.bat\"";
            system(strCall.c_str());
        }
    }
    else if (strName[strName.length() - 1] == 'e')
    {
        int i = -1;
        while (i++ != txt.vectxt.size() - 1)
        {
            string dllname = txt.vectxt.at(i).at(0);
            CStringManager::Replace(dllname, "\r", "");
            CStringManager::Replace(dllname, "\n", "");
            CStringManager::Replace(dllname, "\t", "");
            CStringManager::Replace(dllname, " ", "");
            string strCall = "call \"" + pathPre + dllname + "\\version_release.bat\"";
            system(strCall.c_str());
        }
    }
    ::DeleteFile("@AutomationLog.txt");
    DWORD end = ::GetTickCount();
    printf("编译完成，耗时：%f秒", (end - begin) / 1000.0);
    getchar();
	return 0;
}
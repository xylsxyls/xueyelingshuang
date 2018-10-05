#include "CDump.h"
#include <tchar.h>
#include <Windows.h>
#include <Dbghelp.h>
#include "CGetPath/CGetPathAPI.h"
#pragma comment(lib,"Dbghelp.lib")

void CreateDumpFile(LPCSTR lpstrDumpFilePathName, EXCEPTION_POINTERS *pException)
{
    //创建Dump文件  
    HANDLE hDumpFile = ::CreateFile(lpstrDumpFilePathName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    //Dump信息
    MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
    dumpInfo.ExceptionPointers = pException;
    dumpInfo.ThreadId = ::GetCurrentThreadId();
    dumpInfo.ClientPointers = TRUE;
    //写入Dump文件内容  
    ::MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, nullptr, nullptr);
    ::CloseHandle(hDumpFile);
    return;
}

LPTOP_LEVEL_EXCEPTION_FILTER WINAPI MyDummySetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
    return nullptr;
}

BOOL PreventSetUnhandledExceptionFilter()
{
    HMODULE hKernel32 = ::LoadLibrary(_T("kernel32.dll"));
    if (hKernel32 == nullptr)
    {
        return FALSE;
    }
    void* pOrgEntry = ::GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
    if (pOrgEntry == nullptr)
    {
        return FALSE;
    }
    unsigned char newJump[100] = {};
    DWORD dwOrgEntryAddr = (DWORD)pOrgEntry;
    dwOrgEntryAddr += 5; //add 5 for 5 op-codes for jmp far
    void *pNewFunc = &MyDummySetUnhandledExceptionFilter;
    DWORD dwNewEntryAddr = (DWORD)pNewFunc;
    DWORD dwRelativeAddr = dwNewEntryAddr - dwOrgEntryAddr;
    newJump[0] = 0xE9; //JMP absolute
    ::memcpy(&newJump[1], &dwRelativeAddr, sizeof(pNewFunc));
    SIZE_T bytesWritten;
    BOOL bRet = ::WriteProcessMemory(::GetCurrentProcess(), pOrgEntry, newJump, sizeof(pNewFunc) + 1, &bytesWritten);
    return bRet;
}

LONG WINAPI UnhandledExceptionFilterEx(struct _EXCEPTION_POINTERS *pException)
{
    CreateDumpFile(CDump::instance().GetDumpFilePath().c_str(), pException);
    // TODO: MiniDumpWriteDump
    ::FatalAppExit(-1, _T("Fatal Error"));
    return EXCEPTION_CONTINUE_SEARCH;
}

void RunCrashHandler()
{
    ::SetUnhandledExceptionFilter(UnhandledExceptionFilterEx);
    ::PreventSetUnhandledExceptionFilter();
    return;
}

CDump::CDump()
{

}

CDump& CDump::instance()
{
    static CDump dump;
    return dump;
}

void CDump::DeclareDumpFile(const std::string& dumpFilePath)
{
    m_dumpFilePath = dumpFilePath;
    if (dumpFilePath == "")
    {
        m_dumpFilePath = CGetPath::GetCurrentExePath() + CGetPath::GetCurrentExeName() + ".dmp";
    }
    RunCrashHandler();
    return;
}

std::string CDump::GetDumpFilePath()
{
    return m_dumpFilePath;
}
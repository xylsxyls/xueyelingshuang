#include "mdump.h"
#include <windows.h>
#include <dbghelp.h>
#include <Shlwapi.h>
#include <time.h>
#if _MSC_VER <= 1800
#include "boost/filesystem.hpp"
#include "boost/filesystem/fstream.hpp"
#endif
#include "CStringManager/CStringManagerAPI.h"
#pragma comment(lib, "shlwapi.lib")

#if _MSC_VER <= 1800
class CErrorLog
{
public:
	CErrorLog(const boost::filesystem::path& p) : m_filePath(p)
	{
		try
		{
			m_logFile.open(p, std::ifstream::out);
			if (m_logFile.is_open())
			{
				m_logFile.seekp(0, std::ios_base::end);
			}
		}
		catch (...)
		{

		}
	}

	~CErrorLog()
	{
		close();
	}

	void close()
	{
		if (m_logFile.is_open())
			m_logFile.close();
	}

	boost::filesystem::fstream& operator << (const char* pStr)
	{
		append(std::string(pStr));
		return m_logFile;
	}

	boost::filesystem::fstream& operator << (const std::string& s)
	{
		append(s);
		return m_logFile;
	}

	//filesystem::fstream& operator << (const wchar_t* pStr)
	//{
	//	append(std::wstring(pStr));
	//	return m_logFile;
	//}

	//filesystem::fstream& operator << (const std::wstring& s)
	//{
	//	append(s);
	//	return m_logFile;
	//}

	void append(const std::string& s)
	{
		if (m_logFile.is_open())
		{
			try
			{
				m_logFile << s;
			}
			catch (...)
			{
			}
		}
	}

	//void append(const std::wstring& s)
	//{
	//	append(	stlEx::wide_to_ansi(s) );
	//}

	bool saveExcecptInfo(PEXCEPTION_POINTERS pExInfo)
	{
		if (!m_logFile.is_open())
			return false;

		// time
		//m_logFile << boost::gregorian::to_simple_string(second_clock::local_time()) << "\n";
		CErrorLog& objThis = (*this);
		append("System Information:\n");

		{ //ÄÚ´æ
			MEMORYSTATUSEX memoryStatus;
			memset(&memoryStatus, sizeof(memoryStatus), 0);
			memoryStatus.dwLength = sizeof(memoryStatus);
			GlobalMemoryStatusEx(&memoryStatus);

			objThis << CStringManager::Format("Memory: [Available]%I64uM / [Total]%I64uM \n",
				memoryStatus.ullAvailPhys >> 20, memoryStatus.ullTotalPhys >> 20);
		}

		{ // ´ÅÅÌ¿Õ¼ä
			TCHAR szWinDir[_MAX_PATH] = { 0 };
			boost::filesystem::path sysPath;
			boost::system::error_code ec;
			if (GetWindowsDirectory(szWinDir, _MAX_PATH) > 0)
			{
				sysPath = szWinDir;
				sysPath = sysPath.root_path();

				boost::filesystem::space_info si = boost::filesystem::space(sysPath, ec);
				objThis << CStringManager::Format("system Disk: [Available]%I64uM / [Total]%I64uM\n", si.available >> 20, si.capacity >> 20);
			}

			boost::filesystem::path abs_path = boost::filesystem::system_complete(m_filePath, ec);
			if (sysPath.empty() || sysPath != abs_path.root_path())
			{
				boost::filesystem::space_info si = boost::filesystem::space(abs_path.root_path(), ec);
				objThis << CStringManager::Format("game Disk: [Available]%I64uM / [Total]%I64uM\n", si.available >> 20, si.capacity >> 20);
			}

			objThis << "\n";
		}

		{ // Error info
			PEXCEPTION_RECORD pException = pExInfo->ExceptionRecord;
			PCONTEXT pContext = pExInfo->ContextRecord;
			MEMORY_BASIC_INFORMATION memInfo = { 0 };
			CHAR szCrashModule[_MAX_PATH] = "Unknown";
			if (VirtualQuery((void *)pExInfo->ContextRecord->Eip, &memInfo, sizeof(memInfo)))
			{
				GetModuleFileNameA((HINSTANCE)memInfo.AllocationBase, szCrashModule, _MAX_PATH);
			}
			objThis << CStringManager::Format("Error Module: %s \n", szCrashModule);
			objThis << CStringManager::Format("Code: 0x%08x \n", pException->ExceptionCode);
			objThis << CStringManager::Format("Flag: 0x%08x \n", pException->ExceptionFlags);
			objThis << CStringManager::Format("Addr: 0x%08x \n", pException->ExceptionAddress);

			if (EXCEPTION_ACCESS_VIOLATION == pException->ExceptionCode)
			{
				for (int i = 0; i < (int)pException->NumberParameters; i++)
				{
					objThis << CStringManager::Format("%d: %d(0x%08x) \n", i, pException->ExceptionInformation[i], pException->ExceptionInformation[i]);
				}
				if (pException->NumberParameters >= 2)
				{
					std::string strReadWriteFlag;
					if (pException->ExceptionInformation[0])
					{
						strReadWriteFlag = "Write to";
					}
					else
					{
						strReadWriteFlag = "Read from";
					}
					objThis << CStringManager::Format("%s location %08x caused an access violation. \n",
						strReadWriteFlag.c_str(), pException->ExceptionInformation[1]);
				}
			}
			objThis << CStringManager::Format("Caused error at %04x:%08x. \n", pContext->SegCs, pContext->Eip);

			objThis << " \nRegisters: \n";
			objThis << CStringManager::Format("EAX=%08x CS=%04x EIP=%08x EFLGS=%08x \n",
				pContext->Eax, pContext->SegCs, pContext->Eip, pContext->EFlags);
			objThis << CStringManager::Format("EBX=%08x SS=%04x ESP=%08x EBP=%08x \n",
				pContext->Ebx, pContext->SegSs, pContext->Esp, pContext->Ebp);
			objThis << CStringManager::Format("ECX=%08x DS=%04x ESI=%08x FS=%04x \n",
				pContext->Ecx, pContext->SegDs, pContext->Esi, pContext->SegFs);
			objThis << CStringManager::Format("EDX=%08x ES=%04x EDI=%08x GS=%04x \n",
				pContext->Edx, pContext->SegEs, pContext->Edi, pContext->SegGs);

			int NumCodeBytes = 16;	// Number of code bytes to record.

			objThis << "Bytes at CS:EIP: \n";
			unsigned char *code = (unsigned char*)pContext->Eip;
			BOOL bInvalidEipAddr = !IsBadReadPtr(code, NumCodeBytes);
			if (bInvalidEipAddr)
			{
				objThis << CStringManager::Format("cannot access Eip addr: %p", code);
			}
			else
			{
				for (int codebyte = 0; code != NULL && codebyte < NumCodeBytes; codebyte++)
				{
					if (IsBadReadPtr(&code[codebyte], sizeof(*code)))
					{
						break;
					}
					objThis << CStringManager::Format("%02x ", code[codebyte]);
				}
			}

			objThis << " \n\nCall Stack: \n";

			DWORD pc = pContext->Eip;
			PDWORD pFrame, pPrevFrame;

			pFrame = (PDWORD)pContext->Ebp;
			// Can two DWORDs be read from the supposed frame address?          
			while (pc != 0 && !IsBadWritePtr(pFrame, sizeof(PVOID) * 2))
			{
				TCHAR szModule[MAX_PATH] = { 0 };
				DWORD section = 0, offset = 0;

				try
				{
					if (!GetLogicalAddress((PVOID)pc, szModule, sizeof(szModule), section, offset))
					{
						break;
					}
				}
				catch (...)
				{
					break;
				}
				objThis << CStringManager::Format("%08X %04X:%08X %s \n", pc, section, offset, szModule);

				pc = pFrame[1];

				pPrevFrame = pFrame;

				pFrame = (PDWORD)pFrame[0]; // precede to next higher frame on stack

				if ((DWORD)pFrame & 3)    // Frame pointer must be aligned on a
					break;                  // DWORD boundary.  Bail if not so.

				if (pFrame <= pPrevFrame)
					break;
			}
			objThis << " \n";
		}//while
		return true;
	}

private:
	BOOL GetLogicalAddress(PVOID addr, PTSTR szModule, DWORD len, DWORD& section, DWORD& offset)
	{
		MEMORY_BASIC_INFORMATION mbi;

		if (!VirtualQuery(addr, &mbi, sizeof(mbi)))
			return FALSE;

		DWORD hMod = (DWORD)mbi.AllocationBase;

		if (!hMod)
			return FALSE;

		if (!GetModuleFileName((HMODULE)hMod, szModule, len))
			return FALSE;

		// Point to the DOS header in memory
		PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)hMod;

		// From the DOS header, find the NT (PE) header
		PIMAGE_NT_HEADERS pNtHdr = (PIMAGE_NT_HEADERS)(hMod + pDosHdr->e_lfanew);

		PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHdr);

		DWORD rva = (DWORD)addr - hMod; // RVA is offset from module load address

		// Iterate through the section table, looking for the one that encompasses
		// the linear address.
		for (unsigned i = 0;
			i < pNtHdr->FileHeader.NumberOfSections;
			i++, pSection++)
		{
			DWORD sectionStart = pSection->VirtualAddress;
			DWORD sectionEnd = sectionStart
				+ max(pSection->SizeOfRawData, pSection->Misc.VirtualSize);

			// Is the address in this section???
			if ((rva >= sectionStart) && (rva <= sectionEnd))
			{
				// Yes, address is in the section.  Calculate section and offset,
				// and store in the "section" & "offset" params, which were
				// passed by reference.
				section = i + 1;
				offset = rva - sectionStart;
				return TRUE;
			}
		}

		return FALSE;   // Should never get here!
	}
private:
	boost::filesystem::fstream m_logFile;
	boost::filesystem::path    m_filePath;
};

#endif

namespace WIN32DUMP
{
	typedef BOOL(WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
		CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
		CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
		CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

	CMiniDumper theCrashDumper;
	CHAR CMiniDumper::m_szAppName[MAX_PATH] = { 0 };
	CHAR CMiniDumper::m_szDumpDir[MAX_PATH] = { 0 };
	//func_on_catching_unhandled_exception CMiniDumper::ms_func_after_catching_unhandled_exeception = NULL;

	void CMiniDumper::Enable(const char* pszAppName, bool bShowErrors, const char* pszDumpDir)
		//func_on_catching_unhandled_exception f)
	{
		// if this assert fires then you have two instances of CMiniDumper which is not allowed
		//ATLASSERT( m_szAppName[0] == TEXT('\0') );
		strncpy(m_szAppName, pszAppName, _countof(m_szAppName) - 1);
		m_szAppName[_countof(m_szAppName) - 1] = '\0';

		// eMule may not have the permission to create a DMP file in the directory where the "emule.exe" is located.
		// Need to pre-determine a valid directory.
		strncpy(m_szDumpDir, pszDumpDir, _countof(m_szDumpDir) - 1);
		m_szDumpDir[_countof(m_szDumpDir) - 1] = '\0';
		PathAddBackslashA(m_szDumpDir);

		// save the callback function
		//ms_func_after_catching_unhandled_exeception = f;

		/*
		::CCrashHandler::SetProcessExceptionHandlers();
		::CCrashHandler::SetThreadExceptionHandlers();
		*/

		MINIDUMPWRITEDUMP pfnMiniDumpWriteDump = NULL;
		HMODULE hDbgHelpDll = GetDebugHelperDll((FARPROC*)&pfnMiniDumpWriteDump, bShowErrors);
		if (hDbgHelpDll)
		{
			if (pfnMiniDumpWriteDump)
			{
				SetUnhandledExceptionFilter(TopLevelFilter);

				OutputDebugStringA("Initialize minidump succeed!\n");
			}
			else
			{
				OutputDebugStringA("Get MiniDumpWriteDump api from debug helper dll failed!\n");
			}

			FreeLibrary(hDbgHelpDll);
			hDbgHelpDll = NULL;
			pfnMiniDumpWriteDump = NULL;
		}
		else
		{
			OutputDebugStringA("Load debug helper dll failed!\n");
		}
	}

#define DBGHELP_HINT L("You can get the required DBGHELP.DLL by downloading the \"User Mode Process Dumper\" from \"Microsoft Download Center\". \n \n") \
    L("Extract the \"User Mode Process Dumper\" and locate the \"x86\" folder. Copy the DBGHELP.DLL from the \"x86\" folder into your 11GameBox installation folder and/or into your Windows system/system32 folder.")


	HMODULE CMiniDumper::GetDebugHelperDll(FARPROC* ppfnMiniDumpWriteDump, bool bShowErrors)
	{
		*ppfnMiniDumpWriteDump = NULL;
		HMODULE hDll = ::LoadLibraryA("DBGHELP.DLL");
		if (hDll == NULL)
		{
			if (bShowErrors) {
				// Do *NOT* localize that string (in fact, do not use MFC to load it)!
				//MessageBox(NULL, L"DBGHELP.DLL not found. Please install a DBGHELP.DLL. \n \n" DBGHELP_HINT, m_szAppName, MB_ICONSTOP | MB_OK);
			}
		}
		else
		{
			*ppfnMiniDumpWriteDump = GetProcAddress(hDll, "MiniDumpWriteDump");
			if (*ppfnMiniDumpWriteDump == NULL)
			{
				if (bShowErrors) {
					// Do *NOT* localize that string (in fact, do not use MFC to load it)!
					//MessageBox(NULL, L"DBGHELP.DLL found is too old. Please upgrade to version 5.1 (or later) of DBGHELP.DLL. \n \n" DBGHELP_HINT, m_szAppName, MB_ICONSTOP | MB_OK);
				}
			}
		}
		return hDll;
	}

	LONG CMiniDumper::TopLevelFilter(struct _EXCEPTION_POINTERS* pExceptionInfo)
	{
		LONG lRetValue = EXCEPTION_CONTINUE_SEARCH;
		CHAR szResult[MAX_PATH + 1024] = { 0 };
		MINIDUMPWRITEDUMP pfnMiniDumpWriteDump = NULL;
		HMODULE hDll = GetDebugHelperDll((FARPROC*)&pfnMiniDumpWriteDump, true);

#if _MSC_VER <= 1800
		CErrorLog errLog(boost::filesystem::path(m_szDumpDir) / L"err.log");
		errLog.saveExcecptInfo(pExceptionInfo);
#endif
		if (hDll)
		{
			if (pfnMiniDumpWriteDump)
			{
				// Ask user if they want to save a dump file
				// Do *NOT* localize that string (in fact, do not use MFC to load it)!
				// Create full path for DUMP file
				CHAR szDumpPath[MAX_PATH];
				strncpy(szDumpPath, m_szDumpDir, _countof(szDumpPath) - 1);
				szDumpPath[_countof(szDumpPath) - 1] = L'\0';
				size_t uDumpPathLen = strlen(szDumpPath);

				CHAR szBaseName[MAX_PATH];
				strncpy(szBaseName, m_szAppName, _countof(szBaseName) - 1);
				szBaseName[_countof(szBaseName) - 1] = L'\0';
				size_t uBaseNameLen = strlen(szBaseName);

				time_t tNow = time(NULL);
				strftime(szBaseName + uBaseNameLen, _countof(szBaseName) - uBaseNameLen, "_%Y%m%d_%H%M%S", localtime(&tNow));
				szBaseName[_countof(szBaseName) - 1] = L'\0';

				// Replace spaces and dots in file name.
				CHAR* psz = szBaseName;
				while (*psz != '\0')
				{
					if (*psz == '.')
					{
						*psz = '-';
					}
					else if (*psz == ' ')
					{
						*psz = '_';
					}
					psz++;
				}
				if (uDumpPathLen < _countof(szDumpPath) - 1) {
					strncat(szDumpPath, szBaseName, _countof(szDumpPath) - uDumpPathLen - 1);
					szDumpPath[_countof(szDumpPath) - 1] = L'\0';
					uDumpPathLen = strlen(szDumpPath);
					if (uDumpPathLen < _countof(szDumpPath) - 1) {
						strncat(szDumpPath, ".dmp", _countof(szDumpPath) - uDumpPathLen - 1);
						szDumpPath[_countof(szDumpPath) - 1] = L'\0';
					}
				}

				HANDLE hFile = CreateFileA(szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					_MINIDUMP_EXCEPTION_INFORMATION ExInfo = { 0 };
					ExInfo.ThreadId = GetCurrentThreadId();
					ExInfo.ExceptionPointers = pExceptionInfo;
					ExInfo.ClientPointers = NULL;

					BOOL bOK = (*pfnMiniDumpWriteDump)(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL);
					CloseHandle(hFile);

					if (bOK)
					{
						// Do *NOT* localize that string (in fact, do not use MFC to load it)!
						_snprintf(szResult, _countof(szResult) - 1, "Saved dump file to \"%s\". \n ", szDumpPath);
						szResult[_countof(szResult) - 1] = L'\0';
						lRetValue = EXCEPTION_EXECUTE_HANDLER;

#if _MSC_VER <= 1800
						errLog << "Catch a critical error! \n";
						errLog << std::string(szResult);
#endif
					}
					else
					{
						// Do *NOT* localize that string (in fact, do not use MFC to load it)!
						_snprintf(szResult, _countof(szResult) - 1, "Failed to save dump file to \"%s\". \n Error: %u\n", szDumpPath, GetLastError());
						szResult[_countof(szResult) - 1] = L'\0';

#if _MSC_VER <= 1800
						errLog << szResult;
#endif
					}

				}
				else
				{
					// Do *NOT* localize that string (in fact, do not use MFC to load it)!
					_snprintf(szResult, _countof(szResult) - 1, "Failed to create dump file \"%s\". \n Error: %u\n", szDumpPath, GetLastError());
					szResult[_countof(szResult) - 1] = L'\0';

#if _MSC_VER <= 1800
					errLog << szResult;
#endif
				}
			}
			else
			{
#if _MSC_VER <= 1800
				errLog << "Catch a critical error! Get MiniDumpWriteDump api from debug helper dll failed!\n";
#endif
			}

			FreeLibrary(hDll);
			hDll = NULL;
			pfnMiniDumpWriteDump = NULL;
		}
		else
		{
#if _MSC_VER <= 1800
			errLog << "Catch a critical error! Load dbghelp.dll failed!\n";
#endif
		}

		//if( ms_func_after_catching_unhandled_exeception )
		//{
		//	try
		//	{
		//		ms_func_after_catching_unhandled_exeception();
		//	}
		//	catch (...)
		//	{
		//	}
		//}

#if _MSC_VER <= 1800
		errLog << "\n\n";

		//if (szResult[0] != L('\0'))
		//	MessageBox(NULL, szResult, m_szAppName, MB_ICONINFORMATION | MB_OK);
		errLog.close();
#endif
#ifndef _DEBUG
		// Exit the process only in release builds, so that in debug builds the exceptio is passed to a possible
		// installed debugger
		Sleep(100);
		//ExitProcess(0);
		TerminateProcess(GetCurrentProcess(), -1);
#endif
		return lRetValue;
	}
}
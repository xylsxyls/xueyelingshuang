﻿#include "mdump.h"

#ifdef _MSC_VER

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4091)
#endif
#include <Dbghelp.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <Shlwapi.h>
#include <time.h>
#include <fstream>
#include <stdint.h>
//用boost库可以在日志中查看磁盘剩余空间
#ifdef USE_BOOST
#include "boost/filesystem.hpp"
#include "boost/filesystem/fstream.hpp"
#endif
#pragma comment(lib, "shlwapi.lib")

static std::string Format(const char* fmt, ...)
{
	std::string result;
	va_list args;
	va_start(args, fmt);
#ifdef _WIN32
	int size = _vscprintf(fmt, args);
#elif __unix__
	va_list argcopy;
	va_copy(argcopy, args);
	int size = vsnprintf(nullptr, 0, fmt, argcopy);
#endif
	//?resize分配后string类会自动在最后分配\0，resize(5)则总长6
	result.resize(size);
	if (size != 0)
	{
#ifdef _WIN32
		//?即便分配了足够内存，长度必须加1，否则会崩溃
		vsprintf_s(&result[0], size + 1, fmt, args);
#elif __unix__
		vsnprintf(&result[0], size + 1, fmt, args);
#endif
	}
	va_end(args);
	return result;
}

static std::string GetName(const std::string& path, int32_t flag)
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

static std::string GetCurrentExePath()
{
	char szFilePath[1024] = {};
#ifdef _WIN32
	::GetModuleFileNameA(NULL, szFilePath, 1024);
#elif __unix__
	::readlink("/proc/self/exe", szFilePath, 1024);
#endif
	return GetName(szFilePath, 4);
}

static std::string GetCurrentExeName()
{
	char szFilePath[1024] = {};
#ifdef _WIN32
	::GetModuleFileNameA(NULL, szFilePath, 1024);
	return GetName(szFilePath, 3);
#elif __unix__
	::readlink("/proc/self/exe", szFilePath, 1024);
	return GetName(szFilePath, 1);
#endif
}

#if _MSC_VER <= 1900
class CErrorLog
{
public:
	CErrorLog(const std::string& p) : m_filePath(p)
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

	std::ofstream& operator << (const char* pStr)
	{
		append(std::string(pStr));
		return m_logFile;
	}

	std::ofstream& operator << (const std::string& s)
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
				m_logFile << s.c_str();
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

		{
			//内存
			MEMORYSTATUSEX memoryStatus;
			memset(&memoryStatus, sizeof(memoryStatus), 0);
			memoryStatus.dwLength = sizeof(memoryStatus);
			GlobalMemoryStatusEx(&memoryStatus);

			objThis << Format("Memory: [Available]%I64uM / [Total]%I64uM \n",
				memoryStatus.ullAvailPhys >> 20, memoryStatus.ullTotalPhys >> 20);
		}

#ifdef USE_BOOST
		{
			// 磁盘空间
			TCHAR szWinDir[_MAX_PATH] = { 0 };
			boost::filesystem::path sysPath;
			boost::system::error_code ec;
			if (GetWindowsDirectory(szWinDir, _MAX_PATH) > 0)
			{
				sysPath = szWinDir;
				sysPath = sysPath.root_path();
			
				boost::filesystem::space_info si = boost::filesystem::space(sysPath, ec);
				objThis << Format("system Disk: [Available]%I64uM / [Total]%I64uM\n", si.available >> 20, si.capacity >> 20);
			}
			
			boost::filesystem::path abs_path = boost::filesystem::system_complete(m_filePath, ec);
			if (sysPath.empty() || sysPath != abs_path.root_path())
			{
				boost::filesystem::space_info si = boost::filesystem::space(abs_path.root_path(), ec);
				objThis << Format("game Disk: [Available]%I64uM / [Total]%I64uM\n", si.available >> 20, si.capacity >> 20);
			}
			objThis << "\n";
		}
#endif

		{
			// Error info
			PEXCEPTION_RECORD pException = pExInfo->ExceptionRecord;
			PCONTEXT pContext = pExInfo->ContextRecord;
			MEMORY_BASIC_INFORMATION memInfo = { 0 };
			CHAR szCrashModule[_MAX_PATH] = "Unknown";
#ifdef _WIN64
			if (VirtualQuery((void *)pExInfo->ContextRecord->Rip, &memInfo, sizeof(memInfo)))
			{
				GetModuleFileNameA((HINSTANCE)memInfo.AllocationBase, szCrashModule, _MAX_PATH);
			}
#else
			if (VirtualQuery((void *)pExInfo->ContextRecord->Eip, &memInfo, sizeof(memInfo)))
			{
				GetModuleFileNameA((HINSTANCE)memInfo.AllocationBase, szCrashModule, _MAX_PATH);
			}
#endif
			objThis << Format("Error Module: %s \n", szCrashModule);
			objThis << Format("Code: 0x%08x \n", pException->ExceptionCode);
			objThis << Format("Flag: 0x%08x \n", pException->ExceptionFlags);
			objThis << Format("Addr: 0x%08x \n", pException->ExceptionAddress);

			if (EXCEPTION_ACCESS_VIOLATION == pException->ExceptionCode)
			{
				for (int i = 0; i < (int)pException->NumberParameters; i++)
				{
					objThis << Format("%d: %d(0x%08x) \n", i, pException->ExceptionInformation[i], pException->ExceptionInformation[i]);
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
					objThis << Format("%s location %08x caused an access violation. \n",
						strReadWriteFlag.c_str(), pException->ExceptionInformation[1]);
				}
			}
#ifdef _WIN64
			objThis << Format("Caused error at %04x:%08x. \n", pContext->SegCs, pContext->Rip);

			objThis << " \nRegisters: \n";
			objThis << Format("RAX=%08x CS=%04x RIP=%08x EFLGS=%08x \n",
				pContext->Rax, pContext->SegCs, pContext->Rip, pContext->EFlags);
			objThis << Format("RBX=%08x SS=%04x RSP=%08x RBP=%08x \n",
				pContext->Rbx, pContext->SegSs, pContext->Rsp, pContext->Rbp);
			objThis << Format("RCX=%08x DS=%04x RSI=%08x FS=%04x \n",
				pContext->Rcx, pContext->SegDs, pContext->Rsi, pContext->SegFs);
			objThis << Format("RDX=%08x ES=%04x RDI=%08x GS=%04x \n",
				pContext->Rdx, pContext->SegEs, pContext->Rdi, pContext->SegGs);
#else
			objThis << Format("Caused error at %04x:%08x. \n", pContext->SegCs, pContext->Eip);

			objThis << " \nRegisters: \n";
			objThis << Format("EAX=%08x CS=%04x EIP=%08x EFLGS=%08x \n",
				pContext->Eax, pContext->SegCs, pContext->Eip, pContext->EFlags);
			objThis << Format("EBX=%08x SS=%04x ESP=%08x EBP=%08x \n",
				pContext->Ebx, pContext->SegSs, pContext->Esp, pContext->Ebp);
			objThis << Format("ECX=%08x DS=%04x ESI=%08x FS=%04x \n",
				pContext->Ecx, pContext->SegDs, pContext->Esi, pContext->SegFs);
			objThis << Format("EDX=%08x ES=%04x EDI=%08x GS=%04x \n",
				pContext->Edx, pContext->SegEs, pContext->Edi, pContext->SegGs);
#endif

			int NumCodeBytes = 16;	// Number of code bytes to record.

#ifdef _WIN64
			objThis << "Bytes at CS:RIP: \n";
			unsigned char *code = (unsigned char*)pContext->Rip;
#else
			objThis << "Bytes at CS:EIP: \n";
			unsigned char *code = (unsigned char*)pContext->Eip;
#endif
			BOOL bInvalidEipAddr = !IsBadReadPtr(code, NumCodeBytes);
			if (bInvalidEipAddr)
			{
#ifdef _WIN64
				objThis << Format("cannot access Rip addr: %p", code);
#else
				objThis << Format("cannot access Eip addr: %p", code);
#endif
			}
			else
			{
				for (int codebyte = 0; code != NULL && codebyte < NumCodeBytes; codebyte++)
				{
					if (IsBadReadPtr(&code[codebyte], sizeof(*code)))
					{
						break;
					}
					objThis << Format("%02x ", code[codebyte]);
				}
			}

			objThis << " \n\nCall Stack: \n";

#ifdef _WIN64
			DWORD64 pc = pContext->Rip;
			PDWORD64 pFrame, pPrevFrame;
#else
			DWORD pc = pContext->Eip;
			PDWORD pFrame, pPrevFrame;
#endif

#ifdef _WIN64
			pFrame = (PDWORD64)pContext->Rbp;
#else
			pFrame = (PDWORD)pContext->Ebp;
#endif
			// Can two DWORDs be read from the supposed frame address?
			while (pc != 0 && !IsBadWritePtr(pFrame, sizeof(PVOID) * 2))
			{
				TCHAR szModule[MAX_PATH] = { 0 };
				DWORD section = 0;
				SIZE_T offset = 0;

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
#ifdef _WIN64
				objThis << Format("%08X %04X:%016X %s \n", pc, section, offset, szModule);
#else
				objThis << Format("%08X %04X:%08X %s \n", pc, section, offset, szModule);
#endif

				pc = pFrame[1];

				pPrevFrame = pFrame;

#ifdef _WIN64
				pFrame = (PDWORD64)pFrame[0]; // precede to next higher frame on stack

				if ((DWORD64)pFrame & 3)    // Frame pointer must be aligned on a
					break;                  // DWORD boundary.  Bail if not so.
#else
				pFrame = (PDWORD)pFrame[0]; // precede to next higher frame on stack

				if ((DWORD)pFrame & 3)      // Frame pointer must be aligned on a
					break;                  // DWORD boundary.  Bail if not so.
#endif

				if (pFrame <= pPrevFrame)
					break;
			}
			objThis << " \n";
		}//while
		return true;
	}

private:
	BOOL GetLogicalAddress(PVOID addr, PTSTR szModule, DWORD len, DWORD& section, SIZE_T& offset)
	{
		MEMORY_BASIC_INFORMATION mbi;

		if (!VirtualQuery(addr, &mbi, sizeof(mbi)))
			return FALSE;

		HMODULE hMod = (HMODULE)mbi.AllocationBase;

		if (!hMod)
			return FALSE;

		if (!GetModuleFileName((HMODULE)hMod, szModule, len))
			return FALSE;

		// Point to the DOS header in memory
		PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)hMod;

		// From the DOS header, find the NT (PE) header
		PIMAGE_NT_HEADERS pNtHdr = (PIMAGE_NT_HEADERS)(hMod + pDosHdr->e_lfanew);

		PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHdr);

		SIZE_T rva = (SIZE_T)addr - (SIZE_T)hMod; // RVA is offset from module load address

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
	std::ofstream m_logFile;
	std::string m_filePath;
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

#ifdef _WIN64
#define DBGHELP_HINT L("You can get the required DBGHELP.DLL by downloading the \"User Mode Process Dumper\" from \"Microsoft Download Center\". \n \n") \
    L("Extract the \"User Mode Process Dumper\" and locate the \"x64\" folder. Copy the DBGHELP.DLL from the \"x64\" folder into your 11GameBox installation folder and/or into your Windows system/system32 folder.")
#else
#define DBGHELP_HINT L("You can get the required DBGHELP.DLL by downloading the \"User Mode Process Dumper\" from \"Microsoft Download Center\". \n \n") \
    L("Extract the \"User Mode Process Dumper\" and locate the \"x86\" folder. Copy the DBGHELP.DLL from the \"x86\" folder into your 11GameBox installation folder and/or into your Windows system/system32 folder.")
#endif

	HMODULE CMiniDumper::GetDebugHelperDll(FARPROC* ppfnMiniDumpWriteDump, bool bShowErrors)
	{
		*ppfnMiniDumpWriteDump = NULL;
		HMODULE hDll = ::LoadLibraryA("DBGHELP.DLL");
		if (hDll == NULL)
		{
			if (bShowErrors)
			{
				// Do *NOT* localize that string (in fact, do not use MFC to load it)!
				//MessageBox(NULL, L"DBGHELP.DLL not found. Please install a DBGHELP.DLL. \n \n" DBGHELP_HINT, m_szAppName, MB_ICONSTOP | MB_OK);
			}
		}
		else
		{
			*ppfnMiniDumpWriteDump = GetProcAddress(hDll, "MiniDumpWriteDump");
			if (*ppfnMiniDumpWriteDump == NULL)
			{
				if (bShowErrors)
				{
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

		SYSTEMTIME sys_time;
		GetLocalTime(&sys_time);
		std::string currentTime = Format("%4d%02d%02d_%02d_%02d_%02d_%03d",
			sys_time.wYear, sys_time.wMonth, sys_time.wDay,
			sys_time.wHour, sys_time.wMinute, sys_time.wSecond, sys_time.wMilliseconds);
		std::string errorLogName = GetCurrentExeName() + "_" + currentTime + ".log";
		CErrorLog errLog(std::string(m_szDumpDir) + "/" + errorLogName);
		errLog.saveExcecptInfo(pExceptionInfo);

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

						errLog << "Catch a critical error! \n";
						errLog << std::string(szResult);
					}
					else
					{
						// Do *NOT* localize that string (in fact, do not use MFC to load it)!
						_snprintf(szResult, _countof(szResult) - 1, "Failed to save dump file to \"%s\". \n Error: %u\n", szDumpPath, GetLastError());
						szResult[_countof(szResult) - 1] = L'\0';

						errLog << szResult;
					}

				}
				else
				{
					// Do *NOT* localize that string (in fact, do not use MFC to load it)!
					_snprintf(szResult, _countof(szResult) - 1, "Failed to create dump file \"%s\". \n Error: %u\n", szDumpPath, GetLastError());
					szResult[_countof(szResult) - 1] = L'\0';

					errLog << szResult;
				}
			}
			else
			{
				errLog << "Catch a critical error! Get MiniDumpWriteDump api from debug helper dll failed!\n";
			}

			FreeLibrary(hDll);
			hDll = NULL;
			pfnMiniDumpWriteDump = NULL;
		}
		else
		{
			errLog << "Catch a critical error! Load dbghelp.dll failed!\n";
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

		errLog << "\n\n";

		//if (szResult[0] != L('\0'))
		//	MessageBox(NULL, szResult, m_szAppName, MB_ICONINFORMATION | MB_OK);
		errLog.close();
#ifndef _DEBUG
		// Exit the process only in release builds, so that in debug builds the exceptio is passed to a possible
		// installed debugger
		Sleep(500);
		//ExitProcess(0);
		TerminateProcess(GetCurrentProcess(), -1);
#endif
		return lRetValue;
	}
}

#endif
#ifndef __MDUMP_H__
#define __MDUMP_H__

#ifdef _MSC_VER
#include <windows.h>
//#include <functional>

struct _EXCEPTION_POINTERS;

//typedef std::function<void()> func_on_catching_unhandled_exception;

namespace WIN32DUMP
{
	class CMiniDumper
	{
		friend class CCrashHandler;
	public:
		static void Enable(const char* pszAppName, bool bShowErrors, const char* pszDumpDir);
		//, func_on_catching_unhandled_exception func=NULL);

	private:
		static CHAR m_szAppName[MAX_PATH];
		static CHAR m_szDumpDir[MAX_PATH];
		//static func_on_catching_unhandled_exception ms_func_after_catching_unhandled_exeception;

		static HMODULE GetDebugHelperDll(FARPROC* ppfnMiniDumpWriteDump, bool bShowErrors);
		static LONG WINAPI TopLevelFilter(struct _EXCEPTION_POINTERS* pExceptionInfo);
	};

	extern CMiniDumper theCrashDumper;
}

#endif

#endif //__MDUMP_H__
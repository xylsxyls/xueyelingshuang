#include "CDump.h"
#include <stdint.h>
#ifdef _MSC_VER
#include "mdump.h"
#elif __unix__
#include <execinfo.h>
#include <fstream>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#endif

static bool g_init = false;

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

#ifdef __unix__

static std::string s_dumpFileName;

static void DumpFun(int signalNum)
{
	std::ofstream dumpFile(s_dumpFileName, std::ios::app | std::ios::binary);
	dumpFile << "signalNum = " << signalNum << std::endl;

    void* buffer[1024] = {};
    int32_t nptrs = backtrace(buffer, 1024);
	dumpFile << "backtrace() returned " << nptrs << " addresses" << std::endl;
    char** strings = backtrace_symbols(buffer, nptrs);
    if (strings == nullptr)
	{
		dumpFile << "backtrace_symbols nullptr" << std::endl;
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }

	int32_t index = -1;
	while (index++ != nptrs - 1)
	{
		std::string stack = strings[index];
		dumpFile << "[" << index << "] " << stack << std::endl;
	}
	
    free(strings);

    signal(signalNum, SIG_DFL);
}
#endif

bool CDump::declareDumpFile(const std::string& dumpFileDir)
{
	if (g_init)
	{
		return false;
	}
#ifdef _MSC_VER
	WIN32DUMP::CMiniDumper::Enable(GetCurrentExeName().c_str(), false, (dumpFileDir.empty() ? GetCurrentExePath().c_str() : dumpFileDir.c_str()));
#elif __unix__
	char szBaseName[1024] = {};
	strncpy(szBaseName, (dumpFileDir.empty() ? (GetCurrentExePath() + GetCurrentExeName()).c_str() : (dumpFileDir + GetCurrentExeName()).c_str()), 1024 - 1);
	size_t uBaseNameLen = strlen(szBaseName);

	time_t tNow = time(nullptr);
	strftime(szBaseName + uBaseNameLen, 1024 - uBaseNameLen, "_%Y%m%d_%H%M%S.log", localtime(&tNow));

	s_dumpFileName = szBaseName;

	signal(SIGSEGV, DumpFun);
    signal(SIGABRT, DumpFun);
#endif
	g_init = true;
    return true;
}
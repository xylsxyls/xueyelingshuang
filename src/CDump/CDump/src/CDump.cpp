#include "CDump.h"
#include <stdint.h>
#include "mdump.h"

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
#elif __unix__
	::readlink("/proc/self/exe", szFilePath, 1024);
#endif
	return GetName(szFilePath, 3);
}

bool CDump::declareDumpFile(const std::string& dumpFileDir)
{
	if (g_init)
	{
		return false;
	}
	WIN32DUMP::CMiniDumper::Enable(GetCurrentExeName().c_str(), false, (dumpFileDir.empty() ? GetCurrentExePath().c_str() : dumpFileDir.c_str()));
	g_init = true;
    return true;
}
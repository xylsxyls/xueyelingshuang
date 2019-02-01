#include "CDump.h"
#include "CSystem/CSystemAPI.h"
#include "mdump.h"

bool g_init = false;

bool CDump::declareDumpFile(const std::string& dumpFileDir)
{
	if (g_init)
	{
		return false;
	}
	WIN32DUMP::CMiniDumper::Enable(CSystem::GetCurrentExeName().c_str(), false, (dumpFileDir.empty() ? CSystem::GetCurrentExePath().c_str() : dumpFileDir.c_str()));
	g_init = true;
    return true;
}
#include "NetManagerTestSystem.h"

bool NetManagerTestSystem::commandArguments(int argc, char** argv, std::vector<std::string>& arguments)
{
    if (!CSystem::commandArguments(argc, argv, arguments))
    {
        return false;
    }
    if (!arguments.empty() && (arguments.front() == "--node-host" || arguments.front() == "--client-host"))
    {
        arguments.erase(arguments.begin());
    }
    return true;
}

int32_t NetManagerTestSystem::acquireProcessLease(const std::string& name, bool replace, uint32_t timeoutMs,
    std::vector<uint64_t>& state, const std::string& isolated)
{
    if (!isolated.empty())
    {
        return -1;
    }
    return CSystem::acquireProcessLease(name, replace, timeoutMs, state, std::to_string(CSystem::currentProcessPid()));
}

bool NetManagerTestWatchdog::waitForRecovery()
{
    return false;
}

bool NetManagerTestWatchdog::start(const std::vector<std::string>& arguments)
{
    (void)arguments;
    return true;
}
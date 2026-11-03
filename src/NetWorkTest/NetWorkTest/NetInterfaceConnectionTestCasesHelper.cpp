#include "NetInterfaceConnectionTestCasesHelper.h"
#include <chrono>

uint64_t NetInterfaceConnectionTestCasesHelper::recoveryTestNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}